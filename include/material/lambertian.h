#ifndef LAMBERTIANH
#define LAMBERTIANH

#include "material.h"

class lambertian : public material {
	public:
		lambertian(texture *a, bool texture_map=false) : albedo(a), image_texture(texture_map) {}

		virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered) const {
			vec3 target = rec.p + rec.normal + random_in_unit_sphere();
			scattered = ray(rec.p, target-rec.p, r_in.time());
			//attenuation = albedo->value(0, 0, rec.p);
			if (image_texture) attenuation = albedo->value(rec.u, rec.v, rec.p);
			else attenuation = albedo->value(0, 0, rec.p);

			return true;
		}

		bool image_texture;
		texture *albedo;
};

#endif
