/* C++ standard libraries */
#include <iostream> // cout
#include <fstream>  // file i/o

/* Other headers */
// Include a header once once within a project!
#include "float.h"
#include "../include/hittable_list.h"
#include "../include/sphere.h"
#include "../include/camera.h"
#include "../include/random.h"

using namespace std;

vec3 color(const ray& r, hittable * world);
bool hit_sphere(const vec3& center, float radius, const ray& r);
vec3 random_in_unit_sphere();

/*
 * main
 *
 * Under src directory...
 * Compile: g++ -std=c++11 main.cpp vec3.cpp -o main
 * Run:     ./main
 *
*/
int main(int argc, char * argv[]) {
	cout << "Hello World!" << endl;

	//if ( argc == 1 ) {
	//	cout << "Expected: ./main obj_file_name" << endl;
	//	return 0;
	//}

	ofstream outfile;
	int nx = 200;
	int ny = 100;
	int ns = 100;

	outfile.open ("output.ppm");
	outfile << "P3\n" << nx << " " << ny << "\n255\n";

	vec3 lower_left_corner(-2.0, -1.0, -1.0);
	vec3 horizontal(4.0, 0.0, 0.0); // step interval
	vec3 vertical(0.0, 2.0, 0.0);   // step interval
	vec3 origin(0.0, 0.0, 0.0);

	hittable *list[2];
	list[0] = new sphere(vec3(0, 0, -1), 0.5);
	list[1] = new sphere(vec3(0, -100.5, -1), 100);
	hittable *world = new hittable_list(list, 2);
	camera cam;

	// Send a ray out of eye (0, 0, 0) from BL to UR corner
	for (int j = ny-1; j >= 0; j--) {
		for (int i = 0; i < nx; i++) {
			// Super sampling
			vec3 col(0, 0, 0);
			for (int s = 0; s < ns; s++) {
				// Offset by random_double value [0, 1)
				float u = float(i + random_double()) / float(nx);
				float v = float(j + random_double()) / float(ny);
				ray r = cam.get_ray(u, v);
				col += color(r, world);
			}
			col /= float(ns); // average sum
			// gamma correction (brighter color)
			col = vec3( sqrt(col[0]), sqrt(col[1]), sqrt(col[2]) );

			int ir = int(255.99	* col[0]);
			int ig = int(255.99	* col[1]);
			int ib = int(255.99	* col[2]);

			outfile << ir << " " << ig << " " << ib << "\n";
		}
	}

	outfile.close();

	cout << "Path Tracer Completed!" << endl;
	return 0;
}

vec3 color(const ray& r, hittable * world) {
	hit_record rec;
	if (world->hit(r, 0.001, MAXFLOAT, rec)) {
		// Bounce the ray
		// Sphere center at rec.p + rec.normal
		vec3 target = rec.p + rec.normal + random_in_unit_sphere();
		// 50% reflectors
        return 0.5 * color(ray(rec.p, target - rec.p), world);
	} else {
		// Sky color is a linear interpolation b/w while & blue

		vec3 unit_direction = unit_vector(r.direction());

		// t is a mapped y from [-1, 1] to [0, 1]
		// more accurately, t = 0.5 * (sqrt(2)*unit_direction.y() + 1.0)
		float t = 0.5 * (unit_direction.y() + 1.0);

		return (1.0-t)*vec3(1.0, 1.0, 1.0) + t*vec3(0.5, 0.7, 1.0);
	}
}

bool hit_sphere(const vec3& center, float radius, const ray& r) {
	// Solve t^2 dot(B,B) + 2t dot(B,A-C) + dot(A-C, A-C)-R^2 = 0
	vec3 co = r.origin() - center; // A-C
	float a = dot(r.direction(), r.direction());
	float b = 2.0 * dot(r.direction(), co);
	float c = dot(co, co) - radius*radius;

	float discriminant = b*b - 4*a*c;
	return (discriminant > 0);
}

vec3 random_in_unit_sphere() {
	vec3 p;

	// Keep getting a random point on a unit sphere until we get one
	do {
		// map from [0, 1) to [-1, 1) where -vec3(1,1,1) is offset
		p = 2.0*vec3(random_double(), random_double(), random_double()) - vec3(1,1,1);
	} while (p.squared_length() >= 1.0);

	return p;
}

