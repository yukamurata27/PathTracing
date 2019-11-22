/* C++ standard libraries */
#include <iostream> // cout
#include <fstream>  // file i/o

/* Other headers */
// Include a header once once within a project!
#include "../include/ray.h"

using namespace std;

vec3 color(const ray& r);

/*
 * main
 *
 * Under src directory...
 * Compile: g++ -std=c++11 main.cpp vec3.cpp -o main
 * Run:     ./main
 *
*/
int main(int argc, char * argv[])
{
	cout << "Hello World!" << endl;

	//if ( argc == 1 ) {
	//	cout << "Expected: ./main obj_file_name" << endl;
	//	return 0;
	//}

	ofstream outfile;
	int nx = 200;
	int ny = 100;

	outfile.open ("output.ppm");
	outfile << "P3\n" << nx << " " << ny << "\n255\n";

	vec3 lower_left_corner(-2.0, -1.0, -1.0);
	vec3 horizontal(4.0, 0.0, 0.0); // step interval
	vec3 vertical(0.0, 2.0, 0.0);   // step interval
	vec3 origin(0.0, 0.0, 0.0);

	// Send a ray out of eye (0, 0, 0) from BL to UR corner
	for (int j = ny-1; j >= 0; j--) {
		for (int i = 0; i < nx; i++) {
			float u = float(i) / float(nx);
			float v = float(j) / float(ny);
			ray r(origin, lower_left_corner + u*horizontal + v*vertical);
			vec3 col = color(r);

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

vec3 color(const ray& r) {
	// Sky color is a linear interpolation b/w while & blue

	vec3 unit_direction = unit_vector(r.direction());

	// t is a mapped y from [-1, 1] to [0, 1]
	// more accurately, t = 0.5 * (sqrt(2)*unit_direction.y() + 1.0)
	float t = 0.5 * (unit_direction.y() + 1.0);

	return (1.0-t)*vec3(1.0, 1.0, 1.0) + t*vec3(0.5, 0.7, 1.0);
}

