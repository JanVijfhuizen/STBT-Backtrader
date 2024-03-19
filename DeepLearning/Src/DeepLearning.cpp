#include "pch.h"

#include <numeric>
#include <random>

#include <quote.hpp>
#include <ctime>

int main()
{
	Gnuplot gp("\"C:\\Program Files\\gnuplot\\bin\\gnuplot.exe\"");
	Quote* snp500 = new Quote("^GSPC");

	std::vector<double> v;

	snp500->getHistoricalSpots("2024-01-01", "2024-03-19", "1d");

	for (int i = 0; i < 100; ++i)
	{
		tm tm;
		time_t now = time(0) - (24 * 60 * 60) * (101 - i);
		localtime_s(&tm, &now);

		std::ostringstream oss;
		oss << std::put_time(&tm, "%Y-%m-%d");
		const auto str = oss.str();

		try {
			auto spot = snp500->getSpot(str);
			v.push_back(spot.getClose());
		}
		catch (const std::exception& e) {
			std::cerr << e.what() << std::endl;
		}
	}
	
	gp << "set title 'SNP 500'\n";
	gp << "plot '-' with lines title 'v'\n";
	gp.send(v);
	std::cin.get();

	// Free memory
	delete snp500;
	return 0;
}
