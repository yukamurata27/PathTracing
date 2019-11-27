#ifndef DIELECTRICH
#define DIELECTRICH

#include "material.h"

class dielectric : public material {
	public:
		dielectric(float ri) : ref_idx(ri) {}
		virtual bool scatter(const ray& r_in, const hit_record& rec,
							 vec3& attenuation, ray& scattered) const {

			vec3 outward_normal;
			vec3 reflected = reflect(r_in.direction(), rec.normal);
			float ni_over_nt;
			attenuation = vec3(1.0, 1.0, 1.0); // No color absorbs
			vec3 refracted;

			float reflect_prob;
			float schlick_cosine;

			if (dot(r_in.direction(), rec.normal) > 0) { // from inside to outside
				outward_normal = -rec.normal;
				ni_over_nt = ref_idx;
				schlick_cosine = ref_idx * dot(r_in.direction(), rec.normal) / r_in.direction().length();
			} else { // from outside to inside
				outward_normal = rec.normal;
				ni_over_nt = 1.0 / ref_idx; // ref_idx of air is close to 1.0
				schlick_cosine = -dot(r_in.direction(), rec.normal) / r_in.direction().length();
			}

			if (refract(r_in.direction(), outward_normal, ni_over_nt, refracted)) {
				// Randomize refraction and reflection nicely
				reflect_prob = schlick(schlick_cosine, ref_idx);
			}
			else reflect_prob = 1.0;

			// Based on the probability, return refracted or reflected ray
			if (random_double() < reflect_prob) {
			   scattered = ray(rec.p, reflected);
			}
			else {
			   scattered = ray(rec.p, refracted);
			}

			return true;
		}

		float ref_idx;
};

#endif
