/*
   Lanczos-3 resampling

   Written for the F-1 Spirit remake and shared with Maze of Galious;
   distributed under the GNU General Public License, version 2 or (at your
   option) any later version.

   Uses the libretro/RetroArch GLSL shader conventions (VertexCoord, TexCoord,
   MVPMatrix, TextureSize, Texture) so it loads like the other filters. Runs
   at the final output size: each output pixel is a 6x6 windowed-sinc
   (a = 3) weighted sum of the source pixels around it.

   ANTI_RINGING limits the result to the range of the 2x2 nearest source
   pixels (1.0 = fully), which removes the bright/dark halos plain Lanczos
   leaves around hard edges such as the game's text.
*/

#define ANTI_RINGING 1.0

#if defined(VERTEX)

in vec4 VertexCoord;
in vec4 TexCoord;
out vec2 vTexCoord;

uniform mat4 MVPMatrix;

void main()
{
	gl_Position = MVPMatrix * VertexCoord;
	vTexCoord = TexCoord.xy;
}

#elif defined(FRAGMENT)

in vec2 vTexCoord;
out vec4 FragColor;

uniform vec2 TextureSize;
uniform sampler2D Texture;

#define PI 3.14159265358979

float lanczos(float x)
{
	if (abs(x) < 1e-5)
		return 1.0;

	if (abs(x) >= 3.0)
		return 0.0;

	float px = PI * x;
	return 3.0 * sin(px) * sin(px / 3.0) / (px * px);
}

void main()
{
	vec2 texel = 1.0 / TextureSize;
	vec2 pos = vTexCoord * TextureSize - 0.5;
	vec2 base = floor(pos);
	vec2 f = pos - base;

	vec3 sum = vec3(0.0);
	float wsum = 0.0;
	vec3 lo = vec3(1.0), hi = vec3(0.0);

	for (int j = -2; j <= 3; j++) {
		float wy = lanczos(float(j) - f.y);

		for (int i = -2; i <= 3; i++) {
			vec3 c = texture(Texture, (base + vec2(i, j) + 0.5) * texel).rgb;
			float w = lanczos(float(i) - f.x) * wy;

			sum += c * w;
			wsum += w;

			if ((i == 0 || i == 1) && (j == 0 || j == 1)) {
				lo = min(lo, c);
				hi = max(hi, c);
			}
		}
	}

	vec3 c = sum / wsum;
	c = mix(c, clamp(c, lo, hi), ANTI_RINGING);

	FragColor = vec4(clamp(c, 0.0, 1.0), 1.0);
}

#endif
