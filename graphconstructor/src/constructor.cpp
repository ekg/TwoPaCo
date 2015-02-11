#include <set>
#include <ctime>
#include <string>
#include <vector>
#include <cassert>
#include <cstdint>
#include <cassert>
#include <fstream>
#include <iterator>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <sstream>

#include "test.h"
#include "vertexenumerator.h"

int main(int argc, char * argv[])

{
	assert(Sibelia::Runtests());
	
	try
	{		
		std::vector<std::string> fileName(argv + 2, argv + argc);
		std::stringstream ss(*(argv + 1));
		size_t filterSize;
		ss >> filterSize;
		Sibelia::VertexEnumerator vid(fileName, 4, filterSize);

		std::vector<std::string> all;
		vid.Dump(std::back_inserter(all));
		std::vector<std::string> rev;
		std::transform(all.begin(), all.end(), std::back_inserter(rev), static_cast<std::string(*)(const std::string&)>(Sibelia::DnaString::RevComp));
		all.insert(all.begin(), rev.begin(), rev.end());
		std::sort(all.begin(), all.end());
		all.erase(std::unique(all.begin(), all.end()), all.end());
		std::cout << "All = " << all.size();
	}
	catch (const std::runtime_error & msg)
	{
		std::cerr << msg.what() << std::endl;
	}
	

	return 0;
}