#include "zupply.hpp"

#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

void test_date()
{
	// show current date in local time zone
	auto date = zz::time::Date();
	std::cout << "Local time(Pacific) " << date.to_string() << std::endl;
	// or UTC time
	date.to_utc_time();
	std::cout << "UTC time " << date.to_string() << std::endl;
	// customize to_string format
	auto str = date.to_string("%m/%d %a %H:%M:%S.%frac");
	std::cout << "With format [%m / %d %a %H:%M : %S.%frac] : " << str << std::endl;
	str = date.to_string("%c");
	std::cout << "With format %c(standard, local specific): " << str << std::endl;

	// another way is using static function
	std::cout << "Call UTC directly " << zz::time::Date::utc_time().to_string() << std::endl;
}


int main(int argc, char** argv)
{
#ifdef _MSC_VER
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		test_date();
	}
	catch (std::exception &e)
	{
		std::cout << e.what() << std::endl;
		throw;
	}

	return 0;
}

