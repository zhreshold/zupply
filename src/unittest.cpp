#include "zupply.hpp"
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
		cout << os::current_working_directory() << endl;
		cout << os::absolute_path("") << endl;
		FileEditor fh("temp.txt", std::ios::out);
		fh << os::current_working_directory() << os::endl();
		fh << os::absolute_path("") << os::endl();

		auto ret = os::create_directory_recursive("d:\\openzl\\test\\a\\b\\c\\d\\e\\f");
		cout << ret << endl;

		string path = "d:\\somwe/sdlf\\sdf\\wef\\sdf\\basename";
		cout << os::path_split_directory(path) << endl;
		cout << os::path_split_filename(path) << endl;
		cout << os::path_split_basename(path) << endl;
		cout << os::path_split_extension(path) << endl;
	}

	void test_console_size()
	{
		Size sz = os::console_size();
		cout << "console width: " << sz.width << endl;
		cout << "console height: " << sz.height << endl;
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

		list = fmt::split("sdljf      sdljfkd  \t  sldjfldjs     sljopopwej", ' ');
		for (auto str : list)
		{
			cout << str << ", ";
		}
		cout << endl;
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

	void config_logger_from_file()
	{
		std::string cfgname = "logconfig.txt";
		log::config_from_file(cfgname);
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

namespace test_cfg
{
	void test_cfg_parser()
	{
		std::stringstream ss;
		ss << "a=1\n"
			"b=1\n\n"
			"enabled = false \n"
			"[e]\n"
			"ea=1\n"
			"eb=1\n\n"
			"[c]\n"
			"ca=2\n"
			"cb=2\n\n"
			"[a.d]\n"
			"extra.da=3 # this is a comment \n"
			"db=3\n\n"
			"[A]\n"
			"Aa=4\n"
			"Ab=4\n";
		cfg::CfgParser cfpg(ss);
		cout << cfpg["a"].intValue() << endl;
		cout << cfpg("a")("d")("extra")["da"].intValue() << endl;
		cout << cfpg.root().to_string() << endl;
		bool enabled = cfpg["enabled"].booleanValue();
		cout << "enabled: " << enabled << endl;
	}

	void test_cfg_string()
	{
		cfg::CfgValue vstring("25");
		int v = vstring.intValue();
		cout << v << endl;
		vstring = std::string("1.2321 9324.23 343.239423");
		auto vv = vstring.doubleVector();
		for (auto vvv : vv)
		{
			cout << vvv << endl;
		}
	}
}

void test_arg_parser(int argc, char** argv)
{
	cfg::ArgParser argparser;
	argparser.add_arg_lite('q', "quiet", "enter quiet mode");
	argparser.add_arg_lite('v', "verbose", "enter verbose mode");
	argparser.add_arg_lite('h', "helper", "print helper");
	argparser.add_arg_lite('r', "recurse", "recursive mode");

	argparser.add_argn('i', "input", "input string");
	argparser.add_argn('n', "number", "input integer", "int", 1, 1);
	argparser.add_argn('k', "something", "some input string", "string", 1, -1);
	argparser.parse(argc, argv);
	cout << "success? " << argparser.success() << endl;
	argparser.print_help();
	auto rest = argparser[""];
	auto q = argparser['q'];
	auto qq = argparser["quiet"];
	auto i = argparser['i'];
	cout << argparser.count("input") << endl;
	cout << i[0].str() << "   " << i[1].str() << endl;
	cout << argparser.count("quiet") << endl;
}

int main(int argc, char** argv)
{
#ifdef _MSC_VER
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	test_arg_parser(argc, argv);
	//test_logger::config_logger_from_file();
	//test_time::test_timer();
	//test_time::test_date();
	//test_logger::test_logger();
	//test_filesystem::test_filehandler();
	//test_os::test_directory();
	//test_os::test_console_size();
	//test_formatter::test_formatter();
	//test_math::test_math();
	//test_cfg::test_cfg_parser();
	//test_cfg::test_cfg_string();

	system("pause");

	return 0;
}

