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

			//FileEditor fh3 = std::move(fh2);

			//FileEditor fh4("../../cache/\xe4\xb8\xad\xe6\x96\x87.txt", std::ios::out);
			//fh4.flush();
			//cout << fh3.is_open() << endl;
			cout << fh2.is_open() << endl;
			//cout << fh4.is_open() << endl;
			//fh3 << "\xe4\xb8\xad\xe6\x96\x87.txt" << " good3, support UTF-8" << os::endl();
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
			"enabled = TruE \n"
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
		cout << cfpg["a"].load<int>() << endl;
		cout << cfpg("a")("d")("extra")["da"].load<int>() << endl;
		cout << cfpg.root().to_string() << endl;
		bool enabled = cfpg["enabled"].load<bool>();
		cout << "enabled: " << enabled << endl;
	}


	void test_value()
	{
		cfg::Value v("25");
		auto vint = v.load<int>();
		cout << "v int " << vint << endl;
		v = "1.2321d 9324.23 sdf, bad, sldf;  343.239423, haha, 2309.23f";
		cout << v.str() << endl;
		//double d1, d2, d3;
		//v >> d1 >> d2 >> d3;
		//cout << v.value<double>() << "  " << v.value<double>() << "    " << v.value<double>() << endl;
		auto vvec = v.load<vector<int>>();
		auto vvec2 = v.load<vector<double>>();
		cout << v.load<vector<int>>()[0] << endl;
		cout << v.load<vector<double>>()[1] << endl;
		cout << v.load<vector<double>>()[2] << endl;
	}
}

void test_arg_parser2(int argc, char** argv)
{
	cfg::ArgParser p;
	//p.add_opt('v', "version").call([]{cout << "show version plz" << endl; }).set_help("version info");
	int input;
	//p.add_opt("input").store(input, -1).require().help("input number").type("INT");
	p.add_opt_value('i', "input", input, -1, "input name", "INT");
	bool bbb;
	p.add_opt_flag('b', "brief", "brief intro", &bbb);
	p.add_opt_help('h', "help");
	p.add_opt_version("version", "0.0.1");
	std::vector<int> v({ 1, 2, 3, 4 });
	p.add_opt_value('a', "values", v, v, "vector of int to do", "INT").require().set_min(4);
	p.parse(argc, argv);
	if (p.count_error())
	{
		cout << p.get_error() << endl;
		cout << p.get_help() << endl;
	}
	for (auto vv : v) cout << vv << endl;
	//cout << p.get_help() << endl;
}

int main(int argc, char** argv)
{
#ifdef _MSC_VER
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		test_arg_parser2(argc, argv);
		//test_arg_parser(argc, argv);
		test_logger::config_logger_from_file();
		test_time::test_timer();
		test_time::test_date();
		test_logger::test_logger();
		//test_filesystem::test_filehandler();
		//test_os::test_directory();
		test_os::test_console_size();
		//test_formatter::test_formatter();
		//test_math::test_math();
		//test_cfg::test_cfg_parser();
		//test_cfg::test_value();
	}
	catch (std::exception &e)
	{
		std::cout << e.what() << std::endl;
		throw;
	}
	
	
	//int i = 0;
	//misc::Callback([&]{cout << "test callback!" << i << endl; i = 1; });
	//cout << i << endl;

	system("pause");

	return 0;
}

