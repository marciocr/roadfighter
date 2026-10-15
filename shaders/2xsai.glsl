/*
   2xSaI (2x Scale and Interpolation)

   GLSL port of the 2xSaI algorithm by Derek Liauw Kie Fa (Kreed), 1999,
   released under the GNU GPL. Written for the F-1 Spirit remake and shared
   with Maze of Galious; distributed under the GNU General Public License,
   version 2 or (at your option) any later version.

   Uses the libretro/RetroArch GLSL shader conventions (VertexCoord, TexCoord,
   MVPMatrix, TextureSize, Texture) so it loads like the other filters.

   For every source pixel A, 2xSaI looks at its 4x4 neighbourhood and decides
   the 2x2 output block:

       I E F J        A      | product
       G A B K        -------+---------
       H C D L        product1 | product2
       M N O P
*/

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

vec2 base;
vec2 texel;

/* (x, y) in screen terms: +x is right, +y is down. GL textures are stored
   bottom-up, so "down" is -y in texture coordinates. */
vec3 px(float x, float y)
{
	return texture(Texture, base + vec2(x, -y) * texel).rgb;
}

int result(vec3 a, vec3 b, vec3 c, vec3 d)
{
	int x = 0, y = 0, r = 0;

	if (a == c) x++; else if (b == c) y++;
	if (a == d) x++; else if (b == d) y++;
	if (x <= 1) r++;
	if (y <= 1) r--;

	return r;
}

void main()
{
	texel = 1.0 / TextureSize;
	vec2 pos = vTexCoord * TextureSize;
	base = (floor(pos) + 0.5) * texel;

	vec3 I = px(-1.0, -1.0), E = px(0.0, -1.0), F = px(1.0, -1.0), J = px(2.0, -1.0);
	vec3 G = px(-1.0,  0.0), A = px(0.0,  0.0), B = px(1.0,  0.0), K = px(2.0,  0.0);
	vec3 H = px(-1.0,  1.0), C = px(0.0,  1.0), D = px(1.0,  1.0), L = px(2.0,  1.0);
	vec3 M = px(-1.0,  2.0), N = px(0.0,  2.0), O = px(1.0,  2.0);

	vec3 product, product1, product2;

	if (A == D && B != C) {
		if ((A == E && B == L) || (A == C && A == F && B != E && B == J))
			product = A;
		else
			product = (A + B) * 0.5;

		if ((A == G && C == O) || (A == B && A == H && G != C && C == M))
			product1 = A;
		else
			product1 = (A + C) * 0.5;

		product2 = A;
	} else if (B == C && A != D) {
		if ((B == F && A == H) || (B == E && B == D && A != F && A == I))
			product = B;
		else
			product = (A + B) * 0.5;

		if ((C == H && A == F) || (C == G && C == D && A != H && A == I))
			product1 = C;
		else
			product1 = (A + C) * 0.5;

		product2 = B;
	} else if (A == D && B == C) {
		if (A == B) {
			product = A;
			product1 = A;
			product2 = A;
		} else {
			int r = 0;

			product1 = (A + C) * 0.5;
			product = (A + B) * 0.5;

			r += result(A, B, G, E);
			r -= result(B, A, K, F);
			r -= result(B, A, H, N);
			r += result(A, B, L, O);

			if (r > 0)
				product2 = A;
			else if (r < 0)
				product2 = B;
			else
				product2 = (A + B + C + D) * 0.25;
		}
	} else {
		product2 = (A + B + C + D) * 0.25;

		if (A == C && A == F && B != E && B == J)
			product = A;
		else if (B == E && B == D && A != F && A == I)
			product = B;
		else
			product = (A + B) * 0.5;

		if (A == B && A == H && G != C && C == M)
			product1 = A;
		else if (C == G && C == D && A != H && A == I)
			product1 = C;
		else
			product1 = (A + C) * 0.5;
	}

	/* Which of the 2x2 output pixels this fragment is (screen terms). */
	bool right = fract(pos.x) >= 0.5;
	bool bottom = fract(pos.y) < 0.5;
	vec3 c;

	if (!bottom)
		c = right ? product : A;
	else
		c = right ? product2 : product1;

	FragColor = vec4(c, 1.0);
}

#endif
