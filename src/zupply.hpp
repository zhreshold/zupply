/* Doxygen main page */
/*!	 \mainpage zupply Main Page
#	 zupply - Portable light-weight multi-functional easy to use library for C++

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




#ifndef _ZUPPLY_ZUPPLY_HPP_
#define _ZUPPLY_ZUPPLY_HPP_

///////////////////////////////////////////////////////////////
// Require C++ 11 features
///////////////////////////////////////////////////////////////
#if ((defined(_MSC_VER) && _MSC_VER >= 1800) || __cplusplus >= 201103L)
// VC++ defined an older version of __cplusplus, but it should work later than vc12
#else
#error C++11 required, add -std=c++11 to CFLAG.
#endif

#ifdef _MSC_VER

// Disable silly security warnings
//#ifndef _CRT_SECURE_NO_WARNINGS
//#define _CRT_SECURE_NO_WARNINGS
//#endif

// Define __func__ macro
#ifndef __func__
#define __func__ __FUNCTION__
#endif 

// Define NDEBUG in Release Mode, ensure assert() disabled.
#if (!defined(_DEBUG) && !defined(NDEBUG))
#define NDEBUG
#endif

#if _MSC_VER < 1900
#define ZUPPLY_NOEXCEPT throw()
#else // NON MSVC
#define ZUPPLY_NOEXCEPT noexcept
#endif

#endif

#ifdef ZUPPLY_HEADER_ONLY
#define ZUPPLY_EXPORT inline
#else
#define ZUPPLY_EXPORT
#endif

#include <string>
#include <exception>
#include <fstream>
#include <utility>
#include <iostream>
#include <vector>
#include <locale>
#include <codecvt>
#include <ctime>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <map>
#include <unordered_map>
#include <memory>
#include <type_traits>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <functional>

namespace zz
{
	/*!
	* \namespace	params
	*
	* \brief	namespace for parameters.
	*/
	namespace consts
	{
		static const char* kExceptionPrefixGeneral = "[Zupply Exception] ";
		static const char* kExceptionPrefixLogic = "[Zupply Exception->Logic] ";
		static const char* kExceptionPrefixArgument = "[Zupply Exception->Logic->Argument] ";
		static const char* kExceptionPrefixRuntime = "[Zupply Exception->Runtime] ";
		static const char* kExceptionPrefixIO = "[Zupply Exception->Runtime->IO] ";
		static const char* kExceptionPrefixCast = "[Zupply Exception->Runtime->Cast] ";
		static const char* kExceptionPrefixMemory = "[Zupply Exception->Runtime->Memory] ";
		static const char* kExceptionPrefixStrictWarn = "[Zupply Exception->StrictWarn] ";
	}

	/*!
	* \class	UnCopyable
	*
	* \brief	A not copyable base class, should be inheritated privately.
	*/
	class UnCopyable
	{
	public:
		UnCopyable() {};
		// no copy constructor
		UnCopyable(const UnCopyable&) = delete;
		// no copy operator
		UnCopyable& operator=(const UnCopyable&) = delete;
	};

	/*!
	* \class	UnMovable
	*
	* \brief	A not movable/copyable base class, should be inheritated privately.
	*/
	class UnMovable
	{
	public:
		UnMovable() {};
		// no copy constructor
		UnMovable(const UnMovable&) = delete;
		// no copy operator
		UnMovable& operator=(const UnMovable&) = delete;
		// no move constructor
		UnMovable(UnMovable&&) = delete;
		// no move operator
		UnMovable& operator=(UnMovable&&) = delete;
	};

	/*!
	* \class	Exception
	*
	* \brief	An exception with customized prefix information.
	*/
	class Exception : public std::exception
	{
	public:
		explicit Exception(const char* message, const char* prefix = consts::kExceptionPrefixGeneral)
		{
			message_ = std::string(prefix) + message;
		};
		explicit Exception(const std::string message, const char* prefix = consts::kExceptionPrefixGeneral)
		{
			message_ = std::string(prefix) + message;
		};
		virtual ~Exception() ZUPPLY_NOEXCEPT{};

		const char* what() const ZUPPLY_NOEXCEPT{ return message_.c_str(); };
	private:
		std::string message_;
	};

	/*!
	* \class	LogicException
	*
	* \brief	Exception for signalling logic errors.
	*/
	class LogicException : public Exception
	{
	public:
		explicit LogicException(const char *message) : Exception(message, consts::kExceptionPrefixLogic){};
		explicit LogicException(const std::string &message) : Exception(message, consts::kExceptionPrefixLogic){};
	};

	/*!
	* \class	ArgException
	*
	* \brief	Exception for signalling argument errors.
	*/
	class ArgException : public Exception
	{
	public:
		explicit ArgException(const char *message) : Exception(message, consts::kExceptionPrefixArgument){};
		explicit ArgException(const std::string &message) : Exception(message, consts::kExceptionPrefixArgument){};
	};

	/*!
	* \class	RuntimeException
	*
	* \brief	Exception for signalling unexpected runtime errors.
	*/
	class RuntimeException : public Exception
	{
	public:
		explicit RuntimeException(const char *message) : Exception(message, consts::kExceptionPrefixRuntime){};
		explicit RuntimeException(const std::string &message) : Exception(message, consts::kExceptionPrefixRuntime){};
	};

	/*!
	* \class	CastException
	*
	* \brief	Exception for signalling unsuccessful cast operations.
	*/
	class CastException : public Exception
	{
	public:
		explicit CastException(const char *message) : Exception(message, consts::kExceptionPrefixCast){};
		explicit CastException(const std::string &message) : Exception(message, consts::kExceptionPrefixCast){};
	};

	/*!
	* \class	IOException
	*
	* \brief	Exception for signalling unexpected IO errors.
	*/
	class IOException : public Exception
	{
	public:
		explicit IOException(const char *message) : Exception(message, consts::kExceptionPrefixIO){};
		explicit IOException(const std::string &message) : Exception(message, consts::kExceptionPrefixIO){};
	};

	/*!
	* \class	MemException
	*
	* \brief	Exception for signalling memory errors.
	*/
	class MemException : public Exception
	{
	public:
		explicit MemException(const char *message) : Exception(message, consts::kExceptionPrefixMemory){};
		explicit MemException(const std::string &message) : Exception(message, consts::kExceptionPrefixMemory){};
	};

	/*!
	* \class	WarnException
	*
	* \brief	Exception for signalling warning errors when strict warning is enabled.
	*/
	class WarnException : public Exception
	{
	public:
		explicit WarnException(const char *message) : Exception(message, consts::kExceptionPrefixStrictWarn){};
		explicit WarnException(const std::string &message) : Exception(message, consts::kExceptionPrefixStrictWarn){};
	};

	struct Size
	{
		int width;
		int height;

		Size(int x, int y) : width(x), height(y) {}
		bool operator=(Size& other)
		{
			return (width == other.width) && (height == other.height);
		}
	};

	namespace math
	{
		using std::min;
		using std::max;
		using std::abs;

		template<class T> inline const T clip(const T& value, const T& low, const T& high)
		{
			// fool proof max/min values
			T h = (std::max)(low, high);
			T l = (std::min)(low, high);
			return (std::max)((std::min)(value, h), l);
		}

		template <unsigned long B, unsigned long E>
		struct Pow
		{
			static const unsigned long result = B * Pow<B, E - 1>::result;
		};

		template <unsigned long B>
		struct Pow<B, 0>
		{
			static const unsigned long result = 1;
		};


		inline int round(double value)
		{
#if ((defined _MSC_VER && defined _M_X64) || (defined __GNUC__ && defined __x86_64__ && defined __SSE2__ && !defined __APPLE__)) && !defined(__CUDACC__) && 0
			__m128d t = _mm_set_sd(value);
			return _mm_cvtsd_si32(t);
#elif defined _MSC_VER && defined _M_IX86
			int t;
			__asm
			{
				fld value;
				fistp t;
			}
			return t;
#elif defined _MSC_VER && defined _M_ARM && defined HAVE_TEGRA_OPTIMIZATION
			TEGRA_ROUND(value);
#elif defined CV_ICC || defined __GNUC__
#  ifdef HAVE_TEGRA_OPTIMIZATION
			TEGRA_ROUND(value);
#  else
			return (int)lrint(value);
#  endif
#else
			double intpart, fractpart;
			fractpart = modf(value, &intpart);
			if ((fabs(fractpart) != 0.5) || ((((int)intpart) % 2) != 0))
				return (int)(value + (value >= 0 ? 0.5 : -0.5));
			else
				return (int)intpart;
#endif
		}

	} // namespace math

	namespace misc
	{
		template<typename T>
		inline void unused(const T&) {}

		namespace detail
		{
			// To allow ADL with custom begin/end
			using std::begin;
			using std::end;

			template <typename T>
			auto is_iterable_impl(int)
				-> decltype (
				begin(std::declval<T&>()) != end(std::declval<T&>()), // begin/end and operator !=
				++std::declval<decltype(begin(std::declval<T&>()))&>(), // operator ++
				*begin(std::declval<T&>()), // operator*
				std::true_type{});

			template <typename T>
			std::false_type is_iterable_impl(...);

		}

		template <typename T>
		using is_iterable = decltype(detail::is_iterable_impl<T>(0));

		class Callback
		{
		public:
			Callback(std::function<void()> f) : f_(f) { f_(); }
		private:
			std::function<void()> f_;
		};
	} // namespace misc

	namespace cds
	{
		/*!
		* \struct	NullMutex
		*
		* \brief	A null mutex, no cost.
		*/
		class NullMutex
		{
		public:
			void lock() {}
			void unlock() {}
			bool try_lock()
			{
				return true;
			}
		};


		template <typename Key, typename Value> class AtomicUnorderedMap
		{
			using MapType = std::unordered_map<Key, Value>;
			using MapPtr = std::shared_ptr<MapType>;
		public:
			AtomicUnorderedMap()
			{
				mapPtr_ = std::make_shared<MapType>();
			}

			MapPtr get()
			{
				return std::atomic_load(&mapPtr_);
			}

			bool insert(const Key& key, const Value& value)
			{
				MapPtr p = std::atomic_load(&mapPtr_);
				MapPtr copy;
				do
				{
					if ((*p).count(key) > 0) return false;
					copy = std::make_shared<MapType>(*p);
					(*copy).insert({ key, value });
				} while (!std::atomic_compare_exchange_weak(&mapPtr_, &p, std::move(copy)));
				return true;
			}

			bool erase(const Key& key)
			{
				MapPtr p = std::atomic_load(&mapPtr_);
				MapPtr copy;
				do
				{
					if ((*p).count(key) <= 0) return false;
					copy = std::make_shared<MapType>(*p);
					(*copy).erase(key);
				} while (!std::atomic_compare_exchange_weak(&mapPtr_, &p, std::move(copy)));
				return true;
			}

			void clear()
			{
				MapPtr p = atomic_load(&mapPtr_);
				auto copy = std::make_shared<MapType>();
				do
				{
					;	// do clear when possible does not require old status
				} while (!std::atomic_compare_exchange_weak(&mapPtr_, &p, std::move(copy)));
			}

		private:
			std::shared_ptr<MapType>	mapPtr_;
		};

		template <typename T> class AtomicNonTrivial
		{
		public:
			AtomicNonTrivial()
			{
				ptr_ = std::make_shared<T>();
			}

			std::shared_ptr<T> get()
			{
				return std::atomic_load(&ptr_);
			}

			void set(const T& val)
			{
				std::shared_ptr<T> copy = std::make_shared<T>(val);
				std::atomic_store(&ptr_, copy);
			}

		private:
			std::shared_ptr<T>	ptr_;
		};

	} // namespace cds



	namespace os
	{
		int system(const char *const command, const char *const module_name = 0);
		std::size_t thread_id();
		std::tm localtime(std::time_t t);
		std::tm gmtime(std::time_t t);

		/*!
		* \fn	std::wstring utf8_to_wstring(std::string &u8str);
		*
		* \brief	UTF 8 to wstring.
		*
		* \note	This is actually only useful in windows.
		*
		* \param [in,out]	u8str	The 8str.
		*
		* \return	A std::wstring.
		*/
		std::wstring utf8_to_wstring(std::string &u8str);

		std::string wstring_to_utf8(std::wstring &wstr);

		bool path_exists(std::string &path, bool considerFile = false);


		void fstream_open(std::fstream &stream, std::string &filename, std::ios::openmode openmode);
		void ifstream_open(std::ifstream &stream, std::string &filename, std::ios::openmode openmode);

		bool rename(std::string oldName, std::string newName);

		/*!
		* \fn	std::string endl();
		*
		* \brief	Gets the OS dependent line end characters.
		*
		* \return	A std::string.
		*/
		std::string endl();

		std::string current_working_directory();

		std::string absolute_path(std::string reletivePath);

		std::vector<std::string> path_split(std::string path);

		std::string path_join(std::vector<std::string> elems);

		std::string path_split_filename(std::string path);

		std::string path_split_directory(std::string path);

		std::string path_split_basename(std::string path);

		std::string path_split_extension(std::string path);

		std::string path_append_basename(std::string origPath, std::string whatToAppend);

		bool create_directory(std::string path);

		bool create_directory_recursive(std::string path);

		Size console_size();

	} // namespace os

	namespace time
	{
		/*!
		* \class	Date
		*
		* \brief	A date class.
		*/
		class Date
		{
		public:
			Date();
			virtual ~Date() = default;

			void to_local_time();

			void to_utc_time();

			std::string to_string(const char *format = "%y-%m-%d %H:%M:%S.%frac");

			static Date local_time();

			static Date utc_time();

		private:
			std::time_t		timeStamp_;
			int				fraction_;
			std::string		fractionStr_;
			struct std::tm	calendar_;

		};

		/*!
		* \class	Timer
		*
		* \brief	A timer class.
		*/
		class Timer
		{
		public:
			Timer();

			void reset();

			std::size_t	elapsed_ns();

			std::string elapsed_ns_str();

			std::size_t elapsed_us();

			std::string elapsed_us_str();

			std::size_t elapsed_ms();

			std::string elapsed_ms_str();

			std::size_t elapsed_sec();

			std::string elapsed_sec_str();

			double elapsed_sec_double();

			std::string to_string(const char *format = "[%ms ms]");

		private:
			std::chrono::steady_clock::time_point timeStamp_;
		};

		/*!
		* \fn	void sleep(int milliseconds);
		*
		* \brief	Sleep for specified milliseconds
		*
		* \param	milliseconds	The milliseconds.
		*/
		inline void sleep(int milliseconds)
		{
			if (milliseconds > 0)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
			}
		}

	} // namespace time

	namespace fs
	{
		namespace consts
		{
			static const int kDefaultFileOpenRetryTimes = 5;
			static const int kDefaultFileOpenRetryInterval = 10;
		}

		class FileEditor : private UnCopyable
		{
		public:
			FileEditor() = default;
			FileEditor(std::string filename, bool truncateOrNot = false,
				int retryTimes = consts::kDefaultFileOpenRetryTimes,
				int retryInterval = consts::kDefaultFileOpenRetryInterval)
			{
				// use absolute path
				filename_ = os::absolute_path(filename);
				// try open
				this->try_open(retryTimes, retryInterval, truncateOrNot);
			};

			/*!
			* \fn	File::File(File&& from)
			*
			* \brief	Move constructor.
			*
			* \param [in,out]	other	Source for the instance.
			*/
			FileEditor(FileEditor&& other) : filename_(std::move(other.filename_)),
				stream_(std::move(other.stream_)),
				readPos_(std::move(other.readPos_)),
				writePos_(std::move(other.writePos_))
			{
				other.filename_ = std::string();
			};

			virtual ~FileEditor() { this->close(); }

			// No move operator
			FileEditor& operator=(FileEditor&&) = delete;

			// overload << operator
			template <typename T>
			FileEditor& operator<<(T what) { stream_ << what; return *this; }

			std::string filename() const { return filename_; }

			bool open(bool truncateOrNot = false);
			bool open(std::string filename, bool truncateOrNot = false,
				int retryTimes = consts::kDefaultFileOpenRetryTimes,
				int retryInterval = consts::kDefaultFileOpenRetryInterval);

			bool try_open(int retryTime, int retryInterval, bool truncateOrNot = false);
			bool reopen(bool truncateOrNot = true);
			void close();
			bool is_valid() const { return !filename_.empty(); }
			bool is_open() const { return stream_.is_open(); }
			void flush() { stream_.flush(); }
		private:

			void check_valid() { if (!this->is_valid()) throw RuntimeException("Invalid File Editor!"); }

			std::string		filename_;
			std::fstream	stream_;
			std::streampos	readPos_;
			std::streampos	writePos_;
		};

		class FileReader : private UnCopyable
		{
		public:
			FileReader() = delete;
			FileReader(std::string filename, int retryTimes = consts::kDefaultFileOpenRetryTimes,
				int retryInterval = consts::kDefaultFileOpenRetryInterval)
			{
				// use absolute path
				filename_ = os::absolute_path(filename);
				// try open
				this->try_open(retryTimes, retryInterval);
			}

			FileReader(FileReader&& other) : filename_(std::move(other.filename_)), istream_(std::move(other.istream_))
			{
				other.filename_ = std::string();
			};

			std::string filename() const { return filename_; }

			bool is_open() const { return istream_.is_open(); }
			bool is_valid() const { return !filename_.empty(); }
			bool open();
			bool try_open(int retryTime, int retryInterval);
			void close() { istream_.close(); };
			std::size_t	file_size();

		private:

			void check_valid(){ if (!this->is_valid()) throw RuntimeException("Invalid File Reader!"); }
			std::string		filename_;
			std::ifstream	istream_;
		};

		std::size_t get_file_size(std::string filename);

	} //namespace fs


	namespace fmt
	{
		namespace consts
		{
			static const std::string kFormatSpecifierPlaceHolder = std::string("{}");
		}

		namespace detail
		{
			template<class Facet>
			struct DeletableFacet : Facet
			{
				template<class ...Args>
				DeletableFacet(Args&& ...args) : Facet(std::forward<Args>(args)...) {}
				~DeletableFacet() {}
			};
		} // namespace detail

		inline std::string int_to_zero_pad_str(int num, int length)
		{
			std::ostringstream ss;
			ss << std::setw(length) << std::setfill('0') << num;
			return ss.str();
		}

		bool ends_with(const std::string& str, const std::string& end);

		bool str_equals(const char* s1, const char* s2);

		std::string& ltrim(std::string& str);

		std::string& rtrim(std::string& str);

		std::string& trim(std::string& str);

		std::string& lstrip(std::string& str, std::string what);

		std::string& rstrip(std::string& str, std::string what);

		std::string& rskip(std::string& str, std::string delim);

		std::vector<std::string> split(const std::string s, char delim = ' ');

		std::vector<std::string> split(const std::string s, std::string delim);

		std::vector<std::string> split_multi_delims(const std::string s, std::string delims);

		std::vector<std::string> split_whitespace(const std::string s);

		std::pair<std::string, std::string> split_first_occurance(const std::string s, char delim);

		std::string join(std::vector<std::string> elems, char delim);

		std::vector<std::string>& erase_empty(std::vector<std::string> &vec);

		void replace_first_with_escape(std::string &str, const std::string &replaceWhat, const std::string &replaceWith);

		void replace_all_with_escape(std::string &str, const std::string &replaceWhat, const std::string &replaceWith);

		std::string to_lower_ascii(std::string mixed);

		std::string to_upper_ascii(std::string mixed);

		inline std::u16string utf8_to_utf16(std::string &u8str)
		{
			std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> cvt;
			return cvt.from_bytes(u8str);
		}

		inline std::string utf16_to_utf8(std::u16string &u16str)
		{
			std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> cvt;
			return cvt.to_bytes(u16str);
		}

		inline std::u32string utf8_to_utf32(std::string &u8str)
		{
			std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> cvt;
			return cvt.from_bytes(u8str);
		}

		inline std::string utf32_to_utf8(std::u32string &u32str)
		{
			std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> cvt;
			return cvt.to_bytes(u32str);
		}

		template<typename Arg>
		inline void format_string(std::string &fmt, const Arg &last)
		{
			replace_first_with_escape(fmt, consts::kFormatSpecifierPlaceHolder, std::to_string(last));
		}

		template<typename Arg, typename... Args>
		inline void format_string(std::string &fmt, const Arg& current, const Args&... more)
		{
			replace_first_with_escape(fmt, consts::kFormatSpecifierPlaceHolder, std::to_string(current));
			format_string(fmt, more...);
		}

	} // namespace fmt

	namespace cfg
	{
		//class Value
		//{
		//public:
		//	Value() {}
		//	Value(std::string valueStr) : str_(valueStr) {}
		//	Value(const Value& other) : str_(other.str_) {}
		//	std::string str() const { return str_; }
		//	int	intValue() const { return std::stoi(str_); }
		//	bool booleanValue() const;
		//	long longIntValue() const { return std::stol(str_); }
		//	double doubleValue() const { return std::stod(str_); }
		//	std::vector<double> doubleVector() const;
		//	std::vector<int> intVector() const;
		//	bool empty() { return str_.empty(); }
		//	bool operator== (Value& other) { return other.str() == str_; }
		//	
		//	template <typename T> Value& operator<< (T t)
		//	{
		//		std::ostringstream oss;
		//		oss << t;
		//		if (!str_.empty())
		//		{
		//			if (str_.back() !=)
		//		}
		//		return *this;
		//	}

		//private:
		//	std::string str_;
		//};

		//class Value
		//{
		//public:
		//	Value(){}
		//	Value(std::string str) { ss_.str(str); }
		//	Value(const Value& other) { ss_ << other.ss_.rdbuf(); }
		//	std::string str() const { return ss_.str(); }
		//	bool empty() { return ss_.rdbuf()->in_avail() == 0; }
		//	bool operator== (Value& other) { return ss_.str() == other.str(); }
		//	void operator= (std::string str) { ss_.clear(); ss_.str(str); }
		//	void operator= (const Value& other) { ss_.clear(); ss_ << other.ss_.rdbuf(); }
		//	template <typename T> Value& operator<< (T t) { ss_.clear(); ss_ << t << " "; return *this; }
		//	template <typename T> Value& operator>> (T& t) { ss_.clear(); ss_ >> t; return *this; }
		//	template <typename T> T value() { T ret; *this >> ret; return ret; }
		//	template <typename T> T value(T& t) { *this >> t; return t; }

		//private:
		//	std::stringstream ss_;
		//};

		class Value
		{
		public:
			Value(){}
			Value(const char* cstr) : str_(cstr) {}
			Value(std::string str) : str_(str) {}
			Value(const Value& other) : str_(other.str_) {}
			std::string str() const { return str_; }
			bool empty() const { return str_.empty(); }
			void clear() { str_.clear(); }
			bool operator== (const Value& other) { return str_ == other.str_; }
			Value& operator= (const Value& other) { str_ = other.str_; return *this; }
			template <typename T> std::string store(T t);
			template <typename T> T load(T& t);
			template <typename T> std::vector<T> load(std::vector<T>& t);
			template <> bool load(bool& b);
			template <typename T> T load() { T t; return load(t); }
			
		private:
			std::string str_;
		};

		template <typename T> inline std::string Value::store(T t)
		{
			std::ostringstream oss;
			oss << t;
			str_ = oss.str();
			return str_;
		}

		template <typename T> inline T Value::load(T& t)
		{
			std::istringstream iss(str_);
			iss >> t;
			return t;
		}

		template <> inline bool Value::load(bool& b)
		{
			b = false;
			std::string lowered = fmt::to_lower_ascii(str_);
			std::istringstream iss(lowered);
			iss >> std::boolalpha >> b;
			return b;
		}

		template <typename T> inline std::vector<T> Value::load(std::vector<T>& t)
		{
			std::istringstream iss(str_);
			t.clear();
			T val;
			std::string dummy;
			while (iss >> val || !iss.eof())
			{
				if (iss.fail())
				{
					iss.clear();
					iss >> dummy;
					continue;
				}
				t.push_back(val);
			}
			return t;
		}

		struct CfgLevel
		{
			CfgLevel() : parent(nullptr), depth(0) {}
			CfgLevel(CfgLevel* p, std::size_t d) : parent(p), depth(d) {}

			using value_map_t = std::map<std::string, Value>;
			using section_map_t = std::map<std::string, CfgLevel>;
			//using value_t = std::vector<value_map_t::const_iterator>;
			//using section_t = std::vector<section_map_t::const_iterator>;

			value_map_t values;
			section_map_t sections;
			std::string prefix;
			CfgLevel* parent;
			size_t depth;

			Value operator[](const std::string& name) { return values[name]; }
			CfgLevel& operator()(const std::string& name) { return sections[name]; }
			std::string to_string()
			{
				std::string ret;
				for (auto v : values)
				{
					ret += prefix + v.first + " = " + v.second.str() + "\n";
				}
				for (auto s : sections)
				{
					ret += s.second.to_string();
				}
				return ret;
			}
		};

		class CfgParser
		{
		public:
			CfgParser(std::string filename);
			CfgParser(std::istream& s) : pstream_(&s), ln_(0) { parse(root_); }
			CfgLevel& root() { return root_; }

			Value operator[](const std::string& name) { return root_.values[name]; }
			CfgLevel& operator()(const std::string& name) { return root_.sections[name]; }

		private:
			void parse(CfgLevel& lvl);
			CfgLevel* parse_section(std::string& sline, CfgLevel *lvl);
			CfgLevel* parse_key_section(std::string& vline, CfgLevel *lvl);
			std::string split_key_value(std::string& line);
			void error_handler(std::string msg);

			CfgLevel		root_;
			std::ifstream	stream_;
			std::istream	*pstream_;
			std::string		line_;
			std::size_t		ln_;
		};

		class ArgParser
		{
		public:
			using opt_vec_t = std::vector<Value>;

			ArgParser() : helper_({ "Usage: ", "" }), failbit_(false) {}

			void add_argn(char shortKey = -1, std::string longKey = "",
				std::string help = "", std::string type = "", int minCount = 0, int maxCount = -1);

			void add_arg_lite(char shortKey = -1, std::string longKey = "",
				std::string help = "", std::string type = "")
			{
				add_argn(shortKey, longKey, help, type, 0, 0);
			}

			void parse(int argc, char** argv);

			int count(std::string longKey)
			{
				if (longKeys_.count(longKey) > 0)
				{
					return opts_[longKeys_[longKey]].refCount;
				}
				return 0;
			}

			int count(char shortKey)
			{
				if (shortKeys_.count(shortKey))
				{
					return opts_[shortKeys_[shortKey]].refCount;
				}
				return 0;
			}

			const opt_vec_t unspecified()
			{
				return opts_[""].vec;
			}

			const opt_vec_t operator[](const std::string& longKey)
			{
				if (longKeys_.count(longKey) > 0)
				{
					return opts_[longKeys_[longKey]].vec;
				}
				return opt_vec_t();
			}

			const opt_vec_t operator[](const char shortKey)
			{
				if (shortKeys_.count(shortKey) > 0)
				{
					return opts_[shortKeys_[shortKey]].vec;
				}
				return opt_vec_t();
			}

			void print_help()
			{
				// header
				helper_[1].push_back(']');
				for (auto h : helper_)
				{
					std::cout << h << " ";
				}
				std::cout << std::endl;
				std::cout << "\n  Required arguments:" << std::endl;
				print_help_section(helperRequired_);
				std::cout << "\n  Optional arguments:" << std::endl;
				print_help_section(helperOptional_);
			}

			void help_exit()
			{
				print_help();
				exit(0);
			}

			bool success() const
			{
				return !failbit_;
			}

		private:

			using queue_t = std::vector<std::pair<std::string, int>>;
			using help_t = std::pair<std::string, std::string>;
			struct ArgOption
			{
				ArgOption() : minCount(0), maxCount(-1), refCount(0) {}
				ArgOption(int min, int max) :minCount(min), maxCount(max), refCount(0) {}
				opt_vec_t	vec;
				int			refCount;
				int			minCount;
				int			maxCount;
			};
			enum ArgOptType
			{
				InvalidOpt = -1,
				ShortOpt = 1,
				LongOpt = 2,
				Argument = 3
			};

			queue_t generate_queue(int argc, char** argv);
			int check_type(std::string& opt);
			help_t to_help_string(const char shortOpt, std::string& longOpt, std::string& help, std::string& type);
			void print_help_section(std::vector<help_t>& helpers);

			std::unordered_map<char, std::string> shortKeys_;
			std::unordered_map<std::string, std::string> longKeys_;
			std::unordered_map<std::string, ArgOption> opts_;
			std::vector<std::string> helper_;
			std::vector<help_t> helperRequired_;
			std::vector<help_t> helperOptional_;
			bool	failbit_;
		};

		class ArgOption2
		{
			friend class ArgParser2;	// let ArgParser access private members
		public:
			ArgOption2::ArgOption2(char shortKey, std::string longKey);
			ArgOption2::ArgOption2(char shortKey);
			ArgOption2::ArgOption2(std::string longKey);

			template <typename T> ArgOption2& store(T& dst, T defaultValue)
			{
				dst = defaultValue;
				std::ostringstream oss;
				try
				{
					oss << defaultValue;
				}
				catch (...)
				{
					throw ArgException("Unable to convert default value to string.");
				}
				if (oss.good() && !oss.str().empty()) default_ = oss.str();
				return *this;
			}
			
			template<typename T> ArgOption2& store(std::vector<T>& dst, std::vector<T> defaultValue)
			{
				dst = defaultValue;
				std::ostringstream oss;
				try
				{
					for (auto i : defaultValue)
					oss << i << " ";
				}
				catch (...)
				{
					throw ArgException("Unable to convert default value(s) to string.");
				}
				if (oss.good() && !oss.str().empty()) default_ = oss.str();
				return *this;
			}

			ArgOption2& call(std::function<void()> todo)
			{
				callback_ = todo;
				return *this;
			}

			ArgOption2& call(std::function<void()> todo, std::function<void()> otherwise)
			{
				callback_ = todo;
				otherwise();
				return *this;
			}

			ArgOption2& help(std::string helpInfo)
			{
				help_ = helpInfo;
				return *this;
			}

			ArgOption2& require(bool require = true)
			{
				required_ = require;
				return *this;
			}

			ArgOption2& once(bool onlyOnce = true)
			{
				once_ = onlyOnce;
				return *this;
			}

			ArgOption2& type(std::string type)
			{
				type_ = type;
				return *this;
			}

		private:
			std::string get_help();		//!< get help string

			char			shortKey_;	//!< short key eg. -h
			std::string		longKey_;	//!< long key eg. --help
			std::string		type_;		//!< INT, DOUBLE, STRING, FILE, whatever
			std::string		help_;		//!< detailed help info
			std::string		default_;	//!< default value
			bool			required_;	//!< not optional
			int				min_;		//!< min arguments for this option
			int				max_;		//!< max arguments for this option
			int				size_;		//!< stored argument size
			bool			once_;		//!< should not access multiply times
			int				count_;		//!< reference count
			Value			val_;		//!< stored values
			std::function<void()> callback_;	//!< call this when option found
		};

		class ArgParser2
		{
		public:
			ArgParser2();
			ArgOption2& add_opt(char shortKey);
			ArgOption2& add_opt(std::string longKey);
			ArgOption2& add_opt(char shortKey, std::string longKey);
			template <typename T> ArgOption2& add_opt_value(char shortKey, std::string longKey, T& dst, T defaultValue, std::string help="", std::string type="")
			{
				dst = defaultValue;
				Value dfltValue;
				dfltValue.store(defaultValue);
				auto& opt = add_opt(shortKey, longKey).call([shortKey, &dst, this] {(*this)[shortKey].load(dst); });
				opt.default_ = dfltValue.str();
				return opt;
			}
			template <typename T> ArgOption2& add_opt_value(char shortKey, T& dst, T defaultValue, std::string help = "", std::string type = "")
			{
				return add_opt_value(shortKey, "");
			}

			template <typename T> ArgOption2& add_opt_value(std::string longKey, T& dst, T defaultValue, std::string help = "", std::string type = "")
			{
				return add_opt_value(-1, longKey);
			}

			void parse(int argc, char** argv, bool ignoreUnknown = false);

			std::size_t count_error() { return errors_.size(); }
			std::string get_help();
			Value operator[](const std::string& longKey);
			Value operator[](const char shortKey);

		private:
			//using ArgOptIter = std::vector<ArgOption2>::iterator;
			enum class Type {SHORT_KEY, LONG_KEY, ARGUMENT, INVALID};
			using ArgQueue = std::vector<std::pair<std::string, Type>>;

			Type check_type(std::string str);
			ArgQueue pretty_arguments(int argc, char** argv);
			void error_option(std::string opt, std::string msg = "");
			void parse_option(ArgOption2* ptr);
			void parse_value(ArgOption2* ptr, const std::string& value);

			std::vector<ArgOption2> options_;	//!< hooked options
			std::unordered_map<char, std::size_t> shortKeys_;		//!< short keys -a -b 
			std::unordered_map<std::string, std::size_t> longKeys_;	//!< long keys --long --version
			std::vector<Value>	args_;	//!< anything not belong to option will be stored here
			std::vector<std::string> errors_;	//!< store parsing errors
			std::vector<std::string> info_;	//!< program name[0] from argv[0], other infos from user
		};

	} // namespace cfg

	namespace log
	{
		void zupply_internal_warn(std::string msg);
		void zupply_internal_error(std::string msg);

		namespace detail
		{
			class ScopedRedirect
			{
			public:
				ScopedRedirect(std::ostream &orig) : backup_(orig)
				{
					old = orig.rdbuf(buffer_.rdbuf());
				}

				~ScopedRedirect()
				{
					backup_.rdbuf(old);
					backup_ << buffer_.str();
				}

			private:
				ScopedRedirect(const ScopedRedirect&);
				ScopedRedirect& operator=(const ScopedRedirect&);

				std::ostream &backup_;
				std::streambuf *old;
				std::ostringstream buffer_;
			};
		}

		class ProgBar
		{

		};

		typedef enum LogLevelEnum
		{
			trace = 0,
			debug = 1,
			info = 2,
			warn = 3,
			error = 4,
			fatal = 5,
			off = 6,
			sentinel = 63
		}LogLevels;

		namespace consts
		{
			static const char	*kLevelNames[] { "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL", "OFF"};
			static const char	*kShortLevelNames[] { "T", "D", "I", "W", "E", "F", "O"};
			static const char	*kZupplyInternalLoggerName = "zupply";
			static const int	kAsyncQueueSize = 4096;	//!< asynchrous queue size, must be power of 2

			static const char	*kSinkDatetimeSpecifier = "%datetime";
			static const char	*kSinkLoggerNameSpecifier = "%logger";
			static const char	*kSinkThreadSpecifier = "%thread";
			static const char	*kSinkLevelSpecifier = "%level";
			static const char	*kSinkLevelShortSpecifier = "%lvl";
			static const char	*kSinkMessageSpecifier = "%msg";
			static const char	*kStdoutSinkName = "stdout";
			static const char	*kStderrSinkName = "stderr";
			static const char	*kSimplefileSinkType = "simplefile";
			static const char	*kRotatefileSinkType = "rotatefile";
			static const char	*kOstreamSinkType = "ostream";
			static const char	*kDefaultLoggerFormat = "[%datetime][T%thread][%logger][%level] %msg";
			static const char	*kDefaultLoggerDatetimeFormat = "%y-%m-%d %H:%M:%S.%frac";



			// config file formats
			static const char	*KConfigGlobalSectionSpecifier = "global";
			static const char	*KConfigLoggerSectionSpecifier = "loggers";
			static const char	*KConfigSinkSectionSpecifier = "sinks";
			static const char	*kConfigFormatSpecifier = "format";
			static const char	*kConfigLevelsSpecifier = "levels";
			static const char	*kConfigDateTimeFormatSpecifier = "datetime_format";
			static const char	*kConfigSinkListSpecifier = "sink_list";
			static const char	*kConfigSinkTypeSpecifier = "type";
			static const char	*kConfigSinkFilenameSpecifier = "filename";
		}

		// forward declaration
		namespace detail
		{
			struct LogMessage;
			class LineLogger;
			class SinkInterface;
		} // namespace detail
		typedef std::shared_ptr<detail::SinkInterface> SinkPtr;

		inline bool level_should_log(int levelMask, LogLevels lvl)
		{
			return (LogLevels::sentinel & levelMask & (1 << lvl)) > 0;
		}

		inline std::string level_mask_to_string(int levelMask)
		{
			std::string str("<|");
			for (int i = 0; i < LogLevels::off; ++i)
			{
				if (level_should_log(levelMask, static_cast<LogLevels>(i)))
				{
					str += consts::kLevelNames[i];
					str += "|";
				}
			}
			return str + ">";
		}

		inline LogLevels level_from_str(std::string level)
		{
			std::string upperLevel = fmt::to_upper_ascii(level);
			for (int i = 0; i < LogLevels::off; ++i)
			{
				if (upperLevel == consts::kLevelNames[i])
				{
					return static_cast<LogLevels>(i);
				}
			}
			return LogLevels::off;
		}

		inline int level_mask_from_string(std::string levels)
		{
			int mask = 0;
			auto levelList = fmt::split_whitespace(levels);
			for (auto lvl : levelList)
			{
				auto l = level_from_str(lvl);
				mask |= 1 << static_cast<int>(l);
			}
			return mask & LogLevels::sentinel;
		}

		class LogConfig
		{
		public:
			static LogConfig& instance()
			{
				static LogConfig instance_;
				return instance_;
			}

			static void set_default_format(std::string format)
			{
				LogConfig::instance().set_format(format);
			}

			static void set_default_datetime_format(std::string dateFormat)
			{
				LogConfig::instance().set_datetime_format(dateFormat);
			}

			static void set_default_sink_list(std::vector<std::string> list)
			{
				LogConfig::instance().set_sink_list(list);
			}

			static void set_default_level_mask(int levelMask)
			{
				LogConfig::instance().set_log_level_mask(levelMask);
			}

			std::vector<std::string> sink_list()
			{
				return *sinkList_.get();
			}

			void set_sink_list(std::vector<std::string> &list)
			{
				sinkList_.set(list);
			}

			int log_level_mask()
			{
				return logLevelMask_;
			}

			void set_log_level_mask(int newMask)
			{
				logLevelMask_ = newMask;
			}

			std::string format()
			{
				return *format_.get();
			}

			void set_format(std::string newFormat)
			{
				format_.set(newFormat);
			}

			std::string datetime_format()
			{
				return *datetimeFormat_.get();
			}

			void set_datetime_format(std::string newDatetimeFormat)
			{
				datetimeFormat_.set(newDatetimeFormat);
			}

		private:
			LogConfig()
			{
				// Default configurations
				sinkList_.set({ std::string(consts::kStdoutSinkName), std::string(consts::kStderrSinkName) });	//!< attach console by default
#ifdef NDEBUG
				logLevelMask_ = 0x3C;	//!< 0x3C->b111100: no debug, no trace
#else
				logLevelMask_ = 0x3E;	//!< 0x3E->b111110: debug, no trace
				format_.set(std::string(consts::kDefaultLoggerFormat));
				datetimeFormat_.set(std::string(consts::kDefaultLoggerDatetimeFormat));
#endif
			}

			cds::AtomicNonTrivial<std::vector<std::string>> sinkList_;
			std::atomic_int logLevelMask_;
			cds::AtomicNonTrivial<std::string> format_;
			cds::AtomicNonTrivial<std::string> datetimeFormat_;
		};

		class Logger : UnMovable
		{
		public:
			Logger(std::string name) : name_(name)
			{
				levelMask_ = LogConfig::instance().log_level_mask();
			}

			Logger(std::string name, int levelMask) : name_(name)
			{
				levelMask_ = levelMask;
			}

			// logger.info(format string, arg1, arg2, arg3, ...) call style
			template <typename... Args> detail::LineLogger trace(const char* fmt, const Args&... args);
			template <typename... Args> detail::LineLogger debug(const char* fmt, const Args&... args);
			template <typename... Args> detail::LineLogger info(const char* fmt, const Args&... args);
			template <typename... Args> detail::LineLogger warn(const char* fmt, const Args&... args);
			template <typename... Args> detail::LineLogger error(const char* fmt, const Args&... args);
			template <typename... Args> detail::LineLogger fatal(const char* fmt, const Args&... args);


			// logger.info(msg) << ".." call style
			template <typename T> detail::LineLogger trace(const T& msg);
			template <typename T> detail::LineLogger debug(const T& msg);
			template <typename T> detail::LineLogger info(const T& msg);
			template <typename T> detail::LineLogger warn(const T& msg);
			template <typename T> detail::LineLogger error(const T& msg);
			template <typename T> detail::LineLogger fatal(const T& msg);


			// logger.info() << ".." call  style
			detail::LineLogger trace();
			detail::LineLogger debug();
			detail::LineLogger info();
			detail::LineLogger warn();
			detail::LineLogger error();
			detail::LineLogger fatal();

			//const LogLevels level() const
			//{
			//	return static_cast<LogLevels>(level_.load(std::memory_order_relaxed));
			//}

			void set_level_mask(int levelMask)
			{
				levelMask_ = levelMask & LogLevels::sentinel;
			}

			bool should_log(LogLevels msgLevel) const
			{
				return level_should_log(levelMask_, msgLevel);
			}

			std::string name() const { return name_; };

			std::string to_string();

			SinkPtr get_sink(std::string name);

			void attach_sink(SinkPtr sink);

			void detach_sink(SinkPtr sink);

			void attach_sink_list(std::vector<std::string> &sinkList);

			void attach_console();

			void detach_console();



		private:

			friend detail::LineLogger;

			detail::LineLogger log_if_enabled(LogLevels lvl);

			template <typename... Args>
			detail::LineLogger log_if_enabled(LogLevels lvl, const char* fmt, const Args&... args);

			template<typename T>
			detail::LineLogger log_if_enabled(LogLevels lvl, const T& msg);

			void log_msg(detail::LogMessage msg);

			std::string				name_;
			std::atomic_int			levelMask_;
			cds::AtomicUnorderedMap<std::string, SinkPtr> sinks_;
		};
		typedef std::shared_ptr<Logger> LoggerPtr;

		namespace detail
		{
			template<typename T>
			class mpmc_bounded_queue
			{
			public:

				using item_type = T;
				mpmc_bounded_queue(size_t buffer_size)
					: buffer_(new cell_t[buffer_size]),
					buffer_mask_(buffer_size - 1)
				{
					//queue size must be power of two
					if (!((buffer_size >= 2) && ((buffer_size & (buffer_size - 1)) == 0)))
						throw ArgException("async logger queue size must be power of two");

					for (size_t i = 0; i != buffer_size; i += 1)
						buffer_[i].sequence_.store(i, std::memory_order_relaxed);
					enqueue_pos_.store(0, std::memory_order_relaxed);
					dequeue_pos_.store(0, std::memory_order_relaxed);
				}

				~mpmc_bounded_queue()
				{
					delete[] buffer_;
				}


				bool enqueue(T&& data)
				{
					cell_t* cell;
					size_t pos = enqueue_pos_.load(std::memory_order_relaxed);
					for (;;)
					{
						cell = &buffer_[pos & buffer_mask_];
						size_t seq = cell->sequence_.load(std::memory_order_acquire);
						intptr_t dif = (intptr_t)seq - (intptr_t)pos;
						if (dif == 0)
						{
							if (enqueue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
								break;
						}
						else if (dif < 0)
						{
							return false;
						}
						else
						{
							pos = enqueue_pos_.load(std::memory_order_relaxed);
						}
					}
					cell->data_ = std::move(data);
					cell->sequence_.store(pos + 1, std::memory_order_release);
					return true;
				}

				bool dequeue(T& data)
				{
					cell_t* cell;
					size_t pos = dequeue_pos_.load(std::memory_order_relaxed);
					for (;;)
					{
						cell = &buffer_[pos & buffer_mask_];
						size_t seq =
							cell->sequence_.load(std::memory_order_acquire);
						intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);
						if (dif == 0)
						{
							if (dequeue_pos_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
								break;
						}
						else if (dif < 0)
							return false;
						else
							pos = dequeue_pos_.load(std::memory_order_relaxed);
					}
					data = std::move(cell->data_);
					cell->sequence_.store(pos + buffer_mask_ + 1, std::memory_order_release);
					return true;
				}

			private:
				struct cell_t
				{
					std::atomic<size_t>   sequence_;
					T                     data_;
				};

				static size_t const     cacheline_size = 64;
				typedef char            cacheline_pad_t[cacheline_size];

				cacheline_pad_t         pad0_;
				cell_t* const           buffer_;
				size_t const            buffer_mask_;
				cacheline_pad_t         pad1_;
				std::atomic<size_t>     enqueue_pos_;
				cacheline_pad_t         pad2_;
				std::atomic<size_t>     dequeue_pos_;
				cacheline_pad_t         pad3_;

				mpmc_bounded_queue(mpmc_bounded_queue const&);
				void operator = (mpmc_bounded_queue const&);
			};

			struct LogMessage
			{
				std::string			loggerName_;
				LogLevels			level_;
				time::Date			dateTime_;
				size_t				threadId_;
				std::string			buffer_;
			};

			class LineLogger : private UnCopyable
			{
			public:
				LineLogger(Logger* callbacker, LogLevels lvl, bool enable) :callbackLogger_(callbacker), enabled_(enable)
				{
					msg_.level_ = lvl;
				}

				LineLogger(LineLogger&& other) :
					callbackLogger_(other.callbackLogger_),
					msg_(std::move(other.msg_)),
					enabled_(other.enabled_)
				{
					other.disable();
				}

				~LineLogger()
				{
					if (enabled_)
					{
						msg_.loggerName_ = callbackLogger_->name();
						msg_.dateTime_ = time::Date::local_time();
						msg_.threadId_ = os::thread_id();
						callbackLogger_->log_msg(msg_);
					}
				}

				LineLogger& operator=(LineLogger&&) = delete;

				void write(const char* what)
				{
					if (enabled_)
						msg_.buffer_ += what;
				}

				template <typename... Args>
				void write(const char* fmt, const Args&... args)
				{
					if (!enabled_)
						return;
					std::string buf(fmt);
					fmt::format_string(buf, args...);
					msg_.buffer_ += buf;
				}


				LineLogger& operator<<(const char* what)
				{
					if (enabled_) msg_.buffer_ += what;
					return *this;
				}

				LineLogger& operator<<(const std::string& what)
				{
					if (enabled_) msg_.buffer_ += what;
					return *this;
				}

				template<typename T>
				LineLogger& operator<<(const T& what)
				{
					if (enabled_)
					{
						msg_.buffer_ += dynamic_cast<std::ostringstream &>(std::ostringstream() << std::dec << what).str();
					}
					return *this;
				}

				void disable()
				{
					enabled_ = false;
				}

				bool is_enabled() const
				{
					return enabled_;
				}
			private:
				Logger			*callbackLogger_;
				LogMessage		msg_;
				bool			enabled_;
			};

			class SinkInterface
			{
			public:
				virtual ~SinkInterface() {};
				virtual void log(const LogMessage& msg) = 0;
				virtual void flush() = 0;
				virtual std::string name() const = 0;
				virtual std::string to_string() const = 0;
				virtual void set_level_mask(int levelMask) = 0;
				virtual void set_format(const std::string &fmt) = 0;
			};

			// Due to a bug in VC12, thread join in static object dtor
			// will cause deadlock. TODO: implement async sink when
			// VS2015 ready.
			class AsyncSink : public SinkInterface, private UnCopyable
			{

			};

			class Sink : public SinkInterface, private UnCopyable
			{
			public:
				Sink()
				{
					levelMask_ = 0x3F & LogConfig::instance().log_level_mask();
					set_format(LogConfig::instance().format());
				};

				virtual ~Sink()  {};

				void log(const LogMessage& msg) override
				{
					if (!level_should_log(levelMask_, msg.level_))
					{
						return;
					}

					std::string finalMessage = format_message(msg);
					sink_it(finalMessage);
				}

				void set_level_mask(int levelMask)
				{
					levelMask_ = levelMask & LogLevels::sentinel;
				}

				int level_mask() const
				{
					return levelMask_;
				}

				void set_format(const std::string &format) override
				{
					format_.set(format);
				}

				virtual void sink_it(const std::string &finalMsg) = 0;

			private:
				std::string format_message(const LogMessage &msg)
				{
					std::string ret(*(format_.get()));
					auto dt = msg.dateTime_;
					fmt::replace_all_with_escape(ret, consts::kSinkDatetimeSpecifier, dt.to_string(LogConfig::instance().datetime_format().c_str()));
					fmt::replace_all_with_escape(ret, consts::kSinkLoggerNameSpecifier, msg.loggerName_);
					fmt::replace_all_with_escape(ret, consts::kSinkThreadSpecifier, std::to_string(msg.threadId_));
					fmt::replace_all_with_escape(ret, consts::kSinkLevelSpecifier, consts::kLevelNames[msg.level_]);
					fmt::replace_all_with_escape(ret, consts::kSinkLevelShortSpecifier, consts::kShortLevelNames[msg.level_]);
					fmt::replace_all_with_escape(ret, consts::kSinkMessageSpecifier, msg.buffer_);
					// make sure new line
					if (!fmt::ends_with(ret, os::endl()))
					{
						ret += os::endl();
					}
					return ret;
				}

				std::atomic_int							levelMask_;
				cds::AtomicNonTrivial<std::string>		format_;
			};

			class SimpleFileSink : public Sink
			{
			public:
				SimpleFileSink(const std::string filename, bool truncate) :fileEditor_(filename, truncate)
				{
				}

				void flush() override
				{
					fileEditor_.flush();
				}

				void sink_it(const std::string &finalMsg) override
				{
					fileEditor_ << finalMsg;
				}

				std::string name() const override
				{
					return fileEditor_.filename();
				}

				std::string to_string() const override
				{
					return "SimpleFileSink->" + name() + " " + level_mask_to_string(level_mask());
				}

			private:
				fs::FileEditor fileEditor_;
			};

			class RotateFileSink : public Sink
			{
			public:
				RotateFileSink(const std::string filename, std::size_t maxSizeInByte, bool backup)
					:maxSizeInByte_(maxSizeInByte), backup_(backup)
				{
					if (backup_)
					{
						back_up(filename);
					}
					fileEditor_.open(filename, true);
					currentSize_ = 0;
				}

				void flush() override
				{
					fileEditor_.flush();
				}

				void sink_it(const std::string &finalMsg) override
				{
					currentSize_ += finalMsg.length();
					if (currentSize_ > maxSizeInByte_)
					{
						rotate();
					}
					fileEditor_ << finalMsg;
				}

				std::string name() const override
				{
					return fileEditor_.filename();
				}

				std::string to_string() const override
				{
					return "RotateFileSink->" + name() + " " + level_mask_to_string(level_mask());
				}

			private:
				void back_up(std::string oldFile)
				{
					std::string backupName = os::path_append_basename(oldFile,
						time::Date::local_time().to_string("_%y-%m-%d_%H-%M-%S-%frac"));
					os::rename(oldFile, backupName);
				}

				void rotate()
				{
					std::lock_guard<std::mutex> lock(mutex_);
					// check again in case other thread 
					// just waited for this operation
					if (currentSize_ > maxSizeInByte_)
					{
						if (backup_)
						{
							fileEditor_.close();
							back_up(fileEditor_.filename());
							fileEditor_.open(true);
						}
						else
						{
							fileEditor_.reopen(true);
						}
						currentSize_ = 0;
					}
				}

				fs::FileEditor				fileEditor_;
				std::mutex					mutex_;
				std::size_t					maxSizeInByte_;
				std::atomic<std::size_t>	currentSize_;
				bool						backup_;
			};

			class OStreamSink : public Sink
			{
			public:
				explicit OStreamSink(std::ostream& os, const char *name, bool forceFlush = false)
					:ostream_(os), name_(name), forceFlush_(forceFlush)
				{
				}

				std::string name() const override
				{
					return name_;
				}

				std::string to_string() const override
				{
					return "OStreamSink->" + name() + " " + level_mask_to_string(level_mask());
				}

			private:
				void sink_it(const std::string &finalMsg) override
				{
					ostream_ << finalMsg;
					if (forceFlush_) ostream_.flush();
				}

				void flush() override
				{
					ostream_.flush();
				}

				std::ostream&	ostream_;
				std::string		name_;
				bool			forceFlush_;
			};

			class StdoutSink : public OStreamSink
			{
			public:
				StdoutSink() : OStreamSink(std::cout, consts::kStdoutSinkName, true)
				{
					// stdout by design should not receive warning and error message
					// so add cout and cerr together and filter by level is a better idea
					set_level_mask(0x07 & LogConfig::instance().log_level_mask());
				}
				static std::shared_ptr<StdoutSink> instance()
				{
					static std::shared_ptr<StdoutSink> instance = std::make_shared<StdoutSink>();
					return instance;
				}

				std::string to_string() const override
				{
					return "StdoutSink->" + name() + " " + level_mask_to_string(level_mask());
				}
			};

			class StderrSink : public OStreamSink
			{
			public:
				StderrSink() : OStreamSink(std::cerr, consts::kStderrSinkName, true)
				{
					// stderr by design should only log error/warning msg
					set_level_mask(0x38 & LogConfig::instance().log_level_mask());
				}
				static std::shared_ptr<StderrSink> instance()
				{
					static std::shared_ptr<StderrSink> instance = std::make_shared<StderrSink>();
					return instance;
				}

				std::string to_string() const override
				{
					return "StderrSink->" + name() + " " + level_mask_to_string(level_mask());
				}
			};

			class LoggerRegistry : UnMovable
			{
			public:
				static LoggerRegistry& instance()
				{
					static LoggerRegistry sInstance;
					return sInstance;
				}

				LoggerPtr create(const std::string &name)
				{
					auto ptr = new_registry(name);
					if (!ptr)
					{
						throw RuntimeException("Logger with name: " + name + " already existed.");
					}
					return ptr;
				}

				LoggerPtr ensure_get(std::string &name)
				{
					auto map = loggers_.get();
					LoggerPtr	ptr;
					while (map->find(name) == map->end())
					{
						ptr = new_registry(name);
						map = loggers_.get();
					}
					return map->find(name)->second;
				}

				LoggerPtr get(std::string &name)
				{
					auto ptr = loggers_.get();
					auto pos = ptr->find(name);
					if (pos != ptr->end())
					{
						return pos->second;
					}
					return nullptr;
				}

				std::vector<LoggerPtr> get_all()
				{
					std::vector<LoggerPtr> list;
					auto loggers = loggers_.get();
					for (auto logger : *loggers)
					{
						list.push_back(logger.second);
					}
					return list;
				}

				void drop(const std::string &name)
				{
					loggers_.erase(name);
				}

				void drop_all()
				{
					loggers_.clear();
				}

				void lock()
				{
					lock_ = true;
				}

				void unlock()
				{
					lock_ = false;
				}

				bool is_locked() const
				{
					return lock_;
				}

			private:
				LoggerRegistry(){ lock_ = false; }

				LoggerPtr new_registry(const std::string &name);

				cds::AtomicUnorderedMap<std::string, LoggerPtr> loggers_;
				std::atomic_bool	lock_;
			};


		} // namespace detail

		inline detail::LineLogger Logger::log_if_enabled(LogLevels lvl)
		{
			detail::LineLogger l(this, lvl, should_log(lvl));
			return l;
		}

		template <typename... Args>
		inline detail::LineLogger Logger::log_if_enabled(LogLevels lvl, const char* fmt, const Args&... args)
		{
			detail::LineLogger l(this, lvl, should_log(lvl));
			l.write(fmt, args...);
			return l;
		}

		template<typename T>
		inline detail::LineLogger Logger::log_if_enabled(LogLevels lvl, const T& msg)
		{
			detail::LineLogger l(this, lvl, should_log(lvl));
			l.write(msg);
			return l;
		}

		// logger.info(format string, arg1, arg2, arg3, ...) call style
		template <typename... Args>
		inline detail::LineLogger Logger::trace(const char* fmt, const Args&... args)
		{
			return log_if_enabled(LogLevels::trace, fmt, args...);
		}
		template <typename... Args>
		inline detail::LineLogger Logger::debug(const char* fmt, const Args&... args)
		{
			return log_if_enabled(LogLevels::debug, fmt, args...);
		}
		template <typename... Args>
		inline detail::LineLogger Logger::info(const char* fmt, const Args&... args)
		{
			return log_if_enabled(LogLevels::info, fmt, args...);
		}
		template <typename... Args>
		inline detail::LineLogger Logger::warn(const char* fmt, const Args&... args)
		{
			return log_if_enabled(LogLevels::warn, fmt, args...);
		}
		template <typename... Args>
		inline detail::LineLogger Logger::error(const char* fmt, const Args&... args)
		{
			return log_if_enabled(LogLevels::error, fmt, args...);
		}
		template <typename... Args>
		inline detail::LineLogger Logger::fatal(const char* fmt, const Args&... args)
		{
			return log_if_enabled(LogLevels::fatal, fmt, args...);
		}


		// logger.info(msg) << ".." call style
		template <typename T>
		inline detail::LineLogger Logger::trace(const T& msg)
		{
			return log_if_enabled(LogLevels::trace, msg);
		}
		template <typename T>
		inline detail::LineLogger Logger::debug(const T& msg)
		{
			return log_if_enabled(LogLevels::debug, msg);
		}
		template <typename T>
		inline detail::LineLogger Logger::info(const T& msg)
		{
			return log_if_enabled(LogLevels::info, msg);
		}
		template <typename T>
		inline detail::LineLogger Logger::warn(const T& msg)
		{
			return log_if_enabled(LogLevels::warn, msg);
		}
		template <typename T>
		inline detail::LineLogger Logger::error(const T& msg)
		{
			return log_if_enabled(LogLevels::error, msg);
		}
		template <typename T>
		inline detail::LineLogger Logger::fatal(const T& msg)
		{
			return log_if_enabled(LogLevels::fatal, msg);
		}


		// logger.info() << ".." call  style
		inline detail::LineLogger Logger::trace()
		{
			return log_if_enabled(LogLevels::trace);
		}
		inline detail::LineLogger Logger::debug()
		{
			return log_if_enabled(LogLevels::debug);
		}
		inline detail::LineLogger Logger::info()
		{
			return log_if_enabled(LogLevels::info);
		}
		inline detail::LineLogger Logger::warn()
		{
			return log_if_enabled(LogLevels::warn);
		}
		inline detail::LineLogger Logger::error()
		{
			return log_if_enabled(LogLevels::error);
		}
		inline detail::LineLogger Logger::fatal()
		{
			return log_if_enabled(LogLevels::fatal);
		}

		inline SinkPtr Logger::get_sink(std::string name)
		{
			auto sinkmap = sinks_.get();
			auto f = sinkmap->find(name);
			return (f == sinkmap->end() ? nullptr : f->second);
		}

		inline void Logger::attach_sink(SinkPtr sink)
		{
			if (!sinks_.insert(sink->name(), sink))
			{
				throw RuntimeException("Sink with name: " + sink->name() + " already attached to logger: " + name_);
			}
		}

		inline void Logger::detach_sink(SinkPtr sink)
		{
			sinks_.erase(sink->name());
		}

		inline void Logger::log_msg(detail::LogMessage msg)
		{
			auto sinkmap = sinks_.get();
			for (auto sink : *sinkmap)
			{
				sink.second->log(msg);
			}
		}

		inline std::string Logger::to_string()
		{
			std::string str(name() + ": " + level_mask_to_string(levelMask_));
			str += "\n{\n";
			auto sinkmap = sinks_.get();
			for (auto sink : *sinkmap)
			{
				str += sink.second->to_string() + "\n";
			}
			str += "}";
			return str;
		}

		inline LoggerPtr get_logger(std::string name, bool createIfNotExists = true)
		{
			if (createIfNotExists)
			{
				return detail::LoggerRegistry::instance().ensure_get(name);
			}
			else
			{
				return detail::LoggerRegistry::instance().get(name);
			}
		}

		inline SinkPtr new_stdout_sink()
		{
			return detail::StdoutSink::instance();
		}

		inline SinkPtr new_stderr_sink()
		{
			return detail::StderrSink::instance();
		}

		inline void Logger::attach_console()
		{
			sinks_.insert(std::string(consts::kStdoutSinkName), new_stdout_sink());
			sinks_.insert(std::string(consts::kStderrSinkName), new_stderr_sink());
		}

		inline void Logger::detach_console()
		{
			sinks_.erase(std::string(consts::kStdoutSinkName));
			sinks_.erase(std::string(consts::kStderrSinkName));
		}

		inline SinkPtr get_sink(std::string name)
		{
			SinkPtr psink = nullptr;
			auto loggers = detail::LoggerRegistry::instance().get_all();
			for (auto logger : loggers)
			{
				psink = logger->get_sink(name);
				if (psink)
				{
					return psink;
				}
			}
			return nullptr;
		}

		inline void dump_loggers(std::ostream &out = std::cout)
		{
			auto loggers = detail::LoggerRegistry::instance().get_all();
			out << "{\n";
			for (auto logger : loggers)
			{
				out << logger->to_string() << "\n";
			}
			out << "}" << std::endl;
		}

		inline SinkPtr new_ostream_sink(std::ostream &stream, std::string name, bool forceFlush = false)
		{
			auto sinkptr = get_sink(name);
			if (sinkptr)
			{
				throw RuntimeException(name + " already holded by another sink\n" + sinkptr->to_string());
			}
			return std::make_shared<detail::OStreamSink>(stream, name.c_str(), forceFlush);
		}

		inline SinkPtr new_simple_file_sink(std::string filename, bool truncate = false)
		{
			auto sinkptr = get_sink(os::absolute_path(filename));
			if (sinkptr)
			{
				throw RuntimeException("File: " + filename + " already holded by another sink!\n" + sinkptr->to_string());
			}
			return std::make_shared<detail::SimpleFileSink>(filename, truncate);
		}

		inline SinkPtr new_rotate_file_sink(std::string filename, std::size_t maxSizeInByte = 4194304, bool backupOld = false)
		{
			auto sinkptr = get_sink(os::absolute_path(filename));
			if (sinkptr)
			{
				throw RuntimeException("File: " + filename + " already holded by another sink!\n" + sinkptr->to_string());
			}
			return std::make_shared<detail::RotateFileSink>(filename, maxSizeInByte, backupOld);
		}

		inline void Logger::attach_sink_list(std::vector<std::string> &sinkList)
		{
			for (auto sinkname : sinkList)
			{
				SinkPtr psink = nullptr;
				if (sinkname == consts::kStdoutSinkName)
				{
					psink = new_stdout_sink();
				}
				else if (sinkname == consts::kStderrSinkName)
				{
					psink = new_stderr_sink();
				}
				else
				{
					psink = get_sink(sinkname);
				}
				if (psink && (!this->get_sink(psink->name()))) attach_sink(psink);
			}
		}

		inline void lock_loggers()
		{
			detail::LoggerRegistry::instance().lock();
		}

		inline void unlock_loggers()
		{
			detail::LoggerRegistry::instance().unlock();
		}

		inline void drop_logger(std::string name)
		{
			detail::LoggerRegistry::instance().drop(name);
		}

		inline void drop_all_loggers()
		{
			detail::LoggerRegistry::instance().drop_all();
		}

		inline void drop_sink(std::string name)
		{
			SinkPtr psink = get_sink(name);
			if (nullptr == psink) return;
			auto loggers = detail::LoggerRegistry::instance().get_all();
			for (auto logger : loggers)
			{
				logger->detach_sink(psink);
			}
		}

		inline LoggerPtr detail::LoggerRegistry::new_registry(const std::string &name)
		{
			LoggerPtr newLogger = std::make_shared<Logger>(name);
			auto defaultSinkList = LogConfig::instance().sink_list();
			for (auto sinkname : defaultSinkList)
			{
				SinkPtr psink = nullptr;
				if (sinkname == consts::kStdoutSinkName)
				{
					psink = new_stdout_sink();
				}
				else if (sinkname == consts::kStderrSinkName)
				{
					psink = new_stderr_sink();
				}
				else
				{
					psink = get_sink(sinkname);
				}
				if (psink) newLogger->attach_sink(psink);
			}
			if (loggers_.insert(name, newLogger))
			{
				return newLogger;
			}
			return nullptr;
		}

		namespace detail
		{
			inline void sink_list_revise(std::vector<std::string> &list, std::map<std::string, std::string> &map)
			{
				for (auto m : map)
				{
					for (auto l = list.begin(); l != list.end(); ++l)
					{
						if (m.first == *l)
						{
							l = list.erase(l);
							l = list.insert(l, m.second);
						}
					}
				}
			}

			inline void config_loggers_from_section(cfg::CfgLevel::section_map_t &section, std::map<std::string, std::string> &map)
			{
				for (auto loggerSec : section)
				{
					for (auto value : loggerSec.second.values)
					{
						LoggerPtr logger = nullptr;
						if (consts::kConfigLevelsSpecifier == value.first)
						{
							int mask = level_mask_from_string(value.second.str());
							if (!logger) logger = get_logger(loggerSec.first, true);
							logger->set_level_mask(mask);
						}
						else if (consts::kConfigSinkListSpecifier == value.first)
						{
							auto list = fmt::split_whitespace(value.second.str());
							if (list.empty()) continue;
							sink_list_revise(list, map);
							if (!logger) logger = get_logger(loggerSec.first, true);
							logger->attach_sink_list(list);
						}
						else
						{
							zupply_internal_warn("Unrecognized configuration key: " + value.first);
						}
					}
				}
			}

			inline LoggerPtr get_hidden_logger()
			{
				auto hlogger = get_logger("hidden", false);
				if (!hlogger)
				{
					hlogger = get_logger("hidden", true);
					hlogger->set_level_mask(0);
				}
				return hlogger;
			}

			inline std::map<std::string, std::string> config_sinks_from_section(cfg::CfgLevel::section_map_t &section)
			{
				std::map<std::string, std::string> sinkMap;
				for (auto sinkSec : section)
				{
					std::string type;
					std::string filename;
					std::string fmt;
					std::string levelStr;
					SinkPtr sink = nullptr;

					for (auto value : sinkSec.second.values)
					{
						// entries
						if (consts::kConfigSinkTypeSpecifier == value.first)
						{
							type = value.second.str();
						}
						else if (consts::kConfigSinkFilenameSpecifier == value.first)
						{
							filename = value.second.str();
						}
						else if (consts::kConfigFormatSpecifier == value.first)
						{
							fmt = value.second.str();
						}
						else if (consts::kConfigLevelsSpecifier == value.first)
						{
							levelStr = value.second.str();
						}
						else
						{
							zupply_internal_warn("Unrecognized config key entry: " + value.first);
						}
					}

					// sink
					if (type.empty()) throw RuntimeException("No suitable type specified for sink: " + sinkSec.first);
					if (type == consts::kStdoutSinkName)
					{
						sink = new_stdout_sink();
					}
					else if (type == consts::kStderrSinkName)
					{
						sink = new_stderr_sink();
					}
					else
					{
						if (filename.empty()) throw RuntimeException("No name specified for sink: " + sinkSec.first);
						if (type == consts::kOstreamSinkType)
						{
							zupply_internal_warn("Currently do not support init ostream logger from config file.");
						}
						else if (type == consts::kSimplefileSinkType)
						{
							sink = new_simple_file_sink(filename);
							get_hidden_logger()->attach_sink(sink);
						}
						else if (type == consts::kRotatefileSinkType)
						{
							sink = new_rotate_file_sink(filename);
							get_hidden_logger()->attach_sink(sink);
						}
						else
						{
							zupply_internal_warn("Unrecognized sink type: " + type);
						}
					}
					if (sink)
					{
						if (!fmt.empty()) sink->set_format(fmt);
						if (!levelStr.empty())
						{
							int mask = level_mask_from_string(levelStr);
							sink->set_level_mask(mask);
						}
						if (!get_hidden_logger()->get_sink(sink->name()))
						{
							get_hidden_logger()->attach_sink(sink);
						}
						sinkMap[sinkSec.first] = sink->name();
					}
				}
				return sinkMap;
			}
		} // namespace detail

		inline void config_from_file(std::string cfgFilename)
		{
			cfg::CfgParser parser(cfgFilename);

			// config for specific sinks
			auto sinkSection = parser(consts::KConfigSinkSectionSpecifier).sections;
			auto sinkMap = detail::config_sinks_from_section(sinkSection);

			// global format
			std::string format = parser(consts::KConfigGlobalSectionSpecifier)[consts::kConfigFormatSpecifier].str();
			if (!format.empty()) LogConfig::set_default_format(format);
			// global datetime format
			std::string datefmt = parser(consts::KConfigGlobalSectionSpecifier)[consts::kConfigDateTimeFormatSpecifier].str();
			if (!datefmt.empty()) LogConfig::set_default_datetime_format(datefmt);
			// global log levels
			auto v = parser(consts::KConfigGlobalSectionSpecifier)[consts::kConfigLevelsSpecifier];
			if (!v.str().empty()) LogConfig::set_default_level_mask(level_mask_from_string(v.str()));
			// global sink list
			v = parser(consts::KConfigGlobalSectionSpecifier)[consts::kConfigSinkListSpecifier];
			if (!v.str().empty())
			{
				auto list = fmt::split_whitespace(v.str());
				detail::sink_list_revise(list, sinkMap);
				LogConfig::set_default_sink_list(list);
			}

			// config for specific loggers
			auto loggerSection = parser(consts::KConfigLoggerSectionSpecifier).sections;
			detail::config_loggers_from_section(loggerSection, sinkMap);
		}

		inline void zupply_internal_warn(std::string msg)
		{
			auto zlog = get_logger(consts::kZupplyInternalLoggerName, true);
			zlog->warn() << msg;
		}

		inline void zupply_internal_error(std::string msg)
		{
			auto zlog = get_logger(consts::kZupplyInternalLoggerName, true);
			zlog->error() << msg;
		}
	} // namespace log
} // namespace zz

#endif //END _ZUPPLY_ZUPPLY_HPP_


