#include "libzpp.hpp"
#include <iostream>
#include <fstream>

#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

using namespace zz;
using namespace std;
using namespace zz::time;
using namespace zz::fs;

namespace test_time
{
	using namespace zz::time;

	void test_timer()
	{
		Timer t;
		long long sum = 0;
		for (int i = 0; i < 99999999; ++i)
		{
			++sum;
		}
		printf("Elapsed time %d sec %d ms %d us %u ns\n", t.elapsed_sec(), t.elapsed_ms(), t.elapsed_us(), t.elapsed_ns());
		t.reset();
		for (int i = 0; i < 99999999; ++i)
		{
			++sum;
		}
		printf("Elapsed time %s \n", t.to_string("[%sec sec] [%sec sec] [%ms ms] [%us us] [%ns ns]").c_str());
		printf("Elapsed %f sec\n", t.elapsed_sec_double());
	}

	void test_date()
	{
		auto date = Date::local_time();
		for (auto i = 0; i < 99; ++i)
		{
			cout << date.to_string() << endl;
			date = Date::local_time();
			//sleep(1);
		}

	}
}

namespace test_filesystem
{
	using namespace zz::fs;
	void test_filehandler()
	{
		{
			FileReader fh("../../cache/log_test_no.txt");

			FileEditor fh2("../../cache/\xe4\xb8\xad\xe6\x96\x87.txt", true);
			fh2 << "\xe4\xb8\xad\xe6\x96\x87.txt" << " good, support UTF-8" << os::endl();
			fh2 << "\xe4\xb8\xad\xe6\x96\x87.txt" << " good2, support UTF-8!" << os::endl();
			fh2.flush();

			FileEditor fh3 = std::move(fh2);

			//FileEditor fh4("../../cache/\xe4\xb8\xad\xe6\x96\x87.txt", std::ios::out);
			//fh4.flush();
			cout << fh3.is_open() << endl;
			cout << fh2.is_open() << endl;
			//cout << fh4.is_open() << endl;
			fh3 << "\xe4\xb8\xad\xe6\x96\x87.txt" << " good3, support UTF-8" << os::endl();
			fh2 << "\xe4\xb8\xad\xe6\x96\x87.txt" << " good should not be shown, support UTF-8" << os::endl();
			//fh4 << "\xe4\xb8\xad\xe6\x96\x87.txt" << " good from fh4, support UTF-8" << os::endl();
		}
		FileEditor fh2("../../cache/\xe4\xb8\xad\xe6\x96\x87.txt", false);
		fh2 << "\xe4\xb8\xad\xe6\x96\x87.txt" << " good, support UTF-8" << os::endl();
		fh2 << "\xe4\xb8\xad\xe6\x96\x87.txt" << " good2, support UTF-8!" << os::endl();
		fh2.flush();

	}
}

namespace test_os
{
	void test_directory()
	{
		cout << os::get_current_working_directory() << endl;
		cout << os::get_absolute_path("") << endl;
		FileEditor fh("temp.txt", std::ios::out);
		fh << os::get_current_working_directory() << os::endl();
		fh << os::get_absolute_path("") << os::endl();

		auto ret = os::create_directory_recursive("d:\\openzl\\test\\a\\b\\c\\d\\e\\f");
		cout << ret << endl;

		string path = "d:\\somwe/sdlf\\sdf\\wef\\sdf\\basename";
		cout << os::path_split_directory(path) << endl;
		cout << os::path_split_filename(path) << endl;
		cout << os::path_split_basename(path) << endl;
		cout << os::path_split_extension(path) << endl;
	}
}


namespace test_formatter
{
	void test_formatter()
	{
		std::string str = "somwlojdlks {} here some {}, {}, {}";
		fmt::format_string(str, 1, 2.2, 3.3f);
		cout << str << endl;

		auto list = fmt::split("\\\\DiskStation\\video", '\\');
		for (auto str : list)
		{
			cout << str << ",";
		}
		cout << endl;

		cout << fmt::join(list, '/') << endl;
	}
}


namespace test_logger
{
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
		//auto stdcout = log::new_stdout_sink();

		//std::shared_ptr<log::Logger> logger = std::make_shared<log::Logger>("default");
		//logger->attach_sink(log::new_stdout_sink());
		//auto fl = log::new_simple_file_sink("test1.log", true);
		//logger->attach_console();
		logger->attach_sink(log::new_rotate_file_sink("test1.log", 204800, true));
		//logger->detach_console();
		log::dump_loggers();
		logger->info("test info {} {}", 1, 2.2);
		logger->info() << "call method 2  " << 1;
		logger->warn("method3") << " followed by this" << " " << os::endl();
		logger->debug("Debug message") << "should shown in debug, not in release";
		logger->trace("no trace");

		//for (auto i = 0; i < 4000; ++i)
		//{
		//	log::get_logger("default")->info("Sequence increment {}", i);
		//}

		//std::vector<std::thread> vt;
		//for (auto i = 0; i < 4; ++i)
		//{
		//	vt.push_back(std::thread(&test_log_thread));
		//}

		//for (auto i = 0; i < 4; ++i)
		//{
		//	vt[i].join();
		//}
	}
}

namespace test_math
{
	void test_math()
	{
		double num = 233.790394;
		cout << "Double: " << num << "Round: " << math::round(num) << endl;
		float num2 = 289.303f;
		cout << "Number: " << num2 << " Clip in 0-255: " << math::clip(num2, 0.0f, 255.0f) << endl;
	}
}




int main()
{
#ifdef _MSC_VER
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	
	test_time::test_timer();
	test_time::test_date();
	test_logger::test_logger();
	test_filesystem::test_filehandler();
	test_os::test_directory();
	test_formatter::test_formatter();
	test_math::test_math();

	system("pause");

	return 0;
}

