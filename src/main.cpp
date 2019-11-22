/* C++ standard libraries */
#include <iostream> // cout
#include <fstream>  // file i/o

/* Other headers */

using namespace std;

/*
 * main
 *
 * Under src directory...
 * Compile: g++ -std=c++11 main.cpp -o main
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

	for (int j = ny-1; j >= 0; j--) {
		for (int i = 0; i < nx; i++) {
			float r = float(i) / float(nx);
			float g = float(j) / float(ny);
			float b = 0.2;
			int ir = int(255.99 * r);
			int ig = int(255.99 * g);
			int ib = int(255.99 * b);
			outfile << ir << " " << ig << " " << ib << "\n";
		}
	}

	outfile.close();

	cout << "Path Tracer Completed!" << endl;
	return 0;
}

