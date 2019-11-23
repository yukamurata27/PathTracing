#ifndef SPHEREH
#define SPHEREH

#include "hittable.h"

class sphere: public hittable {
	public:
		sphere() {}
		sphere(vec3 cen, float r): center(cen), radius(r) {};

		// Keep same prototype as the actual virtual function
		virtual bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const;

		vec3 center;
		float radius;
};

bool sphere::hit(const ray& r, float t_min, float t_max, hit_record& rec) const {
	vec3 co = r.origin() - center;
	float a = dot(r.direction(), r.direction());
	float b = dot(co, r.direction()); // 2b
	float c = dot(co, co) - radius*radius;
	float discriminant = b*b - a*c; // b was even, so canceled them

	// When there are 2 intersections (not touch but intersect)
	if (discriminant > 0) {
		// Check first intersection (negative solution is closer to camera than positive one)
		float temp = (-b - sqrt(discriminant)) / a;
		if (t_min < temp && temp < t_max) {
			rec.t = temp;
			rec.p = r.point_at_parameter(rec.t);
			rec.normal = (rec.p - center) / radius; // normalized
			return true;
		}

		// Check second intersection
		temp = (-b + sqrt(discriminant)) / a;
		if (t_min < temp && temp < t_max) {
			rec.t = temp;
			rec.p = r.point_at_parameter(rec.t);
			rec.normal = (rec.p - center) / radius; // normalized
			return true;
		}
	}

	return false;
}

#endif