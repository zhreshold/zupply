#include "zupply.hpp"

#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

using namespace zz;

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

void test_unicode_filename()
{
	std::cout << "Testing unicode filename and text file writing..." << std::endl;
	zz::fs::FileEditor fh("\xe4\xb8\xad\xe6\x96\x87.txt", true);
	fh << zz::fmt::utf16_to_utf8(std::u16string({ 0x4E2D, 0x6587})) << zz::os::endl();
	fh << zz::fmt::utf32_to_utf8(std::u32string({ 0x00004E2D, 0x00006587 })) << zz::os::endl();
}

void test_log_thread()
{
	for (auto i = 0; i < 1000; ++i)
	{
		log::get_logger("default")->info("Sequence increment {}", i);
	}
	log::get_logger("default")->info("thread finished. {}", os::thread_id());
}


void test_logger()
{
	auto logger = log::get_logger("default");
	logger->attach_sink(log::new_rotate_file_sink("test1.log", 204800, true));
	log::dump_loggers();

	std::cout << "NEXT: TESTING LOGGERS IN MULTITHREAD!" << std::endl;
	time::sleep(2000);
	logger->info("test info {} {}", 1, 2.2);
	logger->info() << "call method 2  " << 1;
	logger->warn("method3") << " followed by this" << " " << os::endl();
	logger->debug("Debug message") << "should shown in debug, not in release";
	logger->trace("no trace");

	//for (auto i = 0; i < 4000; ++i)
	//{
	//	log::get_logger("default")->info("Sequence increment {}", i);
	//}

	std::vector<std::thread> vt;
	for (auto i = 0; i < 4; ++i)
	{
		vt.push_back(std::thread(&test_log_thread));
	}

	for (auto i = 0; i < 4; ++i)
	{
		vt[i].join();
	}
}

void config_logger_from_file()
{
	std::string cfgname = "logconfig.txt";
	log::config_from_file(cfgname);
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
		test_unicode_filename();
		test_logger();
	}
	catch (std::exception &e)
	{
		std::cout << e.what() << std::endl;
		throw;
	}

	return 0;
}

