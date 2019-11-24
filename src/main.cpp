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

vec3 color(const ray& r, hittable *world, int depth);
bool hit_sphere(const vec3& center, float radius, const ray& r);
vec3 random_in_unit_sphere();
vec3 reflect(const vec3& v, const vec3& n);
bool refract(const vec3& v, const vec3& n, float ni_over_nt, vec3& refracted);
float schlick(float cosine, float ref_idx);

class material {
	public:
		virtual bool scatter(
			const ray& r_in,
			const hit_record& rec,
			vec3& attenuation,
			ray& scattered) const = 0;
};

class lambertian : public material {
	public:
		lambertian(const vec3& a) : albedo(a) {}

		virtual bool scatter(const ray& r_in,
							const hit_record& rec,
							vec3& attenuation,
							ray& scattered) const {
			vec3 target = rec.p + rec.normal + random_in_unit_sphere();
			scattered = ray(rec.p, target-rec.p);
			attenuation = albedo;
			return true;
		}

		vec3 albedo;
};

class metal : public material {
	public:
		metal(const vec3& a, float f) : albedo(a) {
			if (f < 1) fuzz = f; else fuzz = 1;
		}

		virtual bool scatter(const ray& r_in, const hit_record& rec,
							 vec3& attenuation, ray& scattered) const {
			vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
			scattered = ray(rec.p, reflected);
			attenuation = albedo;
			return (dot(scattered.direction(), rec.normal) > 0);
		}

		vec3 albedo;
		float fuzz;
};

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

	hittable *list[5];
	list[0] = new sphere(vec3(0,0,-1), 0.5, new lambertian(vec3(0.1, 0.2, 0.5)));
	list[1] = new sphere(vec3(0,-100.5,-1), 100, new lambertian(vec3(0.8, 0.8, 0.0)));
	list[2] = new sphere(vec3(1,0,-1), 0.5, new metal(vec3(0.8, 0.6, 0.2), 0.3));
	list[3] = new sphere(vec3(-1,0,-1), 0.5, new dielectric(1.5));
	list[4] = new sphere(vec3(-1,0,-1), -0.45, new dielectric(1.5));
	hittable *world = new hittable_list(list, 5);

	vec3 lookfrom(3,3,2);
	vec3 lookat(0,0,-1);
	float dist_to_focus = (lookfrom-lookat).length();
	float aperture = 2.0;
	camera cam(lookfrom, lookat, vec3(0,1,0), 20, float(nx)/float(ny), aperture, dist_to_focus);

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
				col += color(r, world, 0);
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

vec3 color(const ray& r, hittable *world, int depth) {
	hit_record rec;
	if (world->hit(r, 0.001, MAXFLOAT, rec)) {
		ray scattered;
		vec3 attenuation;
		
		if (depth < 50 && rec.mat_ptr->scatter(r, rec, attenuation, scattered))
			return attenuation*color(scattered, world, depth+1);
		else
			return vec3(0,0,0);			
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

bool refract(const vec3& v, const vec3& n, float ni_over_nt, vec3& refracted) {
	// x component of unit(v):
	// 		dot(unit(n), N)^2 + x^2 = 1^2
	//		x^2 = 1 - dot(unit(n), N)^2
	//		x = sqrt( 1 - dot(unit(n), N)^2 )
	//		=> sin(theta) = sqrt( 1 - dot(unit(n), N)^2 )
	//
	// Snell's law: ni*sin(theta) = nt*sin(theta')
	//		sin(theta') = ni/nt * sqrt( 1 - dot(unit(n), N)^2 )
	//		x' = ni/nt * sqrt( 1 - dot(unit(n),N)^2 )
	//
	// y' component of refracted unit vector
	//		x'^2 + y'^2 = 1
	//		y'^2 = 1 - x'^2
	//		y' = sqrt( 1 - x'^2 )
	//		=> discriminant is inside of this sqrt: 1 - x'^2
	vec3 uv = unit_vector(v);
	float dt = dot(uv, n);
	float discriminant = 1.0 - ni_over_nt*ni_over_nt*(1-dt*dt);
	if (discriminant > 0) {
		// isn't it below?
		// refracted = ni_over_nt*sqrt(1-dt*dt) - n*sqrt(discriminant);
		refracted = ni_over_nt*(uv - n*dt) - n*sqrt(discriminant);
		return true;
	}
	else return false;
}

vec3 reflect(const vec3& v, const vec3& n) {
	// R+L = 2*dot(L, N)*N
	// R = -L + 2*dot(L, N)*N where V = -L
	// R = V - 2*dot(V, N)*N
	return v - 2*dot(v,n)*n;
}

// Approximation of reflection-refraction by Christophe Schlick
float schlick(float cosine, float ref_idx) {
    float r0 = (1-ref_idx) / (1+ref_idx);
    r0 = r0*r0;
    return r0 + (1-r0)*pow((1 - cosine),5);
}




