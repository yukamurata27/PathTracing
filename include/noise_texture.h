#ifndef NOISETEXTUREH
#define NOISETEXTUREH

#include "texture.h"
#include "perlin.h"

class noise_texture : public texture {
	public:
		noise_texture() {}
		noise_texture(float sc=1.0) : scale(sc) {}

		virtual vec3 value(float u, float v, const vec3& p) const {
			// Larger scale, more noise (b/c p jumps more)
			//return vec3(1,1,1) * noise.noise(scale * p);

			// Use turbulence directly
			//return vec3(1,1,1) * noise.turb(scale * p);

			// Use turbulence indirectly
			// -> Make color proportional to something like a sine function,
			// and use turbulence to adjust the phase - so it shifts x in sin(x) - which makes the stripes undulate
			return vec3(1,1,1) * 0.5 * (1 + sin(scale*p.z() + 10*noise.turb(p)));
		}

		perlin noise;
		float scale;
};

#endif
