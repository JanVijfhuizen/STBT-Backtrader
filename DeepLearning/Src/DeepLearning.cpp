#include "pch.h"

#include <numeric>
#include <random>

#include <quote.hpp>

int main()
{
	// S&P 500
	Quote* snp500 = new Quote("^GSPC");

	// Get the historical spots from Yahoo Finance
	snp500->getHistoricalSpots("2017-12-01", "2017-12-31", "1d");

	// Print the spots
	snp500->printSpots();

	// Print a spot
	try {
		Spot spot = snp500->getSpot("2017-12-01");
		spot.printSpot();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	// Get the historical EUR/USD rates
	Quote* eurusd = new Quote("EURUSD=X");
	eurusd->getHistoricalSpots("2018-01-01", "2018-01-10", "1d");
	eurusd->printSpots();

	// Get the historical EUR/AUD rates
	Quote* euraud = new Quote("EURAUD=X");
	euraud->getHistoricalSpots("2018-01-01", "2018-01-10", "1d");
	euraud->printSpots();

	// Free memory
	delete snp500;
	delete eurusd;
	delete euraud;

	///

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
