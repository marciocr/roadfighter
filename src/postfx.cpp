#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>

#include "postfx.h"

#define MAX_PASSES 2

int video_filter = FILTER_OFF;


/* GL 2.0/3.0 entry points (shaders, framebuffer objects), looked up at
   runtime since the rest of the game only uses OpenGL 1.x. */
static PFNGLCREATESHADERPROC p_glCreateShader;
static PFNGLSHADERSOURCEPROC p_glShaderSource;
static PFNGLCOMPILESHADERPROC p_glCompileShader;
static PFNGLGETSHADERIVPROC p_glGetShaderiv;
static PFNGLGETSHADERINFOLOGPROC p_glGetShaderInfoLog;
static PFNGLDELETESHADERPROC p_glDeleteShader;
static PFNGLCREATEPROGRAMPROC p_glCreateProgram;
static PFNGLATTACHSHADERPROC p_glAttachShader;
static PFNGLBINDATTRIBLOCATIONPROC p_glBindAttribLocation;
static PFNGLLINKPROGRAMPROC p_glLinkProgram;
static PFNGLGETPROGRAMIVPROC p_glGetProgramiv;
static PFNGLGETPROGRAMINFOLOGPROC p_glGetProgramInfoLog;
static PFNGLDELETEPROGRAMPROC p_glDeleteProgram;
static PFNGLUSEPROGRAMPROC p_glUseProgram;
static PFNGLGETUNIFORMLOCATIONPROC p_glGetUniformLocation;
static PFNGLUNIFORM1IPROC p_glUniform1i;
static PFNGLUNIFORM2FPROC p_glUniform2f;
static PFNGLUNIFORMMATRIX4FVPROC p_glUniformMatrix4fv;
static PFNGLVERTEXATTRIBPOINTERPROC p_glVertexAttribPointer;
static PFNGLVERTEXATTRIB4FPROC p_glVertexAttrib4f;
static PFNGLENABLEVERTEXATTRIBARRAYPROC p_glEnableVertexAttribArray;
static PFNGLDISABLEVERTEXATTRIBARRAYPROC p_glDisableVertexAttribArray;
static PFNGLACTIVETEXTUREPROC p_glActiveTexture;
static PFNGLGENFRAMEBUFFERSPROC p_glGenFramebuffers;
static PFNGLDELETEFRAMEBUFFERSPROC p_glDeleteFramebuffers;
static PFNGLBINDFRAMEBUFFERPROC p_glBindFramebuffer;
static PFNGLFRAMEBUFFERTEXTURE2DPROC p_glFramebufferTexture2D;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC p_glCheckFramebufferStatus;

static bool gl_loaded = false;
static bool gl_available = false;


static bool load_gl(void)
{
	if (gl_loaded)
		return gl_available;

	gl_loaded = true;

#define LOAD(name) \
	p_##name = (decltype(p_##name))SDL_GL_GetProcAddress(#name); \
	if (p_##name == 0) { \
		SDL_Log("postfx: missing OpenGL function %s", #name); \
		return false; \
	}

	LOAD(glCreateShader);
	LOAD(glShaderSource);
	LOAD(glCompileShader);
	LOAD(glGetShaderiv);
	LOAD(glGetShaderInfoLog);
	LOAD(glDeleteShader);
	LOAD(glCreateProgram);
	LOAD(glAttachShader);
	LOAD(glBindAttribLocation);
	LOAD(glLinkProgram);
	LOAD(glGetProgramiv);
	LOAD(glGetProgramInfoLog);
	LOAD(glDeleteProgram);
	LOAD(glUseProgram);
	LOAD(glGetUniformLocation);
	LOAD(glUniform1i);
	LOAD(glUniform2f);
	LOAD(glUniformMatrix4fv);
	LOAD(glVertexAttribPointer);
	LOAD(glVertexAttrib4f);
	LOAD(glEnableVertexAttribArray);
	LOAD(glDisableVertexAttribArray);
	LOAD(glActiveTexture);
	LOAD(glGenFramebuffers);
	LOAD(glDeleteFramebuffers);
	LOAD(glBindFramebuffer);
	LOAD(glFramebufferTexture2D);
	LOAD(glCheckFramebufferStatus);
#undef LOAD

	gl_available = true;
	return true;
}


/* A render target: a texture with a framebuffer object drawing into it. */
struct Target {
	GLuint fbo, tex;
	int w, h;
};

/* The filter's intermediate passes, for each of two input sizes (kept from
   Maze of Galious, which filters 320x200 or 640x400 images; Road Fighter
   always uses the first, for its 512x384 frame): */
static Target pass_target[2][MAX_PASSES] = {{{0, 0, 0, 0}, {0, 0, 0, 0}}, {{0, 0, 0, 0}, {0, 0, 0, 0}}};
static GLuint lut_tex = 0;
static GLuint orig_tex = 0;	/* the filter's input, for hq2x's second pass */


static bool create_target(Target *t, int w, int h)
{
	glGenTextures(1, &t->tex);
	glBindTexture(GL_TEXTURE_2D, t->tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	p_glGenFramebuffers(1, &t->fbo);
	p_glBindFramebuffer(GL_FRAMEBUFFER, t->fbo);
	p_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t->tex, 0);

	t->w = w;
	t->h = h;

	bool ok = p_glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	p_glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	if (!ok)
		SDL_Log("postfx: incomplete framebuffer (%ix%i)", w, h);

	return ok;
}


static void delete_target(Target *t)
{
	if (t->fbo)
		p_glDeleteFramebuffers(1, &t->fbo);

	if (t->tex)
		glDeleteTextures(1, &t->tex);

	t->fbo = t->tex = 0;
	t->w = t->h = 0;
}


/* ---- Shaders ---- */

static char *read_file(const char *name)
{
	FILE *fp = fopen(name, "rb");
	char *buf;
	long l;

	if (fp == 0) {
		SDL_Log("postfx: can't open %s", name);
		return 0;
	}

	fseek(fp, 0, SEEK_END);
	l = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	buf = new char[l + 1];

	if (l != (long)fread(buf, 1, l, fp)) {
		delete []buf;
		fclose(fp);
		return 0;
	}

	buf[l] = 0;
	fclose(fp);

	/* Our own #version goes first; blank out the file's, if any. */
	char *v = strstr(buf, "#version");

	if (v != 0)
		while (*v != 0 && *v != '\n')
			*v++ = ' ';

	return buf;
}


static GLuint compile(GLenum type, const char *defines, const char *source, const char *name)
{
	const char *parts[3] = {"#version 130\n", defines, source};
	GLuint s = p_glCreateShader(type);
	GLint ok = 0;

	p_glShaderSource(s, 3, parts, 0);
	p_glCompileShader(s);
	p_glGetShaderiv(s, GL_COMPILE_STATUS, &ok);

	if (!ok) {
		char log[2048];
		p_glGetShaderInfoLog(s, sizeof(log), 0, log);
		SDL_Log("postfx: %s: %s", name, log);
		p_glDeleteShader(s);
		return 0;
	}

	return s;
}


/* Loads a shader in the libretro/RetroArch GLSL format: one file holding
   both stages, selected with VERTEX / FRAGMENT. */
static GLuint load_program(const char *name)
{
	char *source = read_file(name);
	GLuint vs, fs, prog;
	GLint ok = 0;

	if (source == 0)
		return 0;

	vs = compile(GL_VERTEX_SHADER, "#define VERTEX\n", source, name);
	fs = compile(GL_FRAGMENT_SHADER, "#define FRAGMENT\n", source, name);
	delete []source;

	if (vs == 0 || fs == 0) {
		if (vs)
			p_glDeleteShader(vs);

		if (fs)
			p_glDeleteShader(fs);

		return 0;
	}

	prog = p_glCreateProgram();
	p_glAttachShader(prog, vs);
	p_glAttachShader(prog, fs);
	p_glBindAttribLocation(prog, 0, "VertexCoord");
	p_glBindAttribLocation(prog, 1, "TexCoord");
	/* The colour attribute is spelled differently across these shaders. */
	p_glBindAttribLocation(prog, 2, "COLOR");
	p_glBindAttribLocation(prog, 2, "Color");
	p_glLinkProgram(prog);
	p_glDeleteShader(vs);
	p_glDeleteShader(fs);
	p_glGetProgramiv(prog, GL_LINK_STATUS, &ok);

	if (!ok) {
		char log[2048];
		p_glGetProgramInfoLog(prog, sizeof(log), 0, log);
		SDL_Log("postfx: %s: %s", name, log);
		p_glDeleteProgram(prog);
		return 0;
	}

	return prog;
}


static const char *stretch_source =
    "#if defined(VERTEX)\n"
    "in vec4 VertexCoord;\n"
    "in vec4 TexCoord;\n"
    "out vec2 vTexCoord;\n"
    "uniform mat4 MVPMatrix;\n"
    "void main() { gl_Position = MVPMatrix * VertexCoord; vTexCoord = TexCoord.xy; }\n"
    "#else\n"
    "in vec2 vTexCoord;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D Texture;\n"
    "void main() { FragColor = vec4(texture(Texture, vTexCoord).rgb, 1.0); }\n"
    "#endif\n";

static GLuint stretch_program = 0;


struct Pass {
	const char *file;
	int scale;	/* output size relative to the input; 0 = straight to the window
			   at its final size (only as the last pass) */
};

struct Filter {
	const char *name;
	int npasses;
	Pass passes[MAX_PASSES];
	bool lut;	/* hq2x: pass 2 also reads the original image and a lookup table */
	GLuint programs[MAX_PASSES];
	bool loaded, failed;
};

static Filter filters[N_FILTERS] = {
	{"OFF", 0, {{0, 0}, {0, 0}}, false, {0, 0}, false, false},
	{"SCALE2X", 1, {{"shaders/scale2x.glsl", 2}, {0, 0}}, false, {0, 0}, false, false},
	{"2XSAI", 1, {{"shaders/2xsai.glsl", 2}, {0, 0}}, false, {0, 0}, false, false},
	{"HQ2X", 2, {{"shaders/hqx-pass1.glsl", 1}, {"shaders/hqx-pass2.glsl", 2}}, true, {0, 0}, false, false},
	{"XBR", 1, {{"shaders/xbr-lv2.glsl", 2}, {0, 0}}, false, {0, 0}, false, false},
	{"OMNISCALE", 1, {{"shaders/omniscale.glsl", 0}, {0, 0}}, false, {0, 0}, false, false},
	{"LANCZOS", 1, {{"shaders/lanczos3.glsl", 0}, {0, 0}}, false, {0, 0}, false, false},
};


const char *postfx_filter_name(int filter)
{
	if (filter < 0 || filter >= N_FILTERS)
		return "OFF";

	return filters[filter].name;
}


static GLuint load_lut(const char *name)
{
	SDL_Surface *sfc = SDL_LoadPNG(name);
	SDL_Surface *rgba;
	GLuint tex = 0;

	if (sfc == 0) {
		SDL_Log("postfx: can't load %s: %s", name, SDL_GetError());
		return 0;
	}

	rgba = SDL_ConvertSurface(sfc, SDL_PIXELFORMAT_ABGR8888);	/* R,G,B,A bytes */
	SDL_DestroySurface(sfc);

	if (rgba == 0)
		return 0;

	/* Uploaded top row first, as RetroArch does: the shader indexes it that way. */
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, rgba->pitch / 4);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rgba->w, rgba->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba->pixels);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
	SDL_DestroySurface(rgba);

	return tex;
}


static bool load_filter(Filter *f)
{
	int i;

	if (f->loaded)
		return true;

	if (f->failed || !load_gl())
		return false;

	f->failed = true;	/* until proven otherwise */

	if (stretch_program == 0) {
		/* Built from memory rather than a file, but through the same path. */
		GLuint vs = compile(GL_VERTEX_SHADER, "#define VERTEX\n", stretch_source, "stretch");
		GLuint fs = compile(GL_FRAGMENT_SHADER, "#define FRAGMENT\n", stretch_source, "stretch");
		GLint ok = 0;

		if (vs == 0 || fs == 0)
			return false;

		stretch_program = p_glCreateProgram();
		p_glAttachShader(stretch_program, vs);
		p_glAttachShader(stretch_program, fs);
		p_glBindAttribLocation(stretch_program, 0, "VertexCoord");
		p_glBindAttribLocation(stretch_program, 1, "TexCoord");
		p_glLinkProgram(stretch_program);
		p_glDeleteShader(vs);
		p_glDeleteShader(fs);
		p_glGetProgramiv(stretch_program, GL_LINK_STATUS, &ok);

		if (!ok) {
			p_glDeleteProgram(stretch_program);
			stretch_program = 0;
			return false;
		}
	}

	for (i = 0;i < f->npasses;i++) {
		f->programs[i] = load_program(f->passes[i].file);

		if (f->programs[i] == 0)
			return false;
	}

	if (f->lut && lut_tex == 0) {
		lut_tex = load_lut("shaders/hq2x-lut.png");

		if (lut_tex == 0)
			return false;
	}

	f->failed = false;
	f->loaded = true;
	return true;
}


bool postfx_set_filter(int filter)
{
	if (filter <= FILTER_OFF || filter >= N_FILTERS) {
		video_filter = FILTER_OFF;
		return true;
	}

	if (!load_filter(&filters[filter])) {
		SDL_Log("postfx: filter %s unavailable, using OFF", filters[filter].name);
		video_filter = FILTER_OFF;
		return false;
	}

	video_filter = filter;
	return true;
}


bool postfx_active(void)
{
	return video_filter != FILTER_OFF && filters[video_filter].loaded;
}


/* Draws a textured quad covering the current viewport with prog. */
static void draw_quad(GLuint prog, GLuint tex, int tex_w, int tex_h, int out_w, int out_h)
{
	static const GLfloat vertices[] = { -1, -1, 0, 1,   1, -1, 0, 1,   1, 1, 0, 1,   -1, 1, 0, 1 };
	static const GLfloat texcoords[] = { 0, 0,   1, 0,   1, 1,   0, 1 };
	static const GLfloat identity[] = { 1, 0, 0, 0,   0, 1, 0, 0,   0, 0, 1, 0,   0, 0, 0, 1 };
	GLint loc;

	p_glUseProgram(prog);

	p_glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);

	/* Every sampler name these shaders use for their input, on unit 0. */
	if ((loc = p_glGetUniformLocation(prog, "Texture")) >= 0)
		p_glUniform1i(loc, 0);

	if ((loc = p_glGetUniformLocation(prog, "decal")) >= 0)
		p_glUniform1i(loc, 0);

	if ((loc = p_glGetUniformLocation(prog, "MVPMatrix")) >= 0)
		p_glUniformMatrix4fv(loc, 1, GL_FALSE, identity);

	if ((loc = p_glGetUniformLocation(prog, "TextureSize")) >= 0)
		p_glUniform2f(loc, tex_w, tex_h);

	if ((loc = p_glGetUniformLocation(prog, "InputSize")) >= 0)
		p_glUniform2f(loc, tex_w, tex_h);

	if ((loc = p_glGetUniformLocation(prog, "OutputSize")) >= 0)
		p_glUniform2f(loc, out_w, out_h);

	if ((loc = p_glGetUniformLocation(prog, "OrigTexture")) >= 0) {
		p_glUniform1i(loc, 1);
		p_glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, orig_tex);
	}

	if ((loc = p_glGetUniformLocation(prog, "LUT")) >= 0) {
		p_glUniform1i(loc, 2);
		p_glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, lut_tex);
	}

	p_glActiveTexture(GL_TEXTURE0);

	p_glEnableVertexAttribArray(0);
	p_glEnableVertexAttribArray(1);
	p_glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, vertices);
	p_glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, texcoords);
	p_glVertexAttrib4f(2, 1, 1, 1, 1);

	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	p_glDisableVertexAttribArray(0);
	p_glDisableVertexAttribArray(1);
}


void postfx_draw(unsigned int tex, int tex_w, int tex_h, int x, int y, int w, int h)
{
	Target *targets = pass_target[tex_w < 640 ? 0 : 1];
	Filter *f;
	GLuint src;
	int i, src_w, src_h;

	if (!postfx_active())
		return;

	f = &filters[video_filter];
	orig_tex = tex;
	src = tex;
	src_w = tex_w;
	src_h = tex_h;

	for (i = 0;i < f->npasses;i++) {
		Target *t = &targets[i];
		int tw = src_w * f->passes[i].scale, th = src_h * f->passes[i].scale;

		if (f->passes[i].scale == 0)
			break;

		if (t->w != tw || t->h != th) {
			delete_target(t);

			if (!create_target(t, tw, th)) {
				delete_target(t);
				video_filter = FILTER_OFF;
				return;
			}
		}

		p_glBindFramebuffer(GL_FRAMEBUFFER, t->fbo);
		glViewport(0, 0, t->w, t->h);
		draw_quad(f->programs[i], src, src_w, src_h, t->w, t->h);

		src = t->tex;
		src_w = t->w;
		src_h = t->h;
	}

	p_glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(x, y, w, h);

	if (i < f->npasses) {
		/* Resolution-independent filter: renders the window directly. */
		draw_quad(f->programs[i], src, src_w, src_h, w, h);
	} else {
		/* Fixed-scale filter: stretch its output to the window (bilinear). */
		glBindTexture(GL_TEXTURE_2D, src);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		draw_quad(stretch_program, src, src_w, src_h, w, h);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	p_glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}


void postfx_release(void)
{
	int i, j;

	if (!gl_available)
		return;

	for (i = 0;i < N_FILTERS;i++) {
		for (j = 0;j < filters[i].npasses;j++) {
			if (filters[i].programs[j])
				p_glDeleteProgram(filters[i].programs[j]);

			filters[i].programs[j] = 0;
		}

		filters[i].loaded = false;
	}

	if (stretch_program)
		p_glDeleteProgram(stretch_program);

	stretch_program = 0;

	if (lut_tex)
		glDeleteTextures(1, &lut_tex);

	lut_tex = 0;

	for (i = 0;i < MAX_PASSES;i++) {
		delete_target(&pass_target[0][i]);
		delete_target(&pass_target[1][i]);
	}

	video_filter = FILTER_OFF;
}
