#include "zupply.hpp"

#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

void test_date()
{
	// show current date in local time zone
	zz::time::Date date;
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

void test_timer()
{
	const int repeat = 999999;
	// create a timer
	zz::time::Timer t;
	// time consuming function
	long long sum = 0;
	for (int i = 0; i < repeat; ++i) sum += i;
	std::cout << "Summation: " << sum << " elapsed time: " << t.to_string() << std::endl;
	// reset timer, start new timer
	t.reset();
	// another time consuming function
	for (int i = 0; i < repeat; ++i) sum -= i;
	// use formatter
	std::cout << "Subtraction: " << sum << t.to_string(" elapsed time: [%us us]") << std::endl;
	// different quantization
	std::cout << "sec: " << t.elapsed_sec() << std::endl;
	std::cout << "msec: " << t.elapsed_ms() << std::endl;
	std::cout << "usec: " << t.elapsed_us() << std::endl;
	std::cout << "nsec: " << t.elapsed_ns() << std::endl;
	// use double, no quantize
	std::cout << "sec in double: " << t.elapsed_sec_double() << std::endl;
	// sleep for 2000 ms
	zz::time::sleep(2000);
	std::cout << "After sleep for 2 sec: " << t.elapsed_sec_double() << std::endl;
}


int main(int argc, char** argv)
{
#ifdef _MSC_VER
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		test_date();
		test_timer();
	}
	catch (std::exception &e)
	{
		std::cout << e.what() << std::endl;
		throw;
	}

	return 0;
}

