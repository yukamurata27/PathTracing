#ifndef SPHEREH
#define SPHEREH

#include "hittable.h"

void get_sphere_uv(const vec3& p, float& u, float& v);

class sphere: public hittable {
	public:
		sphere() {}
		sphere(vec3 cen, float r, material *m)
			: center(cen), radius(r), mat_ptr(m) {};

		// Keep same prototype as the actual virtual function
		virtual bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const;
		virtual bool bounding_box(float t0, float t1, aabb& box) const;

		vec3 center;
		float radius;
		material *mat_ptr;
};

// Using namespace to point to the "hit" function in the namespace "sphere"
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
			rec.mat_ptr = mat_ptr;
			get_sphere_uv((rec.p-center)/radius, rec.u, rec.v);
			return true;
		}

		// Check second intersection
		temp = (-b + sqrt(discriminant)) / a;
		if (t_min < temp && temp < t_max) {
			rec.t = temp;
			rec.p = r.point_at_parameter(rec.t);
			rec.normal = (rec.p - center) / radius; // normalized
			rec.mat_ptr = mat_ptr;
			get_sphere_uv((rec.p-center)/radius, rec.u, rec.v);
			return true;
		}
	}

	return false;
}

bool sphere::bounding_box(float t0, float t1, aabb& box) const {
	box = aabb(
			center - vec3(radius, radius, radius),
			center + vec3(radius, radius, radius)
		  );
	return true;
}

void get_sphere_uv(const vec3& p, float& u, float& v) {
	float phi = atan2(p.z(), p.x());
	float theta = asin(p.y());
	u = 1-(phi + M_PI) / (2*M_PI);
	v = (theta + M_PI/2) / M_PI;
}

#endif
