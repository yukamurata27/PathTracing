/* C++ standard libraries */
#include <iostream> // cout
#include <fstream>  // file i/o

/* Other headers */
// Include a header once once within a project!
#include "float.h"
#include "../include/hittable_list.h"
#include "../include/sphere.h"
#include "../include/moving_sphere.h"
#include "../include/xy_rect.h"
#include "../include/yz_rect.h"
#include "../include/xz_rect.h"
#include "../include/flip_normals.h"

#include "../include/camera.h"
#include "../include/random.h"
#include "../include/constant_texture.h"
#include "../include/checker_texture.h"
#include "../include/noise_texture.h"
#include "../include/image_texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../libs/stb/stb_image.h"


using namespace std;


/* Global variables */
bool texture_map;
enum scene { random_s, moving_spheres_zoomin_s, two_spheres_s, two_perlin_spheres_s, image_texture_s, simple_light_s, cornell_box_s };


/* Function prototypes */
hittable *get_world(scene s);
camera set_camera(scene s, int nx, int ny);
vec3 color(const ray& r, hittable *world, int depth);
bool hit_sphere(const vec3& center, float radius, const ray& r);
vec3 random_in_unit_sphere();
vec3 reflect(const vec3& v, const vec3& n);
bool refract(const vec3& v, const vec3& n, float ni_over_nt, vec3& refracted);
float schlick(float cosine, float ref_idx);
hittable *random_scene();
hittable *moving_spheres_zoomin();
hittable *two_spheres();
hittable *two_perlin_spheres();
hittable *image_textured_spheres();
hittable *simple_light();
hittable *cornell_box();


/* Class definitions */
class material {
	public:
		virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered) const = 0;
		virtual vec3 emitted(float u, float v, const vec3& p) const {
			return vec3(0,0,0);
		}
};

class lambertian : public material {
	public:
		lambertian(texture *a) : albedo(a) {}

		virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered) const {
			vec3 target = rec.p + rec.normal + random_in_unit_sphere();
			scattered = ray(rec.p, target-rec.p, r_in.time());
			//attenuation = albedo->value(0, 0, rec.p);
			if (texture_map) attenuation = albedo->value(rec.u, rec.v, rec.p);
			else attenuation = albedo->value(0, 0, rec.p);

			return true;
		}

		texture *albedo;
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

class diffuse_light : public material {
	public:
		diffuse_light(texture *a) : emit(a) {}

		virtual bool scatter(const ray& r_in, const hit_record& rec,
			vec3& attenuation, ray& scattered) const { return false; }
		virtual vec3 emitted(float u, float v, const vec3& p) const {
			return emit->value(u, v, p);
		}

		texture *emit;
};


/*
 * main
 *
 * Under src directory...
 * Compile: g++ -std=c++11 main.cpp vec3.cpp -o main
 * Run:	    ./main
 *
*/
int main(int argc, char * argv[]) {
	cout << "Rendering begins..." << endl;

	//if ( argc == 1 ) {
	//	cout << "Expected: ./main obj_file_name" << endl;
	//	return 0;
	//}

	// Choose from { random_s, moving_spheres_zoomin_s, two_spheres_s, two_perlin_spheres_s, image_texture_s, simple_light_s }
	scene s = cornell_box_s;
	texture_map = s == image_texture_s ? true : false;

	int nx, ny;
	if (s == cornell_box_s) {
		nx = 300;
		ny = 300;
	} else {
		nx = 500; //200;
		ny = 300; //100;
	}
	int ns = 100;

	ofstream outfile;
	outfile.open ("../rendered_img/output.ppm");
	outfile << "P3\n" << nx << " " << ny << "\n255\n";

	vec3 lower_left_corner(-2.0, -1.0, -1.0);
	vec3 horizontal(4.0, 0.0, 0.0); // step interval
	vec3 vertical(0.0, 2.0, 0.0);   // step interval
	vec3 origin(0.0, 0.0, 0.0);

	hittable *world = get_world(s);
	camera cam = set_camera(s, nx, ny);

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

hittable *get_world(scene s) {
	switch(s)
	{
		case random_s:
			return random_scene();
		case moving_spheres_zoomin_s:
			return moving_spheres_zoomin();
		case two_spheres_s:
			return two_spheres();
		case two_perlin_spheres_s:
			return two_perlin_spheres();
		case image_texture_s:
			return image_textured_spheres();
		case simple_light_s:
			return simple_light();
		case cornell_box_s:
			return cornell_box();
	}
	
	// Default scene
	return two_spheres();
}

camera set_camera(scene s, int nx, int ny) {
	vec3 lookfrom, lookat;
	float dist_to_focus, aperture, vfov;

	switch(s)
	{
		case random_s:
			lookfrom = vec3(13,2,3);
			lookat = vec3(0,0,0);
			dist_to_focus = 10.0;
			aperture = 0.0;
			vfov = 20.0;
			return camera(lookfrom, lookat, vec3(0,1,0), vfov, float(nx)/float(ny), aperture, dist_to_focus, 0.0, 1.0);
		case moving_spheres_zoomin_s:
			lookfrom = vec3(3,3,2);
			lookat = vec3(0,0,-1);
			dist_to_focus = (lookfrom-lookat).length();
			aperture = 0.0;
			vfov = 20.0;
			return camera(lookfrom, lookat, vec3(0,1,0), vfov, float(nx)/float(ny), aperture, dist_to_focus, 0.0, 1.0);
		case two_spheres_s:
			lookfrom = vec3(13,2,3);
			lookat = vec3(0,0,0);
			dist_to_focus = 10.0;
			aperture = 0.0;
			vfov = 20.0;
			return camera(lookfrom, lookat, vec3(0,1,0), vfov, float(nx)/float(ny), aperture, dist_to_focus, 0.0, 1.0);
		case two_perlin_spheres_s:
			lookfrom = vec3(13,2,3);
			lookat = vec3(0,0,0);
			dist_to_focus = 10.0;
			aperture = 0.0;
			vfov = 20.0;
			return camera(lookfrom, lookat, vec3(0,1,0), vfov, float(nx)/float(ny), aperture, dist_to_focus, 0.0, 1.0);
		case image_texture_s:
			lookfrom = vec3(13,2,3);
			lookat = vec3(0,0,0);
			dist_to_focus = 10.0;
			aperture = 0.0;
			vfov = 20.0;
			return camera(lookfrom, lookat, vec3(0,1,0), vfov, float(nx)/float(ny), aperture, dist_to_focus, 0.0, 1.0);
		case simple_light_s:
			lookfrom = vec3(13,2,3);
			lookat = vec3(0,0,0);
			dist_to_focus = 10.0;
			aperture = 0.0;
			vfov = 20.0;
			return camera(lookfrom, lookat, vec3(0,1,0), vfov, float(nx)/float(ny), aperture, dist_to_focus, 0.0, 1.0);
		case cornell_box_s:
			lookfrom = vec3(278, 278, -800);
			lookat = vec3(278,278,0);
			dist_to_focus = 10.0;
			aperture = 0.0;
			vfov = 40.0;
			return camera(lookfrom, lookat, vec3(0,1,0), vfov, float(nx)/float(ny), aperture, dist_to_focus, 0.0, 1.0);
	}
	
	// Default camera
	lookfrom = vec3(13,2,3);
	lookat = vec3(0,0,0);
	dist_to_focus = 10.0;
	aperture = 0.0;
	return camera(lookfrom, lookat, vec3(0,1,0), 20, float(nx)/float(ny), aperture, dist_to_focus, 0.0, 1.0);
}

vec3 color(const ray& r, hittable *world, int depth) {
	hit_record rec;
	if (world->hit(r, 0.001, MAXFLOAT, rec)) {
		ray scattered;
		vec3 attenuation;
		vec3 emitted = rec.mat_ptr->emitted(rec.u, rec.v, rec.p); // Light emission
		
		// Simply add emission from the material to all
		if (depth < 50 && rec.mat_ptr->scatter(r, rec, attenuation, scattered))
			if (texture_map) return attenuation;
			else return emitted + attenuation*color(scattered, world, depth+1);
		else
			return emitted;		
	} else {
		return vec3(0,0,0);
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

hittable *random_scene() {
	int n = 8; // Only use mulptiole of 4
	int arr_size = pow(4, n/4)+4;
	hittable **list = new hittable*[arr_size];

	// Checker ground
	texture *checker = new checker_texture(
    						new constant_texture(vec3(0.2, 0.3, 0.1)),
    						new constant_texture(vec3(0.9, 0.9, 0.9)));
	list[0] = new sphere(vec3(0,-1000,0), 1000, new lambertian(checker));

	// Plane ground
	//list[0] =  new sphere(vec3(0,-1000,0), 1000, new lambertian(new constant_texture(vec3(0.5, 0.5, 0.5))));
	
	int i = 1;
	for (int a = -n/4; a < n/4; a++) {
		for (int b = -n/4; b < n/4; b++) {
			float choose_mat = random_double();
			vec3 center(a+2.5*random_double(),0.2,b+2.5*random_double());

			if ((center-vec3(4,0.2,0)).length() > 0.9) {
				if (choose_mat < 0.8) {  // diffuse
					list[i++] = new moving_sphere(
						center,
						center+vec3(0, 0.5*random_double(), 0),
						0.0, 1.0, 0.2,
						new lambertian(new constant_texture(
							vec3(random_double()*random_double(),
								random_double()*random_double(),
								random_double()*random_double())
						))
					);
				}
				else if (choose_mat < 0.95) { // metal
					list[i++] = new sphere(center, 0.2,
							new metal(vec3(0.5*(1 + random_double()),
										   0.5*(1 + random_double()),
										   0.5*(1 + random_double())),
									  0.5*random_double()));
				}
				else {  // glass
					list[i++] = new sphere(center, 0.2, new dielectric(1.5));
				}
			}
		}
	}

	list[i++] = new sphere(vec3(0, 1, 0), 1.0, new dielectric(1.5));
	list[i++] = new sphere(vec3(-4, 1, 0), 1.0, new lambertian(new constant_texture(vec3(0.4, 0.2, 0.1))));
	list[i++] = new sphere(vec3(4, 1, 0), 1.0, new metal(vec3(0.7, 0.6, 0.5), 0.0));

	return new hittable_list(list,i);
}

hittable *moving_spheres_zoomin() {
	hittable **list = new hittable*[4];
	list[0] = new moving_sphere(vec3(0,0,-1), vec3(0,0,-1)+vec3(0, 0.5*random_double(), 0), 0.0, 1.0, 0.2,
									new lambertian(new constant_texture(
											vec3(random_double()*random_double(),
											random_double()*random_double(),
											random_double()*random_double())
										))
					);
	list[1] = new moving_sphere(vec3(1,0,-1), vec3(1,0,-1)+vec3(0, 0.5*random_double(), 0), 0.0, 1.0, 0.2,
									new lambertian(new constant_texture(
											vec3(random_double()*random_double(),
											random_double()*random_double(),
											random_double()*random_double())
										))
					);
	list[2] = new moving_sphere(vec3(-1,0,-1), vec3(-1,0,-1)+vec3(0, 0.5*random_double(), 0), 0.0, 1.0, 0.2,
									new lambertian(new constant_texture(
											vec3(random_double()*random_double(),
											random_double()*random_double(),
											random_double()*random_double())
										))
					);
	list[3] = new sphere(vec3(0,-100.5,-1), 100, new lambertian(new constant_texture(vec3(0.5, 0.5, 0.5))));
	return new hittable_list(list, 4);
}

hittable *two_spheres() {
	texture *checker = new checker_texture(
		new constant_texture(vec3(0.2, 0.3, 0.1)),
		new constant_texture(vec3(0.9, 0.9, 0.9))
	);

	hittable **list = new hittable*[2];
	list[0] = new sphere(vec3(0,-10, 0), 10, new lambertian(checker));
	list[1] = new sphere(vec3(0, 10, 0), 10, new lambertian(checker));

	return new hittable_list(list, 2);
}

hittable *two_perlin_spheres() {
	texture *pertext = new noise_texture(4);
	hittable **list = new hittable*[2];
	list[0] = new sphere(vec3(0,-1000, 0), 1000, new lambertian(pertext));
	list[1] = new sphere(vec3(0, 2, 0), 2, new lambertian(pertext));
	return new hittable_list(list, 2);
}

hittable *image_textured_spheres() {
	int nx, ny, nn;
	unsigned char *tex_data = stbi_load("../texture_img/earthmap.jpg", &nx, &ny, &nn, 0);
	material *mat = new lambertian(new image_texture(tex_data, nx, ny));

	texture *pertext = new noise_texture(4);
	hittable **list = new hittable*[2];
	list[0] = new sphere(vec3(0,-1000, 0), 1000, new lambertian(pertext));
	list[1] = new sphere(vec3(0, 2, 0), 2, mat);
	return new hittable_list(list, 2);
}

hittable *simple_light() {
	texture *pertext = new noise_texture(4);
	hittable **list = new hittable*[4];
	list[0] = new sphere(vec3(0,-1000, 0), 1000, new lambertian(pertext));
	list[1] = new sphere(vec3(0, 2, 0), 2, new lambertian(pertext));
	list[2] = new sphere(vec3(0, 7, 0), 2, new diffuse_light(new constant_texture(vec3(4,4,4))));
    list[3] = new xy_rect(3, 5, 1, 3, -2, new diffuse_light(new constant_texture(vec3(4,4,4))));
	return new hittable_list(list,4);
}

hittable *cornell_box() {
	hittable **list = new hittable*[6];
	int i = 0;
	material *red = new lambertian(new constant_texture(vec3(0.65, 0.05, 0.05)));
	material *white = new lambertian(new constant_texture(vec3(0.73, 0.73, 0.73)));
	material *green = new lambertian(new constant_texture(vec3(0.12, 0.45, 0.15)));
	material *light = new diffuse_light(new constant_texture(vec3(15, 15, 15)));

	list[i++] = new flip_normals(new yz_rect(0, 555, 0, 555, 555, green));
	list[i++] = new yz_rect(0, 555, 0, 555, 0, red);
	list[i++] = new xz_rect(213, 343, 227, 332, 554, light);
	list[i++] = new flip_normals(new xz_rect(0, 555, 0, 555, 555, white));
	list[i++] = new xz_rect(0, 555, 0, 555, 0, white);
	list[i++] = new flip_normals(new xy_rect(0, 555, 0, 555, 555, white));

	return new hittable_list(list,i);
}




