#ifndef LAMBERTIANH
#define LAMBERTIANH

#include "material.h"
#include "../onb.h"

class lambertian : public material {
	public:
		lambertian(texture *a, bool texture_map=false) : albedo(a), image_texture(texture_map) {}

		virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& alb, ray& scattered, float& pdf) const {
			onb uvw; // w is normal direction
			uvw.build_from_w(rec.normal);
			vec3 direction = uvw.local(random_cosine_direction());
			
			scattered = ray(rec.p, unit_vector(direction), r_in.time());
			alb = albedo->value(rec.u, rec.v, rec.p);
			
			pdf = dot(uvw.w(), scattered.direction()) / M_PI; // Why not 1/(2*PI) now??
			
			return true;
		}

		float scattering_pdf(const ray& r_in, const hit_record& rec, const ray& scattered) const {
			float cosine = dot(rec.normal, unit_vector(scattered.direction()));
			if (cosine < 0) return 0;
			return cosine / M_PI;
		}

		bool image_texture;
		texture *albedo;
};

#endif
