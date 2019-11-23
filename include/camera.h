#ifndef CAMERAH
#define CAMERAH

#include "ray.h"

class camera {
	public:
		// Still stick to z=-1 plane
		camera(vec3 lookfrom, vec3 lookat, vec3 vup, float vfov, float aspect) {
			// vfov is top to bottom in degrees
			float theta = vfov * M_PI / 180;
			float half_height = tan(theta / 2);
			float half_width = aspect * half_height;

			vec3 u, v, w;
			origin = lookfrom;
			w = unit_vector(lookfrom - lookat); // depth
            u = unit_vector(cross(vup, w)); // x direction
            v = cross(w, u); // y direction (upvector)
			lower_left_corner = origin - half_width*u - half_height*v - w;
			horizontal = 2 * half_width * u;
			vertical = 2 * half_height * v;
		}

		ray get_ray(float u, float v) {
			return ray(origin, lower_left_corner + u*horizontal + v*vertical - origin);
		}

		vec3 origin;
		vec3 lower_left_corner;
		vec3 horizontal;
		vec3 vertical;
};

#endif