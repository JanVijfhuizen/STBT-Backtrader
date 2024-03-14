#include "pch.h"

#include <numeric>
#include <random>

int main()
{
	Gnuplot gp("\"C:\\Program Files\\gnuplot\\bin\\gnuplot.exe\"");

	std::random_device rd;
	std::mt19937 mt(rd());
	std::normal_distribution normDist(0., 1.);

	std::vector<double> v0, v1;
	for (int i = 0; i < 1000; ++i)
	{
		v0.push_back(normDist(mt));
		v1.push_back(normDist(mt));
	}
	std::partial_sum(v0.begin(), v0.end(), v0.begin());
	std::partial_sum(v1.begin(), v1.end(), v1.begin());

	gp << "set title 'set graph of random lines'\n";
	gp << "plot '-' with lines title 'v0'," << "'-' with lines title 'v1'\n";
	gp.send(v0);
	gp.send(v1);
	std::cin.get();
	return 0;
}
