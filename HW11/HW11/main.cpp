#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <math.h>
#include <vector>
#include <thread>
using namespace std;

void outputField(double** data, int ni, int nj);

// support for complex numbers
struct Complex {
	double r;
	double i;

	Complex(double a, double b) : r(a), i(b) { }

	double magnitude2() {return r * r + i * i;}

	Complex operator*(const Complex &a) {
		return Complex(r * a.r - i * a.i, i * a.r + r * a.i);
	}
	Complex operator+(const Complex &a) {
		return Complex(r + a.r, i + a.i);
	}
};

// computes value of julia set at [i,j]*/
double juliaValue(int i, int j, int ni, int nj)
{
	double fi = -1.0 + 2.0 * i / ni;	// fi = [-1:1)
	double fj = -1.0 + 2.0 * j / nj;	// fj = [-1:1)

	Complex c(-0.8, 0.156);	// coefficient for the image
	Complex a(fi, fj);		// pixel pos as a complex number
	
	int k;
	for (k = 0; k < 200; k++) {
		a = a*a +c;
		if (a.magnitude2() > 1000) break;	// check for divergence
	}
	return k;				// return 
}

void calculatePixels(int i_start, int i_end, int ni, int nj, double** julia) {
	for (int i = i_start; i < i_end; i++)
		for (int j = 0; j < nj; j++)
			julia[i][j] = juliaValue(i, j, ni, nj);
}

int main(int num_args, char **args)
{
	const int ni = 4000;
	const int nj = 4000;

	/*allocate memory for our domain*/
	double** julia = new double* [ni];
	for (int i = 0; i < ni; i++) julia[i] = new double[nj];
	
	//Threading
	int num_threads = thread::hardware_concurrency();
	if (num_args > 1) {
		num_threads = atoi(args[1]);
	}

	cout << "Running program with: " << num_threads << " threads\n";

	vector<thread> threads;
	threads.reserve(num_threads);


	/*start timing*/
	auto clock_start = chrono::high_resolution_clock::now();

	// compute pixels

	for (int t = 0; t < num_threads; t++) {
		int i_start = t * (ni / num_threads);
		int i_end = (t + 1) * (ni / num_threads);
		if (i_end > ni) i_end = ni;
		threads.emplace_back(calculatePixels, i_start, i_end, ni, nj, julia);
	}

	for (thread& t : threads) t.join();

	/*capture ending time*/
	auto clock_end = chrono::high_resolution_clock::now();

	outputField(julia, ni, nj);

	std::chrono::duration<float> delta = clock_end - clock_start;
	cout << "Simulation took " << delta.count() << "s\n";

	// free memory
	for (int i = 0; i < ni; i++) delete[] julia[i];
	delete[] julia;

	return 0;
}

/*saves output in VTK format*/
void outputField(double** data, int ni, int nj)
{
	stringstream name;
	name << "julia.vti";

	/*open output file*/
	ofstream out(name.str());
	if (!out.is_open()) { cerr << "Could not open " << name.str() << endl; return; }

	/*ImageData is vtk format for structured Cartesian meshes*/
	out << "<VTKFile type=\"ImageData\">\n";
	out << "<ImageData Origin=\"" << "0 0 0\" ";
	out << "Spacing=\"1 1 1\" ";
	out << "WholeExtent=\"0 " << ni - 1 << " 0 " << nj - 1 << " 0 0\">\n";

	/*output data stored on nodes (point data)*/
	out << "<PointData>\n";

	/*potential, scalar*/
	out << "<DataArray Name=\"julia\" NumberOfComponents=\"1\" format=\"ascii\" type=\"Float32\">\n";
	for (int j = 0; j < nj; j++)
	{
		for (int i = 0; i < ni; i++) out << data[i][j] << " ";
		out << "\n";
	}
	out << "</DataArray>\n";

	/*close out tags*/
	out << "</PointData>\n";
	out << "</ImageData>\n";
	out << "</VTKFile>\n";
	out.close();
}
