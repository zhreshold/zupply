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
 *   DateTime since: June-2015
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
#include <conio.h>
#elif ZUPPLY_OS_UNIX
#include <unistd.h>	/* POSIX flags */
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <ftw.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
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
#include <cstring>

// UTF8CPP
#include <stdexcept>
#include <iterator>

namespace zz
{
	// \cond
	// This namespace scopes all thirdparty libraries
	namespace thirdparty
	{
		namespace utf8
		{
			// The typedefs for 8-bit, 16-bit and 32-bit unsigned integers
			// You may need to change them to match your system. 
			// These typedefs have the same names as ones from cstdint, or boost/cstdint
			typedef unsigned char   uint8_t;
			typedef unsigned short  uint16_t;
			typedef unsigned int    uint32_t;

			// Helper code - not intended to be directly called by the library users. May be changed at any time
			namespace internal
			{
				// Unicode constants
				// Leading (high) surrogates: 0xd800 - 0xdbff
				// Trailing (low) surrogates: 0xdc00 - 0xdfff
				const uint16_t LEAD_SURROGATE_MIN = 0xd800u;
				const uint16_t LEAD_SURROGATE_MAX = 0xdbffu;
				const uint16_t TRAIL_SURROGATE_MIN = 0xdc00u;
				const uint16_t TRAIL_SURROGATE_MAX = 0xdfffu;
				const uint16_t LEAD_OFFSET = LEAD_SURROGATE_MIN - (0x10000 >> 10);
				const uint32_t SURROGATE_OFFSET = 0x10000u - (LEAD_SURROGATE_MIN << 10) - TRAIL_SURROGATE_MIN;

				// Maximum valid value for a Unicode code point
				const uint32_t CODE_POINT_MAX = 0x0010ffffu;

				template<typename octet_type>
				inline uint8_t mask8(octet_type oc)
				{
					return static_cast<uint8_t>(0xff & oc);
				}
				template<typename u16_type>
				inline uint16_t mask16(u16_type oc)
				{
					return static_cast<uint16_t>(0xffff & oc);
				}
				template<typename octet_type>
				inline bool is_trail(octet_type oc)
				{
					return ((mask8(oc) >> 6) == 0x2);
				}

				template <typename u16>
				inline bool is_surrogate(u16 cp)
				{
					return (cp >= LEAD_SURROGATE_MIN && cp <= TRAIL_SURROGATE_MAX);
				}

				template <typename u32>
				inline bool is_code_point_valid(u32 cp)
				{
					return (cp <= CODE_POINT_MAX && !is_surrogate(cp) && cp != 0xfffe && cp != 0xffff);
				}

				template <typename octet_iterator>
				inline typename std::iterator_traits<octet_iterator>::difference_type
					sequence_length(octet_iterator lead_it)
				{
						uint8_t lead = mask8(*lead_it);
						if (lead < 0x80)
							return 1;
						else if ((lead >> 5) == 0x6)
							return 2;
						else if ((lead >> 4) == 0xe)
							return 3;
						else if ((lead >> 3) == 0x1e)
							return 4;
						else
							return 0;
					}

				enum utf_error { OK, NOT_ENOUGH_ROOM, INVALID_LEAD, INCOMPLETE_SEQUENCE, OVERLONG_SEQUENCE, INVALID_CODE_POINT };

				template <typename octet_iterator>
				utf_error validate_next(octet_iterator& it, octet_iterator end, uint32_t* code_point)
				{
					uint32_t cp = mask8(*it);
					// Check the lead octet
					typedef typename std::iterator_traits<octet_iterator>::difference_type octet_difference_type;
					octet_difference_type length = sequence_length(it);

					// "Shortcut" for ASCII characters
					if (length == 1) {
						if (end - it > 0) {
							if (code_point)
								*code_point = cp;
							++it;
							return OK;
						}
						else
							return NOT_ENOUGH_ROOM;
					}

					// Do we have enough memory?     
					if (std::distance(it, end) < length)
						return NOT_ENOUGH_ROOM;

					// Check trail octets and calculate the code point
					switch (length) {
					case 0:
						return INVALID_LEAD;
						break;
					case 2:
						if (is_trail(*(++it))) {
							cp = ((cp << 6) & 0x7ff) + ((*it) & 0x3f);
						}
						else {
							--it;
							return INCOMPLETE_SEQUENCE;
						}
						break;
					case 3:
						if (is_trail(*(++it))) {
							cp = ((cp << 12) & 0xffff) + ((mask8(*it) << 6) & 0xfff);
							if (is_trail(*(++it))) {
								cp += (*it) & 0x3f;
							}
							else {
								std::advance(it, -2);
								return INCOMPLETE_SEQUENCE;
							}
						}
						else {
							--it;
							return INCOMPLETE_SEQUENCE;
						}
						break;
					case 4:
						if (is_trail(*(++it))) {
							cp = ((cp << 18) & 0x1fffff) + ((mask8(*it) << 12) & 0x3ffff);
							if (is_trail(*(++it))) {
								cp += (mask8(*it) << 6) & 0xfff;
								if (is_trail(*(++it))) {
									cp += (*it) & 0x3f;
								}
								else {
									std::advance(it, -3);
									return INCOMPLETE_SEQUENCE;
								}
							}
							else {
								std::advance(it, -2);
								return INCOMPLETE_SEQUENCE;
							}
						}
						else {
							--it;
							return INCOMPLETE_SEQUENCE;
						}
						break;
					}
					// Is the code point valid?
					if (!is_code_point_valid(cp)) {
						for (octet_difference_type i = 0; i < length - 1; ++i)
							--it;
						return INVALID_CODE_POINT;
					}

					if (code_point)
						*code_point = cp;

					if (cp < 0x80) {
						if (length != 1) {
							std::advance(it, -(length - 1));
							return OVERLONG_SEQUENCE;
						}
					}
					else if (cp < 0x800) {
						if (length != 2) {
							std::advance(it, -(length - 1));
							return OVERLONG_SEQUENCE;
						}
					}
					else if (cp < 0x10000) {
						if (length != 3) {
							std::advance(it, -(length - 1));
							return OVERLONG_SEQUENCE;
						}
					}

					++it;
					return OK;
				}

				template <typename octet_iterator>
				inline utf_error validate_next(octet_iterator& it, octet_iterator end) {
					return validate_next(it, end, 0);
				}

			} // namespace internal 

			/// The library API - functions intended to be called by the users

			// Byte order mark
			const uint8_t bom[] = { 0xef, 0xbb, 0xbf };

			template <typename octet_iterator>
			octet_iterator find_invalid(octet_iterator start, octet_iterator end)
			{
				octet_iterator result = start;
				while (result != end) {
					internal::utf_error err_code = internal::validate_next(result, end);
					if (err_code != internal::OK)
						return result;
				}
				return result;
			}

			template <typename octet_iterator>
			inline bool is_valid(octet_iterator start, octet_iterator end)
			{
				return (find_invalid(start, end) == end);
			}

			template <typename octet_iterator>
			inline bool is_bom(octet_iterator it)
			{
				return (
					(internal::mask8(*it++)) == bom[0] &&
					(internal::mask8(*it++)) == bom[1] &&
					(internal::mask8(*it)) == bom[2]
					);
			}

			// Exceptions that may be thrown from the library functions.
			class invalid_code_point : public std::exception {
				uint32_t cp;
			public:
				invalid_code_point(uint32_t _cp) : cp(_cp) {}
				virtual const char* what() const throw() { return "Invalid code point"; }
				uint32_t code_point() const { return cp; }
			};

			class invalid_utf8 : public std::exception {
				uint8_t u8;
			public:
				invalid_utf8(uint8_t u) : u8(u) {}
				virtual const char* what() const throw() { return "Invalid UTF-8"; }
				uint8_t utf8_octet() const { return u8; }
			};

			class invalid_utf16 : public std::exception {
				uint16_t u16;
			public:
				invalid_utf16(uint16_t u) : u16(u) {}
				virtual const char* what() const throw() { return "Invalid UTF-16"; }
				uint16_t utf16_word() const { return u16; }
			};

			class not_enough_room : public std::exception {
			public:
				virtual const char* what() const throw() { return "Not enough space"; }
			};

			/// The library API - functions intended to be called by the users

			template <typename octet_iterator, typename output_iterator>
			output_iterator replace_invalid(octet_iterator start, octet_iterator end, output_iterator out, uint32_t replacement)
			{
				while (start != end) {
					octet_iterator sequence_start = start;
					internal::utf_error err_code = internal::validate_next(start, end);
					switch (err_code) {
					case internal::OK:
						for (octet_iterator it = sequence_start; it != start; ++it)
							*out++ = *it;
						break;
					case internal::NOT_ENOUGH_ROOM:
						throw not_enough_room();
					case internal::INVALID_LEAD:
						append(replacement, out);
						++start;
						break;
					case internal::INCOMPLETE_SEQUENCE:
					case internal::OVERLONG_SEQUENCE:
					case internal::INVALID_CODE_POINT:
						append(replacement, out);
						++start;
						// just one replacement mark for the sequence
						while (internal::is_trail(*start) && start != end)
							++start;
						break;
					}
				}
				return out;
			}

			template <typename octet_iterator, typename output_iterator>
			inline output_iterator replace_invalid(octet_iterator start, octet_iterator end, output_iterator out)
			{
				static const uint32_t replacement_marker = internal::mask16(0xfffd);
				return replace_invalid(start, end, out, replacement_marker);
			}

			template <typename octet_iterator>
			octet_iterator append(uint32_t cp, octet_iterator result)
			{
				if (!internal::is_code_point_valid(cp))
					throw invalid_code_point(cp);

				if (cp < 0x80)                        // one octet
					*(result++) = static_cast<uint8_t>(cp);
				else if (cp < 0x800) {                // two octets
					*(result++) = static_cast<uint8_t>((cp >> 6) | 0xc0);
					*(result++) = static_cast<uint8_t>((cp & 0x3f) | 0x80);
				}
				else if (cp < 0x10000) {              // three octets
					*(result++) = static_cast<uint8_t>((cp >> 12) | 0xe0);
					*(result++) = static_cast<uint8_t>(((cp >> 6) & 0x3f) | 0x80);
					*(result++) = static_cast<uint8_t>((cp & 0x3f) | 0x80);
				}
				else if (cp <= internal::CODE_POINT_MAX) {      // four octets
					*(result++) = static_cast<uint8_t>((cp >> 18) | 0xf0);
					*(result++) = static_cast<uint8_t>(((cp >> 12) & 0x3f) | 0x80);
					*(result++) = static_cast<uint8_t>(((cp >> 6) & 0x3f) | 0x80);
					*(result++) = static_cast<uint8_t>((cp & 0x3f) | 0x80);
				}
				else
					throw invalid_code_point(cp);

				return result;
			}

			template <typename octet_iterator>
			uint32_t next(octet_iterator& it, octet_iterator end)
			{
				uint32_t cp = 0;
				internal::utf_error err_code = internal::validate_next(it, end, &cp);
				switch (err_code) {
				case internal::OK:
					break;
				case internal::NOT_ENOUGH_ROOM:
					throw not_enough_room();
				case internal::INVALID_LEAD:
				case internal::INCOMPLETE_SEQUENCE:
				case internal::OVERLONG_SEQUENCE:
					throw invalid_utf8(*it);
				case internal::INVALID_CODE_POINT:
					throw invalid_code_point(cp);
				}
				return cp;
			}

			template <typename octet_iterator>
			uint32_t peek_next(octet_iterator it, octet_iterator end)
			{
				return next(it, end);
			}

			template <typename octet_iterator>
			uint32_t prior(octet_iterator& it, octet_iterator start)
			{
				octet_iterator end = it;
				while (internal::is_trail(*(--it)))
				if (it < start)
					throw invalid_utf8(*it); // error - no lead byte in the sequence
				octet_iterator temp = it;
				return next(temp, end);
			}

			/// Deprecated in versions that include "prior"
			template <typename octet_iterator>
			uint32_t previous(octet_iterator& it, octet_iterator pass_start)
			{
				octet_iterator end = it;
				while (internal::is_trail(*(--it)))
				if (it == pass_start)
					throw invalid_utf8(*it); // error - no lead byte in the sequence
				octet_iterator temp = it;
				return next(temp, end);
			}

			template <typename octet_iterator, typename distance_type>
			void advance(octet_iterator& it, distance_type n, octet_iterator end)
			{
				for (distance_type i = 0; i < n; ++i)
					next(it, end);
			}

			template <typename octet_iterator>
			typename std::iterator_traits<octet_iterator>::difference_type
				distance(octet_iterator first, octet_iterator last)
			{
					typename std::iterator_traits<octet_iterator>::difference_type dist;
					for (dist = 0; first < last; ++dist)
						next(first, last);
					return dist;
				}

			template <typename u16bit_iterator, typename octet_iterator>
			octet_iterator utf16to8(u16bit_iterator start, u16bit_iterator end, octet_iterator result)
			{
				while (start != end) {
					uint32_t cp = internal::mask16(*start++);
					// Take care of surrogate pairs first
					if (internal::is_surrogate(cp)) {
						if (start != end) {
							uint32_t trail_surrogate = internal::mask16(*start++);
							if (trail_surrogate >= internal::TRAIL_SURROGATE_MIN && trail_surrogate <= internal::TRAIL_SURROGATE_MAX)
								cp = (cp << 10) + trail_surrogate + internal::SURROGATE_OFFSET;
							else
								throw invalid_utf16(static_cast<uint16_t>(trail_surrogate));
						}
						else
							throw invalid_utf16(static_cast<uint16_t>(*start));

					}
					result = append(cp, result);
				}
				return result;
			}

			template <typename u16bit_iterator, typename octet_iterator>
			u16bit_iterator utf8to16(octet_iterator start, octet_iterator end, u16bit_iterator result)
			{
				while (start != end) {
					uint32_t cp = next(start, end);
					if (cp > 0xffff) { //make a surrogate pair
						*result++ = static_cast<uint16_t>((cp >> 10) + internal::LEAD_OFFSET);
						*result++ = static_cast<uint16_t>((cp & 0x3ff) + internal::TRAIL_SURROGATE_MIN);
					}
					else
						*result++ = static_cast<uint16_t>(cp);
				}
				return result;
			}

			template <typename octet_iterator, typename u32bit_iterator>
			octet_iterator utf32to8(u32bit_iterator start, u32bit_iterator end, octet_iterator result)
			{
				while (start != end)
					result = append(*(start++), result);

				return result;
			}

			template <typename octet_iterator, typename u32bit_iterator>
			u32bit_iterator utf8to32(octet_iterator start, octet_iterator end, u32bit_iterator result)
			{
				while (start < end)
					(*result++) = next(start, end);

				return result;
			}

			// The iterator class
			template <typename octet_iterator>
			class iterator : public std::iterator <std::bidirectional_iterator_tag, uint32_t> {
				octet_iterator it;
				octet_iterator range_start;
				octet_iterator range_end;
			public:
				iterator() {}
				explicit iterator(const octet_iterator& octet_it,
					const octet_iterator& _range_start,
					const octet_iterator& _range_end) :
					it(octet_it), range_start(_range_start), range_end(_range_end)
				{
					if (it < range_start || it > range_end)
						throw std::out_of_range("Invalid utf-8 iterator position");
				}
				// the default "big three" are OK
				octet_iterator base() const { return it; }
				uint32_t operator * () const
				{
					octet_iterator temp = it;
					return next(temp, range_end);
				}
				bool operator == (const iterator& rhs) const
				{
					if (range_start != rhs.range_start || range_end != rhs.range_end)
						throw std::logic_error("Comparing utf-8 iterators defined with different ranges");
					return (it == rhs.it);
				}
				bool operator != (const iterator& rhs) const
				{
					return !(operator == (rhs));
				}
				iterator& operator ++ ()
				{
					next(it, range_end);
					return *this;
				}
				iterator operator ++ (int)
				{
					iterator temp = *this;
					next(it, range_end);
					return temp;
				}
				iterator& operator -- ()
				{
					prior(it, range_start);
					return *this;
				}
				iterator operator -- (int)
				{
					iterator temp = *this;
					prior(it, range_start);
					return temp;
				}
			}; // class iterator

			namespace unchecked
			{
				template <typename octet_iterator>
				octet_iterator append(uint32_t cp, octet_iterator result)
				{
					if (cp < 0x80)                        // one octet
						*(result++) = static_cast<char const>(cp);
					else if (cp < 0x800) {                // two octets
						*(result++) = static_cast<char const>((cp >> 6) | 0xc0);
						*(result++) = static_cast<char const>((cp & 0x3f) | 0x80);
					}
					else if (cp < 0x10000) {              // three octets
						*(result++) = static_cast<char const>((cp >> 12) | 0xe0);
						*(result++) = static_cast<char const>(((cp >> 6) & 0x3f) | 0x80);
						*(result++) = static_cast<char const>((cp & 0x3f) | 0x80);
					}
					else {                                // four octets
						*(result++) = static_cast<char const>((cp >> 18) | 0xf0);
						*(result++) = static_cast<char const>(((cp >> 12) & 0x3f) | 0x80);
						*(result++) = static_cast<char const>(((cp >> 6) & 0x3f) | 0x80);
						*(result++) = static_cast<char const>((cp & 0x3f) | 0x80);
					}
					return result;
				}

				template <typename octet_iterator>
				uint32_t next(octet_iterator& it)
				{
					uint32_t cp = internal::mask8(*it);
					typename std::iterator_traits<octet_iterator>::difference_type length = utf8::internal::sequence_length(it);
					switch (length) {
					case 1:
						break;
					case 2:
						it++;
						cp = ((cp << 6) & 0x7ff) + ((*it) & 0x3f);
						break;
					case 3:
						++it;
						cp = ((cp << 12) & 0xffff) + ((internal::mask8(*it) << 6) & 0xfff);
						++it;
						cp += (*it) & 0x3f;
						break;
					case 4:
						++it;
						cp = ((cp << 18) & 0x1fffff) + ((internal::mask8(*it) << 12) & 0x3ffff);
						++it;
						cp += (internal::mask8(*it) << 6) & 0xfff;
						++it;
						cp += (*it) & 0x3f;
						break;
					}
					++it;
					return cp;
				}

				template <typename octet_iterator>
				uint32_t peek_next(octet_iterator it)
				{
					return next(it);
				}

				template <typename octet_iterator>
				uint32_t prior(octet_iterator& it)
				{
					while (internal::is_trail(*(--it)));
					octet_iterator temp = it;
					return next(temp);
				}

				// Deprecated in versions that include prior, but only for the sake of consistency (see utf8::previous)
				template <typename octet_iterator>
				inline uint32_t previous(octet_iterator& it)
				{
					return prior(it);
				}

				template <typename octet_iterator, typename distance_type>
				void advance(octet_iterator& it, distance_type n)
				{
					for (distance_type i = 0; i < n; ++i)
						next(it);
				}

				template <typename octet_iterator>
				typename std::iterator_traits<octet_iterator>::difference_type
					distance(octet_iterator first, octet_iterator last)
				{
						typename std::iterator_traits<octet_iterator>::difference_type dist;
						for (dist = 0; first < last; ++dist)
							next(first);
						return dist;
					}

				template <typename u16bit_iterator, typename octet_iterator>
				octet_iterator utf16to8(u16bit_iterator start, u16bit_iterator end, octet_iterator result)
				{
					while (start != end) {
						uint32_t cp = internal::mask16(*start++);
						// Take care of surrogate pairs first
						if (internal::is_surrogate(cp)) {
							uint32_t trail_surrogate = internal::mask16(*start++);
							cp = (cp << 10) + trail_surrogate + internal::SURROGATE_OFFSET;
						}
						result = append(cp, result);
					}
					return result;
				}

				template <typename u16bit_iterator, typename octet_iterator>
				u16bit_iterator utf8to16(octet_iterator start, octet_iterator end, u16bit_iterator result)
				{
					while (start != end) {
						uint32_t cp = next(start);
						if (cp > 0xffff) { //make a surrogate pair
							*result++ = static_cast<uint16_t>((cp >> 10) + internal::LEAD_OFFSET);
							*result++ = static_cast<uint16_t>((cp & 0x3ff) + internal::TRAIL_SURROGATE_MIN);
						}
						else
							*result++ = static_cast<uint16_t>(cp);
					}
					return result;
				}

				template <typename octet_iterator, typename u32bit_iterator>
				octet_iterator utf32to8(u32bit_iterator start, u32bit_iterator end, octet_iterator result)
				{
					while (start != end)
						result = append(*(start++), result);

					return result;
				}

				template <typename octet_iterator, typename u32bit_iterator>
				u32bit_iterator utf8to32(octet_iterator start, octet_iterator end, u32bit_iterator result)
				{
					while (start < end)
						(*result++) = next(start);

					return result;
				}

				// The iterator class
				template <typename octet_iterator>
				class iterator : public std::iterator <std::bidirectional_iterator_tag, uint32_t> {
					octet_iterator it;
				public:
					iterator() {}
					explicit iterator(const octet_iterator& octet_it) : it(octet_it) {}
					// the default "big three" are OK
					octet_iterator base() const { return it; }
					uint32_t operator * () const
					{
						octet_iterator temp = it;
						return next(temp);
					}
					bool operator == (const iterator& rhs) const
					{
						return (it == rhs.it);
					}
					bool operator != (const iterator& rhs) const
					{
						return !(operator == (rhs));
					}
					iterator& operator ++ ()
					{
						std::advance(it, internal::sequence_length(it));
						return *this;
					}
					iterator operator ++ (int)
					{
						iterator temp = *this;
						std::advance(it, internal::sequence_length(it));
						return temp;
					}
					iterator& operator -- ()
					{
						prior(it);
						return *this;
					}
					iterator operator -- (int)
					{
						iterator temp = *this;
						prior(it);
						return temp;
					}
				}; // class iterator

			} // namespace utf8::unchecked
		} // namespace utf8
	}
	// \endcond 
	
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

		std::string ltrim(std::string str)
		{
			str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::not1(std::ptr_fun<int, int>(&std::isspace))));
			return str;
		}

		std::string rtrim(std::string str)
		{
			str.erase(std::find_if(str.rbegin(), str.rend(), std::not1(std::ptr_fun<int, int>(&std::isspace))).base(), str.end());
			return str;
		}

		std::string trim(std::string str)
		{
			return ltrim(rtrim(str));
		}

		std::string lstrip(std::string str, std::string what)
		{
			auto pos = str.find(what);
			if (0 == pos)
			{
				str.erase(pos, what.length());
			}
			return str;
		}

		std::string rstrip(std::string str, std::string what)
		{
			auto pos = str.rfind(what);
			if (str.length() - what.length() == pos)
			{
				str.erase(pos, what.length());
			}
			return str;
		}

		std::string lskip(std::string str, std::string delim)
		{
			auto pos = str.find(delim);
			if (pos == std::string::npos)
			{
				str = std::string();
			}
			else
			{
				str = str.substr(pos+1);
			}
			return str;
		}

		std::string rskip(std::string str, std::string delim)
		{
			auto pos = str.rfind(delim);
			if (pos == 0)
			{
				str = std::string();
			}
			else if (std::string::npos != pos)
			{
				str = str.substr(0, pos);
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

		void replace_sequential_with_escape(std::string &str, const std::string &replaceWhat, const std::vector<std::string> &replaceWith)
		{
			std::size_t foundAt = std::string::npos;
			std::size_t candidatePos = 0;
			while ((foundAt = str.find(replaceWhat, foundAt + 1)) != std::string::npos && replaceWith.size() > candidatePos)
			{
				if (foundAt > 0 && str[foundAt - 1] == consts::kFormatSpecifierEscapeChar)
				{
					str.erase(foundAt > 0 ? foundAt - 1 : 0, 1);
					++foundAt;
				}
				else
				{
					str.replace(foundAt, replaceWhat.length(), replaceWith[candidatePos]);
					foundAt += replaceWith[candidatePos].length();
					++candidatePos;
				}
			}
		}

		bool str_equals(const char* s1, const char* s2)
		{
			if (s1 == nullptr && s2 == nullptr) return true;
			if (s1 == nullptr || s2 == nullptr) return false;
			return std::string(s1) == std::string(s2);	// this is safe, not with strcmp
		}

		//std::string& left_zero_padding(std::string &str, int width)
		//{
		//	int toPad = width - static_cast<int>(str.length());
		//	while (toPad > 0)
		//	{
		//		str = consts::kZeroPaddingStr + str;
		//		--toPad;
		//	}
		//	return str;
		//}

		std::string to_lower_ascii(std::string mixed)
		{
			std::transform(mixed.begin(), mixed.end(), mixed.begin(), ::tolower);
			return mixed;
		}

		std::string to_upper_ascii(std::string mixed)
		{
			std::transform(mixed.begin(), mixed.end(), mixed.begin(), ::toupper);
			return mixed;
		}
		
		std::u16string utf8_to_utf16(std::string u8str)
		{
			try
			{
				std::u16string ret;
				thirdparty::utf8::utf8to16(u8str.begin(), u8str.end(), std::back_inserter(ret));
				return ret;
			}
			catch (...)
			{
				throw RuntimeException("Invalid UTF-8 string.");
			}
		}

		std::string utf16_to_utf8(std::u16string u16str)
		{
			try
			{
				std::vector<unsigned char> u8vec;
				thirdparty::utf8::utf16to8(u16str.begin(), u16str.end(), std::back_inserter(u8vec));
				auto ptr = reinterpret_cast<char*>(u8vec.data());
				return std::string(ptr, ptr + u8vec.size());
			}
			catch (...)
			{
				throw RuntimeException("Invalid UTF-16 string.");
			}
		}

		std::u32string utf8_to_utf32(std::string u8str)
		{
			try
			{
				std::u32string ret;
				thirdparty::utf8::utf8to32(u8str.begin(), u8str.end(), std::back_inserter(ret));
				return ret;
			}
			catch (...)
			{
				throw RuntimeException("Invalid UTF-8 string.");
			}
		}

		std::string utf32_to_utf8(std::u32string u32str)
		{
			try
			{
				std::vector<unsigned char> u8vec;
				thirdparty::utf8::utf32to8(u32str.begin(), u32str.end(), std::back_inserter(u8vec));
				auto ptr = reinterpret_cast<char*>(u8vec.data());
				return std::string(ptr, ptr + u8vec.size());
			}
			catch (...)
			{
				throw RuntimeException("Invalid UTF-32 string.");
			}
		}

	} // namespace fmt

	namespace fs
	{
		FileEditor::FileEditor(std::string filename, bool truncateOrNot, int retryTimes, int retryInterval)
		{
			// use absolute path
			filename_ = os::absolute_path(filename);
			// try open
			this->try_open(retryTimes, retryInterval, truncateOrNot);
		};

		//FileEditor::FileEditor(FileEditor&& other) : filename_(std::move(other.filename_)),
		//	stream_(std::move(other.stream_)),
		//	readPos_(std::move(other.readPos_)),
		//	writePos_(std::move(other.writePos_))
		//{
		//	other.filename_ = std::string();
		//};

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

		FileReader::FileReader(std::string filename, int retryTimes, int retryInterval)
		{
			// use absolute path
			filename_ = os::absolute_path(filename);
			// try open
			this->try_open(retryTimes, retryInterval);
		}

		//FileReader::FileReader(FileReader&& other) : filename_(std::move(other.filename_)), istream_(std::move(other.istream_))
		//{
		//	other.filename_ = std::string();
		//};

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
			static const unsigned		kMaxDateTimeLength = 2048;
			static const char			*kDateFractionSpecifier = "%frac";
			static const unsigned	    kDateFractionWidth = 3;
			static const char			*kTimerPrecisionSecSpecifier = "%sec";
			static const char			*kTimerPrecisionMsSpecifier = "%ms";
			static const char			*kTimerPrecisionUsSpecifier = "%us";
			static const char			*kTimerPrecisionNsSpecifier = "%ns";
			static const char			*kDateTimeSpecifier = "%datetime";
		}

		DateTime::DateTime()
		{
			auto now = std::chrono::system_clock::now();
			timeStamp_ = std::chrono::system_clock::to_time_t(now);
			calendar_ = os::localtime(timeStamp_);
			fraction_ = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
				% math::Pow<10, consts::kDateFractionWidth>::result;
			fractionStr_ = fmt::int_to_zero_pad_str(fraction_, consts::kDateFractionWidth);
		}

		void DateTime::to_local_time()
		{
			calendar_ = os::localtime(timeStamp_);
		}

		void DateTime::to_utc_time()
		{
			calendar_ = os::gmtime(timeStamp_);
		}

		std::string DateTime::to_string(const char *format)
		{
			std::string fmt(format);
			fmt::replace_all_with_escape(fmt, consts::kDateFractionSpecifier, fractionStr_);
			std::vector<char> mbuf(fmt.length() + 100);
			std::size_t size = strftime(mbuf.data(), mbuf.size(), fmt.c_str(), &calendar_);
			while (size == 0)
			{
				if (mbuf.size() > consts::kMaxDateTimeLength)
				{
					return std::string("String size exceed limit!");
				}
				mbuf.resize(mbuf.size() * 2);
				size = strftime(mbuf.data(), mbuf.size(), fmt.c_str(), &calendar_);
			}
			return std::string(mbuf.begin(), mbuf.begin() + size);
		}

		DateTime DateTime::local_time()
		{
			DateTime date;
			return date;
		}

		DateTime DateTime::utc_time()
		{
			DateTime date;
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
			elapsed_ = 0;
			paused_ = false;
		}

		void Timer::pause()
		{
			if (paused_) return;
			elapsed_ += elapsed_ns();
			
			paused_ = true;
		}

		void Timer::resume()
		{
			if (!paused_) return;
			timeStamp_ = std::chrono::steady_clock::now();
			paused_ = false;
		}

		std::size_t	Timer::elapsed_ns()
		{
			if (paused_) return elapsed_;
			return static_cast<std::size_t>(std::chrono::duration_cast<std::chrono::nanoseconds>
				(std::chrono::steady_clock::now() - timeStamp_).count()) + elapsed_;
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
			static std::mutex threadIdMutex;
			static std::map<std::thread::id, std::size_t> threadIdHashmap;
			std::lock_guard<std::mutex> lock(threadIdMutex);
			auto id = std::this_thread::get_id();
			if (threadIdHashmap.count(id) < 1)
			{
				threadIdHashmap[id] = threadIdHashmap.size() + 1;
			}
			return threadIdHashmap[id];
			//return static_cast<size_t>(std::hash<std::thread::id>()(std::this_thread::get_id()));
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


		void fstream_open(std::fstream &stream, std::string filename, std::ios::openmode openmode)
		{
			// make sure directory exists for the target file
			create_directory_recursive(path_split_directory(filename));
#if ZUPPLY_OS_WINDOWS
			stream.open(utf8_to_wstring(filename), openmode);
#else
			stream.open(filename, openmode);
#endif
		}

		void ifstream_open(std::ifstream &stream, std::string filename, std::ios::openmode openmode)
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
			return (!::rename(oldName.c_str(), newName.c_str()));
#endif
		}

		bool copyfile(std::string src, std::string dst, bool replaceDst)
		{
			if (!replaceDst)
			{
				if (path_exists(dst, true)) return false;
			}
			remove_all(dst);
			std::ifstream  srcf;
			std::fstream  dstf;
			ifstream_open(srcf, src, std::ios::binary);
			fstream_open(dstf, dst, std::ios::binary|std::ios::trunc);
			dstf << srcf.rdbuf();
			return true;
		}

		bool movefile(std::string src, std::string dst, bool replaceDst)
		{
			if (!replaceDst)
			{
				if (path_exists(dst, true)) return false;
			}
			return os::rename(src, dst);
		}

		bool remove_all(std::string path)
		{
			if (!os::path_exists(path, true)) return true;
			if (os::path_exists(path, false))
			{
				return remove_dir(path);
			}
			return remove_file(path);
		}

#if ZUPPLY_OS_UNIX 
		// for nftw tree walk directory operations, so for unix only
		int nftw_remove(const char *path, const struct stat *sb, int flag, struct FTW *ftwbuf)
		{
			misc::unused(sb);
			misc::unused(flag);
			misc::unused(ftwbuf);
			return ::remove(path);
		}
#endif

		bool remove_dir(std::string path, bool recursive)
		{
			// as long as dir does not exist, return true
			if (!path_exists(path, false)) return true;
#if ZUPPLY_OS_WINDOWS
			std::wstring root = utf8_to_wstring(path);
			bool            bSubdirectory = false;       // Flag, indicating whether subdirectories have been found
			HANDLE          hFile;                       // Handle to directory
			std::wstring     strFilePath;                 // Filepath
			std::wstring     strPattern;                  // Pattern
			WIN32_FIND_DATAW FileInformation;             // File information


			strPattern = root + L"\\*.*";
			hFile = ::FindFirstFileW(strPattern.c_str(), &FileInformation);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				do
				{
					if (FileInformation.cFileName[0] != '.')
					{
						strFilePath.erase();
						strFilePath = root + L"\\" + FileInformation.cFileName;

						if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
						{
							if (recursive)
							{
								// Delete subdirectory
								bool iRC = remove_dir(wstring_to_utf8(strFilePath), recursive);
								if (!iRC) return false;
							}
							else
								bSubdirectory = true;
						}
						else
						{
							// Set file attributes
							if (::SetFileAttributesW(strFilePath.c_str(),
								FILE_ATTRIBUTE_NORMAL) == FALSE)
								return false;

							// Delete file
							if (::DeleteFileW(strFilePath.c_str()) == FALSE)
								return false;
						}
					}
				} while (::FindNextFileW(hFile, &FileInformation) == TRUE);

				// Close handle
				::FindClose(hFile);

				DWORD dwError = ::GetLastError();
				if (dwError != ERROR_NO_MORE_FILES)
					return false;
				else
				{
					if (!bSubdirectory)
					{
						// Set directory attributes
						if (::SetFileAttributesW(root.c_str(),
							FILE_ATTRIBUTE_NORMAL) == FALSE)
							return false;

						// Delete directory
						if (::RemoveDirectoryW(root.c_str()) == FALSE)
							return false;
					}
				}
			}

			return true;
#else
			if (recursive) ::nftw(path.c_str(), nftw_remove, 20, FTW_DEPTH);
			else ::remove(path.c_str());
			return (path_exists(path, false) == false);
#endif
		}

		bool remove_file(std::string path)
		{
#if ZUPPLY_OS_WINDOWS
			_wremove(utf8_to_wstring(path).c_str());
#else
			unlink(path.c_str());
#endif
			return (os::path_exists(path, true) == false);
		}

		std::string last_error()
		{
#if ZUPPLY_OS_WINDOWS
			DWORD error = GetLastError();
			if (error)
			{
				LPVOID lpMsgBuf;
				DWORD bufLen = FormatMessage(
					FORMAT_MESSAGE_ALLOCATE_BUFFER |
					FORMAT_MESSAGE_FROM_SYSTEM |
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					error,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPTSTR)&lpMsgBuf,
					0, NULL);
				if (bufLen)
				{
					LPCSTR lpMsgStr = (LPCSTR)lpMsgBuf;
					std::string result(lpMsgStr, lpMsgStr + bufLen);

					LocalFree(lpMsgBuf);

					return result;
				}
			}
#else
			char* errStr = strerror(errno);
			if (errStr) return std::string(errStr);
#endif
			return std::string();
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
				return std::string(".");
			}
			else
			{
				std::wstring ret(buffer);
				free(buffer);
				return wstring_to_utf8(ret);
			}
#elif _GNU_SOURCE
			char *buffer = get_current_dir_name();
			if (buffer == nullptr)
			{
				// failed
				return std::string(".");
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
				return std::string(".");
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

		bool path_exists(std::string path, bool considerFile)
		{
#if ZUPPLY_OS_WINDOWS
			DWORD fileType = GetFileAttributesW(utf8_to_wstring(path).c_str());
			if (fileType == INVALID_FILE_ATTRIBUTES) {
				return false;
			}
			return considerFile ? true : ((fileType & FILE_ATTRIBUTE_DIRECTORY) == 0 ? false : true);
#elif ZUPPLY_OS_UNIX
			struct stat st;
			int ret = stat(path.c_str(), &st);
			return considerFile ? (ret == 0) : S_ISDIR(st.st_mode);			
#endif
			return false;
		}

		bool is_file(std::string path)
		{
#if ZUPPLY_OS_WINDOWS
			DWORD fileType = GetFileAttributesW(utf8_to_wstring(path).c_str());
			if (fileType == INVALID_FILE_ATTRIBUTES || ((fileType & FILE_ATTRIBUTE_DIRECTORY) != 0)) 
			{
				return false;
			}
			return true;
#elif ZUPPLY_OS_UNIX
			struct stat st;
			if (stat(path.c_str(), &st) != 0) return false;
			if (S_ISDIR(st.st_mode)) return false;
			return true;
#endif
			return false;
		}

		bool is_directory(std::string path)
		{
#if ZUPPLY_OS_WINDOWS
			DWORD fileType = GetFileAttributesW(utf8_to_wstring(path).c_str());
			if (fileType == INVALID_FILE_ATTRIBUTES) return false;
			if ((fileType & FILE_ATTRIBUTE_DIRECTORY) != 0) return true;
			return false;
#elif ZUPPLY_OS_UNIX
			struct stat st;
			if (stat(path.c_str(), &st) != 0) return false;
			if (S_ISDIR(st.st_mode)) return true;
			return false;
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
				return reletivePath;
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
				if (ENOENT == errno)
				{
					if (fmt::starts_with(reletivePath, current_working_directory()))
					{
						return reletivePath;
					}
					else
					{
						return path_join({ current_working_directory(), reletivePath });
					}
				}
				return reletivePath;
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

		std::string normalize_path(std::string dirtyPath)
		{
			std::string absPath = absolute_path(dirtyPath);
#if ZUPPLY_OS_WINDOWS
			return absPath;
#else
			std::vector<std::string> parts = path_split(dirtyPath);
			std::vector<std::string> ret;
			for (auto i = parts.begin(); i != parts.end(); ++i)
			{
				if (*i == ".") continue;
				if (*i == "..")
				{
					if (ret.size() < 1) throw RuntimeException("Invalid path: " + dirtyPath);
					ret.pop_back();
				}
				else
				{
					ret.push_back(*i);
				}
			}
			return "/" + path_join(ret);
#endif
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
			std::string tmp = normalize_path(path);
			std::string target = tmp;

			while (!path_exists(tmp))
			{
				if (tmp.empty()) return false;	// invalid path
				tmp = normalize_path(tmp + "/../");
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

	namespace cds
	{
		bool RWLockable::Counters::is_waiting_for_write() const {
			return writeClaim_ != writeDone_;
		}

		bool RWLockable::Counters::is_waiting_for_read() const {
			return read_ != 0;
		}

		bool RWLockable::Counters::is_my_turn_to_write(Counters const & claim) const {
			return writeDone_ == claim.writeClaim_ - 1;
		}

		bool RWLockable::Counters::want_to_read(RWLockable::Counters * buf) const {
			if (read_ == UINT16_MAX) {
				return false;
			}
			*buf = *this;
			buf->read_ += 1;
			return true;
		}

		bool RWLockable::Counters::want_to_write(RWLockable::Counters * buf) const {
			if (writeClaim_ == UINT8_MAX) {
				return false;
			}
			*buf = *this;
			buf->writeClaim_ += 1;
			return true;
		}

		RWLockable::Counters RWLockable::Counters::done_reading() const {
			Counters c = *this;
			c.read_ -= 1;
			return c;
		}

		RWLockable::Counters RWLockable::Counters::done_writing() const {
			Counters c = *this;
			c.writeDone_ += 1;
			if (c.writeDone_ == UINT8_MAX) {
				c.writeClaim_ = c.writeDone_ = 0;
			}
			return c;
		}

		RWLock RWLockable::lock_for_read() {
			bool written = false;
			do {
				Counters exp = counters_.load(std::memory_order_acquire);
				do {
					if (exp.is_waiting_for_write()) {
						break;
					}
					Counters claim;
					if (!exp.want_to_read(&claim)) {
						break;
					}
					written = counters_.compare_exchange_weak(
						exp, claim,
						std::memory_order_release,
						std::memory_order_acquire
						);
				} while (!written);
				// todo: if (!written) progressive backoff
			} while (!written);
			return RWLock(this, false);
		}

		RWLock RWLockable::lock_for_write() {
			Counters exp = counters_.load(std::memory_order_acquire);
			Counters claim;
			do {
				while (!exp.want_to_write(&claim)) {
					// todo: progressive backoff
					exp = counters_.load(std::memory_order_acquire);
				}
			} while (!counters_.compare_exchange_weak(
				exp, claim,
				std::memory_order_release,
				std::memory_order_acquire
				));
			while (exp.is_waiting_for_read() || !exp.is_my_turn_to_write(claim)) {
				// todo: progressive backoff
				exp = counters_.load(std::memory_order_acquire);
			}
			return RWLock(this, true);
		}

		void RWLockable::unlock_read() {
			Counters exp = counters_.load(std::memory_order_consume);
			Counters des;
			do {
				des = exp.done_reading();
			} while (!counters_.compare_exchange_weak(
				exp, des,
				std::memory_order_release,
				std::memory_order_consume
				));
		}

		void RWLockable::unlock_write() {
			Counters exp = counters_.load(std::memory_order_consume);
			Counters des;
			do {
				des = exp.done_writing();
			} while (!counters_.compare_exchange_weak(
				exp, des,
				std::memory_order_release,
				std::memory_order_consume
				));
		}

		bool RWLockable::is_lock_free() const {
			return counters_.is_lock_free();
		}

		RWLock::RWLock(RWLockable * const lockable, bool const exclusive)
			: lockable_(lockable)
			, lockType_(exclusive ? LockType::write : LockType::read)
		{}

		RWLock::RWLock()
			: lockable_(nullptr)
			, lockType_(LockType::none)
		{}


		RWLock::RWLock(RWLock&& rhs) {
			lockable_ = rhs.lockable_;
			lockType_ = rhs.lockType_;
			rhs.lockable_ = nullptr;
			rhs.lockType_ = LockType::none;
		}

		RWLock& RWLock::operator =(RWLock&& rhs) {
			std::swap(lockable_, rhs.lockable_);
			std::swap(lockType_, rhs.lockType_);
			return *this;
		}


		RWLock::~RWLock() {
			if (lockable_ == nullptr) {
				return;
			}
			switch (lockType_) {
			case LockType::read:
				lockable_->unlock_read();
				break;
			case LockType::write:
				lockable_->unlock_write();
				break;
			default:
				// do nothing
				break;
			}
		}

		void RWLock::unlock() {
			(*this) = RWLock();
			// emptyLock's dtor will now activate
		}

		RWLock::LockType RWLock::get_lock_type() const {
			return lockType_;
		}
	} // namespace cds

	namespace cfg
	{

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

		CfgLevel* CfgParser::parse_section(std::string sline, CfgLevel* lvl)
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
			std::string key = fmt::trim(std::string(line.substr(0, pos)));
			line = fmt::trim(std::string(line.substr(pos + 1)));
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
				line_ = fmt::trim(line_);
				if (line_.empty()) continue;
				if (line_[0] == '[')
				{
					// start a section
					if (line_.back() != ']')
					{
						error_handler("missing right ]");
					}
					ls = &lvl;
					ls = parse_section(std::string(line_.substr(1, line_.length()-2)), ls);
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

		std::vector<Value> ArgParser::arguments() const
		{
			return args_;
		}

		void ArgParser::register_keys(char shortKey, std::string longKey, std::size_t pos)
		{
			if (shortKey != -1)
			{
				if (shortKey < 32 || shortKey > 126)
				{
					// unsupported ASCII characters
					throw ArgException("Unsupported ASCII character: " + std::string(1, shortKey));
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
			if (opt.length() == 1 && opt[0] == '-') return Type::INVALID;
			if (opt.length() == 2 && opt == "--") return Type::INVALID;
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

		ArgParser::ArgQueue ArgParser::pretty_arguments(int argc, const char** argv)
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

		void ArgParser::parse(int argc, const char** argv, bool ignoreUnknown)
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

	namespace log
	{
		namespace detail
		{
			LoggerRegistry& LoggerRegistry::instance()
			{
				static LoggerRegistry sInstance;
				return sInstance;
			}

			LoggerPtr LoggerRegistry::create(const std::string &name)
			{
				auto ptr = new_registry(name);
				if (!ptr)
				{
					throw RuntimeException("Logger with name: " + name + " already existed.");
				}
				return ptr;
			}

			LoggerPtr LoggerRegistry::ensure_get(std::string &name)
			{
				LoggerPtr ptr;
				if (loggers_.get(name, ptr))
				{
					return ptr;
				}
				
				do
				{
					ptr = new_registry(name);
				} while (ptr == nullptr);

				return ptr;
			}

			LoggerPtr LoggerRegistry::get(std::string &name)
			{
				LoggerPtr ptr;
				if (loggers_.get(name, ptr)) return ptr;
				return nullptr;
			}

			std::vector<LoggerPtr> LoggerRegistry::get_all()
			{
				std::vector<LoggerPtr> list;
				auto loggers = loggers_.snapshot();
				for (auto logger : loggers)
				{
					list.push_back(logger.second);
				}
				return list;
			}

			void LoggerRegistry::drop(const std::string &name)
			{
				loggers_.erase(name);
			}

			void LoggerRegistry::drop_all()
			{
				loggers_.clear();
			}

			void LoggerRegistry::lock()
			{
				lock_ = true;
			}

			void LoggerRegistry::unlock()
			{
				lock_ = false;
			}

			bool LoggerRegistry::is_locked() const
			{
				return lock_;
			}

			LoggerPtr LoggerRegistry::new_registry(const std::string &name)
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

			RotateFileSink::RotateFileSink(const std::string filename, std::size_t maxSizeInByte, bool backup)
				:maxSizeInByte_(maxSizeInByte), backup_(backup)
			{
				if (backup_)
				{
					back_up(filename);
				}
				fileEditor_.open(filename, true);
				currentSize_ = 0;
			}

			void RotateFileSink::back_up(std::string oldFile)
			{
				std::string backupName = os::path_append_basename(oldFile,
					time::DateTime::local_time().to_string("_%y-%m-%d_%H-%M-%S-%frac"));
				os::rename(oldFile, backupName);
			}

			void RotateFileSink::rotate()
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

			void sink_list_revise(std::vector<std::string> &list, std::map<std::string, std::string> &map)
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

			void config_loggers_from_section(cfg::CfgLevel::section_map_t &section, std::map<std::string, std::string> &map)
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

			LoggerPtr get_hidden_logger()
			{
				auto hlogger = get_logger("hidden", false);
				if (!hlogger)
				{
					hlogger = get_logger("hidden", true);
					hlogger->set_level_mask(0);
				}
				return hlogger;
			}

			std::map<std::string, std::string> config_sinks_from_section(cfg::CfgLevel::section_map_t &section)
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

			void zupply_internal_warn(std::string msg)
			{
				auto zlog = get_logger(consts::kZupplyInternalLoggerName, true);
				zlog->warn() << msg;
			}

			void zupply_internal_error(std::string msg)
			{
				auto zlog = get_logger(consts::kZupplyInternalLoggerName, true);
				zlog->error() << msg;
			}
		} // namespace log::detail

		LogConfig::LogConfig()
		{
			// Default configurations
			sinkList_.set({ std::string(consts::kStdoutSinkName), std::string(consts::kStderrSinkName) });	//!< attach console by default
#ifdef NDEBUG
			logLevelMask_ = 0x3C;	//!< 0x3C->b111100: no debug, no trace
#else
			logLevelMask_ = 0x3E;	//!< 0x3E->b111110: debug, no trace
#endif
			format_.set(std::string(consts::kDefaultLoggerFormat));
			datetimeFormat_.set(std::string(consts::kDefaultLoggerDatetimeFormat));
		}

		LogConfig& LogConfig::instance()
		{
			static LogConfig instance_;
			return instance_;
		}

		void LogConfig::set_default_format(std::string format)
		{
			LogConfig::instance().set_format(format);
		}

		void LogConfig::set_default_datetime_format(std::string dateFormat)
		{
			LogConfig::instance().set_datetime_format(dateFormat);
		}

		void LogConfig::set_default_sink_list(std::vector<std::string> list)
		{
			LogConfig::instance().set_sink_list(list);
		}

		void LogConfig::set_default_level_mask(int levelMask)
		{
			LogConfig::instance().set_log_level_mask(levelMask);
		}

		std::vector<std::string> LogConfig::sink_list()
		{
			return sinkList_.get();
		}

		void LogConfig::set_sink_list(std::vector<std::string> &list)
		{
			sinkList_.set(list);
		}

		int LogConfig::log_level_mask()
		{
			return logLevelMask_;
		}

		void LogConfig::set_log_level_mask(int newMask)
		{
			logLevelMask_ = newMask;
		}

		std::string LogConfig::format()
		{
			return format_.get();
		}

		void LogConfig::set_format(std::string newFormat)
		{
			format_.set(newFormat);
		}

		std::string LogConfig::datetime_format()
		{
			return datetimeFormat_.get();
		}

		void LogConfig::set_datetime_format(std::string newDatetimeFormat)
		{
			datetimeFormat_.set(newDatetimeFormat);
		}

		// logger.info() << ".." call  style
		detail::LineLogger Logger::trace()
		{
			return log_if_enabled(LogLevels::trace);
		}
		detail::LineLogger Logger::debug()
		{
			return log_if_enabled(LogLevels::debug);
		}
		detail::LineLogger Logger::info()
		{
			return log_if_enabled(LogLevels::info);
		}
		detail::LineLogger Logger::warn()
		{
			return log_if_enabled(LogLevels::warn);
		}
		detail::LineLogger Logger::error()
		{
			return log_if_enabled(LogLevels::error);
		}
		detail::LineLogger Logger::fatal()
		{
			return log_if_enabled(LogLevels::fatal);
		}

		SinkPtr Logger::get_sink(std::string name)
		{
			SinkPtr ptr;
			if (sinks_.get(name, ptr))
			{
				return ptr;
			}
			return nullptr;
		}

		void Logger::attach_sink(SinkPtr sink)
		{
			if (!sinks_.insert(sink->name(), sink))
			{
				throw RuntimeException("Sink with name: " + sink->name() + " already attached to logger: " + name_);
			}
		}

		void Logger::detach_sink(SinkPtr sink)
		{
			sinks_.erase(sink->name());
		}

		void Logger::log_msg(detail::LogMessage msg)
		{
			auto sinks = sinks_.snapshot();
			for (auto s : sinks)
			{
				s.second->log(msg);
			}
		}

		std::string Logger::to_string()
		{
			std::string str(name() + ": " + level_mask_to_string(levelMask_));
			str += "\n{\n";
			auto sinkmap = sinks_.snapshot();
			for (auto sink : sinkmap)
			{
				str += sink.second->to_string() + "\n";
			}
			str += "}";
			return str;
		}

		std::string level_mask_to_string(int levelMask)
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

		LogLevels level_from_str(std::string level)
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

		int level_mask_from_string(std::string levels)
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

		LoggerPtr get_logger(std::string name, bool createIfNotExists)
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

		SinkPtr new_stdout_sink()
		{
			return detail::StdoutSink::instance();
		}

		SinkPtr new_stderr_sink()
		{
			return detail::StderrSink::instance();
		}

		void Logger::attach_console()
		{
			sinks_.insert(std::string(consts::kStdoutSinkName), new_stdout_sink());
			sinks_.insert(std::string(consts::kStderrSinkName), new_stderr_sink());
		}

		void Logger::detach_console()
		{
			sinks_.erase(std::string(consts::kStdoutSinkName));
			sinks_.erase(std::string(consts::kStderrSinkName));
		}

		SinkPtr get_sink(std::string name)
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

		void dump_loggers(std::ostream &out)
		{
			auto loggers = detail::LoggerRegistry::instance().get_all();
			out << "{\n";
			for (auto logger : loggers)
			{
				out << logger->to_string() << "\n";
			}
			out << "}" << std::endl;
		}

		SinkPtr new_ostream_sink(std::ostream &stream, std::string name, bool forceFlush)
		{
			auto sinkptr = get_sink(name);
			if (sinkptr)
			{
				throw RuntimeException(name + " already holded by another sink\n" + sinkptr->to_string());
			}
			return std::make_shared<detail::OStreamSink>(stream, name.c_str(), forceFlush);
		}

		SinkPtr new_simple_file_sink(std::string filename, bool truncate)
		{
			auto sinkptr = get_sink(os::absolute_path(filename));
			if (sinkptr)
			{
				throw RuntimeException("File: " + filename + " already holded by another sink!\n" + sinkptr->to_string());
			}
			return std::make_shared<detail::SimpleFileSink>(filename, truncate);
		}

		SinkPtr new_rotate_file_sink(std::string filename, std::size_t maxSizeInByte, bool backupOld)
		{
			auto sinkptr = get_sink(os::absolute_path(filename));
			if (sinkptr)
			{
				throw RuntimeException("File: " + filename + " already holded by another sink!\n" + sinkptr->to_string());
			}
			return std::make_shared<detail::RotateFileSink>(filename, maxSizeInByte, backupOld);
		}

		void Logger::attach_sink_list(std::vector<std::string> &sinkList)
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

		void lock_loggers()
		{
			detail::LoggerRegistry::instance().lock();
		}

		void unlock_loggers()
		{
			detail::LoggerRegistry::instance().unlock();
		}

		void drop_logger(std::string name)
		{
			detail::LoggerRegistry::instance().drop(name);
		}

		void drop_all_loggers()
		{
			detail::LoggerRegistry::instance().drop_all();
		}

		void drop_sink(std::string name)
		{
			SinkPtr psink = get_sink(name);
			if (nullptr == psink) return;
			auto loggers = detail::LoggerRegistry::instance().get_all();
			for (auto logger : loggers)
			{
				logger->detach_sink(psink);
			}
		}

		void detail::config_from_parser(cfg::CfgParser& parser)
		{
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

		void config_from_file(std::string cfgFilename)
		{
			cfg::CfgParser parser(cfgFilename);
			detail::config_from_parser(parser);
		}

		void config_from_stringstream(std::stringstream& ss)
		{
			cfg::CfgParser parser(ss);
			detail::config_from_parser(parser);
		}

	} // namespace log

} // end namesapce zz
