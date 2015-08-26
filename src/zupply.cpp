/********************************************************************//*
 *
 *   Script File: zupply.cpp
 *
 *   Description:
 *
 *   Implementations of core modules
 *
 *
 *   Author: Joshua Zhang
 *   Date since: June-2015
 *
 *   Copyright (c) <2015> <JOSHUA Z. ZHANG>
 *
 *	 Open source according to MIT License.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ***************************************************************************/

// Unless you are very confident, don't set either OS flag
#if defined(ZUPPLY_OS_UNIX) && defined(ZUPPLY_OS_WINDOWS)
#error Both Unix and Windows flags are set, which is not allowed!
#elif defined(ZUPPLY_OS_UNIX)
#pragma message Using defined Unix flag
#elif defined(ZUPPLY_OS_WINDOWS)
#pragma message Using defined Windows flag
#else
#if defined(unix)        || defined(__unix)      || defined(__unix__) \
	|| defined(linux) || defined(__linux) || defined(__linux__) \
	|| defined(sun) || defined(__sun) \
	|| defined(BSD) || defined(__OpenBSD__) || defined(__NetBSD__) \
	|| defined(__FreeBSD__) || defined (__DragonFly__) \
	|| defined(sgi) || defined(__sgi) \
	|| (defined(__MACOSX__) || defined(__APPLE__)) \
	|| defined(__CYGWIN__) || defined(__MINGW32__)
#define ZUPPLY_OS_UNIX	1	//!< Unix like OS
#undef ZUPPLY_OS_WINDOWS
#elif defined(_MSC_VER) || defined(WIN32)  || defined(_WIN32) || defined(__WIN32__) \
	|| defined(WIN64) || defined(_WIN64) || defined(__WIN64__)
#define ZUPPLY_OS_WINDOWS	1	//!< Microsoft Windows
#undef ZUPPLY_OS_UNIX
#else
#error Unable to support this unknown OS.
#endif
#endif

#if ZUPPLY_OS_WINDOWS
#include <windows.h>
#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#elif ZUPPLY_OS_UNIX
#include <unistd.h>	/* POSIX flags */
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/ioctl.h>
#endif

#include "zupply.hpp"

#include <chrono>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <functional>
#include <cctype>
#include <cstdio>
#include <cmath>


namespace zz
{
	namespace fmt
	{
		namespace consts
		{
			static const char			kFormatSpecifierEscapeChar = '%';
			static const std::string	kZeroPaddingStr = std::string("0");
		}

		bool is_digit(char c)
		{
			return c >= '0' && c <= '9';
		}

		bool wild_card_match(const char* str, const char* pattern)
		{
			while (*pattern)
			{
				switch (*pattern)
				{
				case '?':
					if (!*str) return false;
					++str;
					++pattern;
					break;
				case '*':
					if (wild_card_match(str, pattern + 1)) return true;
					if (*str && wild_card_match(str + 1, pattern)) return true;
					return false;
					break;
				default:
					if (*str++ != *pattern++) return false;
					break;
				}
			}
			return !*str && !*pattern;
		}


		std::string& ltrim(std::string& str)
		{
			str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::not1(std::ptr_fun<int, int>(&std::isspace))));
			return str;
		}

		std::string& rtrim(std::string& str)
		{
			str.erase(std::find_if(str.rbegin(), str.rend(), std::not1(std::ptr_fun<int, int>(&std::isspace))).base(), str.end());
			return str;
		}

		std::string& trim(std::string& str)
		{
			return ltrim(rtrim(str));
		}

		std::string& lstrip(std::string& str, std::string what)
		{
			auto pos = str.find(what);
			if (0 == pos)
			{
				str.erase(pos, what.length());
			}
			return str;
		}

		std::string& rstrip(std::string& str, std::string what)
		{
			auto pos = str.rfind(what);
			if (str.length() - what.length() == pos)
			{
				str.erase(pos, what.length());
			}
			return str;
		}

		std::string& rskip(std::string& str, std::string delim)
		{
			auto pos = str.rfind(delim);
			if (pos == 0)
			{
				str = std::string();
			}
			else if (std::string::npos != pos)
			{
				str = str.substr(0, pos - 1);
			}
			return str;
		}

		std::vector<std::string> split(const std::string s, char delim)
		{
			std::vector<std::string> elems;
			std::istringstream ss(s);
			std::string item;
			while (std::getline(ss, item, delim))
			{
				if (!item.empty()) elems.push_back(item);
			}
			return elems;
		}

		std::vector<std::string> split(const std::string s, std::string delim)
		{
			std::vector<std::string> elems;
			std::string ss(s);
			std::string item;
			size_t pos = 0;
			while ((pos = ss.find(delim)) != std::string::npos) {
				item = ss.substr(0, pos);
				if (!item.empty()) elems.push_back(item);
				ss.erase(0, pos + delim.length());
			}
			if (!ss.empty()) elems.push_back(ss);
			return elems;
		}

		std::vector<std::string> split_multi_delims(const std::string s, std::string delims)
		{
			std::vector<std::string> elems;
			std::string ss(s);
			std::string item;
			size_t pos = 0;
			while ((pos = ss.find_first_of(delims)) != std::string::npos) {
				item = ss.substr(0, pos);
				if (!item.empty()) elems.push_back(item);
				ss.erase(0, pos + 1);
			}
			if (!ss.empty()) elems.push_back(ss);
			return elems;
		}

		std::vector<std::string> split_whitespace(const std::string s)
		{
			auto list = split_multi_delims(s, " \t\n");
			std::vector<std::string> ret;
			for (auto elem : list)
			{
				auto rest = fmt::trim(elem);
				if (!rest.empty()) ret.push_back(rest);
			}
			return ret;
		}

		std::pair<std::string, std::string> split_first_occurance(const std::string s, char delim)
		{
			auto pos = s.find_first_of(delim);
			std::string first = s.substr(0, pos);
			std::string second = (pos != std::string::npos? s.substr(pos+1) : std::string());
			return std::make_pair(first, second);
		}

		std::vector<std::string>& erase_empty(std::vector<std::string> &vec)
		{
			for (auto it = vec.begin(); it != vec.end();)
			{
				if (it->empty())
				{
					it = vec.erase(it);
				}
				else
				{
					++it;
				}
			}
			return vec;
		}

		std::string join(std::vector<std::string> elems, char delim)
		{
			std::string str;
			elems = erase_empty(elems);
			if (elems.empty()) return str;
			str = elems[0];
			for (std::size_t i = 1; i < elems.size(); ++i)
			{
				if (elems[i].empty()) continue;
				str += delim + elems[i];
			}
			return str;
		}

		bool starts_with(const std::string& str, const std::string& start)
		{
			return (str.length() >= start.length()) && (str.compare(0, start.length(), start) == 0);
		}

		bool ends_with(const std::string& str, const std::string& end)
		{
			return (str.length() >= end.length()) && (str.compare(str.length() - end.length(), end.length(), end) == 0);
		}

		std::string& replace_all(std::string& str, char replaceWhat, char replaceWith)
		{
			std::replace(str.begin(), str.end(), replaceWhat, replaceWith);
			return str;
		}

		std::string& replace_all(std::string& str, const std::string& replaceWhat, const std::string& replaceWith)
		{
			if (replaceWhat == replaceWith) return str;
			std::size_t foundAt = std::string::npos;
			while ((foundAt = str.find(replaceWhat, foundAt + 1)) != std::string::npos)
			{
				str.replace(foundAt, replaceWhat.length(), replaceWith);
			}
			return str;
		}

		void replace_first_with_escape(std::string &str, const std::string &replaceWhat, const std::string &replaceWith)
		{
			std::size_t foundAt = std::string::npos;
			while ((foundAt = str.find(replaceWhat, foundAt + 1)) != std::string::npos)
			{
				if (foundAt > 0 && str[foundAt - 1] == consts::kFormatSpecifierEscapeChar)
				{
					str.erase(foundAt > 0 ? foundAt - 1 : 0, 1);
					++foundAt;
				}
				else
				{
					str.replace(foundAt, replaceWhat.length(), replaceWith);
					return;
				}
			}
		}

		void replace_all_with_escape(std::string &str, const std::string &replaceWhat, const std::string &replaceWith)
		{
			std::size_t foundAt = std::string::npos;
			while ((foundAt = str.find(replaceWhat, foundAt + 1)) != std::string::npos)
			{
				if (foundAt > 0 && str[foundAt - 1] == consts::kFormatSpecifierEscapeChar)
				{
					str.erase(foundAt > 0 ? foundAt - 1 : 0, 1);
					++foundAt;
				}
				else
				{
					str.replace(foundAt, replaceWhat.length(), replaceWith);
					foundAt += replaceWith.length();
				}
			}
		}

		bool str_equals(const char* s1, const char* s2)
		{
			if (s1 == nullptr && s2 == nullptr) return true;
			if (s1 == nullptr || s2 == nullptr) return false;
			return std::string(s1) == std::string(s2);	// this is safe, not with strcmp
		}

		std::string& left_zero_padding(std::string &str, int width)
		{
			int toPad = width - static_cast<int>(str.length());
			while (toPad > 0)
			{
				str = consts::kZeroPaddingStr + str;
				--toPad;
			}
			return str;
		}

		std::string to_lower_ascii(std::string mixed)
		{
			std::transform(mixed.begin(), mixed.end(), mixed.begin(), std::tolower);
			return mixed;
		}

		std::string to_upper_ascii(std::string mixed)
		{
			std::transform(mixed.begin(), mixed.end(), mixed.begin(), std::toupper);
			return mixed;
		}

	} // namespace fmt

	namespace fs
	{

		bool FileEditor::open(bool truncateOrNot)
		{
			if (this->is_open()) return true;
			std::ios::openmode mode = std::ios::in | std::ios::out | std::ios::binary;
			if (truncateOrNot)
			{
				mode |= std::ios::trunc;
			}
			else
			{
				mode |= std::ios::app;
			}
			os::fstream_open(stream_, filename_, mode);
			if (this->is_open()) return true;
			return false;
		}

		bool FileEditor::open(std::string filename, bool truncateOrNot, int retryTimes, int retryInterval)
		{
			this->close();
			// use absolute path
			filename_ = os::absolute_path(filename);
			// try open
			return this->try_open(retryTimes, retryInterval, truncateOrNot);
		}

		bool FileEditor::reopen(bool truncateOrNot)
		{
			this->close();
			return this->open(truncateOrNot);
		}

		void FileEditor::close()
		{
			stream_.close();
			stream_.clear();
			// unregister this file
			//detail::FileEditorRegistry::instance().erase(filename_);
		}

		bool FileEditor::try_open(int retryTime, int retryInterval, bool truncateOrNot)
		{
			while (retryTime > 0 && (!this->open(truncateOrNot)))
			{
				time::sleep(retryInterval);
				--retryTime;
			}
			return this->is_open();
		}

		bool FileReader::open()
		{
			if (this->is_open()) return true;
			this->check_valid();
			os::ifstream_open(istream_, filename_, std::ios::in | std::ios::binary);
			if (this->is_open()) return true;
			return false;
		}

		bool FileReader::try_open(int retryTime, int retryInterval)
		{
			while (retryTime > 0 && (!this->open()))
			{
				time::sleep(retryInterval);
				--retryTime;
			}
			return this->is_open();
		}

		std::size_t FileReader::file_size()
		{
			if (is_open())
			{
				auto curPos = istream_.tellg();
				istream_.seekg(0, istream_.end);
				std::size_t size = static_cast<std::size_t>(istream_.tellg());
				istream_.seekg(curPos);
				return size;
			}
			return 0;
		}

		std::size_t get_file_size(std::string filename)
		{
			std::ifstream fs;
			os::ifstream_open(fs, filename, std::ios::in | std::ios::ate);
			std::size_t size = static_cast<std::size_t>(fs.tellg());
			return size;
		}

	} //namespace fs



	namespace time
	{
		namespace consts
		{
			static const char			*kDateFractionSpecifier = "%frac";
			static const unsigned	    kDateFractionWidth = 3;
			static const char			*kTimerPrecisionSecSpecifier = "%sec";
			static const char			*kTimerPrecisionMsSpecifier = "%ms";
			static const char			*kTimerPrecisionUsSpecifier = "%us";
			static const char			*kTimerPrecisionNsSpecifier = "%ns";
			static const char			*kDateTimeSpecifier = "%datetime";
		}

		Date::Date()
		{
			auto now = std::chrono::system_clock::now();
			timeStamp_ = std::chrono::system_clock::to_time_t(now);
			calendar_ = os::localtime(timeStamp_);
			fraction_ = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
				% math::Pow<10, consts::kDateFractionWidth>::result;
			fractionStr_ = fmt::int_to_zero_pad_str(fraction_, consts::kDateFractionWidth);
		}

		void Date::to_local_time()
		{
			calendar_ = os::localtime(timeStamp_);
		}

		void Date::to_utc_time()
		{
			calendar_ = os::gmtime(timeStamp_);
		}

		std::string Date::to_string(const char *format)
		{
			std::string fmt(format);
			fmt::replace_all_with_escape(fmt, consts::kDateFractionSpecifier, fractionStr_);
			std::ostringstream buf;
			buf << std::put_time(&calendar_, fmt.c_str());
			return buf.str();
		}

		Date Date::local_time()
		{
			Date date;
			return date;
		}

		Date Date::utc_time()
		{
			Date date;
			date.to_utc_time();
			return date;
		}

		Timer::Timer()
		{
			this->reset();
		};

		void Timer::reset()
		{
			timeStamp_ = std::chrono::steady_clock::now();
		}

		std::size_t	Timer::elapsed_ns()
		{
			return static_cast<std::size_t>(std::chrono::duration_cast<std::chrono::nanoseconds>
				(std::chrono::steady_clock::now() - timeStamp_).count());
		}

		std::string Timer::elapsed_ns_str()
		{
			return std::to_string(elapsed_ns());
		}

		std::size_t Timer::elapsed_us()
		{
			return elapsed_ns() / 1000;
		}

		std::string Timer::elapsed_us_str()
		{
			return std::to_string(elapsed_us());
		}

		std::size_t Timer::elapsed_ms()
		{
			return elapsed_ns() / 1000000;
		}

		std::string Timer::elapsed_ms_str()
		{
			return std::to_string(elapsed_ms());
		}

		std::size_t Timer::elapsed_sec()
		{
			return elapsed_ns() / 1000000000;
		}

		std::string Timer::elapsed_sec_str()
		{
			return std::to_string(elapsed_sec());
		}

		double Timer::elapsed_sec_double()
		{
			return static_cast<double>(elapsed_ns()) / 1000000000;
		}

		std::string Timer::to_string(const char *format)
		{
			std::string str(format);
			fmt::replace_all_with_escape(str, consts::kTimerPrecisionSecSpecifier, elapsed_sec_str());
			fmt::replace_all_with_escape(str, consts::kTimerPrecisionMsSpecifier, elapsed_ms_str());
			fmt::replace_all_with_escape(str, consts::kTimerPrecisionUsSpecifier, elapsed_us_str());
			fmt::replace_all_with_escape(str, consts::kTimerPrecisionNsSpecifier, elapsed_ns_str());
			return str;
		}


	} // namespace time



	namespace os
	{
		namespace consts
		{
			static const std::string kEndLineCRLF = std::string("\r\n");
			static const std::string kEndLineLF = std::string("\n");
		}

		int system(const char *const command, const char *const moduleName)
		{
			misc::unused(moduleName);
#if ZUPPLY_OS_WINDOWS
			PROCESS_INFORMATION pi;
			STARTUPINFO si;
			std::memset(&pi, 0, sizeof(PROCESS_INFORMATION));
			std::memset(&si, 0, sizeof(STARTUPINFO));
			GetStartupInfoA(&si);
			si.cb = sizeof(si);
			si.wShowWindow = SW_HIDE;
			si.dwFlags |= SW_HIDE | STARTF_USESHOWWINDOW;
			const BOOL res = CreateProcessA((LPCTSTR)moduleName, (LPTSTR)command, 0, 0, FALSE, 0, 0, 0, &si, &pi);
			if (res) {
				WaitForSingleObject(pi.hProcess, INFINITE);
				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
				return 0;
			}
			else return std::system(command);
#elif ZUPPLY_OS_UNIX
			const unsigned int l = std::strlen(command);
			if (l) {
				char *const ncommand = new char[l + 16];
				std::strncpy(ncommand, command, l);
				std::strcpy(ncommand + l, " 2> /dev/null"); // Make command silent.
				const int out_val = std::system(ncommand);
				delete[] ncommand;
				return out_val;
			}
			else return -1;
#else
			misc::unused(command);
			return -1;
#endif
		}

		std::size_t thread_id()
		{
#if ZUPPLY_OS_WINDOWS
			// It exists because the std::this_thread::get_id() is much slower(espcially under VS 2013)
			return  static_cast<size_t>(::GetCurrentThreadId());
#elif __linux__
			return  static_cast<size_t>(syscall(SYS_gettid));
#else //Default to standard C++11 (OSX and other Unix)
			return static_cast<size_t>(std::hash<std::thread::id>()(std::this_thread::get_id()));
#endif

		}

		Size console_size()
		{
			Size ret(-1, -1);
#if ZUPPLY_OS_WINDOWS
			CONSOLE_SCREEN_BUFFER_INFO csbi;
			GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
			ret.width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
			ret.height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#elif ZUPPLY_OS_UNIX
			struct winsize w;
			ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
			ret.width = w.ws_col;
			ret.height = w.ws_row;
#endif
			return ret;
		}

		/**
		* \fn	std::tm localtime(std::time_t t)
		*
		* \brief	Thread safe localtime
		*
		* \param	t	The std::time_t to process.
		*
		* \return	A std::tm.
		*/
		std::tm localtime(std::time_t t)
		{
			std::tm temp;
#if ZUPPLY_OS_WINDOWS
			localtime_s(&temp, &t);
			return temp;
#elif ZUPPLY_OS_UNIX
			// POSIX SUSv2 thread safe localtime_r
			return *localtime_r(&t, &temp);
#else
			return temp; // return default tm struct, no idea what it is
#endif
		}

		/**
		* \fn	std::tm gmtime(std::time_t t)
		*
		* \brief	Thread safe gmtime
		*
		* \param	t	The std::time_t to process.
		*
		* \return	A std::tm.
		*/
		std::tm gmtime(std::time_t t)
		{
			std::tm temp;
#if ZUPPLY_OS_WINDOWS
			gmtime_s(&temp, &t);
			return temp;
#elif ZUPPLY_OS_UNIX
			// POSIX SUSv2 thread safe gmtime_r
			return *gmtime_r(&t, &temp);
#else
			return temp; // return default tm struct, no idea what it is
#endif 
		}

		std::wstring utf8_to_wstring(std::string &u8str)
		{
#if ZUPPLY_OS_WINDOWS
			// windows use 16 bit wstring 
			std::u16string u16str = fmt::utf8_to_utf16(u8str);
			return std::wstring(u16str.begin(), u16str.end());
#else
			// otherwise use 32 bit wstring
			std::u32string u32str = fmt::utf8_to_utf32(u8str);
			return std::wstring(u32str.begin(), u32str.end());
#endif
		}

		std::string wstring_to_utf8(std::wstring &wstr)
		{
#if ZUPPLY_OS_WINDOWS
			// windows use 16 bit wstring 
			std::u16string u16str(wstr.begin(), wstr.end());
			return fmt::utf16_to_utf8(u16str);
#else
			// otherwise use 32 bit wstring
			std::u32string u32str(wstr.begin(), wstr.end());
			return fmt::utf32_to_utf8(u32str);
#endif
		}

		std::vector<std::string> path_split(std::string path)
		{
#if ZUPPLY_OS_WINDOWS
			std::replace(path.begin(), path.end(), '\\', '/');
			return fmt::split(path, '/');
#else
			return fmt::split(path, '/');
#endif
		}

		std::string path_join(std::vector<std::string> elems)
		{
#if ZUPPLY_OS_WINDOWS
			return fmt::join(elems, '\\');
#else
			return fmt::join(elems, '/');
#endif
		}

		std::string path_split_filename(std::string path)
		{
#if ZUPPLY_OS_WINDOWS
			std::string::size_type pos = fmt::trim(path).find_last_of("/\\");
#else
			std::string::size_type pos = fmt::trim(path).find_last_of("/");
#endif
			if (pos == std::string::npos) return path;
			if (pos != path.length())
			{
				return path.substr(pos + 1);
			}
			return std::string();
		}

		std::string path_split_directory(std::string path)
		{
#if ZUPPLY_OS_WINDOWS
			std::string::size_type pos = fmt::trim(path).find_last_of("/\\");
#else
			std::string::size_type pos = fmt::trim(path).find_last_of("/");
#endif
			if (pos != std::string::npos)
			{
				return path.substr(0, pos);
			}
			return std::string();
		}

		std::string path_split_extension(std::string path)
		{
			std::string filename = path_split_filename(path);
			auto list = fmt::split(filename, '.');
			if (list.size() > 1)
			{
				return list.back();
			}
			return std::string();
		}

		std::string path_split_basename(std::string path)
		{
			std::string filename = path_split_filename(path);
			auto list = fmt::split(filename, '.');
			if (list.size() == 1)
			{
				return list[0];
			}
			else if (list.size() > 1)
			{
				return fmt::join({ list.begin(), list.end() - 1 }, '.');
			}
			return std::string();
		}

		std::string path_append_basename(std::string origPath, std::string whatToAppend)
		{
			std::string newPath = path_join({ path_split_directory(origPath), path_split_basename(origPath) })
				+ whatToAppend;
			std::string ext = path_split_extension(origPath);
			if (!ext.empty())
			{
				newPath += "." + ext;
			}
			return newPath;
		}


		void fstream_open(std::fstream &stream, std::string &filename, std::ios::openmode openmode)
		{
			// make sure directory exists for the target file
			create_directory_recursive(path_split_directory(filename));
#if ZUPPLY_OS_WINDOWS
			stream.open(utf8_to_wstring(filename), openmode);
#else
			stream.open(filename, openmode);
#endif
		}

		void ifstream_open(std::ifstream &stream, std::string &filename, std::ios::openmode openmode)
		{
#if ZUPPLY_OS_WINDOWS
			stream.open(utf8_to_wstring(filename), openmode);
#else
			stream.open(filename, openmode);
#endif
		}

		bool rename(std::string oldName, std::string newName)
		{
#if ZUPPLY_OS_WINDOWS
			return (!_wrename(utf8_to_wstring(oldName).c_str(), utf8_to_wstring(newName).c_str()));
#else
			return (!rename(oldName.c_str(), newName.c_str()));
#endif
		}

		std::string endl()
		{
#if ZUPPLY_OS_WINDOWS
			return consts::kEndLineCRLF;
#else // *nix, OSX, and almost everything else(OS 9 or ealier use CR only, but they are antiques now)
			return consts::kEndLineLF;
#endif
		}

		std::string current_working_directory()
		{
#if ZUPPLY_OS_WINDOWS
			wchar_t *buffer = nullptr;
			if ((buffer = _wgetcwd(nullptr, 0)) == nullptr)
			{
				// failed
				return std::string();
			}
			else
			{
				std::wstring ret(buffer);
				free(buffer);
				return wstring_to_utf8(ret);
			}
#elif __GNUC__
			char *buffer = get_current_dir_name();
			if (buffer == nullptr)
			{
				// failed
				return std::string();
			}
			else
			{
				// success
				std::string ret(buffer);
				free(buffer);
				return ret;
			}
#else
			char *buffer = getcwd(nullptr, 0);
			if (buffer == nullptr)
			{
				// failed
				return std::string();
			}
			else
			{
				// success
				std::string ret(buffer);
				free(buffer);
				return ret;
			}
#endif
		}

		bool path_exists(std::string &path, bool considerFile)
		{
#if ZUPPLY_OS_WINDOWS
			DWORD fileType = GetFileAttributesW(utf8_to_wstring(path).c_str());
			if (fileType == INVALID_FILE_ATTRIBUTES) {
				return false;
			}
			return considerFile ? true : ((fileType & FILE_ATTRIBUTE_DIRECTORY) == 0 ? false : true);
#elif ZUPPLY_OS_UNIX
			misc::unused(considerFile);
			struct stat st;
			return (stat(path.c_str(), &st) == 0);
#endif  
			return false;
		}

		std::string absolute_path(std::string reletivePath)
		{
#if ZUPPLY_OS_WINDOWS
			wchar_t *buffer = nullptr;
			std::wstring widePath = utf8_to_wstring(reletivePath);
			buffer = _wfullpath(buffer, widePath.c_str(), _MAX_PATH);
			if (buffer == nullptr)
			{
				// failed
				return std::string();
			}
			else
			{
				std::wstring ret(buffer);
				free(buffer);
				return wstring_to_utf8(ret);
			}
#elif ZUPPLY_OS_UNIX
			char *buffer = realpath(reletivePath.c_str(), nullptr);
			if (buffer == nullptr)
			{
				// failed
				return std::string();
			}
			else
			{
				std::string ret(buffer);
				free(buffer);
				return ret;
			}
#endif
			return std::string();
		}

		bool create_directory(std::string path)
		{
#if ZUPPLY_OS_WINDOWS
			std::wstring widePath = utf8_to_wstring(path);
			int ret = _wmkdir(widePath.c_str());
			if (0 == ret) return true;	// success
			if (EEXIST == ret) return true; // already exists
			return false;
#elif ZUPPLY_OS_UNIX
			// read/write/search permissions for owner and group
			// and with read/search permissions for others
			int status = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			if (0 == status) return true; // success
			if (EEXIST == errno) return true; // already exists
			return false;
#endif
			return false;
		}

		bool create_directory_recursive(std::string path)
		{
			std::string tmp = absolute_path(path);
			std::string target = tmp;

			while (!path_exists(tmp))
			{
				if (tmp.empty()) return false;	// invalid path
				tmp = absolute_path(tmp + "/../");
			}

			// tmp is the root from where to build
			auto list = path_split(fmt::lstrip(target, tmp));
			for (auto sub : list)
			{
				tmp = path_join({ tmp, sub });
				if (!create_directory(tmp)) break;
			}
			return path_exists(path);
		}

	}// namespace os

	namespace cfg
	{
		//bool Value::booleanValue() const
		//{
		//	std::string lowered = fmt::to_lower_ascii(str_);
		//	if ("true" == lowered) return true;
		//	if ("false" == lowered) return false;
		//	try
		//	{
		//		int iv = intValue();
		//		if (1 == iv) return true;
		//		if (0 == iv) return false;
		//	}
		//	catch (...)
		//	{
		//	}
		//	throw ArgException("Invalid boolean value: " + str_);
		//}

		//std::vector<double> Value::doubleVector() const
		//{
		//	std::vector<double> vec;
		//	std::string::size_type sz;
		//	std::string str(str_);
		//	double val;
		//	while (1)
		//	{
		//		try
		//		{
		//			val = std::stod(str, &sz);
		//		}
		//		catch (...)
		//		{
		//			return vec;
		//		}
		//		vec.push_back(val);
		//		str = str.substr(sz);
		//		if (str.empty()) return vec;
		//	}
		//}

		//std::vector<int> Value::intVector() const
		//{
		//	auto dvec = doubleVector();
		//	std::vector<int> ivec;
		//	for (auto d : dvec)
		//	{
		//		ivec.push_back(math::round(d));
		//	}
		//	return ivec;
		//}

		CfgParser::CfgParser(std::string filename) : ln_(0)
		{
			os::ifstream_open(stream_, filename, std::ios::in|std::ios::binary);
			if (!stream_.is_open())
			{
				throw IOException("Failed to open config file: " + filename);
			}
			pstream_ = &stream_;
			parse(root_);
		}

		void CfgParser::error_handler(std::string msg)
		{
			throw RuntimeException("Parser failed at line<" + std::to_string(ln_) + "> " + line_ + "\n" + msg);
		}

		CfgLevel* CfgParser::parse_section(std::string& sline, CfgLevel* lvl)
		{
			if (sline.empty()) return lvl;
			if (sline.back() == '.') error_handler("section ends with .");
			auto pos = sline.find('.');
			std::string currentSection;
			std::string nextSection;
			if (std::string::npos == pos)
			{
				currentSection = sline;
			}
			else
			{
				currentSection = sline.substr(0, pos);
				nextSection = sline.substr(pos + 1);
			}
			CfgLevel *l = &lvl->sections[currentSection];
			l->depth = lvl->depth + 1;
			l->parent = lvl;
			l->prefix = lvl->prefix + currentSection + ".";
			return parse_section(nextSection, l);
		}

		CfgLevel* CfgParser::parse_key_section(std::string& vline, CfgLevel *lvl)
		{
			if (vline.back() == '.') error_handler("key ends with .");
			auto pos = vline.find('.');
			if (std::string::npos == pos) return lvl;
			std::string current = vline.substr(0, pos);
			vline = vline.substr(pos + 1);
			CfgLevel *l = &lvl->sections[current];
			l->depth = lvl->depth + 1;
			l->parent = lvl;
			l->prefix = lvl->prefix + current + ".";
			return parse_key_section(vline, l);
		}

		std::string CfgParser::split_key_value(std::string& line)
		{
			auto pos1 = line.find_first_of(':');
			auto pos2 = line.find_first_of('=');
			if (std::string::npos == pos1 && std::string::npos == pos2)
			{
				throw RuntimeException("':' or '=' expected in key-value pair, none found.");
			}
			auto pos = (std::min)(pos1, pos2);
			std::string key = fmt::trim(line.substr(0, pos));
			line = fmt::trim(line.substr(pos + 1));
			return key;
		}

		void CfgParser::parse(CfgLevel& lvl)
		{
			CfgLevel *ls = &lvl;
			while (std::getline(*pstream_, line_))
			{
				++ln_;
				line_ = fmt::rskip(line_, "#");
				line_ = fmt::rskip(line_, ";");
				fmt::trim(line_);
				if (line_.empty()) continue;
				if (line_[0] == '[')
				{
					// start a section
					if (line_.back() != ']')
					{
						error_handler("missing right ]");
					}
					ls = &lvl;
					ls = parse_section(line_.substr(1, line_.length()-2), ls);
				}
				else
				{
					// parse section inline with key
					std::string key = split_key_value(line_);
					CfgLevel *lv = ls;
					lv = parse_key_section(key, lv);
					if (lv->values.find(key) != lv->values.end())
					{
						error_handler("duplicate key");
					}
					lv->values[key] = line_;
				}
			}
		}

		ArgOption::ArgOption(char shortKey, std::string longKey) :
			shortKey_(shortKey), longKey_(longKey),
			type_(""), default_(""), required_(false),
			min_(0), max_(-1), once_(false), count_(0),
			val_(""), size_(0)
		{}

		ArgOption::ArgOption(char shortKey) : ArgOption(shortKey, "")
		{}

		ArgOption::ArgOption(std::string longKey) : ArgOption(-1, longKey)
		{}

		ArgOption& ArgOption::call(std::function<void()> todo)
		{
			callback_.push_back(todo);
			return *this;
		}

		ArgOption& ArgOption::call(std::function<void()> todo, std::function<void()> otherwise)
		{
			callback_.push_back(todo);
			othercall_.push_back(otherwise);
			return *this;
		}

		ArgOption& ArgOption::set_help(std::string helpInfo)
		{
			help_ = helpInfo;
			return *this;
		}

		ArgOption& ArgOption::require(bool require)
		{
			required_ = require;
			return *this;
		}

		ArgOption& ArgOption::set_once(bool onlyOnce)
		{
			once_ = onlyOnce;
			return *this;
		}

		ArgOption& ArgOption::set_type(std::string type)
		{
			type_ = type;
			return *this;
		}

		ArgOption& ArgOption::set_min(int minCount)
		{
			min_ = minCount > 0 ? minCount : 0;
			return *this;
		}

		ArgOption& ArgOption::set_max(int maxCount)
		{
			max_ = maxCount;
			if (max_ < 0) max_ = -1;
			return *this;
		}

		std::string ArgOption::get_help()
		{
			// target: '-i, --input=FILE		this is an example input(default: abc.txt)'
			const std::size_t alignment = 26;
			std::string ret;
			if (shortKey_ != -1)
			{
				ret += "-";
				ret.push_back(shortKey_);
			}
			if (!longKey_.empty())
			{
				if (!ret.empty()) ret += ", ";
				ret += "--";
				ret += longKey_;
			}
			if (!type_.empty())
			{
				ret.push_back('=');
				ret += type_;
			}
			if (ret.size() < alignment)
			{
				ret.resize(alignment);
			}
			else
			{
				// super long option, then create a new line
				ret.push_back('\n');
				std::string tmp;
				tmp.resize(alignment);
				ret += tmp;
			}
			ret += help_;
			if (!default_.empty())
			{
				ret += "(default: ";
				ret += default_;
				ret += ")";
			}
			return ret;
		}

		ArgParser::ArgParser() : info_({ "program", "?" })
		{
			add_opt_internal(-1, "", false);	// reserve for help
			add_opt_internal(-1, "", false);	// reserve for version
		}

		void ArgParser::register_keys(char shortKey, std::string longKey, std::size_t pos)
		{
			if (shortKey != -1)
			{
				if (shortKey < 32 || shortKey > 126)
				{
					// unsupported ASCII characters
					throw ArgException("Unsupported ASCII character: " + shortKey);
				}
				auto opt = shortKeys_.find(shortKey);
				if (opt != shortKeys_.end())
				{
					std::string tmp("Short key: ");
					tmp.push_back(shortKey);
					throw ArgException(tmp + " already occupied -> " + options_[opt->second].get_help());
				}
				// register short key
				shortKeys_[shortKey] = pos;
			}

			if (!longKey.empty())
			{
				auto opt = longKeys_.find(longKey);
				if (opt != longKeys_.end())
				{
					throw ArgException("Long key: " + longKey + " already occupied -> " + options_[opt->second].get_help());
				}
				// register long key
				longKeys_[longKey] = pos;
			}
		}

		ArgOption& ArgParser::add_opt_internal(char shortKey, std::string longKey, bool active)
		{
			if (shortKey == -1 && longKey.empty())
			{
				if (active)	throw ArgException("At least one valid key required!");
			}

			options_.push_back(ArgOption(shortKey, longKey));
			register_keys(shortKey, longKey, options_.size()-1);
			return options_.back();
		}

		ArgOption& ArgParser::add_opt(char shortKey, std::string longKey)
		{
			return add_opt_internal(shortKey, longKey);
		}

		ArgOption& ArgParser::add_opt(char shortKey)
		{
			return add_opt_internal(shortKey, "");
		}

		ArgOption& ArgParser::add_opt(std::string longKey)
		{
			return add_opt_internal(-1, longKey);
		}

		void ArgParser::add_opt_help(char shortKey, std::string longKey, std::string help)
		{
			auto& opt = options_[0];	// options_[0] reserved for help
			register_keys(shortKey, longKey, 0);
			opt.shortKey_ = shortKey;
			opt.longKey_ = longKey;
			opt.set_help(help)
				.call([this]{std::cout << this->get_help() << std::endl; std::exit(0); });
		}

		void ArgParser::add_opt_help(std::string longKey, std::string help)
		{
			add_opt_help(-1, longKey, help);
		}

		void ArgParser::add_opt_version(char shortKey, std::string longKey, std::string version, std::string help)
		{
			info_[1] = version;			// info_[1] reserved for version string
			auto& opt = options_[1];	// options_[1] reserved for version option
			register_keys(shortKey, longKey, 1);
			opt.shortKey_ = shortKey;
			opt.longKey_ = longKey;
			opt.set_help(help)
				.call([this]{std::cout << this->version() << std::endl; std::exit(0); });
		}

		void ArgParser::add_opt_version(std::string longKey, std::string version, std::string help)
		{
			add_opt_version(-1, longKey, version, help);
		}

		ArgOption& ArgParser::add_opt_flag(char shortKey, std::string longKey, std::string help, bool* dst)
		{
			auto& opt = add_opt(shortKey, longKey).set_help(help);
			if (nullptr != dst)
			{
				opt.call([dst]{*dst = true; }, [dst]{*dst = false; });
			}
			return opt;
		}

		ArgOption& ArgParser::add_opt_flag(std::string longKey, std::string help, bool* dst)
		{
			return add_opt_flag(-1, longKey, help, dst);
		}

		ArgParser::Type ArgParser::check_type(std::string opt)
		{
			if (opt.length() < 2) return Type::INVALID;
			if (opt[0] == '-' && opt[1] == '-')
			{
				if (opt.length() < 3) return Type::INVALID;
				return Type::LONG_KEY;
			}
			else if (opt[0] == '-')
			{
				return Type::SHORT_KEY;
			}
			else
			{
				return Type::ARGUMENT;
			}
		}

		ArgParser::ArgQueue ArgParser::pretty_arguments(int argc, char** argv)
		{
			ArgQueue queue;
			for (int p = 1; p < argc; ++p)
			{
				std::string opt(argv[p]);
				Type type = check_type(opt);
				if (Type::SHORT_KEY == type)
				{
					// parse options like -abc=somevalue
					auto lr = fmt::split_first_occurance(opt, '=');
					auto first = lr.first; // -abc
					for (std::size_t i = 1; i < first.length(); ++i)
					{
						// put a b c into queue
						queue.push_back(std::make_pair("-" + first.substr(i, 1), Type::SHORT_KEY));
					}
					if (!lr.second.empty())
					{
						// put somevalue into queue
						queue.push_back(std::make_pair(lr.second, Type::ARGUMENT));
					}
				}
				else if (Type::LONG_KEY == type)
				{
					// parse long option like: --long=somevalue
					auto lr = fmt::split_first_occurance(opt, '=');
					queue.push_back(std::make_pair(lr.first, Type::LONG_KEY));
					if (!lr.second.empty())
					{
						queue.push_back(std::make_pair(lr.second, Type::ARGUMENT));
					}
				}
				else if (Type::ARGUMENT == type)
				{
					if (opt.length() > 2)
					{
						if (opt.front() == '\'' && opt.back() == '\'')
						{
							opt = opt.substr(1, opt.length() - 2);
						}
					}
					queue.push_back(std::make_pair(opt, Type::ARGUMENT));
				}
				else
				{
					queue.push_back(std::make_pair(opt, Type::INVALID));
				}
			}
			return queue;
		}

		void ArgParser::error_option(std::string opt, std::string msg)
		{
			errors_.push_back("Error parsing option: " + opt + " " + msg);
		}

		void ArgParser::parse_option(ArgOption* ptr)
		{
			++ptr->count_;
		}

		void ArgParser::parse_value(ArgOption* ptr, const std::string& value)
		{
			if (ptr == nullptr) args_.push_back(value);
			else
			{
				if (ptr->max_ < 0 || ptr->size_ < ptr->max_)
				{
					// boundless or not yet reached maximum
					ptr->val_ = ptr->val_.str() + value + " ";
					++ptr->size_;
				}
				else
				{
					args_.push_back(value);
					ptr = nullptr;
				}
			}
		}

		void ArgParser::parse(int argc, char** argv, bool ignoreUnknown)
		{
			if (argc < 1)
			{
				errors_.push_back("Argc < 1");
				return;
			}

			// 0. prog name
			info_[0] = os::path_split_filename(std::string(argv[0]));

			// 1. make prettier argument queue
			auto queue = pretty_arguments(argc, argv);

			// 2. parse one by one
			ArgOption *opt = nullptr;
			for (auto q : queue)
			{
				if (Type::SHORT_KEY == q.second)
				{
					auto iter = shortKeys_.find(q.first[1]);
					if (iter == shortKeys_.end())
					{
						if (!ignoreUnknown) error_option(q.first, "unknown option.");
						continue;
					}
					opt = &options_[iter->second];
					parse_option(opt);
				}
				else if (Type::LONG_KEY == q.second)
				{
					auto iter = longKeys_.find(q.first.substr(2));
					if (iter == longKeys_.end())
					{
						if (!ignoreUnknown) error_option(q.first, "unknown option.");
						continue;
					}
					opt = &options_[iter->second];
					parse_option(opt);
				}
				else if (Type::ARGUMENT == q.second)
				{
					parse_value(opt, q.first);
				}
				else
				{
					error_option(q.first, "invalid option.");
				}
			}

			// 3. callbacks
			for (auto o : options_)
			{
				if (o.count_ > 0)
				{
					// callback functions
					for (auto call : o.callback_)
					{
						call();
					}
				}
				else
				{
					// not found, call the other functions
					for (auto othercall : o.othercall_)
					{
						othercall();
					}
				}
			}
			

			// 4. check errors
			for (auto o : options_)
			{
				if (o.required_ && (o.count_ < 1))
				{
					errors_.push_back("Required option not found: <" + o.get_help() + ">");
				}
				if (o.min_ > o.size_)
				{
					errors_.push_back("<" + o.get_help() + "> need at least " + std::to_string(o.min_) + " arguments.");
				}
				if (o.count_ > 1 && o.once_)
				{
					errors_.push_back("<" + o.get_help() + "> limited to be called only once.");
				}
			}
		}

		std::string ArgParser::get_help()
		{
			std::string ret("Usage: ");
			ret += info_[0];
			ret += "\n  version: " + info_[1] + "\n";
			for (std::size_t i = 2; i < info_.size(); ++i)
			{
				ret += "  " + info_[i] + "\n";
			}

			ret.push_back('\n');
			for (auto opt : options_)
			{
				ret += "  " + opt.get_help() + "\n";
			}
			return ret;
		}

		std::string ArgParser::get_error()
		{
			std::string ret;
			for (auto e : errors_)
			{
				ret += e;
				ret.push_back('\n');
			}
			return ret;
		}

		int	ArgParser::count(char shortKey)
		{
			auto iter = shortKeys_.find(shortKey);
			return iter == shortKeys_.end() ? 0 : options_[iter->second].count_;
		}

		int ArgParser::count(std::string longKey)
		{
			auto iter = longKeys_.find(longKey);
			return iter == longKeys_.end() ? 0 : options_[iter->second].count_;
		}

		Value ArgParser::operator[](const std::string& longKey)
		{
			auto iter = longKeys_.find(longKey);
			if (iter == longKeys_.end())
			{
				return Value();
			}
			else
			{
				return options_[iter->second].val_;
			}
		}

		Value ArgParser::operator[](const char shortKey)
		{
			auto iter = shortKeys_.find(shortKey);
			if (iter == shortKeys_.end())
			{
				return Value();
			}
			else
			{
				return options_[iter->second].val_;
			}
		}

	} // namespace cfg

} // end namesapce zz
