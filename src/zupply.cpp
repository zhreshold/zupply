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
#define ZUPPLY_OS_UNIX	1	//!< Unix like OS(POSIX compliant)
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
#if _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <windows.h>
#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <io.h>
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
#include <deque>

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

		namespace stbi
		{
			namespace decode
			{
				enum
				{
					STBI_default = 0, // only used for req_comp

					STBI_grey = 1,
					STBI_grey_alpha = 2,
					STBI_rgb = 3,
					STBI_rgb_alpha = 4
				};

				typedef unsigned char stbi_uc;
				//////////////////////////////////////////////////////////////////////////////
				//
				// PRIMARY API - works on images of any type
				//

				//
				// load image by filename, open file, or memory buffer
				//

				typedef struct
				{
					int(*read)  (void *user, char *data, int size);   // fill 'data' with 'size' bytes.  return number of bytes actually read
					void(*skip)  (void *user, int n);                 // skip the next 'n' bytes, or 'unget' the last -n bytes if negative
					int(*eof)   (void *user);                       // returns nonzero if we are at end of file/data
				} stbi_io_callbacks;

				stbi_uc *stbi_load(char              const *filename, int *x, int *y, int *comp, int req_comp);
				stbi_uc *stbi_load_from_memory(stbi_uc           const *buffer, int len, int *x, int *y, int *comp, int req_comp);
				stbi_uc *stbi_load_from_callbacks(stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp, int req_comp);

#ifndef STBI_NO_STDIO
				stbi_uc *stbi_load_from_file(FILE *f, int *x, int *y, int *comp, int req_comp);
				// for stbi_load_from_file, file pointer is left pointing immediately after image
#endif

#ifndef STBI_NO_LINEAR
				float *stbi_loadf(char const *filename, int *x, int *y, int *comp, int req_comp);
				float *stbi_loadf_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp);
				float *stbi_loadf_from_callbacks(stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp, int req_comp);

#ifndef STBI_NO_STDIO
				float *stbi_loadf_from_file(FILE *f, int *x, int *y, int *comp, int req_comp);
#endif
#endif

#ifndef STBI_NO_HDR
				void   stbi_hdr_to_ldr_gamma(float gamma);
				void   stbi_hdr_to_ldr_scale(float scale);
#endif

#ifndef STBI_NO_LINEAR
				void   stbi_ldr_to_hdr_gamma(float gamma);
				void   stbi_ldr_to_hdr_scale(float scale);
#endif // STBI_NO_HDR

				// stbi_is_hdr is always defined, but always returns false if STBI_NO_HDR
				int    stbi_is_hdr_from_callbacks(stbi_io_callbacks const *clbk, void *user);
				int    stbi_is_hdr_from_memory(stbi_uc const *buffer, int len);
#ifndef STBI_NO_STDIO
				int      stbi_is_hdr(char const *filename);
				int      stbi_is_hdr_from_file(FILE *f);
#endif // STBI_NO_STDIO


				// get a VERY brief reason for failure
				// NOT THREADSAFE
				const char *stbi_failure_reason(void);

				// free the loaded image -- this is just free()
				void     stbi_image_free(void *retval_from_stbi_load);

				// get image dimensions & components without fully decoding
				int      stbi_info_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp);
				int      stbi_info_from_callbacks(stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp);

#ifndef STBI_NO_STDIO
				int      stbi_info(char const *filename, int *x, int *y, int *comp);
				int      stbi_info_from_file(FILE *f, int *x, int *y, int *comp);

#endif



				// for image formats that explicitly notate that they have premultiplied alpha,
				// we just return the colors as stored in the file. set this flag to force
				// unpremultiplication. results are undefined if the unpremultiply overflow.
				void stbi_set_unpremultiply_on_load(int flag_true_if_should_unpremultiply);

				// indicate whether we should process iphone images back to canonical format,
				// or just pass them through "as-is"
				void stbi_convert_iphone_png_to_rgb(int flag_true_if_should_convert);

				// flip the image vertically, so the first pixel in the output array is the bottom left
				void stbi_set_flip_vertically_on_load(int flag_true_if_should_flip);

				// ZLIB client - used by PNG, available for other purposes

				char *stbi_zlib_decode_malloc_guesssize(const char *buffer, int len, int initial_size, int *outlen);
				char *stbi_zlib_decode_malloc_guesssize_headerflag(const char *buffer, int len, int initial_size, int *outlen, int parse_header);
				char *stbi_zlib_decode_malloc(const char *buffer, int len, int *outlen);
				int   stbi_zlib_decode_buffer(char *obuffer, int olen, const char *ibuffer, int ilen);

				char *stbi_zlib_decode_noheader_malloc(const char *buffer, int len, int *outlen);
				int   stbi_zlib_decode_noheader_buffer(char *obuffer, int olen, const char *ibuffer, int ilen);

				// implementation
#if defined(STBI_ONLY_JPEG) || defined(STBI_ONLY_PNG) || defined(STBI_ONLY_BMP) \
	|| defined(STBI_ONLY_TGA) || defined(STBI_ONLY_GIF) || defined(STBI_ONLY_PSD) \
	|| defined(STBI_ONLY_HDR) || defined(STBI_ONLY_PIC) || defined(STBI_ONLY_PNM) \
	|| defined(STBI_ONLY_ZLIB)
#ifndef STBI_ONLY_JPEG
#define STBI_NO_JPEG
#endif
#ifndef STBI_ONLY_PNG
#define STBI_NO_PNG
#endif
#ifndef STBI_ONLY_BMP
#define STBI_NO_BMP
#endif
#ifndef STBI_ONLY_PSD
#define STBI_NO_PSD
#endif
#ifndef STBI_ONLY_TGA
#define STBI_NO_TGA
#endif
#ifndef STBI_ONLY_GIF
#define STBI_NO_GIF
#endif
#ifndef STBI_ONLY_HDR
#define STBI_NO_HDR
#endif
#ifndef STBI_ONLY_PIC
#define STBI_NO_PIC
#endif
#ifndef STBI_ONLY_PNM
#define STBI_NO_PNM
#endif
#endif

#if defined(STBI_NO_PNG) && !defined(STBI_SUPPORT_ZLIB) && !defined(STBI_NO_ZLIB)
#define STBI_NO_ZLIB
#endif


#include <stdarg.h>
#include <stddef.h> // ptrdiff_t on osx
#include <stdlib.h>
#include <string.h>

#if !defined(STBI_NO_LINEAR) || !defined(STBI_NO_HDR)
#include <math.h>  // ldexp
#endif

#ifndef STBI_NO_STDIO
#include <stdio.h>
#endif

#ifndef STBI_ASSERT
#include <assert.h>
#define STBI_ASSERT(x) assert(x)
#endif


#ifndef _MSC_VER
#ifdef __cplusplus
#define stbi_inline inline
#else
#define stbi_inline
#endif
#else
#define stbi_inline __forceinline
#endif


#ifdef _MSC_VER
				typedef unsigned short stbi__uint16;
				typedef   signed short stbi__int16;
				typedef unsigned int   stbi__uint32;
				typedef   signed int   stbi__int32;
#else
#include <stdint.h>
				typedef uint16_t stbi__uint16;
				typedef int16_t  stbi__int16;
				typedef uint32_t stbi__uint32;
				typedef int32_t  stbi__int32;
#endif

				// should produce compiler error if size is wrong
				typedef unsigned char validate_uint32[sizeof(stbi__uint32) == 4 ? 1 : -1];

#ifdef _MSC_VER
#define STBI_NOTUSED(v)  (void)(v)
#else
#define STBI_NOTUSED(v)  (void)sizeof(v)
#endif

#ifdef _MSC_VER
#define STBI_HAS_LROTL
#endif

#ifdef STBI_HAS_LROTL
#define stbi_lrot(x,y)  _lrotl(x,y)
#else
#define stbi_lrot(x,y)  (((x) << (y)) | ((x) >> (32 - (y))))
#endif

#if defined(STBI_MALLOC) && defined(STBI_FREE) && defined(STBI_REALLOC)
				// ok
#elif !defined(STBI_MALLOC) && !defined(STBI_FREE) && !defined(STBI_REALLOC)
				// ok
#else
#error "Must define all or none of STBI_MALLOC, STBI_FREE, and STBI_REALLOC."
#endif

#ifndef STBI_MALLOC
#define STBI_MALLOC(sz)    malloc(sz)
#define STBI_REALLOC(p,sz) realloc(p,sz)
#define STBI_FREE(p)       free(p)
#endif

				// x86/x64 detection
#if defined(__x86_64__) || defined(_M_X64)
#define STBI__X64_TARGET
#elif defined(__i386) || defined(_M_IX86)
#define STBI__X86_TARGET
#endif

#if defined(__GNUC__) && (defined(STBI__X86_TARGET) || defined(STBI__X64_TARGET)) && !defined(__SSE2__) && !defined(STBI_NO_SIMD)
				// NOTE: not clear do we actually need this for the 64-bit path?
				// gcc doesn't support sse2 intrinsics unless you compile with -msse2,
				// (but compiling with -msse2 allows the compiler to use SSE2 everywhere;
				// this is just broken and gcc are jerks for not fixing it properly
				// http://www.virtualdub.org/blog/pivot/entry.php?id=363 )
#define STBI_NO_SIMD
#endif

#if defined(__MINGW32__) && defined(STBI__X86_TARGET) && !defined(STBI_MINGW_ENABLE_SSE2) && !defined(STBI_NO_SIMD)
				// Note that __MINGW32__ doesn't actually mean 32-bit, so we have to avoid STBI__X64_TARGET
				//
				// 32-bit MinGW wants ESP to be 16-byte aligned, but this is not in the
				// Windows ABI and VC++ as well as Windows DLLs don't maintain that invariant.
				// As a result, enabling SSE2 on 32-bit MinGW is dangerous when not
				// simultaneously enabling "-mstackrealign".
				//
				// See https://github.com/nothings/stb/issues/81 for more information.
				//
				// So default to no SSE2 on 32-bit MinGW. If you've read this far and added
				// -mstackrealign to your build settings, feel free to #define STBI_MINGW_ENABLE_SSE2.
#define STBI_NO_SIMD
#endif

#if !defined(STBI_NO_SIMD) && defined(STBI__X86_TARGET)
#define STBI_SSE2
#include <emmintrin.h>

#ifdef _MSC_VER

#if _MSC_VER >= 1400  // not VC6
#include <intrin.h> // __cpuid
				int stbi__cpuid3(void)
				{
					int info[4];
					__cpuid(info, 1);
					return info[3];
				}
#else
				int stbi__cpuid3(void)
				{
					int res;
					__asm {
						mov  eax, 1
							cpuid
							mov  res, edx
					}
					return res;
				}
#endif

#define STBI_SIMD_ALIGN(type, name) __declspec(align(16)) type name

				int stbi__sse2_available()
				{
					int info3 = stbi__cpuid3();
					return ((info3 >> 26) & 1) != 0;
				}
#else // assume GCC-style if not VC++
#define STBI_SIMD_ALIGN(type, name) type name __attribute__((aligned(16)))

				int stbi__sse2_available()
				{
#if defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 408 // GCC 4.8 or later
					// GCC 4.8+ has a nice way to do this
					return __builtin_cpu_supports("sse2");
#else
					// portable way to do this, preferably without using GCC inline ASM?
					// just bail for now.
					return 0;
#endif
				}
#endif
#endif

				// ARM NEON
#if defined(STBI_NO_SIMD) && defined(STBI_NEON)
#undef STBI_NEON
#endif

#ifdef STBI_NEON
#include <arm_neon.h>
				// assume GCC or Clang on ARM targets
#define STBI_SIMD_ALIGN(type, name) type name __attribute__((aligned(16)))
#endif

#ifndef STBI_SIMD_ALIGN
#define STBI_SIMD_ALIGN(type, name) type name
#endif

				///////////////////////////////////////////////
				//
				//  stbi__context struct and start_xxx functions

				// stbi__context structure is our basic context used by all images, so it
				// contains all the IO context, plus some basic image information
				typedef struct
				{
					stbi__uint32 img_x, img_y;
					int img_n, img_out_n;

					stbi_io_callbacks io;
					void *io_user_data;

					int read_from_callbacks;
					int buflen;
					stbi_uc buffer_start[128];

					stbi_uc *img_buffer, *img_buffer_end;
					stbi_uc *img_buffer_original, *img_buffer_original_end;
				} stbi__context;


				void stbi__refill_buffer(stbi__context *s);

				// initialize a memory-decode context
				void stbi__start_mem(stbi__context *s, stbi_uc const *buffer, int len)
				{
					s->io.read = NULL;
					s->read_from_callbacks = 0;
					s->img_buffer = s->img_buffer_original = (stbi_uc *)buffer;
					s->img_buffer_end = s->img_buffer_original_end = (stbi_uc *)buffer + len;
				}

				// initialize a callback-based context
				void stbi__start_callbacks(stbi__context *s, stbi_io_callbacks *c, void *user)
				{
					s->io = *c;
					s->io_user_data = user;
					s->buflen = sizeof(s->buffer_start);
					s->read_from_callbacks = 1;
					s->img_buffer_original = s->buffer_start;
					stbi__refill_buffer(s);
					s->img_buffer_original_end = s->img_buffer_end;
				}

#ifndef STBI_NO_STDIO

				int stbi__stdio_read(void *user, char *data, int size)
				{
					return (int)fread(data, 1, size, (FILE*)user);
				}

				void stbi__stdio_skip(void *user, int n)
				{
					fseek((FILE*)user, n, SEEK_CUR);
				}

				int stbi__stdio_eof(void *user)
				{
					return feof((FILE*)user);
				}

				stbi_io_callbacks stbi__stdio_callbacks =
				{
					stbi__stdio_read,
					stbi__stdio_skip,
					stbi__stdio_eof,
				};

				void stbi__start_file(stbi__context *s, FILE *f)
				{
					stbi__start_callbacks(s, &stbi__stdio_callbacks, (void *)f);
				}

				// void stop_file(stbi__context *s) { }

#endif // !STBI_NO_STDIO

				void stbi__rewind(stbi__context *s)
				{
					// conceptually rewind SHOULD rewind to the beginning of the stream,
					// but we just rewind to the beginning of the initial buffer, because
					// we only use it after doing 'test', which only ever looks at at most 92 bytes
					s->img_buffer = s->img_buffer_original;
					s->img_buffer_end = s->img_buffer_original_end;
				}

#ifndef STBI_NO_JPEG
				int      stbi__jpeg_test(stbi__context *s);
				stbi_uc *stbi__jpeg_load(stbi__context *s, int *x, int *y, int *comp, int req_comp);
				int      stbi__jpeg_info(stbi__context *s, int *x, int *y, int *comp);
#endif

#ifndef STBI_NO_PNG
				int      stbi__png_test(stbi__context *s);
				stbi_uc *stbi__png_load(stbi__context *s, int *x, int *y, int *comp, int req_comp);
				int      stbi__png_info(stbi__context *s, int *x, int *y, int *comp);
#endif

#ifndef STBI_NO_BMP
				int      stbi__bmp_test(stbi__context *s);
				stbi_uc *stbi__bmp_load(stbi__context *s, int *x, int *y, int *comp, int req_comp);
				int      stbi__bmp_info(stbi__context *s, int *x, int *y, int *comp);
#endif

#ifndef STBI_NO_TGA
				int      stbi__tga_test(stbi__context *s);
				stbi_uc *stbi__tga_load(stbi__context *s, int *x, int *y, int *comp, int req_comp);
				int      stbi__tga_info(stbi__context *s, int *x, int *y, int *comp);
#endif

#ifndef STBI_NO_PSD
				int      stbi__psd_test(stbi__context *s);
				stbi_uc *stbi__psd_load(stbi__context *s, int *x, int *y, int *comp, int req_comp);
				int      stbi__psd_info(stbi__context *s, int *x, int *y, int *comp);
#endif

#ifndef STBI_NO_HDR
				int      stbi__hdr_test(stbi__context *s);
				float   *stbi__hdr_load(stbi__context *s, int *x, int *y, int *comp, int req_comp);
				int      stbi__hdr_info(stbi__context *s, int *x, int *y, int *comp);
#endif

#ifndef STBI_NO_PIC
				int      stbi__pic_test(stbi__context *s);
				stbi_uc *stbi__pic_load(stbi__context *s, int *x, int *y, int *comp, int req_comp);
				int      stbi__pic_info(stbi__context *s, int *x, int *y, int *comp);
#endif

#ifndef STBI_NO_GIF
				int      stbi__gif_test(stbi__context *s);
				stbi_uc *stbi__gif_load(stbi__context *s, int *x, int *y, int *comp, int req_comp);
				int      stbi__gif_info(stbi__context *s, int *x, int *y, int *comp);
#endif

#ifndef STBI_NO_PNM
				int      stbi__pnm_test(stbi__context *s);
				stbi_uc *stbi__pnm_load(stbi__context *s, int *x, int *y, int *comp, int req_comp);
				int      stbi__pnm_info(stbi__context *s, int *x, int *y, int *comp);
#endif

				// this is not threadsafe
				const char *stbi__g_failure_reason;

				const char *stbi_failure_reason(void)
				{
					return stbi__g_failure_reason;
				}

				int stbi__err(const char *str)
				{
					stbi__g_failure_reason = str;
					return 0;
				}

				void *stbi__malloc(size_t size)
				{
					return STBI_MALLOC(size);
				}

				// stbi__err - error
				// stbi__errpf - error returning pointer to float
				// stbi__errpuc - error returning pointer to unsigned char

#ifdef STBI_NO_FAILURE_STRINGS
#define stbi__err(x,y)  0
#elif defined(STBI_FAILURE_USERMSG)
#define stbi__err(x,y)  stbi__err(y)
#else
#define stbi__err(x,y)  stbi__err(x)
#endif

#define stbi__errpf(x,y)   ((float *)(size_t) (stbi__err(x,y)?NULL:NULL))
#define stbi__errpuc(x,y)  ((unsigned char *)(size_t) (stbi__err(x,y)?NULL:NULL))

				void stbi_image_free(void *retval_from_stbi_load)
				{
					STBI_FREE(retval_from_stbi_load);
				}

#ifndef STBI_NO_LINEAR
				float   *stbi__ldr_to_hdr(stbi_uc *data, int x, int y, int comp);
#endif

#ifndef STBI_NO_HDR
				stbi_uc *stbi__hdr_to_ldr(float   *data, int x, int y, int comp);
#endif

				int stbi__vertically_flip_on_load = 0;

				void stbi_set_flip_vertically_on_load(int flag_true_if_should_flip)
				{
					stbi__vertically_flip_on_load = flag_true_if_should_flip;
				}

				unsigned char *stbi__load_main(stbi__context *s, int *x, int *y, int *comp, int req_comp)
				{
#ifndef STBI_NO_JPEG
					if (stbi__jpeg_test(s)) return stbi__jpeg_load(s, x, y, comp, req_comp);
#endif
#ifndef STBI_NO_PNG
					if (stbi__png_test(s))  return stbi__png_load(s, x, y, comp, req_comp);
#endif
#ifndef STBI_NO_BMP
					if (stbi__bmp_test(s))  return stbi__bmp_load(s, x, y, comp, req_comp);
#endif
#ifndef STBI_NO_GIF
					if (stbi__gif_test(s))  return stbi__gif_load(s, x, y, comp, req_comp);
#endif
#ifndef STBI_NO_PSD
					if (stbi__psd_test(s))  return stbi__psd_load(s, x, y, comp, req_comp);
#endif
#ifndef STBI_NO_PIC
					if (stbi__pic_test(s))  return stbi__pic_load(s, x, y, comp, req_comp);
#endif
#ifndef STBI_NO_PNM
					if (stbi__pnm_test(s))  return stbi__pnm_load(s, x, y, comp, req_comp);
#endif

#ifndef STBI_NO_HDR
					if (stbi__hdr_test(s)) {
						float *hdr = stbi__hdr_load(s, x, y, comp, req_comp);
						return stbi__hdr_to_ldr(hdr, *x, *y, req_comp ? req_comp : *comp);
					}
#endif

#ifndef STBI_NO_TGA
					// test tga last because it's a crappy test!
					if (stbi__tga_test(s))
						return stbi__tga_load(s, x, y, comp, req_comp);
#endif

					return stbi__errpuc("unknown image type", "Image not of any known type, or corrupt");
				}

				unsigned char *stbi__load_flip(stbi__context *s, int *x, int *y, int *comp, int req_comp)
				{
					unsigned char *result = stbi__load_main(s, x, y, comp, req_comp);

					if (stbi__vertically_flip_on_load && result != NULL) {
						int w = *x, h = *y;
						int depth = req_comp ? req_comp : *comp;
						int row, col, z;
						stbi_uc temp;

						// @OPTIMIZE: use a bigger temp buffer and memcpy multiple pixels at once
						for (row = 0; row < (h >> 1); row++) {
							for (col = 0; col < w; col++) {
								for (z = 0; z < depth; z++) {
									temp = result[(row * w + col) * depth + z];
									result[(row * w + col) * depth + z] = result[((h - row - 1) * w + col) * depth + z];
									result[((h - row - 1) * w + col) * depth + z] = temp;
								}
							}
						}
					}

					return result;
				}

#ifndef STBI_NO_HDR
				void stbi__float_postprocess(float *result, int *x, int *y, int *comp, int req_comp)
				{
					if (stbi__vertically_flip_on_load && result != NULL) {
						int w = *x, h = *y;
						int depth = req_comp ? req_comp : *comp;
						int row, col, z;
						float temp;

						// @OPTIMIZE: use a bigger temp buffer and memcpy multiple pixels at once
						for (row = 0; row < (h >> 1); row++) {
							for (col = 0; col < w; col++) {
								for (z = 0; z < depth; z++) {
									temp = result[(row * w + col) * depth + z];
									result[(row * w + col) * depth + z] = result[((h - row - 1) * w + col) * depth + z];
									result[((h - row - 1) * w + col) * depth + z] = temp;
								}
							}
						}
					}
				}
#endif

#ifndef STBI_NO_STDIO

				FILE *stbi__fopen(char const *filename, char const *mode)
				{
					FILE *f;
#if defined(_MSC_VER) && _MSC_VER >= 1400
					if (0 != fopen_s(&f, filename, mode))
						f = 0;
#else
					f = fopen(filename, mode);
#endif
					return f;
				}


				stbi_uc *stbi_load(char const *filename, int *x, int *y, int *comp, int req_comp)
				{
					FILE *f = stbi__fopen(filename, "rb");
					unsigned char *result;
					if (!f) return stbi__errpuc("can't fopen", "Unable to open file");
					result = stbi_load_from_file(f, x, y, comp, req_comp);
					fclose(f);
					return result;
				}

				stbi_uc *stbi_load_from_file(FILE *f, int *x, int *y, int *comp, int req_comp)
				{
					unsigned char *result;
					stbi__context s;
					stbi__start_file(&s, f);
					result = stbi__load_flip(&s, x, y, comp, req_comp);
					if (result) {
						// need to 'unget' all the characters in the IO buffer
						fseek(f, -(int)(s.img_buffer_end - s.img_buffer), SEEK_CUR);
					}
					return result;
				}
#endif //!STBI_NO_STDIO

				stbi_uc *stbi_load_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp)
				{
					stbi__context s;
					stbi__start_mem(&s, buffer, len);
					return stbi__load_flip(&s, x, y, comp, req_comp);
				}

				stbi_uc *stbi_load_from_callbacks(stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp, int req_comp)
				{
					stbi__context s;
					stbi__start_callbacks(&s, (stbi_io_callbacks *)clbk, user);
					return stbi__load_flip(&s, x, y, comp, req_comp);
				}

#ifndef STBI_NO_LINEAR
				float *stbi__loadf_main(stbi__context *s, int *x, int *y, int *comp, int req_comp)
				{
					unsigned char *data;
#ifndef STBI_NO_HDR
					if (stbi__hdr_test(s)) {
						float *hdr_data = stbi__hdr_load(s, x, y, comp, req_comp);
						if (hdr_data)
							stbi__float_postprocess(hdr_data, x, y, comp, req_comp);
						return hdr_data;
					}
#endif
					data = stbi__load_flip(s, x, y, comp, req_comp);
					if (data)
						return stbi__ldr_to_hdr(data, *x, *y, req_comp ? req_comp : *comp);
					return stbi__errpf("unknown image type", "Image not of any known type, or corrupt");
				}

				float *stbi_loadf_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp)
				{
					stbi__context s;
					stbi__start_mem(&s, buffer, len);
					return stbi__loadf_main(&s, x, y, comp, req_comp);
				}

				float *stbi_loadf_from_callbacks(stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp, int req_comp)
				{
					stbi__context s;
					stbi__start_callbacks(&s, (stbi_io_callbacks *)clbk, user);
					return stbi__loadf_main(&s, x, y, comp, req_comp);
				}

#ifndef STBI_NO_STDIO
				float *stbi_loadf(char const *filename, int *x, int *y, int *comp, int req_comp)
				{
					float *result;
					FILE *f = stbi__fopen(filename, "rb");
					if (!f) return stbi__errpf("can't fopen", "Unable to open file");
					result = stbi_loadf_from_file(f, x, y, comp, req_comp);
					fclose(f);
					return result;
				}

				float *stbi_loadf_from_file(FILE *f, int *x, int *y, int *comp, int req_comp)
				{
					stbi__context s;
					stbi__start_file(&s, f);
					return stbi__loadf_main(&s, x, y, comp, req_comp);
				}
#endif // !STBI_NO_STDIO

#endif // !STBI_NO_LINEAR

				// these is-hdr-or-not is defined independent of whether STBI_NO_LINEAR is
				// defined, for API simplicity; if STBI_NO_LINEAR is defined, it always
				// reports false!

				int stbi_is_hdr_from_memory(stbi_uc const *buffer, int len)
				{
#ifndef STBI_NO_HDR
					stbi__context s;
					stbi__start_mem(&s, buffer, len);
					return stbi__hdr_test(&s);
#else
					STBI_NOTUSED(buffer);
					STBI_NOTUSED(len);
					return 0;
#endif
				}

#ifndef STBI_NO_STDIO
				int      stbi_is_hdr(char const *filename)
				{
					FILE *f = stbi__fopen(filename, "rb");
					int result = 0;
					if (f) {
						result = stbi_is_hdr_from_file(f);
						fclose(f);
					}
					return result;
				}

				int      stbi_is_hdr_from_file(FILE *f)
				{
#ifndef STBI_NO_HDR
					stbi__context s;
					stbi__start_file(&s, f);
					return stbi__hdr_test(&s);
#else
					STBI_NOTUSED(f);
					return 0;
#endif
				}
#endif // !STBI_NO_STDIO

				int      stbi_is_hdr_from_callbacks(stbi_io_callbacks const *clbk, void *user)
				{
#ifndef STBI_NO_HDR
					stbi__context s;
					stbi__start_callbacks(&s, (stbi_io_callbacks *)clbk, user);
					return stbi__hdr_test(&s);
#else
					STBI_NOTUSED(clbk);
					STBI_NOTUSED(user);
					return 0;
#endif
				}

				float stbi__h2l_gamma_i = 1.0f / 2.2f, stbi__h2l_scale_i = 1.0f;
				float stbi__l2h_gamma = 2.2f, stbi__l2h_scale = 1.0f;

#ifndef STBI_NO_LINEAR
				void   stbi_ldr_to_hdr_gamma(float gamma) { stbi__l2h_gamma = gamma; }
				void   stbi_ldr_to_hdr_scale(float scale) { stbi__l2h_scale = scale; }
#endif

				void   stbi_hdr_to_ldr_gamma(float gamma) { stbi__h2l_gamma_i = 1 / gamma; }
				void   stbi_hdr_to_ldr_scale(float scale) { stbi__h2l_scale_i = 1 / scale; }


				//////////////////////////////////////////////////////////////////////////////
				//
				// Common code used by all image loaders
				//

				enum
				{
					STBI__SCAN_load = 0,
					STBI__SCAN_type,
					STBI__SCAN_header
				};

				void stbi__refill_buffer(stbi__context *s)
				{
					int n = (s->io.read)(s->io_user_data, (char*)s->buffer_start, s->buflen);
					if (n == 0) {
						// at end of file, treat same as if from memory, but need to handle case
						// where s->img_buffer isn't pointing to safe memory, e.g. 0-byte file
						s->read_from_callbacks = 0;
						s->img_buffer = s->buffer_start;
						s->img_buffer_end = s->buffer_start + 1;
						*s->img_buffer = 0;
					}
					else {
						s->img_buffer = s->buffer_start;
						s->img_buffer_end = s->buffer_start + n;
					}
				}

				stbi_inline  stbi_uc stbi__get8(stbi__context *s)
				{
					if (s->img_buffer < s->img_buffer_end)
						return *s->img_buffer++;
					if (s->read_from_callbacks) {
						stbi__refill_buffer(s);
						return *s->img_buffer++;
					}
					return 0;
				}

				stbi_inline  int stbi__at_eof(stbi__context *s)
				{
					if (s->io.read) {
						if (!(s->io.eof)(s->io_user_data)) return 0;
						// if feof() is true, check if buffer = end
						// special case: we've only got the special 0 character at the end
						if (s->read_from_callbacks == 0) return 1;
					}

					return s->img_buffer >= s->img_buffer_end;
				}

				void stbi__skip(stbi__context *s, int n)
				{
					if (n < 0) {
						s->img_buffer = s->img_buffer_end;
						return;
					}
					if (s->io.read) {
						int blen = (int)(s->img_buffer_end - s->img_buffer);
						if (blen < n) {
							s->img_buffer = s->img_buffer_end;
							(s->io.skip)(s->io_user_data, n - blen);
							return;
						}
					}
					s->img_buffer += n;
				}

				int stbi__getn(stbi__context *s, stbi_uc *buffer, int n)
				{
					if (s->io.read) {
						int blen = (int)(s->img_buffer_end - s->img_buffer);
						if (blen < n) {
							int res, count;

							memcpy(buffer, s->img_buffer, blen);

							count = (s->io.read)(s->io_user_data, (char*)buffer + blen, n - blen);
							res = (count == (n - blen));
							s->img_buffer = s->img_buffer_end;
							return res;
						}
					}

					if (s->img_buffer + n <= s->img_buffer_end) {
						memcpy(buffer, s->img_buffer, n);
						s->img_buffer += n;
						return 1;
					}
					else
						return 0;
				}

				int stbi__get16be(stbi__context *s)
				{
					int z = stbi__get8(s);
					return (z << 8) + stbi__get8(s);
				}

				stbi__uint32 stbi__get32be(stbi__context *s)
				{
					stbi__uint32 z = stbi__get16be(s);
					return (z << 16) + stbi__get16be(s);
				}

#if defined(STBI_NO_BMP) && defined(STBI_NO_TGA) && defined(STBI_NO_GIF)
				// nothing
#else
				int stbi__get16le(stbi__context *s)
				{
					int z = stbi__get8(s);
					return z + (stbi__get8(s) << 8);
				}
#endif

#ifndef STBI_NO_BMP
				stbi__uint32 stbi__get32le(stbi__context *s)
				{
					stbi__uint32 z = stbi__get16le(s);
					return z + (stbi__get16le(s) << 16);
				}
#endif

#define STBI__BYTECAST(x)  ((stbi_uc) ((x) & 255))  // truncate int to byte without warnings


				//////////////////////////////////////////////////////////////////////////////
				//
				//  generic converter from built-in img_n to req_comp
				//    individual types do this automatically as much as possible (e.g. jpeg
				//    does all cases internally since it needs to colorspace convert anyway,
				//    and it never has alpha, so very few cases ). png can automatically
				//    interleave an alpha=255 channel, but falls back to this for other cases
				//
				//  assume data buffer is malloced, so malloc a new one and free that one
				//  only failure mode is malloc failing

				stbi_uc stbi__compute_y(int r, int g, int b)
				{
					return (stbi_uc)(((r * 77) + (g * 150) + (29 * b)) >> 8);
				}

				unsigned char *stbi__convert_format(unsigned char *data, int img_n, int req_comp, unsigned int x, unsigned int y)
				{
					int i, j;
					unsigned char *good;

					if (req_comp == img_n) return data;
					STBI_ASSERT(req_comp >= 1 && req_comp <= 4);

					good = (unsigned char *)stbi__malloc(req_comp * x * y);
					if (good == NULL) {
						STBI_FREE(data);
						return stbi__errpuc("outofmem", "Out of memory");
					}

					for (j = 0; j < (int)y; ++j) {
						unsigned char *src = data + j * x * img_n;
						unsigned char *dest = good + j * x * req_comp;

#define COMBO(a,b)  ((a)*8+(b))
#define CASE(a,b)   case COMBO(a,b): for(i=x-1; i >= 0; --i, src += a, dest += b)
						// convert source image with img_n components to one with req_comp components;
						// avoid switch per pixel, so use switch per scanline and massive macros
						switch (COMBO(img_n, req_comp)) {
							CASE(1, 2) dest[0] = src[0], dest[1] = 255; break;
							CASE(1, 3) dest[0] = dest[1] = dest[2] = src[0]; break;
							CASE(1, 4) dest[0] = dest[1] = dest[2] = src[0], dest[3] = 255; break;
							CASE(2, 1) dest[0] = src[0]; break;
							CASE(2, 3) dest[0] = dest[1] = dest[2] = src[0]; break;
							CASE(2, 4) dest[0] = dest[1] = dest[2] = src[0], dest[3] = src[1]; break;
							CASE(3, 4) dest[0] = src[0], dest[1] = src[1], dest[2] = src[2], dest[3] = 255; break;
							CASE(3, 1) dest[0] = stbi__compute_y(src[0], src[1], src[2]); break;
							CASE(3, 2) dest[0] = stbi__compute_y(src[0], src[1], src[2]), dest[1] = 255; break;
							CASE(4, 1) dest[0] = stbi__compute_y(src[0], src[1], src[2]); break;
							CASE(4, 2) dest[0] = stbi__compute_y(src[0], src[1], src[2]), dest[1] = src[3]; break;
							CASE(4, 3) dest[0] = src[0], dest[1] = src[1], dest[2] = src[2]; break;
						default: STBI_ASSERT(0);
						}
#undef CASE
					}

					STBI_FREE(data);
					return good;
				}

#ifndef STBI_NO_LINEAR
				float   *stbi__ldr_to_hdr(stbi_uc *data, int x, int y, int comp)
				{
					int i, k, n;
					float *output = (float *)stbi__malloc(x * y * comp * sizeof(float));
					if (output == NULL) { STBI_FREE(data); return stbi__errpf("outofmem", "Out of memory"); }
					// compute number of non-alpha components
					if (comp & 1) n = comp; else n = comp - 1;
					for (i = 0; i < x*y; ++i) {
						for (k = 0; k < n; ++k) {
							output[i*comp + k] = (float)(pow(data[i*comp + k] / 255.0f, stbi__l2h_gamma) * stbi__l2h_scale);
						}
						if (k < comp) output[i*comp + k] = data[i*comp + k] / 255.0f;
					}
					STBI_FREE(data);
					return output;
				}
#endif

#ifndef STBI_NO_HDR
#define stbi__float2int(x)   ((int) (x))
				stbi_uc *stbi__hdr_to_ldr(float   *data, int x, int y, int comp)
				{
					int i, k, n;
					stbi_uc *output = (stbi_uc *)stbi__malloc(x * y * comp);
					if (output == NULL) { STBI_FREE(data); return stbi__errpuc("outofmem", "Out of memory"); }
					// compute number of non-alpha components
					if (comp & 1) n = comp; else n = comp - 1;
					for (i = 0; i < x*y; ++i) {
						for (k = 0; k < n; ++k) {
							float z = (float)pow(data[i*comp + k] * stbi__h2l_scale_i, stbi__h2l_gamma_i) * 255 + 0.5f;
							if (z < 0) z = 0;
							if (z > 255) z = 255;
							output[i*comp + k] = (stbi_uc)stbi__float2int(z);
						}
						if (k < comp) {
							float z = data[i*comp + k] * 255 + 0.5f;
							if (z < 0) z = 0;
							if (z > 255) z = 255;
							output[i*comp + k] = (stbi_uc)stbi__float2int(z);
						}
					}
					STBI_FREE(data);
					return output;
				}
#endif

				//////////////////////////////////////////////////////////////////////////////
				//
				//  "baseline" JPEG/JFIF decoder
				//
				//    simple implementation
				//      - doesn't support delayed output of y-dimension
				//      - simple interface (only one output format: 8-bit interleaved RGB)
				//      - doesn't try to recover corrupt jpegs
				//      - doesn't allow partial loading, loading multiple at once
				//      - still fast on x86 (copying globals into locals doesn't help x86)
				//      - allocates lots of intermediate memory (full size of all components)
				//        - non-interleaved case requires this anyway
				//        - allows good upsampling (see next)
				//    high-quality
				//      - upsampled channels are bilinearly interpolated, even across blocks
				//      - quality integer IDCT derived from IJG's 'slow'
				//    performance
				//      - fast huffman; reasonable integer IDCT
				//      - some SIMD kernels for common paths on targets with SSE2/NEON
				//      - uses a lot of intermediate memory, could cache poorly

#ifndef STBI_NO_JPEG

				// huffman decoding acceleration
#define FAST_BITS   9  // larger handles more cases; smaller stomps less cache

				typedef struct
				{
					stbi_uc  fast[1 << FAST_BITS];
					// weirdly, repacking this into AoS is a 10% speed loss, instead of a win
					stbi__uint16 code[256];
					stbi_uc  values[256];
					stbi_uc  size[257];
					unsigned int maxcode[18];
					int    delta[17];   // old 'firstsymbol' - old 'firstcode'
				} stbi__huffman;

				typedef struct
				{
					stbi__context *s;
					stbi__huffman huff_dc[4];
					stbi__huffman huff_ac[4];
					stbi_uc dequant[4][64];
					stbi__int16 fast_ac[4][1 << FAST_BITS];

					// sizes for components, interleaved MCUs
					int img_h_max, img_v_max;
					int img_mcu_x, img_mcu_y;
					int img_mcu_w, img_mcu_h;

					// definition of jpeg image component
					struct
					{
						int id;
						int h, v;
						int tq;
						int hd, ha;
						int dc_pred;

						int x, y, w2, h2;
						stbi_uc *data;
						void *raw_data, *raw_coeff;
						stbi_uc *linebuf;
						short   *coeff;   // progressive only
						int      coeff_w, coeff_h; // number of 8x8 coefficient blocks
					} img_comp[4];

					stbi__uint32   code_buffer; // jpeg entropy-coded buffer
					int            code_bits;   // number of valid bits
					unsigned char  marker;      // marker seen while filling entropy buffer
					int            nomore;      // flag if we saw a marker so must stop

					int            progressive;
					int            spec_start;
					int            spec_end;
					int            succ_high;
					int            succ_low;
					int            eob_run;

					int scan_n, order[4];
					int restart_interval, todo;

					// kernels
					void(*idct_block_kernel)(stbi_uc *out, int out_stride, short data[64]);
					void(*YCbCr_to_RGB_kernel)(stbi_uc *out, const stbi_uc *y, const stbi_uc *pcb, const stbi_uc *pcr, int count, int step);
					stbi_uc *(*resample_row_hv_2_kernel)(stbi_uc *out, stbi_uc *in_near, stbi_uc *in_far, int w, int hs);
				} stbi__jpeg;

				int stbi__build_huffman(stbi__huffman *h, int *count)
				{
					int i, j, k = 0, code;
					// build size list for each symbol (from JPEG spec)
					for (i = 0; i < 16; ++i)
					for (j = 0; j < count[i]; ++j)
						h->size[k++] = (stbi_uc)(i + 1);
					h->size[k] = 0;

					// compute actual symbols (from jpeg spec)
					code = 0;
					k = 0;
					for (j = 1; j <= 16; ++j) {
						// compute delta to add to code to compute symbol id
						h->delta[j] = k - code;
						if (h->size[k] == j) {
							while (h->size[k] == j)
								h->code[k++] = (stbi__uint16)(code++);
							if (code - 1 >= (1 << j)) return stbi__err("bad code lengths", "Corrupt JPEG");
						}
						// compute largest code + 1 for this size, preshifted as needed later
						h->maxcode[j] = code << (16 - j);
						code <<= 1;
					}
					h->maxcode[j] = 0xffffffff;

					// build non-spec acceleration table; 255 is flag for not-accelerated
					memset(h->fast, 255, 1 << FAST_BITS);
					for (i = 0; i < k; ++i) {
						int s = h->size[i];
						if (s <= FAST_BITS) {
							int c = h->code[i] << (FAST_BITS - s);
							int m = 1 << (FAST_BITS - s);
							for (j = 0; j < m; ++j) {
								h->fast[c + j] = (stbi_uc)i;
							}
						}
					}
					return 1;
				}

				// build a table that decodes both magnitude and value of small ACs in
				// one go.
				void stbi__build_fast_ac(stbi__int16 *fast_ac, stbi__huffman *h)
				{
					int i;
					for (i = 0; i < (1 << FAST_BITS); ++i) {
						stbi_uc fast = h->fast[i];
						fast_ac[i] = 0;
						if (fast < 255) {
							int rs = h->values[fast];
							int run = (rs >> 4) & 15;
							int magbits = rs & 15;
							int len = h->size[fast];

							if (magbits && len + magbits <= FAST_BITS) {
								// magnitude code followed by receive_extend code
								int k = ((i << len) & ((1 << FAST_BITS) - 1)) >> (FAST_BITS - magbits);
								int m = 1 << (magbits - 1);
								if (k < m) k += (-1 << magbits) + 1;
								// if the result is small enough, we can fit it in fast_ac table
								if (k >= -128 && k <= 127)
									fast_ac[i] = (stbi__int16)((k << 8) + (run << 4) + (len + magbits));
							}
						}
					}
				}

				void stbi__grow_buffer_unsafe(stbi__jpeg *j)
				{
					do {
						int b = j->nomore ? 0 : stbi__get8(j->s);
						if (b == 0xff) {
							int c = stbi__get8(j->s);
							if (c != 0) {
								j->marker = (unsigned char)c;
								j->nomore = 1;
								return;
							}
						}
						j->code_buffer |= b << (24 - j->code_bits);
						j->code_bits += 8;
					} while (j->code_bits <= 24);
				}

				// (1 << n) - 1
				stbi__uint32 stbi__bmask[17] = { 0, 1, 3, 7, 15, 31, 63, 127, 255, 511, 1023, 2047, 4095, 8191, 16383, 32767, 65535 };

				// decode a jpeg huffman value from the bitstream
				stbi_inline  int stbi__jpeg_huff_decode(stbi__jpeg *j, stbi__huffman *h)
				{
					unsigned int temp;
					int c, k;

					if (j->code_bits < 16) stbi__grow_buffer_unsafe(j);

					// look at the top FAST_BITS and determine what symbol ID it is,
					// if the code is <= FAST_BITS
					c = (j->code_buffer >> (32 - FAST_BITS)) & ((1 << FAST_BITS) - 1);
					k = h->fast[c];
					if (k < 255) {
						int s = h->size[k];
						if (s > j->code_bits)
							return -1;
						j->code_buffer <<= s;
						j->code_bits -= s;
						return h->values[k];
					}

					// naive test is to shift the code_buffer down so k bits are
					// valid, then test against maxcode. To speed this up, we've
					// preshifted maxcode left so that it has (16-k) 0s at the
					// end; in other words, regardless of the number of bits, it
					// wants to be compared against something shifted to have 16;
					// that way we don't need to shift inside the loop.
					temp = j->code_buffer >> 16;
					for (k = FAST_BITS + 1;; ++k)
					if (temp < h->maxcode[k])
						break;
					if (k == 17) {
						// error! code not found
						j->code_bits -= 16;
						return -1;
					}

					if (k > j->code_bits)
						return -1;

					// convert the huffman code to the symbol id
					c = ((j->code_buffer >> (32 - k)) & stbi__bmask[k]) + h->delta[k];
					STBI_ASSERT((((j->code_buffer) >> (32 - h->size[c])) & stbi__bmask[h->size[c]]) == h->code[c]);

					// convert the id to a symbol
					j->code_bits -= k;
					j->code_buffer <<= k;
					return h->values[c];
				}

				// bias[n] = (-1<<n) + 1
				int const stbi__jbias[16] = { 0, -1, -3, -7, -15, -31, -63, -127, -255, -511, -1023, -2047, -4095, -8191, -16383, -32767 };

				// combined JPEG 'receive' and JPEG 'extend', since baseline
				// always extends everything it receives.
				stbi_inline  int stbi__extend_receive(stbi__jpeg *j, int n)
				{
					unsigned int k;
					int sgn;
					if (j->code_bits < n) stbi__grow_buffer_unsafe(j);

					sgn = (stbi__int32)j->code_buffer >> 31; // sign bit is always in MSB
					k = stbi_lrot(j->code_buffer, n);
					STBI_ASSERT(n >= 0 && n < (int)(sizeof(stbi__bmask) / sizeof(*stbi__bmask)));
					j->code_buffer = k & ~stbi__bmask[n];
					k &= stbi__bmask[n];
					j->code_bits -= n;
					return k + (stbi__jbias[n] & ~sgn);
				}

				// get some unsigned bits
				stbi_inline  int stbi__jpeg_get_bits(stbi__jpeg *j, int n)
				{
					unsigned int k;
					if (j->code_bits < n) stbi__grow_buffer_unsafe(j);
					k = stbi_lrot(j->code_buffer, n);
					j->code_buffer = k & ~stbi__bmask[n];
					k &= stbi__bmask[n];
					j->code_bits -= n;
					return k;
				}

				stbi_inline  int stbi__jpeg_get_bit(stbi__jpeg *j)
				{
					unsigned int k;
					if (j->code_bits < 1) stbi__grow_buffer_unsafe(j);
					k = j->code_buffer;
					j->code_buffer <<= 1;
					--j->code_bits;
					return k & 0x80000000;
				}

				// given a value that's at position X in the zigzag stream,
				// where does it appear in the 8x8 matrix coded as row-major?
				stbi_uc stbi__jpeg_dezigzag[64 + 15] =
				{
					0, 1, 8, 16, 9, 2, 3, 10,
					17, 24, 32, 25, 18, 11, 4, 5,
					12, 19, 26, 33, 40, 48, 41, 34,
					27, 20, 13, 6, 7, 14, 21, 28,
					35, 42, 49, 56, 57, 50, 43, 36,
					29, 22, 15, 23, 30, 37, 44, 51,
					58, 59, 52, 45, 38, 31, 39, 46,
					53, 60, 61, 54, 47, 55, 62, 63,
					// let corrupt input sample past end
					63, 63, 63, 63, 63, 63, 63, 63,
					63, 63, 63, 63, 63, 63, 63
				};

				// decode one 64-entry block--
				int stbi__jpeg_decode_block(stbi__jpeg *j, short data[64], stbi__huffman *hdc, stbi__huffman *hac, stbi__int16 *fac, int b, stbi_uc *dequant)
				{
					int diff, dc, k;
					int t;

					if (j->code_bits < 16) stbi__grow_buffer_unsafe(j);
					t = stbi__jpeg_huff_decode(j, hdc);
					if (t < 0) return stbi__err("bad huffman code", "Corrupt JPEG");

					// 0 all the ac values now so we can do it 32-bits at a time
					memset(data, 0, 64 * sizeof(data[0]));

					diff = t ? stbi__extend_receive(j, t) : 0;
					dc = j->img_comp[b].dc_pred + diff;
					j->img_comp[b].dc_pred = dc;
					data[0] = (short)(dc * dequant[0]);

					// decode AC components, see JPEG spec
					k = 1;
					do {
						unsigned int zig;
						int c, r, s;
						if (j->code_bits < 16) stbi__grow_buffer_unsafe(j);
						c = (j->code_buffer >> (32 - FAST_BITS)) & ((1 << FAST_BITS) - 1);
						r = fac[c];
						if (r) { // fast-AC path
							k += (r >> 4) & 15; // run
							s = r & 15; // combined length
							j->code_buffer <<= s;
							j->code_bits -= s;
							// decode into unzigzag'd location
							zig = stbi__jpeg_dezigzag[k++];
							data[zig] = (short)((r >> 8) * dequant[zig]);
						}
						else {
							int rs = stbi__jpeg_huff_decode(j, hac);
							if (rs < 0) return stbi__err("bad huffman code", "Corrupt JPEG");
							s = rs & 15;
							r = rs >> 4;
							if (s == 0) {
								if (rs != 0xf0) break; // end block
								k += 16;
							}
							else {
								k += r;
								// decode into unzigzag'd location
								zig = stbi__jpeg_dezigzag[k++];
								data[zig] = (short)(stbi__extend_receive(j, s) * dequant[zig]);
							}
						}
					} while (k < 64);
					return 1;
				}

				int stbi__jpeg_decode_block_prog_dc(stbi__jpeg *j, short data[64], stbi__huffman *hdc, int b)
				{
					int diff, dc;
					int t;
					if (j->spec_end != 0) return stbi__err("can't merge dc and ac", "Corrupt JPEG");

					if (j->code_bits < 16) stbi__grow_buffer_unsafe(j);

					if (j->succ_high == 0) {
						// first scan for DC coefficient, must be first
						memset(data, 0, 64 * sizeof(data[0])); // 0 all the ac values now
						t = stbi__jpeg_huff_decode(j, hdc);
						diff = t ? stbi__extend_receive(j, t) : 0;

						dc = j->img_comp[b].dc_pred + diff;
						j->img_comp[b].dc_pred = dc;
						data[0] = (short)(dc << j->succ_low);
					}
					else {
						// refinement scan for DC coefficient
						if (stbi__jpeg_get_bit(j))
							data[0] += (short)(1 << j->succ_low);
					}
					return 1;
				}

				// @OPTIMIZE: store non-zigzagged during the decode passes,
				// and only de-zigzag when dequantizing
				int stbi__jpeg_decode_block_prog_ac(stbi__jpeg *j, short data[64], stbi__huffman *hac, stbi__int16 *fac)
				{
					int k;
					if (j->spec_start == 0) return stbi__err("can't merge dc and ac", "Corrupt JPEG");

					if (j->succ_high == 0) {
						int shift = j->succ_low;

						if (j->eob_run) {
							--j->eob_run;
							return 1;
						}

						k = j->spec_start;
						do {
							unsigned int zig;
							int c, r, s;
							if (j->code_bits < 16) stbi__grow_buffer_unsafe(j);
							c = (j->code_buffer >> (32 - FAST_BITS)) & ((1 << FAST_BITS) - 1);
							r = fac[c];
							if (r) { // fast-AC path
								k += (r >> 4) & 15; // run
								s = r & 15; // combined length
								j->code_buffer <<= s;
								j->code_bits -= s;
								zig = stbi__jpeg_dezigzag[k++];
								data[zig] = (short)((r >> 8) << shift);
							}
							else {
								int rs = stbi__jpeg_huff_decode(j, hac);
								if (rs < 0) return stbi__err("bad huffman code", "Corrupt JPEG");
								s = rs & 15;
								r = rs >> 4;
								if (s == 0) {
									if (r < 15) {
										j->eob_run = (1 << r);
										if (r)
											j->eob_run += stbi__jpeg_get_bits(j, r);
										--j->eob_run;
										break;
									}
									k += 16;
								}
								else {
									k += r;
									zig = stbi__jpeg_dezigzag[k++];
									data[zig] = (short)(stbi__extend_receive(j, s) << shift);
								}
							}
						} while (k <= j->spec_end);
					}
					else {
						// refinement scan for these AC coefficients

						short bit = (short)(1 << j->succ_low);

						if (j->eob_run) {
							--j->eob_run;
							for (k = j->spec_start; k <= j->spec_end; ++k) {
								short *p = &data[stbi__jpeg_dezigzag[k]];
								if (*p != 0)
								if (stbi__jpeg_get_bit(j))
								if ((*p & bit) == 0) {
									if (*p > 0)
										*p += bit;
									else
										*p -= bit;
								}
							}
						}
						else {
							k = j->spec_start;
							do {
								int r, s;
								int rs = stbi__jpeg_huff_decode(j, hac); // @OPTIMIZE see if we can use the fast path here, advance-by-r is so slow, eh
								if (rs < 0) return stbi__err("bad huffman code", "Corrupt JPEG");
								s = rs & 15;
								r = rs >> 4;
								if (s == 0) {
									if (r < 15) {
										j->eob_run = (1 << r) - 1;
										if (r)
											j->eob_run += stbi__jpeg_get_bits(j, r);
										r = 64; // force end of block
									}
									else {
										// r=15 s=0 should write 16 0s, so we just do
										// a run of 15 0s and then write s (which is 0),
										// so we don't have to do anything special here
									}
								}
								else {
									if (s != 1) return stbi__err("bad huffman code", "Corrupt JPEG");
									// sign bit
									if (stbi__jpeg_get_bit(j))
										s = bit;
									else
										s = -bit;
								}

								// advance by r
								while (k <= j->spec_end) {
									short *p = &data[stbi__jpeg_dezigzag[k++]];
									if (*p != 0) {
										if (stbi__jpeg_get_bit(j))
										if ((*p & bit) == 0) {
											if (*p > 0)
												*p += bit;
											else
												*p -= bit;
										}
									}
									else {
										if (r == 0) {
											*p = (short)s;
											break;
										}
										--r;
									}
								}
							} while (k <= j->spec_end);
						}
					}
					return 1;
				}

				// take a -128..127 value and stbi__clamp it and convert to 0..255
				stbi_inline  stbi_uc stbi__clamp(int x)
				{
					// trick to use a single test to catch both cases
					if ((unsigned int)x > 255) {
						if (x < 0) return 0;
						if (x > 255) return 255;
					}
					return (stbi_uc)x;
				}

#define stbi__f2f(x)  ((int) (((x) * 4096 + 0.5)))
#define stbi__fsh(x)  ((x) << 12)

				// derived from jidctint -- DCT_ISLOW
#define STBI__IDCT_1D(s0,s1,s2,s3,s4,s5,s6,s7) \
	int t0, t1, t2, t3, p1, p2, p3, p4, p5, x0, x1, x2, x3; \
	p2 = s2;                                    \
	p3 = s6;                                    \
	p1 = (p2 + p3) * stbi__f2f(0.5411961f);       \
	t2 = p1 + p3*stbi__f2f(-1.847759065f);      \
	t3 = p1 + p2*stbi__f2f(0.765366865f);      \
	p2 = s0;                                    \
	p3 = s4;                                    \
	t0 = stbi__fsh(p2 + p3);                      \
	t1 = stbi__fsh(p2 - p3);                      \
	x0 = t0 + t3;                                 \
	x3 = t0 - t3;                                 \
	x1 = t1 + t2;                                 \
	x2 = t1 - t2;                                 \
	t0 = s7;                                    \
	t1 = s5;                                    \
	t2 = s3;                                    \
	t3 = s1;                                    \
	p3 = t0 + t2;                                 \
	p4 = t1 + t3;                                 \
	p1 = t0 + t3;                                 \
	p2 = t1 + t2;                                 \
	p5 = (p3 + p4)*stbi__f2f(1.175875602f);      \
	t0 = t0*stbi__f2f(0.298631336f);           \
	t1 = t1*stbi__f2f(2.053119869f);           \
	t2 = t2*stbi__f2f(3.072711026f);           \
	t3 = t3*stbi__f2f(1.501321110f);           \
	p1 = p5 + p1*stbi__f2f(-0.899976223f);      \
	p2 = p5 + p2*stbi__f2f(-2.562915447f);      \
	p3 = p3*stbi__f2f(-1.961570560f);           \
	p4 = p4*stbi__f2f(-0.390180644f);           \
	t3 += p1 + p4;                                \
	t2 += p2 + p3;                                \
	t1 += p2 + p4;                                \
	t0 += p1 + p3;

				void stbi__idct_block(stbi_uc *out, int out_stride, short data[64])
				{
					int i, val[64], *v = val;
					stbi_uc *o;
					short *d = data;

					// columns
					for (i = 0; i < 8; ++i, ++d, ++v) {
						// if all zeroes, shortcut -- this avoids dequantizing 0s and IDCTing
						if (d[8] == 0 && d[16] == 0 && d[24] == 0 && d[32] == 0
							&& d[40] == 0 && d[48] == 0 && d[56] == 0) {
							//    no shortcut                 0     seconds
							//    (1|2|3|4|5|6|7)==0          0     seconds
							//    all separate               -0.047 seconds
							//    1 && 2|3 && 4|5 && 6|7:    -0.047 seconds
							int dcterm = d[0] << 2;
							v[0] = v[8] = v[16] = v[24] = v[32] = v[40] = v[48] = v[56] = dcterm;
						}
						else {
							STBI__IDCT_1D(d[0], d[8], d[16], d[24], d[32], d[40], d[48], d[56])
								// constants scaled things up by 1<<12; let's bring them back
								// down, but keep 2 extra bits of precision
								x0 += 512; x1 += 512; x2 += 512; x3 += 512;
							v[0] = (x0 + t3) >> 10;
							v[56] = (x0 - t3) >> 10;
							v[8] = (x1 + t2) >> 10;
							v[48] = (x1 - t2) >> 10;
							v[16] = (x2 + t1) >> 10;
							v[40] = (x2 - t1) >> 10;
							v[24] = (x3 + t0) >> 10;
							v[32] = (x3 - t0) >> 10;
						}
					}

					for (i = 0, v = val, o = out; i < 8; ++i, v += 8, o += out_stride) {
						// no fast case since the first 1D IDCT spread components out
						STBI__IDCT_1D(v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7])
							// constants scaled things up by 1<<12, plus we had 1<<2 from first
							// loop, plus horizontal and vertical each scale by sqrt(8) so together
							// we've got an extra 1<<3, so 1<<17 total we need to remove.
							// so we want to round that, which means adding 0.5 * 1<<17,
							// aka 65536. Also, we'll end up with -128 to 127 that we want
							// to encode as 0..255 by adding 128, so we'll add that before the shift
							x0 += 65536 + (128 << 17);
						x1 += 65536 + (128 << 17);
						x2 += 65536 + (128 << 17);
						x3 += 65536 + (128 << 17);
						// tried computing the shifts into temps, or'ing the temps to see
						// if any were out of range, but that was slower
						o[0] = stbi__clamp((x0 + t3) >> 17);
						o[7] = stbi__clamp((x0 - t3) >> 17);
						o[1] = stbi__clamp((x1 + t2) >> 17);
						o[6] = stbi__clamp((x1 - t2) >> 17);
						o[2] = stbi__clamp((x2 + t1) >> 17);
						o[5] = stbi__clamp((x2 - t1) >> 17);
						o[3] = stbi__clamp((x3 + t0) >> 17);
						o[4] = stbi__clamp((x3 - t0) >> 17);
					}
				}

#ifdef STBI_SSE2
				// sse2 integer IDCT. not the fastest possible implementation but it
				// produces bit-identical results to the generic C version so it's
				// fully "transparent".
				void stbi__idct_simd(stbi_uc *out, int out_stride, short data[64])
				{
					// This is constructed to match our regular (generic) integer IDCT exactly.
					__m128i row0, row1, row2, row3, row4, row5, row6, row7;
					__m128i tmp;

					// dot product constant: even elems=x, odd elems=y
#define dct_const(x,y)  _mm_setr_epi16((x),(y),(x),(y),(x),(y),(x),(y))

					// out(0) = c0[even]*x + c0[odd]*y   (c0, x, y 16-bit, out 32-bit)
					// out(1) = c1[even]*x + c1[odd]*y
#define dct_rot(out0,out1, x,y,c0,c1) \
	__m128i c0##lo = _mm_unpacklo_epi16((x), (y)); \
	__m128i c0##hi = _mm_unpackhi_epi16((x), (y)); \
	__m128i out0##_l = _mm_madd_epi16(c0##lo, c0); \
	__m128i out0##_h = _mm_madd_epi16(c0##hi, c0); \
	__m128i out1##_l = _mm_madd_epi16(c0##lo, c1); \
	__m128i out1##_h = _mm_madd_epi16(c0##hi, c1)

					// out = in << 12  (in 16-bit, out 32-bit)
#define dct_widen(out, in) \
	__m128i out##_l = _mm_srai_epi32(_mm_unpacklo_epi16(_mm_setzero_si128(), (in)), 4); \
	__m128i out##_h = _mm_srai_epi32(_mm_unpackhi_epi16(_mm_setzero_si128(), (in)), 4)

					// wide add
#define dct_wadd(out, a, b) \
	__m128i out##_l = _mm_add_epi32(a##_l, b##_l); \
	__m128i out##_h = _mm_add_epi32(a##_h, b##_h)

					// wide sub
#define dct_wsub(out, a, b) \
	__m128i out##_l = _mm_sub_epi32(a##_l, b##_l); \
	__m128i out##_h = _mm_sub_epi32(a##_h, b##_h)

					// butterfly a/b, add bias, then shift by "s" and pack
#define dct_bfly32o(out0, out1, a,b,bias,s) \
					{
					\
						__m128i abiased_l = _mm_add_epi32(a##_l, bias); \
						__m128i abiased_h = _mm_add_epi32(a##_h, bias); \
						dct_wadd(sum, abiased, b); \
						dct_wsub(dif, abiased, b); \
						out0 = _mm_packs_epi32(_mm_srai_epi32(sum_l, s), _mm_srai_epi32(sum_h, s)); \
						out1 = _mm_packs_epi32(_mm_srai_epi32(dif_l, s), _mm_srai_epi32(dif_h, s)); \
	}

						// 8-bit interleave step (for transposes)
#define dct_interleave8(a, b) \
	tmp = a; \
	a = _mm_unpacklo_epi8(a, b); \
	b = _mm_unpackhi_epi8(tmp, b)

						// 16-bit interleave step (for transposes)
#define dct_interleave16(a, b) \
	tmp = a; \
	a = _mm_unpacklo_epi16(a, b); \
	b = _mm_unpackhi_epi16(tmp, b)

#define dct_pass(bias,shift) \
					{
						\
						/* even part */ \
						dct_rot(t2e, t3e, row2, row6, rot0_0, rot0_1); \
						__m128i sum04 = _mm_add_epi16(row0, row4); \
						__m128i dif04 = _mm_sub_epi16(row0, row4); \
						dct_widen(t0e, sum04); \
						dct_widen(t1e, dif04); \
						dct_wadd(x0, t0e, t3e); \
						dct_wsub(x3, t0e, t3e); \
						dct_wadd(x1, t1e, t2e); \
						dct_wsub(x2, t1e, t2e); \
						/* odd part */ \
						dct_rot(y0o, y2o, row7, row3, rot2_0, rot2_1); \
						dct_rot(y1o, y3o, row5, row1, rot3_0, rot3_1); \
						__m128i sum17 = _mm_add_epi16(row1, row7); \
						__m128i sum35 = _mm_add_epi16(row3, row5); \
						dct_rot(y4o, y5o, sum17, sum35, rot1_0, rot1_1); \
						dct_wadd(x4, y0o, y4o); \
						dct_wadd(x5, y1o, y5o); \
						dct_wadd(x6, y2o, y5o); \
						dct_wadd(x7, y3o, y4o); \
						dct_bfly32o(row0, row7, x0, x7, bias, shift); \
						dct_bfly32o(row1, row6, x1, x6, bias, shift); \
						dct_bfly32o(row2, row5, x2, x5, bias, shift); \
						dct_bfly32o(row3, row4, x3, x4, bias, shift); \
	}

					__m128i rot0_0 = dct_const(stbi__f2f(0.5411961f), stbi__f2f(0.5411961f) + stbi__f2f(-1.847759065f));
					__m128i rot0_1 = dct_const(stbi__f2f(0.5411961f) + stbi__f2f(0.765366865f), stbi__f2f(0.5411961f));
					__m128i rot1_0 = dct_const(stbi__f2f(1.175875602f) + stbi__f2f(-0.899976223f), stbi__f2f(1.175875602f));
					__m128i rot1_1 = dct_const(stbi__f2f(1.175875602f), stbi__f2f(1.175875602f) + stbi__f2f(-2.562915447f));
					__m128i rot2_0 = dct_const(stbi__f2f(-1.961570560f) + stbi__f2f(0.298631336f), stbi__f2f(-1.961570560f));
					__m128i rot2_1 = dct_const(stbi__f2f(-1.961570560f), stbi__f2f(-1.961570560f) + stbi__f2f(3.072711026f));
					__m128i rot3_0 = dct_const(stbi__f2f(-0.390180644f) + stbi__f2f(2.053119869f), stbi__f2f(-0.390180644f));
					__m128i rot3_1 = dct_const(stbi__f2f(-0.390180644f), stbi__f2f(-0.390180644f) + stbi__f2f(1.501321110f));

					// rounding biases in column/row passes, see stbi__idct_block for explanation.
					__m128i bias_0 = _mm_set1_epi32(512);
					__m128i bias_1 = _mm_set1_epi32(65536 + (128 << 17));

					// load
					row0 = _mm_load_si128((const __m128i *) (data + 0 * 8));
					row1 = _mm_load_si128((const __m128i *) (data + 1 * 8));
					row2 = _mm_load_si128((const __m128i *) (data + 2 * 8));
					row3 = _mm_load_si128((const __m128i *) (data + 3 * 8));
					row4 = _mm_load_si128((const __m128i *) (data + 4 * 8));
					row5 = _mm_load_si128((const __m128i *) (data + 5 * 8));
					row6 = _mm_load_si128((const __m128i *) (data + 6 * 8));
					row7 = _mm_load_si128((const __m128i *) (data + 7 * 8));

					// column pass
					dct_pass(bias_0, 10);

					{
						// 16bit 8x8 transpose pass 1
						dct_interleave16(row0, row4);
						dct_interleave16(row1, row5);
						dct_interleave16(row2, row6);
						dct_interleave16(row3, row7);

						// transpose pass 2
						dct_interleave16(row0, row2);
						dct_interleave16(row1, row3);
						dct_interleave16(row4, row6);
						dct_interleave16(row5, row7);

						// transpose pass 3
						dct_interleave16(row0, row1);
						dct_interleave16(row2, row3);
						dct_interleave16(row4, row5);
						dct_interleave16(row6, row7);
					}

					// row pass
					dct_pass(bias_1, 17);

					{
						// pack
						__m128i p0 = _mm_packus_epi16(row0, row1); // a0a1a2a3...a7b0b1b2b3...b7
						__m128i p1 = _mm_packus_epi16(row2, row3);
						__m128i p2 = _mm_packus_epi16(row4, row5);
						__m128i p3 = _mm_packus_epi16(row6, row7);

						// 8bit 8x8 transpose pass 1
						dct_interleave8(p0, p2); // a0e0a1e1...
						dct_interleave8(p1, p3); // c0g0c1g1...

						// transpose pass 2
						dct_interleave8(p0, p1); // a0c0e0g0...
						dct_interleave8(p2, p3); // b0d0f0h0...

						// transpose pass 3
						dct_interleave8(p0, p2); // a0b0c0d0...
						dct_interleave8(p1, p3); // a4b4c4d4...

						// store
						_mm_storel_epi64((__m128i *) out, p0); out += out_stride;
						_mm_storel_epi64((__m128i *) out, _mm_shuffle_epi32(p0, 0x4e)); out += out_stride;
						_mm_storel_epi64((__m128i *) out, p2); out += out_stride;
						_mm_storel_epi64((__m128i *) out, _mm_shuffle_epi32(p2, 0x4e)); out += out_stride;
						_mm_storel_epi64((__m128i *) out, p1); out += out_stride;
						_mm_storel_epi64((__m128i *) out, _mm_shuffle_epi32(p1, 0x4e)); out += out_stride;
						_mm_storel_epi64((__m128i *) out, p3); out += out_stride;
						_mm_storel_epi64((__m128i *) out, _mm_shuffle_epi32(p3, 0x4e));
					}

#undef dct_const
#undef dct_rot
#undef dct_widen
#undef dct_wadd
#undef dct_wsub
#undef dct_bfly32o
#undef dct_interleave8
#undef dct_interleave16
#undef dct_pass
				}

#endif // STBI_SSE2

#ifdef STBI_NEON

				// NEON integer IDCT. should produce bit-identical
				// results to the generic C version.
				void stbi__idct_simd(stbi_uc *out, int out_stride, short data[64])
				{
					int16x8_t row0, row1, row2, row3, row4, row5, row6, row7;

					int16x4_t rot0_0 = vdup_n_s16(stbi__f2f(0.5411961f));
					int16x4_t rot0_1 = vdup_n_s16(stbi__f2f(-1.847759065f));
					int16x4_t rot0_2 = vdup_n_s16(stbi__f2f(0.765366865f));
					int16x4_t rot1_0 = vdup_n_s16(stbi__f2f(1.175875602f));
					int16x4_t rot1_1 = vdup_n_s16(stbi__f2f(-0.899976223f));
					int16x4_t rot1_2 = vdup_n_s16(stbi__f2f(-2.562915447f));
					int16x4_t rot2_0 = vdup_n_s16(stbi__f2f(-1.961570560f));
					int16x4_t rot2_1 = vdup_n_s16(stbi__f2f(-0.390180644f));
					int16x4_t rot3_0 = vdup_n_s16(stbi__f2f(0.298631336f));
					int16x4_t rot3_1 = vdup_n_s16(stbi__f2f(2.053119869f));
					int16x4_t rot3_2 = vdup_n_s16(stbi__f2f(3.072711026f));
					int16x4_t rot3_3 = vdup_n_s16(stbi__f2f(1.501321110f));

#define dct_long_mul(out, inq, coeff) \
	int32x4_t out##_l = vmull_s16(vget_low_s16(inq), coeff); \
	int32x4_t out##_h = vmull_s16(vget_high_s16(inq), coeff)

#define dct_long_mac(out, acc, inq, coeff) \
	int32x4_t out##_l = vmlal_s16(acc##_l, vget_low_s16(inq), coeff); \
	int32x4_t out##_h = vmlal_s16(acc##_h, vget_high_s16(inq), coeff)

#define dct_widen(out, inq) \
	int32x4_t out##_l = vshll_n_s16(vget_low_s16(inq), 12); \
	int32x4_t out##_h = vshll_n_s16(vget_high_s16(inq), 12)

					// wide add
#define dct_wadd(out, a, b) \
	int32x4_t out##_l = vaddq_s32(a##_l, b##_l); \
	int32x4_t out##_h = vaddq_s32(a##_h, b##_h)

					// wide sub
#define dct_wsub(out, a, b) \
	int32x4_t out##_l = vsubq_s32(a##_l, b##_l); \
	int32x4_t out##_h = vsubq_s32(a##_h, b##_h)

					// butterfly a/b, then shift using "shiftop" by "s" and pack
#define dct_bfly32o(out0,out1, a,b,shiftop,s) \
					{
					\
						dct_wadd(sum, a, b); \
						dct_wsub(dif, a, b); \
						out0 = vcombine_s16(shiftop(sum_l, s), shiftop(sum_h, s)); \
						out1 = vcombine_s16(shiftop(dif_l, s), shiftop(dif_h, s)); \
	}

#define dct_pass(shiftop, shift) \
					{ \
					/* even part */ \
					int16x8_t sum26 = vaddq_s16(row2, row6); \
					dct_long_mul(p1e, sum26, rot0_0); \
					dct_long_mac(t2e, p1e, row6, rot0_1); \
					dct_long_mac(t3e, p1e, row2, rot0_2); \
					int16x8_t sum04 = vaddq_s16(row0, row4); \
					int16x8_t dif04 = vsubq_s16(row0, row4); \
					dct_widen(t0e, sum04); \
					dct_widen(t1e, dif04); \
					dct_wadd(x0, t0e, t3e); \
					dct_wsub(x3, t0e, t3e); \
					dct_wadd(x1, t1e, t2e); \
					dct_wsub(x2, t1e, t2e); \
					/* odd part */ \
					int16x8_t sum15 = vaddq_s16(row1, row5); \
					int16x8_t sum17 = vaddq_s16(row1, row7); \
					int16x8_t sum35 = vaddq_s16(row3, row5); \
					int16x8_t sum37 = vaddq_s16(row3, row7); \
					int16x8_t sumodd = vaddq_s16(sum17, sum35); \
					dct_long_mul(p5o, sumodd, rot1_0); \
					dct_long_mac(p1o, p5o, sum17, rot1_1); \
					dct_long_mac(p2o, p5o, sum35, rot1_2); \
					dct_long_mul(p3o, sum37, rot2_0); \
					dct_long_mul(p4o, sum15, rot2_1); \
					dct_wadd(sump13o, p1o, p3o); \
					dct_wadd(sump24o, p2o, p4o); \
					dct_wadd(sump23o, p2o, p3o); \
					dct_wadd(sump14o, p1o, p4o); \
					dct_long_mac(x4, sump13o, row7, rot3_0); \
					dct_long_mac(x5, sump24o, row5, rot3_1); \
					dct_long_mac(x6, sump23o, row3, rot3_2); \
					dct_long_mac(x7, sump14o, row1, rot3_3); \
					dct_bfly32o(row0, row7, x0, x7, shiftop, shift); \
					dct_bfly32o(row1, row6, x1, x6, shiftop, shift); \
					dct_bfly32o(row2, row5, x2, x5, shiftop, shift); \
					dct_bfly32o(row3, row4, x3, x4, shiftop, shift); \
					}

					// load
					row0 = vld1q_s16(data + 0 * 8);
					row1 = vld1q_s16(data + 1 * 8);
					row2 = vld1q_s16(data + 2 * 8);
					row3 = vld1q_s16(data + 3 * 8);
					row4 = vld1q_s16(data + 4 * 8);
					row5 = vld1q_s16(data + 5 * 8);
					row6 = vld1q_s16(data + 6 * 8);
					row7 = vld1q_s16(data + 7 * 8);

					// add DC bias
					row0 = vaddq_s16(row0, vsetq_lane_s16(1024, vdupq_n_s16(0), 0));

					// column pass
					dct_pass(vrshrn_n_s32, 10);

					// 16bit 8x8 transpose
					{
						// these three map to a single VTRN.16, VTRN.32, and VSWP, respectively.
						// whether compilers actually get this is another story, sadly.
#define dct_trn16(x, y) { int16x8x2_t t = vtrnq_s16(x, y); x = t.val[0]; y = t.val[1]; }
#define dct_trn32(x, y) { int32x4x2_t t = vtrnq_s32(vreinterpretq_s32_s16(x), vreinterpretq_s32_s16(y)); x = vreinterpretq_s16_s32(t.val[0]); y = vreinterpretq_s16_s32(t.val[1]); }
#define dct_trn64(x, y) { int16x8_t x0 = x; int16x8_t y0 = y; x = vcombine_s16(vget_low_s16(x0), vget_low_s16(y0)); y = vcombine_s16(vget_high_s16(x0), vget_high_s16(y0)); }

						// pass 1
						dct_trn16(row0, row1); // a0b0a2b2a4b4a6b6
						dct_trn16(row2, row3);
						dct_trn16(row4, row5);
						dct_trn16(row6, row7);

						// pass 2
						dct_trn32(row0, row2); // a0b0c0d0a4b4c4d4
						dct_trn32(row1, row3);
						dct_trn32(row4, row6);
						dct_trn32(row5, row7);

						// pass 3
						dct_trn64(row0, row4); // a0b0c0d0e0f0g0h0
						dct_trn64(row1, row5);
						dct_trn64(row2, row6);
						dct_trn64(row3, row7);

#undef dct_trn16
#undef dct_trn32
#undef dct_trn64
					}

					// row pass
					// vrshrn_n_s32 only supports shifts up to 16, we need
					// 17. so do a non-rounding shift of 16 first then follow
					// up with a rounding shift by 1.
					dct_pass(vshrn_n_s32, 16);

					{
						// pack and round
						uint8x8_t p0 = vqrshrun_n_s16(row0, 1);
						uint8x8_t p1 = vqrshrun_n_s16(row1, 1);
						uint8x8_t p2 = vqrshrun_n_s16(row2, 1);
						uint8x8_t p3 = vqrshrun_n_s16(row3, 1);
						uint8x8_t p4 = vqrshrun_n_s16(row4, 1);
						uint8x8_t p5 = vqrshrun_n_s16(row5, 1);
						uint8x8_t p6 = vqrshrun_n_s16(row6, 1);
						uint8x8_t p7 = vqrshrun_n_s16(row7, 1);

						// again, these can translate into one instruction, but often don't.
#define dct_trn8_8(x, y) { uint8x8x2_t t = vtrn_u8(x, y); x = t.val[0]; y = t.val[1]; }
#define dct_trn8_16(x, y) { uint16x4x2_t t = vtrn_u16(vreinterpret_u16_u8(x), vreinterpret_u16_u8(y)); x = vreinterpret_u8_u16(t.val[0]); y = vreinterpret_u8_u16(t.val[1]); }
#define dct_trn8_32(x, y) { uint32x2x2_t t = vtrn_u32(vreinterpret_u32_u8(x), vreinterpret_u32_u8(y)); x = vreinterpret_u8_u32(t.val[0]); y = vreinterpret_u8_u32(t.val[1]); }

						// sadly can't use interleaved stores here since we only write
						// 8 bytes to each scan line!

						// 8x8 8-bit transpose pass 1
						dct_trn8_8(p0, p1);
						dct_trn8_8(p2, p3);
						dct_trn8_8(p4, p5);
						dct_trn8_8(p6, p7);

						// pass 2
						dct_trn8_16(p0, p2);
						dct_trn8_16(p1, p3);
						dct_trn8_16(p4, p6);
						dct_trn8_16(p5, p7);

						// pass 3
						dct_trn8_32(p0, p4);
						dct_trn8_32(p1, p5);
						dct_trn8_32(p2, p6);
						dct_trn8_32(p3, p7);

						// store
						vst1_u8(out, p0); out += out_stride;
						vst1_u8(out, p1); out += out_stride;
						vst1_u8(out, p2); out += out_stride;
						vst1_u8(out, p3); out += out_stride;
						vst1_u8(out, p4); out += out_stride;
						vst1_u8(out, p5); out += out_stride;
						vst1_u8(out, p6); out += out_stride;
						vst1_u8(out, p7);

#undef dct_trn8_8
#undef dct_trn8_16
#undef dct_trn8_32
					}

#undef dct_long_mul
#undef dct_long_mac
#undef dct_widen
#undef dct_wadd
#undef dct_wsub
#undef dct_bfly32o
#undef dct_pass
				}

#endif // STBI_NEON

#define STBI__MARKER_none  0xff
				// if there's a pending marker from the entropy stream, return that
				// otherwise, fetch from the stream and get a marker. if there's no
				// marker, return 0xff, which is never a valid marker value
				stbi_uc stbi__get_marker(stbi__jpeg *j)
				{
					stbi_uc x;
					if (j->marker != STBI__MARKER_none) { x = j->marker; j->marker = STBI__MARKER_none; return x; }
					x = stbi__get8(j->s);
					if (x != 0xff) return STBI__MARKER_none;
					while (x == 0xff)
						x = stbi__get8(j->s);
					return x;
				}

				// in each scan, we'll have scan_n components, and the order
				// of the components is specified by order[]
#define STBI__RESTART(x)     ((x) >= 0xd0 && (x) <= 0xd7)

				// after a restart interval, stbi__jpeg_reset the entropy decoder and
				// the dc prediction
				void stbi__jpeg_reset(stbi__jpeg *j)
				{
					j->code_bits = 0;
					j->code_buffer = 0;
					j->nomore = 0;
					j->img_comp[0].dc_pred = j->img_comp[1].dc_pred = j->img_comp[2].dc_pred = 0;
					j->marker = STBI__MARKER_none;
					j->todo = j->restart_interval ? j->restart_interval : 0x7fffffff;
					j->eob_run = 0;
					// no more than 1<<31 MCUs if no restart_interal? that's plenty safe,
					// since we don't even allow 1<<30 pixels
				}

				int stbi__parse_entropy_coded_data(stbi__jpeg *z)
				{
					stbi__jpeg_reset(z);
					if (!z->progressive) {
						if (z->scan_n == 1) {
							int i, j;
							STBI_SIMD_ALIGN(short, data[64]);
							int n = z->order[0];
							// non-interleaved data, we just need to process one block at a time,
							// in trivial scanline order
							// number of blocks to do just depends on how many actual "pixels" this
							// component has, independent of interleaved MCU blocking and such
							int w = (z->img_comp[n].x + 7) >> 3;
							int h = (z->img_comp[n].y + 7) >> 3;
							for (j = 0; j < h; ++j) {
								for (i = 0; i < w; ++i) {
									int ha = z->img_comp[n].ha;
									if (!stbi__jpeg_decode_block(z, data, z->huff_dc + z->img_comp[n].hd, z->huff_ac + ha, z->fast_ac[ha], n, z->dequant[z->img_comp[n].tq])) return 0;
									z->idct_block_kernel(z->img_comp[n].data + z->img_comp[n].w2*j * 8 + i * 8, z->img_comp[n].w2, data);
									// every data block is an MCU, so countdown the restart interval
									if (--z->todo <= 0) {
										if (z->code_bits < 24) stbi__grow_buffer_unsafe(z);
										// if it's NOT a restart, then just bail, so we get corrupt data
										// rather than no data
										if (!STBI__RESTART(z->marker)) return 1;
										stbi__jpeg_reset(z);
									}
								}
							}
							return 1;
						}
						else { // interleaved
							int i, j, k, x, y;
							STBI_SIMD_ALIGN(short, data[64]);
							for (j = 0; j < z->img_mcu_y; ++j) {
								for (i = 0; i < z->img_mcu_x; ++i) {
									// scan an interleaved mcu... process scan_n components in order
									for (k = 0; k < z->scan_n; ++k) {
										int n = z->order[k];
										// scan out an mcu's worth of this component; that's just determined
										// by the basic H and V specified for the component
										for (y = 0; y < z->img_comp[n].v; ++y) {
											for (x = 0; x < z->img_comp[n].h; ++x) {
												int x2 = (i*z->img_comp[n].h + x) * 8;
												int y2 = (j*z->img_comp[n].v + y) * 8;
												int ha = z->img_comp[n].ha;
												if (!stbi__jpeg_decode_block(z, data, z->huff_dc + z->img_comp[n].hd, z->huff_ac + ha, z->fast_ac[ha], n, z->dequant[z->img_comp[n].tq])) return 0;
												z->idct_block_kernel(z->img_comp[n].data + z->img_comp[n].w2*y2 + x2, z->img_comp[n].w2, data);
											}
										}
									}
									// after all interleaved components, that's an interleaved MCU,
									// so now count down the restart interval
									if (--z->todo <= 0) {
										if (z->code_bits < 24) stbi__grow_buffer_unsafe(z);
										if (!STBI__RESTART(z->marker)) return 1;
										stbi__jpeg_reset(z);
									}
								}
							}
							return 1;
						}
					}
					else {
						if (z->scan_n == 1) {
							int i, j;
							int n = z->order[0];
							// non-interleaved data, we just need to process one block at a time,
							// in trivial scanline order
							// number of blocks to do just depends on how many actual "pixels" this
							// component has, independent of interleaved MCU blocking and such
							int w = (z->img_comp[n].x + 7) >> 3;
							int h = (z->img_comp[n].y + 7) >> 3;
							for (j = 0; j < h; ++j) {
								for (i = 0; i < w; ++i) {
									short *data = z->img_comp[n].coeff + 64 * (i + j * z->img_comp[n].coeff_w);
									if (z->spec_start == 0) {
										if (!stbi__jpeg_decode_block_prog_dc(z, data, &z->huff_dc[z->img_comp[n].hd], n))
											return 0;
									}
									else {
										int ha = z->img_comp[n].ha;
										if (!stbi__jpeg_decode_block_prog_ac(z, data, &z->huff_ac[ha], z->fast_ac[ha]))
											return 0;
									}
									// every data block is an MCU, so countdown the restart interval
									if (--z->todo <= 0) {
										if (z->code_bits < 24) stbi__grow_buffer_unsafe(z);
										if (!STBI__RESTART(z->marker)) return 1;
										stbi__jpeg_reset(z);
									}
								}
							}
							return 1;
						}
						else { // interleaved
							int i, j, k, x, y;
							for (j = 0; j < z->img_mcu_y; ++j) {
								for (i = 0; i < z->img_mcu_x; ++i) {
									// scan an interleaved mcu... process scan_n components in order
									for (k = 0; k < z->scan_n; ++k) {
										int n = z->order[k];
										// scan out an mcu's worth of this component; that's just determined
										// by the basic H and V specified for the component
										for (y = 0; y < z->img_comp[n].v; ++y) {
											for (x = 0; x < z->img_comp[n].h; ++x) {
												int x2 = (i*z->img_comp[n].h + x);
												int y2 = (j*z->img_comp[n].v + y);
												short *data = z->img_comp[n].coeff + 64 * (x2 + y2 * z->img_comp[n].coeff_w);
												if (!stbi__jpeg_decode_block_prog_dc(z, data, &z->huff_dc[z->img_comp[n].hd], n))
													return 0;
											}
										}
									}
									// after all interleaved components, that's an interleaved MCU,
									// so now count down the restart interval
									if (--z->todo <= 0) {
										if (z->code_bits < 24) stbi__grow_buffer_unsafe(z);
										if (!STBI__RESTART(z->marker)) return 1;
										stbi__jpeg_reset(z);
									}
								}
							}
							return 1;
						}
					}
				}

				void stbi__jpeg_dequantize(short *data, stbi_uc *dequant)
				{
					int i;
					for (i = 0; i < 64; ++i)
						data[i] *= dequant[i];
				}

				void stbi__jpeg_finish(stbi__jpeg *z)
				{
					if (z->progressive) {
						// dequantize and idct the data
						int i, j, n;
						for (n = 0; n < z->s->img_n; ++n) {
							int w = (z->img_comp[n].x + 7) >> 3;
							int h = (z->img_comp[n].y + 7) >> 3;
							for (j = 0; j < h; ++j) {
								for (i = 0; i < w; ++i) {
									short *data = z->img_comp[n].coeff + 64 * (i + j * z->img_comp[n].coeff_w);
									stbi__jpeg_dequantize(data, z->dequant[z->img_comp[n].tq]);
									z->idct_block_kernel(z->img_comp[n].data + z->img_comp[n].w2*j * 8 + i * 8, z->img_comp[n].w2, data);
								}
							}
						}
					}
				}

				int stbi__process_marker(stbi__jpeg *z, int m)
				{
					int L;
					switch (m) {
					case STBI__MARKER_none: // no marker found
						return stbi__err("expected marker", "Corrupt JPEG");

					case 0xDD: // DRI - specify restart interval
						if (stbi__get16be(z->s) != 4) return stbi__err("bad DRI len", "Corrupt JPEG");
						z->restart_interval = stbi__get16be(z->s);
						return 1;

					case 0xDB: // DQT - define quantization table
						L = stbi__get16be(z->s) - 2;
						while (L > 0) {
							int q = stbi__get8(z->s);
							int p = q >> 4;
							int t = q & 15, i;
							if (p != 0) return stbi__err("bad DQT type", "Corrupt JPEG");
							if (t > 3) return stbi__err("bad DQT table", "Corrupt JPEG");
							for (i = 0; i < 64; ++i)
								z->dequant[t][stbi__jpeg_dezigzag[i]] = stbi__get8(z->s);
							L -= 65;
						}
						return L == 0;

					case 0xC4: // DHT - define huffman table
						L = stbi__get16be(z->s) - 2;
						while (L > 0) {
							stbi_uc *v;
							int sizes[16], i, n = 0;
							int q = stbi__get8(z->s);
							int tc = q >> 4;
							int th = q & 15;
							if (tc > 1 || th > 3) return stbi__err("bad DHT header", "Corrupt JPEG");
							for (i = 0; i < 16; ++i) {
								sizes[i] = stbi__get8(z->s);
								n += sizes[i];
							}
							L -= 17;
							if (tc == 0) {
								if (!stbi__build_huffman(z->huff_dc + th, sizes)) return 0;
								v = z->huff_dc[th].values;
							}
							else {
								if (!stbi__build_huffman(z->huff_ac + th, sizes)) return 0;
								v = z->huff_ac[th].values;
							}
							for (i = 0; i < n; ++i)
								v[i] = stbi__get8(z->s);
							if (tc != 0)
								stbi__build_fast_ac(z->fast_ac[th], z->huff_ac + th);
							L -= n;
						}
						return L == 0;
					}
					// check for comment block or APP blocks
					if ((m >= 0xE0 && m <= 0xEF) || m == 0xFE) {
						stbi__skip(z->s, stbi__get16be(z->s) - 2);
						return 1;
					}
					return 0;
				}

				// after we see SOS
				int stbi__process_scan_header(stbi__jpeg *z)
				{
					int i;
					int Ls = stbi__get16be(z->s);
					z->scan_n = stbi__get8(z->s);
					if (z->scan_n < 1 || z->scan_n > 4 || z->scan_n > (int)z->s->img_n) return stbi__err("bad SOS component count", "Corrupt JPEG");
					if (Ls != 6 + 2 * z->scan_n) return stbi__err("bad SOS len", "Corrupt JPEG");
					for (i = 0; i < z->scan_n; ++i) {
						int id = stbi__get8(z->s), which;
						int q = stbi__get8(z->s);
						for (which = 0; which < z->s->img_n; ++which)
						if (z->img_comp[which].id == id)
							break;
						if (which == z->s->img_n) return 0; // no match
						z->img_comp[which].hd = q >> 4;   if (z->img_comp[which].hd > 3) return stbi__err("bad DC huff", "Corrupt JPEG");
						z->img_comp[which].ha = q & 15;   if (z->img_comp[which].ha > 3) return stbi__err("bad AC huff", "Corrupt JPEG");
						z->order[i] = which;
					}

					{
						int aa;
						z->spec_start = stbi__get8(z->s);
						z->spec_end = stbi__get8(z->s); // should be 63, but might be 0
						aa = stbi__get8(z->s);
						z->succ_high = (aa >> 4);
						z->succ_low = (aa & 15);
						if (z->progressive) {
							if (z->spec_start > 63 || z->spec_end > 63 || z->spec_start > z->spec_end || z->succ_high > 13 || z->succ_low > 13)
								return stbi__err("bad SOS", "Corrupt JPEG");
						}
						else {
							if (z->spec_start != 0) return stbi__err("bad SOS", "Corrupt JPEG");
							if (z->succ_high != 0 || z->succ_low != 0) return stbi__err("bad SOS", "Corrupt JPEG");
							z->spec_end = 63;
						}
					}

					return 1;
				}

				int stbi__process_frame_header(stbi__jpeg *z, int scan)
				{
					stbi__context *s = z->s;
					int Lf, p, i, q, h_max = 1, v_max = 1, c;
					Lf = stbi__get16be(s);         if (Lf < 11) return stbi__err("bad SOF len", "Corrupt JPEG"); // JPEG
					p = stbi__get8(s);            if (p != 8) return stbi__err("only 8-bit", "JPEG format not supported: 8-bit only"); // JPEG baseline
					s->img_y = stbi__get16be(s);   if (s->img_y == 0) return stbi__err("no header height", "JPEG format not supported: delayed height"); // Legal, but we don't handle it--but neither does IJG
					s->img_x = stbi__get16be(s);   if (s->img_x == 0) return stbi__err("0 width", "Corrupt JPEG"); // JPEG requires
					c = stbi__get8(s);
					if (c != 3 && c != 1) return stbi__err("bad component count", "Corrupt JPEG");    // JFIF requires
					s->img_n = c;
					for (i = 0; i < c; ++i) {
						z->img_comp[i].data = NULL;
						z->img_comp[i].linebuf = NULL;
					}

					if (Lf != 8 + 3 * s->img_n) return stbi__err("bad SOF len", "Corrupt JPEG");

					for (i = 0; i < s->img_n; ++i) {
						z->img_comp[i].id = stbi__get8(s);
						if (z->img_comp[i].id != i + 1)   // JFIF requires
						if (z->img_comp[i].id != i)  // some version of jpegtran outputs non-JFIF-compliant files!
							return stbi__err("bad component ID", "Corrupt JPEG");
						q = stbi__get8(s);
						z->img_comp[i].h = (q >> 4);  if (!z->img_comp[i].h || z->img_comp[i].h > 4) return stbi__err("bad H", "Corrupt JPEG");
						z->img_comp[i].v = q & 15;    if (!z->img_comp[i].v || z->img_comp[i].v > 4) return stbi__err("bad V", "Corrupt JPEG");
						z->img_comp[i].tq = stbi__get8(s);  if (z->img_comp[i].tq > 3) return stbi__err("bad TQ", "Corrupt JPEG");
					}

					if (scan != STBI__SCAN_load) return 1;

					if ((1 << 30) / s->img_x / s->img_n < s->img_y) return stbi__err("too large", "Image too large to decode");

					for (i = 0; i < s->img_n; ++i) {
						if (z->img_comp[i].h > h_max) h_max = z->img_comp[i].h;
						if (z->img_comp[i].v > v_max) v_max = z->img_comp[i].v;
					}

					// compute interleaved mcu info
					z->img_h_max = h_max;
					z->img_v_max = v_max;
					z->img_mcu_w = h_max * 8;
					z->img_mcu_h = v_max * 8;
					z->img_mcu_x = (s->img_x + z->img_mcu_w - 1) / z->img_mcu_w;
					z->img_mcu_y = (s->img_y + z->img_mcu_h - 1) / z->img_mcu_h;

					for (i = 0; i < s->img_n; ++i) {
						// number of effective pixels (e.g. for non-interleaved MCU)
						z->img_comp[i].x = (s->img_x * z->img_comp[i].h + h_max - 1) / h_max;
						z->img_comp[i].y = (s->img_y * z->img_comp[i].v + v_max - 1) / v_max;
						// to simplify generation, we'll allocate enough memory to decode
						// the bogus oversized data from using interleaved MCUs and their
						// big blocks (e.g. a 16x16 iMCU on an image of width 33); we won't
						// discard the extra data until colorspace conversion
						z->img_comp[i].w2 = z->img_mcu_x * z->img_comp[i].h * 8;
						z->img_comp[i].h2 = z->img_mcu_y * z->img_comp[i].v * 8;
						z->img_comp[i].raw_data = stbi__malloc(z->img_comp[i].w2 * z->img_comp[i].h2 + 15);

						if (z->img_comp[i].raw_data == NULL) {
							for (--i; i >= 0; --i) {
								STBI_FREE(z->img_comp[i].raw_data);
								z->img_comp[i].raw_data = NULL;
							}
							return stbi__err("outofmem", "Out of memory");
						}
						// align blocks for idct using mmx/sse
						z->img_comp[i].data = (stbi_uc*)(((size_t)z->img_comp[i].raw_data + 15) & ~15);
						z->img_comp[i].linebuf = NULL;
						if (z->progressive) {
							z->img_comp[i].coeff_w = (z->img_comp[i].w2 + 7) >> 3;
							z->img_comp[i].coeff_h = (z->img_comp[i].h2 + 7) >> 3;
							z->img_comp[i].raw_coeff = STBI_MALLOC(z->img_comp[i].coeff_w * z->img_comp[i].coeff_h * 64 * sizeof(short)+15);
							z->img_comp[i].coeff = (short*)(((size_t)z->img_comp[i].raw_coeff + 15) & ~15);
						}
						else {
							z->img_comp[i].coeff = 0;
							z->img_comp[i].raw_coeff = 0;
						}
					}

					return 1;
				}

				// use comparisons since in some cases we handle more than one case (e.g. SOF)
#define stbi__DNL(x)         ((x) == 0xdc)
#define stbi__SOI(x)         ((x) == 0xd8)
#define stbi__EOI(x)         ((x) == 0xd9)
#define stbi__SOF(x)         ((x) == 0xc0 || (x) == 0xc1 || (x) == 0xc2)
#define stbi__SOS(x)         ((x) == 0xda)

#define stbi__SOF_progressive(x)   ((x) == 0xc2)

				int stbi__decode_jpeg_header(stbi__jpeg *z, int scan)
				{
					int m;
					z->marker = STBI__MARKER_none; // initialize cached marker to empty
					m = stbi__get_marker(z);
					if (!stbi__SOI(m)) return stbi__err("no SOI", "Corrupt JPEG");
					if (scan == STBI__SCAN_type) return 1;
					m = stbi__get_marker(z);
					while (!stbi__SOF(m)) {
						if (!stbi__process_marker(z, m)) return 0;
						m = stbi__get_marker(z);
						while (m == STBI__MARKER_none) {
							// some files have extra padding after their blocks, so ok, we'll scan
							if (stbi__at_eof(z->s)) return stbi__err("no SOF", "Corrupt JPEG");
							m = stbi__get_marker(z);
						}
					}
					z->progressive = stbi__SOF_progressive(m);
					if (!stbi__process_frame_header(z, scan)) return 0;
					return 1;
				}

				// decode image to YCbCr format
				int stbi__decode_jpeg_image(stbi__jpeg *j)
				{
					int m;
					for (m = 0; m < 4; m++) {
						j->img_comp[m].raw_data = NULL;
						j->img_comp[m].raw_coeff = NULL;
					}
					j->restart_interval = 0;
					if (!stbi__decode_jpeg_header(j, STBI__SCAN_load)) return 0;
					m = stbi__get_marker(j);
					while (!stbi__EOI(m)) {
						if (stbi__SOS(m)) {
							if (!stbi__process_scan_header(j)) return 0;
							if (!stbi__parse_entropy_coded_data(j)) return 0;
							if (j->marker == STBI__MARKER_none) {
								// handle 0s at the end of image data from IP Kamera 9060
								while (!stbi__at_eof(j->s)) {
									int x = stbi__get8(j->s);
									if (x == 255) {
										j->marker = stbi__get8(j->s);
										break;
									}
									else if (x != 0) {
										return stbi__err("junk before marker", "Corrupt JPEG");
									}
								}
								// if we reach eof without hitting a marker, stbi__get_marker() below will fail and we'll eventually return 0
							}
						}
						else {
							if (!stbi__process_marker(j, m)) return 0;
						}
						m = stbi__get_marker(j);
					}
					if (j->progressive)
						stbi__jpeg_finish(j);
					return 1;
				}

				//  jfif-centered resampling (across block boundaries)

				typedef stbi_uc *(*resample_row_func)(stbi_uc *out, stbi_uc *in0, stbi_uc *in1,
					int w, int hs);

#define stbi__div4(x) ((stbi_uc) ((x) >> 2))

				stbi_uc *resample_row_1(stbi_uc *out, stbi_uc *in_near, stbi_uc *in_far, int w, int hs)
				{
					STBI_NOTUSED(out);
					STBI_NOTUSED(in_far);
					STBI_NOTUSED(w);
					STBI_NOTUSED(hs);
					return in_near;
				}

				stbi_uc* stbi__resample_row_v_2(stbi_uc *out, stbi_uc *in_near, stbi_uc *in_far, int w, int hs)
				{
					// need to generate two samples vertically for every one in input
					int i;
					STBI_NOTUSED(hs);
					for (i = 0; i < w; ++i)
						out[i] = stbi__div4(3 * in_near[i] + in_far[i] + 2);
					return out;
				}

				stbi_uc*  stbi__resample_row_h_2(stbi_uc *out, stbi_uc *in_near, stbi_uc *in_far, int w, int hs)
				{
					// need to generate two samples horizontally for every one in input
					int i;
					stbi_uc *input = in_near;

					if (w == 1) {
						// if only one sample, can't do any interpolation
						out[0] = out[1] = input[0];
						return out;
					}

					out[0] = input[0];
					out[1] = stbi__div4(input[0] * 3 + input[1] + 2);
					for (i = 1; i < w - 1; ++i) {
						int n = 3 * input[i] + 2;
						out[i * 2 + 0] = stbi__div4(n + input[i - 1]);
						out[i * 2 + 1] = stbi__div4(n + input[i + 1]);
					}
					out[i * 2 + 0] = stbi__div4(input[w - 2] * 3 + input[w - 1] + 2);
					out[i * 2 + 1] = input[w - 1];

					STBI_NOTUSED(in_far);
					STBI_NOTUSED(hs);

					return out;
				}

#define stbi__div16(x) ((stbi_uc) ((x) >> 4))

				stbi_uc *stbi__resample_row_hv_2(stbi_uc *out, stbi_uc *in_near, stbi_uc *in_far, int w, int hs)
				{
					// need to generate 2x2 samples for every one in input
					int i, t0, t1;
					if (w == 1) {
						out[0] = out[1] = stbi__div4(3 * in_near[0] + in_far[0] + 2);
						return out;
					}

					t1 = 3 * in_near[0] + in_far[0];
					out[0] = stbi__div4(t1 + 2);
					for (i = 1; i < w; ++i) {
						t0 = t1;
						t1 = 3 * in_near[i] + in_far[i];
						out[i * 2 - 1] = stbi__div16(3 * t0 + t1 + 8);
						out[i * 2] = stbi__div16(3 * t1 + t0 + 8);
					}
					out[w * 2 - 1] = stbi__div4(t1 + 2);

					STBI_NOTUSED(hs);

					return out;
				}

#if defined(STBI_SSE2) || defined(STBI_NEON)
				stbi_uc *stbi__resample_row_hv_2_simd(stbi_uc *out, stbi_uc *in_near, stbi_uc *in_far, int w, int hs)
				{
					// need to generate 2x2 samples for every one in input
					int i = 0, t0, t1;

					if (w == 1) {
						out[0] = out[1] = stbi__div4(3 * in_near[0] + in_far[0] + 2);
						return out;
					}

					t1 = 3 * in_near[0] + in_far[0];
					// process groups of 8 pixels for as long as we can.
					// note we can't handle the last pixel in a row in this loop
					// because we need to handle the filter boundary conditions.
					for (; i < ((w - 1) & ~7); i += 8) {
#if defined(STBI_SSE2)
						// load and perform the vertical filtering pass
						// this uses 3*x + y = 4*x + (y - x)
						__m128i zero = _mm_setzero_si128();
						__m128i farb = _mm_loadl_epi64((__m128i *) (in_far + i));
						__m128i nearb = _mm_loadl_epi64((__m128i *) (in_near + i));
						__m128i farw = _mm_unpacklo_epi8(farb, zero);
						__m128i nearw = _mm_unpacklo_epi8(nearb, zero);
						__m128i diff = _mm_sub_epi16(farw, nearw);
						__m128i nears = _mm_slli_epi16(nearw, 2);
						__m128i curr = _mm_add_epi16(nears, diff); // current row

						// horizontal filter works the same based on shifted vers of current
						// row. "prev" is current row shifted right by 1 pixel; we need to
						// insert the previous pixel value (from t1).
						// "next" is current row shifted left by 1 pixel, with first pixel
						// of next block of 8 pixels added in.
						__m128i prv0 = _mm_slli_si128(curr, 2);
						__m128i nxt0 = _mm_srli_si128(curr, 2);
						__m128i prev = _mm_insert_epi16(prv0, t1, 0);
						__m128i next = _mm_insert_epi16(nxt0, 3 * in_near[i + 8] + in_far[i + 8], 7);

						// horizontal filter, polyphase implementation since it's convenient:
						// even pixels = 3*cur + prev = cur*4 + (prev - cur)
						// odd  pixels = 3*cur + next = cur*4 + (next - cur)
						// note the shared term.
						__m128i bias = _mm_set1_epi16(8);
						__m128i curs = _mm_slli_epi16(curr, 2);
						__m128i prvd = _mm_sub_epi16(prev, curr);
						__m128i nxtd = _mm_sub_epi16(next, curr);
						__m128i curb = _mm_add_epi16(curs, bias);
						__m128i even = _mm_add_epi16(prvd, curb);
						__m128i odd = _mm_add_epi16(nxtd, curb);

						// interleave even and odd pixels, then undo scaling.
						__m128i int0 = _mm_unpacklo_epi16(even, odd);
						__m128i int1 = _mm_unpackhi_epi16(even, odd);
						__m128i de0 = _mm_srli_epi16(int0, 4);
						__m128i de1 = _mm_srli_epi16(int1, 4);

						// pack and write output
						__m128i outv = _mm_packus_epi16(de0, de1);
						_mm_storeu_si128((__m128i *) (out + i * 2), outv);
#elif defined(STBI_NEON)
						// load and perform the vertical filtering pass
						// this uses 3*x + y = 4*x + (y - x)
						uint8x8_t farb = vld1_u8(in_far + i);
						uint8x8_t nearb = vld1_u8(in_near + i);
						int16x8_t diff = vreinterpretq_s16_u16(vsubl_u8(farb, nearb));
						int16x8_t nears = vreinterpretq_s16_u16(vshll_n_u8(nearb, 2));
						int16x8_t curr = vaddq_s16(nears, diff); // current row

						// horizontal filter works the same based on shifted vers of current
						// row. "prev" is current row shifted right by 1 pixel; we need to
						// insert the previous pixel value (from t1).
						// "next" is current row shifted left by 1 pixel, with first pixel
						// of next block of 8 pixels added in.
						int16x8_t prv0 = vextq_s16(curr, curr, 7);
						int16x8_t nxt0 = vextq_s16(curr, curr, 1);
						int16x8_t prev = vsetq_lane_s16(t1, prv0, 0);
						int16x8_t next = vsetq_lane_s16(3 * in_near[i + 8] + in_far[i + 8], nxt0, 7);

						// horizontal filter, polyphase implementation since it's convenient:
						// even pixels = 3*cur + prev = cur*4 + (prev - cur)
						// odd  pixels = 3*cur + next = cur*4 + (next - cur)
						// note the shared term.
						int16x8_t curs = vshlq_n_s16(curr, 2);
						int16x8_t prvd = vsubq_s16(prev, curr);
						int16x8_t nxtd = vsubq_s16(next, curr);
						int16x8_t even = vaddq_s16(curs, prvd);
						int16x8_t odd = vaddq_s16(curs, nxtd);

						// undo scaling and round, then store with even/odd phases interleaved
						uint8x8x2_t o;
						o.val[0] = vqrshrun_n_s16(even, 4);
						o.val[1] = vqrshrun_n_s16(odd, 4);
						vst2_u8(out + i * 2, o);
#endif

						// "previous" value for next iter
						t1 = 3 * in_near[i + 7] + in_far[i + 7];
					}

					t0 = t1;
					t1 = 3 * in_near[i] + in_far[i];
					out[i * 2] = stbi__div16(3 * t1 + t0 + 8);

					for (++i; i < w; ++i) {
						t0 = t1;
						t1 = 3 * in_near[i] + in_far[i];
						out[i * 2 - 1] = stbi__div16(3 * t0 + t1 + 8);
						out[i * 2] = stbi__div16(3 * t1 + t0 + 8);
					}
					out[w * 2 - 1] = stbi__div4(t1 + 2);

					STBI_NOTUSED(hs);

					return out;
				}
#endif

				stbi_uc *stbi__resample_row_generic(stbi_uc *out, stbi_uc *in_near, stbi_uc *in_far, int w, int hs)
				{
					// resample with nearest-neighbor
					int i, j;
					STBI_NOTUSED(in_far);
					for (i = 0; i < w; ++i)
					for (j = 0; j < hs; ++j)
						out[i*hs + j] = in_near[i];
					return out;
				}

#ifdef STBI_JPEG_OLD
				// this is the same YCbCr-to-RGB calculation that stb_image has used
				// historically before the algorithm changes in 1.49
#define float2fixed(x)  ((int) ((x) * 65536 + 0.5))
				void stbi__YCbCr_to_RGB_row(stbi_uc *out, const stbi_uc *y, const stbi_uc *pcb, const stbi_uc *pcr, int count, int step)
				{
					int i;
					for (i = 0; i < count; ++i) {
						int y_fixed = (y[i] << 16) + 32768; // rounding
						int r, g, b;
						int cr = pcr[i] - 128;
						int cb = pcb[i] - 128;
						r = y_fixed + cr*float2fixed(1.40200f);
						g = y_fixed - cr*float2fixed(0.71414f) - cb*float2fixed(0.34414f);
						b = y_fixed + cb*float2fixed(1.77200f);
						r >>= 16;
						g >>= 16;
						b >>= 16;
						if ((unsigned)r > 255) { if (r < 0) r = 0; else r = 255; }
						if ((unsigned)g > 255) { if (g < 0) g = 0; else g = 255; }
						if ((unsigned)b > 255) { if (b < 0) b = 0; else b = 255; }
						out[0] = (stbi_uc)r;
						out[1] = (stbi_uc)g;
						out[2] = (stbi_uc)b;
						out[3] = 255;
						out += step;
					}
				}
#else
				// this is a reduced-precision calculation of YCbCr-to-RGB introduced
				// to make sure the code produces the same results in both SIMD and scalar
#define float2fixed(x)  (((int) ((x) * 4096.0f + 0.5f)) << 8)
				void stbi__YCbCr_to_RGB_row(stbi_uc *out, const stbi_uc *y, const stbi_uc *pcb, const stbi_uc *pcr, int count, int step)
				{
					int i;
					for (i = 0; i < count; ++i) {
						int y_fixed = (y[i] << 20) + (1 << 19); // rounding
						int r, g, b;
						int cr = pcr[i] - 128;
						int cb = pcb[i] - 128;
						r = y_fixed + cr* float2fixed(1.40200f);
						g = y_fixed + (cr*-float2fixed(0.71414f)) + ((cb*-float2fixed(0.34414f)) & 0xffff0000);
						b = y_fixed + cb* float2fixed(1.77200f);
						r >>= 20;
						g >>= 20;
						b >>= 20;
						if ((unsigned)r > 255) { if (r < 0) r = 0; else r = 255; }
						if ((unsigned)g > 255) { if (g < 0) g = 0; else g = 255; }
						if ((unsigned)b > 255) { if (b < 0) b = 0; else b = 255; }
						out[0] = (stbi_uc)r;
						out[1] = (stbi_uc)g;
						out[2] = (stbi_uc)b;
						out[3] = 255;
						out += step;
					}
				}
#endif

#if defined(STBI_SSE2) || defined(STBI_NEON)
				void stbi__YCbCr_to_RGB_simd(stbi_uc *out, stbi_uc const *y, stbi_uc const *pcb, stbi_uc const *pcr, int count, int step)
				{
					int i = 0;

#ifdef STBI_SSE2
					// step == 3 is pretty ugly on the final interleave, and i'm not convinced
					// it's useful in practice (you wouldn't use it for textures, for example).
					// so just accelerate step == 4 case.
					if (step == 4) {
						// this is a fairly straightforward implementation and not super-optimized.
						__m128i signflip = _mm_set1_epi8(-0x80);
						__m128i cr_const0 = _mm_set1_epi16((short)(1.40200f*4096.0f + 0.5f));
						__m128i cr_const1 = _mm_set1_epi16(-(short)(0.71414f*4096.0f + 0.5f));
						__m128i cb_const0 = _mm_set1_epi16(-(short)(0.34414f*4096.0f + 0.5f));
						__m128i cb_const1 = _mm_set1_epi16((short)(1.77200f*4096.0f + 0.5f));
						__m128i y_bias = _mm_set1_epi8((char)(unsigned char)128);
						__m128i xw = _mm_set1_epi16(255); // alpha channel

						for (; i + 7 < count; i += 8) {
							// load
							__m128i y_bytes = _mm_loadl_epi64((__m128i *) (y + i));
							__m128i cr_bytes = _mm_loadl_epi64((__m128i *) (pcr + i));
							__m128i cb_bytes = _mm_loadl_epi64((__m128i *) (pcb + i));
							__m128i cr_biased = _mm_xor_si128(cr_bytes, signflip); // -128
							__m128i cb_biased = _mm_xor_si128(cb_bytes, signflip); // -128

							// unpack to short (and left-shift cr, cb by 8)
							__m128i yw = _mm_unpacklo_epi8(y_bias, y_bytes);
							__m128i crw = _mm_unpacklo_epi8(_mm_setzero_si128(), cr_biased);
							__m128i cbw = _mm_unpacklo_epi8(_mm_setzero_si128(), cb_biased);

							// color transform
							__m128i yws = _mm_srli_epi16(yw, 4);
							__m128i cr0 = _mm_mulhi_epi16(cr_const0, crw);
							__m128i cb0 = _mm_mulhi_epi16(cb_const0, cbw);
							__m128i cb1 = _mm_mulhi_epi16(cbw, cb_const1);
							__m128i cr1 = _mm_mulhi_epi16(crw, cr_const1);
							__m128i rws = _mm_add_epi16(cr0, yws);
							__m128i gwt = _mm_add_epi16(cb0, yws);
							__m128i bws = _mm_add_epi16(yws, cb1);
							__m128i gws = _mm_add_epi16(gwt, cr1);

							// descale
							__m128i rw = _mm_srai_epi16(rws, 4);
							__m128i bw = _mm_srai_epi16(bws, 4);
							__m128i gw = _mm_srai_epi16(gws, 4);

							// back to byte, set up for transpose
							__m128i brb = _mm_packus_epi16(rw, bw);
							__m128i gxb = _mm_packus_epi16(gw, xw);

							// transpose to interleave channels
							__m128i t0 = _mm_unpacklo_epi8(brb, gxb);
							__m128i t1 = _mm_unpackhi_epi8(brb, gxb);
							__m128i o0 = _mm_unpacklo_epi16(t0, t1);
							__m128i o1 = _mm_unpackhi_epi16(t0, t1);

							// store
							_mm_storeu_si128((__m128i *) (out + 0), o0);
							_mm_storeu_si128((__m128i *) (out + 16), o1);
							out += 32;
						}
					}
#endif

#ifdef STBI_NEON
					// in this version, step=3 support would be easy to add. but is there demand?
					if (step == 4) {
						// this is a fairly straightforward implementation and not super-optimized.
						uint8x8_t signflip = vdup_n_u8(0x80);
						int16x8_t cr_const0 = vdupq_n_s16((short)(1.40200f*4096.0f + 0.5f));
						int16x8_t cr_const1 = vdupq_n_s16(-(short)(0.71414f*4096.0f + 0.5f));
						int16x8_t cb_const0 = vdupq_n_s16(-(short)(0.34414f*4096.0f + 0.5f));
						int16x8_t cb_const1 = vdupq_n_s16((short)(1.77200f*4096.0f + 0.5f));

						for (; i + 7 < count; i += 8) {
							// load
							uint8x8_t y_bytes = vld1_u8(y + i);
							uint8x8_t cr_bytes = vld1_u8(pcr + i);
							uint8x8_t cb_bytes = vld1_u8(pcb + i);
							int8x8_t cr_biased = vreinterpret_s8_u8(vsub_u8(cr_bytes, signflip));
							int8x8_t cb_biased = vreinterpret_s8_u8(vsub_u8(cb_bytes, signflip));

							// expand to s16
							int16x8_t yws = vreinterpretq_s16_u16(vshll_n_u8(y_bytes, 4));
							int16x8_t crw = vshll_n_s8(cr_biased, 7);
							int16x8_t cbw = vshll_n_s8(cb_biased, 7);

							// color transform
							int16x8_t cr0 = vqdmulhq_s16(crw, cr_const0);
							int16x8_t cb0 = vqdmulhq_s16(cbw, cb_const0);
							int16x8_t cr1 = vqdmulhq_s16(crw, cr_const1);
							int16x8_t cb1 = vqdmulhq_s16(cbw, cb_const1);
							int16x8_t rws = vaddq_s16(yws, cr0);
							int16x8_t gws = vaddq_s16(vaddq_s16(yws, cb0), cr1);
							int16x8_t bws = vaddq_s16(yws, cb1);

							// undo scaling, round, convert to byte
							uint8x8x4_t o;
							o.val[0] = vqrshrun_n_s16(rws, 4);
							o.val[1] = vqrshrun_n_s16(gws, 4);
							o.val[2] = vqrshrun_n_s16(bws, 4);
							o.val[3] = vdup_n_u8(255);

							// store, interleaving r/g/b/a
							vst4_u8(out, o);
							out += 8 * 4;
						}
					}
#endif

					for (; i < count; ++i) {
						int y_fixed = (y[i] << 20) + (1 << 19); // rounding
						int r, g, b;
						int cr = pcr[i] - 128;
						int cb = pcb[i] - 128;
						r = y_fixed + cr* float2fixed(1.40200f);
						g = y_fixed + cr*-float2fixed(0.71414f) + ((cb*-float2fixed(0.34414f)) & 0xffff0000);
						b = y_fixed + cb* float2fixed(1.77200f);
						r >>= 20;
						g >>= 20;
						b >>= 20;
						if ((unsigned)r > 255) { if (r < 0) r = 0; else r = 255; }
						if ((unsigned)g > 255) { if (g < 0) g = 0; else g = 255; }
						if ((unsigned)b > 255) { if (b < 0) b = 0; else b = 255; }
						out[0] = (stbi_uc)r;
						out[1] = (stbi_uc)g;
						out[2] = (stbi_uc)b;
						out[3] = 255;
						out += step;
					}
				}
#endif

				// set up the kernels
				void stbi__setup_jpeg(stbi__jpeg *j)
				{
					j->idct_block_kernel = stbi__idct_block;
					j->YCbCr_to_RGB_kernel = stbi__YCbCr_to_RGB_row;
					j->resample_row_hv_2_kernel = stbi__resample_row_hv_2;

#ifdef STBI_SSE2
					if (stbi__sse2_available()) {
						j->idct_block_kernel = stbi__idct_simd;
#ifndef STBI_JPEG_OLD
						j->YCbCr_to_RGB_kernel = stbi__YCbCr_to_RGB_simd;
#endif
						j->resample_row_hv_2_kernel = stbi__resample_row_hv_2_simd;
					}
#endif

#ifdef STBI_NEON
					j->idct_block_kernel = stbi__idct_simd;
#ifndef STBI_JPEG_OLD
					j->YCbCr_to_RGB_kernel = stbi__YCbCr_to_RGB_simd;
#endif
					j->resample_row_hv_2_kernel = stbi__resample_row_hv_2_simd;
#endif
				}

				// clean up the temporary component buffers
				void stbi__cleanup_jpeg(stbi__jpeg *j)
				{
					int i;
					for (i = 0; i < j->s->img_n; ++i) {
						if (j->img_comp[i].raw_data) {
							STBI_FREE(j->img_comp[i].raw_data);
							j->img_comp[i].raw_data = NULL;
							j->img_comp[i].data = NULL;
						}
						if (j->img_comp[i].raw_coeff) {
							STBI_FREE(j->img_comp[i].raw_coeff);
							j->img_comp[i].raw_coeff = 0;
							j->img_comp[i].coeff = 0;
						}
						if (j->img_comp[i].linebuf) {
							STBI_FREE(j->img_comp[i].linebuf);
							j->img_comp[i].linebuf = NULL;
						}
					}
				}

				typedef struct
				{
					resample_row_func resample;
					stbi_uc *line0, *line1;
					int hs, vs;   // expansion factor in each axis
					int w_lores; // horizontal pixels pre-expansion
					int ystep;   // how far through vertical expansion we are
					int ypos;    // which pre-expansion row we're on
				} stbi__resample;

				stbi_uc *load_jpeg_image(stbi__jpeg *z, int *out_x, int *out_y, int *comp, int req_comp)
				{
					int n, decode_n;
					z->s->img_n = 0; // make stbi__cleanup_jpeg safe

					// validate req_comp
					if (req_comp < 0 || req_comp > 4) return stbi__errpuc("bad req_comp", "Internal error");

					// load a jpeg image from whichever source, but leave in YCbCr format
					if (!stbi__decode_jpeg_image(z)) { stbi__cleanup_jpeg(z); return NULL; }

					// determine actual number of components to generate
					n = req_comp ? req_comp : z->s->img_n;

					if (z->s->img_n == 3 && n < 3)
						decode_n = 1;
					else
						decode_n = z->s->img_n;

					// resample and color-convert
					{
						int k;
						unsigned int i, j;
						stbi_uc *output;
						stbi_uc *coutput[4];

						stbi__resample res_comp[4];

						for (k = 0; k < decode_n; ++k) {
							stbi__resample *r = &res_comp[k];

							// allocate line buffer big enough for upsampling off the edges
							// with upsample factor of 4
							z->img_comp[k].linebuf = (stbi_uc *)stbi__malloc(z->s->img_x + 3);
							if (!z->img_comp[k].linebuf) { stbi__cleanup_jpeg(z); return stbi__errpuc("outofmem", "Out of memory"); }

							r->hs = z->img_h_max / z->img_comp[k].h;
							r->vs = z->img_v_max / z->img_comp[k].v;
							r->ystep = r->vs >> 1;
							r->w_lores = (z->s->img_x + r->hs - 1) / r->hs;
							r->ypos = 0;
							r->line0 = r->line1 = z->img_comp[k].data;

							if (r->hs == 1 && r->vs == 1) r->resample = resample_row_1;
							else if (r->hs == 1 && r->vs == 2) r->resample = stbi__resample_row_v_2;
							else if (r->hs == 2 && r->vs == 1) r->resample = stbi__resample_row_h_2;
							else if (r->hs == 2 && r->vs == 2) r->resample = z->resample_row_hv_2_kernel;
							else                               r->resample = stbi__resample_row_generic;
						}

						// can't error after this so, this is safe
						output = (stbi_uc *)stbi__malloc(n * z->s->img_x * z->s->img_y + 1);
						if (!output) { stbi__cleanup_jpeg(z); return stbi__errpuc("outofmem", "Out of memory"); }

						// now go ahead and resample
						for (j = 0; j < z->s->img_y; ++j) {
							stbi_uc *out = output + n * z->s->img_x * j;
							for (k = 0; k < decode_n; ++k) {
								stbi__resample *r = &res_comp[k];
								int y_bot = r->ystep >= (r->vs >> 1);
								coutput[k] = r->resample(z->img_comp[k].linebuf,
									y_bot ? r->line1 : r->line0,
									y_bot ? r->line0 : r->line1,
									r->w_lores, r->hs);
								if (++r->ystep >= r->vs) {
									r->ystep = 0;
									r->line0 = r->line1;
									if (++r->ypos < z->img_comp[k].y)
										r->line1 += z->img_comp[k].w2;
								}
							}
							if (n >= 3) {
								stbi_uc *y = coutput[0];
								if (z->s->img_n == 3) {
									z->YCbCr_to_RGB_kernel(out, y, coutput[1], coutput[2], z->s->img_x, n);
								}
								else
								for (i = 0; i < z->s->img_x; ++i) {
									out[0] = out[1] = out[2] = y[i];
									out[3] = 255; // not used if n==3
									out += n;
								}
							}
							else {
								stbi_uc *y = coutput[0];
								if (n == 1)
								for (i = 0; i < z->s->img_x; ++i) out[i] = y[i];
								else
								for (i = 0; i < z->s->img_x; ++i) *out++ = y[i], *out++ = 255;
							}
						}
						stbi__cleanup_jpeg(z);
						*out_x = z->s->img_x;
						*out_y = z->s->img_y;
						if (comp) *comp = z->s->img_n; // report original components, not output
						return output;
					}
				}

				unsigned char *stbi__jpeg_load(stbi__context *s, int *x, int *y, int *comp, int req_comp)
				{
					stbi__jpeg j;
					j.s = s;
					stbi__setup_jpeg(&j);
					return load_jpeg_image(&j, x, y, comp, req_comp);
				}

				int stbi__jpeg_test(stbi__context *s)
				{
					int r;
					stbi__jpeg j;
					j.s = s;
					stbi__setup_jpeg(&j);
					r = stbi__decode_jpeg_header(&j, STBI__SCAN_type);
					stbi__rewind(s);
					return r;
				}

				int stbi__jpeg_info_raw(stbi__jpeg *j, int *x, int *y, int *comp)
				{
					if (!stbi__decode_jpeg_header(j, STBI__SCAN_header)) {
						stbi__rewind(j->s);
						return 0;
					}
					if (x) *x = j->s->img_x;
					if (y) *y = j->s->img_y;
					if (comp) *comp = j->s->img_n;
					return 1;
				}

				int stbi__jpeg_info(stbi__context *s, int *x, int *y, int *comp)
				{
					stbi__jpeg j;
					j.s = s;
					return stbi__jpeg_info_raw(&j, x, y, comp);
				}
#endif

				// public domain zlib decode    v0.2  Sean Barrett 2006-11-18
				//    simple implementation
				//      - all input must be provided in an upfront buffer
				//      - all output is written to a single output buffer (can malloc/realloc)
				//    performance
				//      - fast huffman

#ifndef STBI_NO_ZLIB

				// fast-way is faster to check than jpeg huffman, but slow way is slower
#define STBI__ZFAST_BITS  9 // accelerate all cases in default tables
#define STBI__ZFAST_MASK  ((1 << STBI__ZFAST_BITS) - 1)

				// zlib-style huffman encoding
				// (jpegs packs from left, zlib from right, so can't share code)
				typedef struct
				{
					stbi__uint16 fast[1 << STBI__ZFAST_BITS];
					stbi__uint16 firstcode[16];
					int maxcode[17];
					stbi__uint16 firstsymbol[16];
					stbi_uc  size[288];
					stbi__uint16 value[288];
				} stbi__zhuffman;

				stbi_inline  int stbi__bitreverse16(int n)
				{
					n = ((n & 0xAAAA) >> 1) | ((n & 0x5555) << 1);
					n = ((n & 0xCCCC) >> 2) | ((n & 0x3333) << 2);
					n = ((n & 0xF0F0) >> 4) | ((n & 0x0F0F) << 4);
					n = ((n & 0xFF00) >> 8) | ((n & 0x00FF) << 8);
					return n;
				}

				stbi_inline  int stbi__bit_reverse(int v, int bits)
				{
					STBI_ASSERT(bits <= 16);
					// to bit reverse n bits, reverse 16 and shift
					// e.g. 11 bits, bit reverse and shift away 5
					return stbi__bitreverse16(v) >> (16 - bits);
				}

				int stbi__zbuild_huffman(stbi__zhuffman *z, stbi_uc *sizelist, int num)
				{
					int i, k = 0;
					int code, next_code[16], sizes[17];

					// DEFLATE spec for generating codes
					memset(sizes, 0, sizeof(sizes));
					memset(z->fast, 0, sizeof(z->fast));
					for (i = 0; i < num; ++i)
						++sizes[sizelist[i]];
					sizes[0] = 0;
					for (i = 1; i < 16; ++i)
					if (sizes[i] >(1 << i))
						return stbi__err("bad sizes", "Corrupt PNG");
					code = 0;
					for (i = 1; i < 16; ++i) {
						next_code[i] = code;
						z->firstcode[i] = (stbi__uint16)code;
						z->firstsymbol[i] = (stbi__uint16)k;
						code = (code + sizes[i]);
						if (sizes[i])
						if (code - 1 >= (1 << i)) return stbi__err("bad codelengths", "Corrupt PNG");
						z->maxcode[i] = code << (16 - i); // preshift for inner loop
						code <<= 1;
						k += sizes[i];
					}
					z->maxcode[16] = 0x10000; // sentinel
					for (i = 0; i < num; ++i) {
						int s = sizelist[i];
						if (s) {
							int c = next_code[s] - z->firstcode[s] + z->firstsymbol[s];
							stbi__uint16 fastv = (stbi__uint16)((s << 9) | i);
							z->size[c] = (stbi_uc)s;
							z->value[c] = (stbi__uint16)i;
							if (s <= STBI__ZFAST_BITS) {
								int j = stbi__bit_reverse(next_code[s], s);
								while (j < (1 << STBI__ZFAST_BITS)) {
									z->fast[j] = fastv;
									j += (1 << s);
								}
							}
							++next_code[s];
						}
					}
					return 1;
				}

				// zlib-from-memory implementation for PNG reading
				//    because PNG allows splitting the zlib stream arbitrarily,
				//    and it's annoying structurally to have PNG call ZLIB call PNG,
				//    we require PNG read all the IDATs and combine them into a single
				//    memory buffer

				typedef struct
				{
					stbi_uc *zbuffer, *zbuffer_end;
					int num_bits;
					stbi__uint32 code_buffer;

					char *zout;
					char *zout_start;
					char *zout_end;
					int   z_expandable;

					stbi__zhuffman z_length, z_distance;
				} stbi__zbuf;

				stbi_inline  stbi_uc stbi__zget8(stbi__zbuf *z)
				{
					if (z->zbuffer >= z->zbuffer_end) return 0;
					return *z->zbuffer++;
				}

				void stbi__fill_bits(stbi__zbuf *z)
				{
					do {
						STBI_ASSERT(z->code_buffer < (1U << z->num_bits));
						z->code_buffer |= (unsigned int)stbi__zget8(z) << z->num_bits;
						z->num_bits += 8;
					} while (z->num_bits <= 24);
				}

				stbi_inline  unsigned int stbi__zreceive(stbi__zbuf *z, int n)
				{
					unsigned int k;
					if (z->num_bits < n) stbi__fill_bits(z);
					k = z->code_buffer & ((1 << n) - 1);
					z->code_buffer >>= n;
					z->num_bits -= n;
					return k;
				}

				int stbi__zhuffman_decode_slowpath(stbi__zbuf *a, stbi__zhuffman *z)
				{
					int b, s, k;
					// not resolved by fast table, so compute it the slow way
					// use jpeg approach, which requires MSbits at top
					k = stbi__bit_reverse(a->code_buffer, 16);
					for (s = STBI__ZFAST_BITS + 1;; ++s)
					if (k < z->maxcode[s])
						break;
					if (s == 16) return -1; // invalid code!
					// code size is s, so:
					b = (k >> (16 - s)) - z->firstcode[s] + z->firstsymbol[s];
					STBI_ASSERT(z->size[b] == s);
					a->code_buffer >>= s;
					a->num_bits -= s;
					return z->value[b];
				}

				stbi_inline  int stbi__zhuffman_decode(stbi__zbuf *a, stbi__zhuffman *z)
				{
					int b, s;
					if (a->num_bits < 16) stbi__fill_bits(a);
					b = z->fast[a->code_buffer & STBI__ZFAST_MASK];
					if (b) {
						s = b >> 9;
						a->code_buffer >>= s;
						a->num_bits -= s;
						return b & 511;
					}
					return stbi__zhuffman_decode_slowpath(a, z);
				}

				int stbi__zexpand(stbi__zbuf *z, char *zout, int n)  // need to make room for n bytes
				{
					char *q;
					int cur, limit;
					z->zout = zout;
					if (!z->z_expandable) return stbi__err("output buffer limit", "Corrupt PNG");
					cur = (int)(z->zout - z->zout_start);
					limit = (int)(z->zout_end - z->zout_start);
					while (cur + n > limit)
						limit *= 2;
					q = (char *)STBI_REALLOC(z->zout_start, limit);
					if (q == NULL) return stbi__err("outofmem", "Out of memory");
					z->zout_start = q;
					z->zout = q + cur;
					z->zout_end = q + limit;
					return 1;
				}

				int stbi__zlength_base[31] = {
					3, 4, 5, 6, 7, 8, 9, 10, 11, 13,
					15, 17, 19, 23, 27, 31, 35, 43, 51, 59,
					67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0 };

				int stbi__zlength_extra[31] =
				{ 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 0, 0 };

				int stbi__zdist_base[32] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
					257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577, 0, 0 };

				int stbi__zdist_extra[32] =
				{ 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };

				int stbi__parse_huffman_block(stbi__zbuf *a)
				{
					char *zout = a->zout;
					for (;;) {
						int z = stbi__zhuffman_decode(a, &a->z_length);
						if (z < 256) {
							if (z < 0) return stbi__err("bad huffman code", "Corrupt PNG"); // error in huffman codes
							if (zout >= a->zout_end) {
								if (!stbi__zexpand(a, zout, 1)) return 0;
								zout = a->zout;
							}
							*zout++ = (char)z;
						}
						else {
							stbi_uc *p;
							int len, dist;
							if (z == 256) {
								a->zout = zout;
								return 1;
							}
							z -= 257;
							len = stbi__zlength_base[z];
							if (stbi__zlength_extra[z]) len += stbi__zreceive(a, stbi__zlength_extra[z]);
							z = stbi__zhuffman_decode(a, &a->z_distance);
							if (z < 0) return stbi__err("bad huffman code", "Corrupt PNG");
							dist = stbi__zdist_base[z];
							if (stbi__zdist_extra[z]) dist += stbi__zreceive(a, stbi__zdist_extra[z]);
							if (zout - a->zout_start < dist) return stbi__err("bad dist", "Corrupt PNG");
							if (zout + len > a->zout_end) {
								if (!stbi__zexpand(a, zout, len)) return 0;
								zout = a->zout;
							}
							p = (stbi_uc *)(zout - dist);
							if (dist == 1) { // run of one byte; common in images.
								stbi_uc v = *p;
								if (len) { do *zout++ = v; while (--len); }
							}
							else {
								if (len) { do *zout++ = *p++; while (--len); }
							}
						}
					}
				}

				int stbi__compute_huffman_codes(stbi__zbuf *a)
				{
					stbi_uc length_dezigzag[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
					stbi__zhuffman z_codelength;
					stbi_uc lencodes[286 + 32 + 137];//padding for maximum single op
					stbi_uc codelength_sizes[19];
					int i, n;

					int hlit = stbi__zreceive(a, 5) + 257;
					int hdist = stbi__zreceive(a, 5) + 1;
					int hclen = stbi__zreceive(a, 4) + 4;

					memset(codelength_sizes, 0, sizeof(codelength_sizes));
					for (i = 0; i < hclen; ++i) {
						int s = stbi__zreceive(a, 3);
						codelength_sizes[length_dezigzag[i]] = (stbi_uc)s;
					}
					if (!stbi__zbuild_huffman(&z_codelength, codelength_sizes, 19)) return 0;

					n = 0;
					while (n < hlit + hdist) {
						int c = stbi__zhuffman_decode(a, &z_codelength);
						if (c < 0 || c >= 19) return stbi__err("bad codelengths", "Corrupt PNG");
						if (c < 16)
							lencodes[n++] = (stbi_uc)c;
						else if (c == 16) {
							c = stbi__zreceive(a, 2) + 3;
							memset(lencodes + n, lencodes[n - 1], c);
							n += c;
						}
						else if (c == 17) {
							c = stbi__zreceive(a, 3) + 3;
							memset(lencodes + n, 0, c);
							n += c;
						}
						else {
							STBI_ASSERT(c == 18);
							c = stbi__zreceive(a, 7) + 11;
							memset(lencodes + n, 0, c);
							n += c;
						}
					}
					if (n != hlit + hdist) return stbi__err("bad codelengths", "Corrupt PNG");
					if (!stbi__zbuild_huffman(&a->z_length, lencodes, hlit)) return 0;
					if (!stbi__zbuild_huffman(&a->z_distance, lencodes + hlit, hdist)) return 0;
					return 1;
				}

				int stbi__parse_uncomperssed_block(stbi__zbuf *a)
				{
					stbi_uc header[4];
					int len, nlen, k;
					if (a->num_bits & 7)
						stbi__zreceive(a, a->num_bits & 7); // discard
					// drain the bit-packed data into header
					k = 0;
					while (a->num_bits > 0) {
						header[k++] = (stbi_uc)(a->code_buffer & 255); // suppress MSVC run-time check
						a->code_buffer >>= 8;
						a->num_bits -= 8;
					}
					STBI_ASSERT(a->num_bits == 0);
					// now fill header the normal way
					while (k < 4)
						header[k++] = stbi__zget8(a);
					len = header[1] * 256 + header[0];
					nlen = header[3] * 256 + header[2];
					if (nlen != (len ^ 0xffff)) return stbi__err("zlib corrupt", "Corrupt PNG");
					if (a->zbuffer + len > a->zbuffer_end) return stbi__err("read past buffer", "Corrupt PNG");
					if (a->zout + len > a->zout_end)
					if (!stbi__zexpand(a, a->zout, len)) return 0;
					memcpy(a->zout, a->zbuffer, len);
					a->zbuffer += len;
					a->zout += len;
					return 1;
				}

				int stbi__parse_zlib_header(stbi__zbuf *a)
				{
					int cmf = stbi__zget8(a);
					int cm = cmf & 15;
					/* int cinfo = cmf >> 4; */
					int flg = stbi__zget8(a);
					if ((cmf * 256 + flg) % 31 != 0) return stbi__err("bad zlib header", "Corrupt PNG"); // zlib spec
					if (flg & 32) return stbi__err("no preset dict", "Corrupt PNG"); // preset dictionary not allowed in png
					if (cm != 8) return stbi__err("bad compression", "Corrupt PNG"); // DEFLATE required for png
					// window = 1 << (8 + cinfo)... but who cares, we fully buffer output
					return 1;
				}

				// @TODO: should statically initialize these for optimal thread safety
				stbi_uc stbi__zdefault_length[288], stbi__zdefault_distance[32];
				void stbi__init_zdefaults(void)
				{
					int i;   // use <= to match clearly with spec
					for (i = 0; i <= 143; ++i)     stbi__zdefault_length[i] = 8;
					for (; i <= 255; ++i)     stbi__zdefault_length[i] = 9;
					for (; i <= 279; ++i)     stbi__zdefault_length[i] = 7;
					for (; i <= 287; ++i)     stbi__zdefault_length[i] = 8;

					for (i = 0; i <= 31; ++i)     stbi__zdefault_distance[i] = 5;
				}

				int stbi__parse_zlib(stbi__zbuf *a, int parse_header)
				{
					int final, type;
					if (parse_header)
					if (!stbi__parse_zlib_header(a)) return 0;
					a->num_bits = 0;
					a->code_buffer = 0;
					do {
						final = stbi__zreceive(a, 1);
						type = stbi__zreceive(a, 2);
						if (type == 0) {
							if (!stbi__parse_uncomperssed_block(a)) return 0;
						}
						else if (type == 3) {
							return 0;
						}
						else {
							if (type == 1) {
								// use fixed code lengths
								if (!stbi__zdefault_distance[31]) stbi__init_zdefaults();
								if (!stbi__zbuild_huffman(&a->z_length, stbi__zdefault_length, 288)) return 0;
								if (!stbi__zbuild_huffman(&a->z_distance, stbi__zdefault_distance, 32)) return 0;
							}
							else {
								if (!stbi__compute_huffman_codes(a)) return 0;
							}
							if (!stbi__parse_huffman_block(a)) return 0;
						}
					} while (!final);
					return 1;
				}

				int stbi__do_zlib(stbi__zbuf *a, char *obuf, int olen, int exp, int parse_header)
				{
					a->zout_start = obuf;
					a->zout = obuf;
					a->zout_end = obuf + olen;
					a->z_expandable = exp;

					return stbi__parse_zlib(a, parse_header);
				}

				char *stbi_zlib_decode_malloc_guesssize(const char *buffer, int len, int initial_size, int *outlen)
				{
					stbi__zbuf a;
					char *p = (char *)stbi__malloc(initial_size);
					if (p == NULL) return NULL;
					a.zbuffer = (stbi_uc *)buffer;
					a.zbuffer_end = (stbi_uc *)buffer + len;
					if (stbi__do_zlib(&a, p, initial_size, 1, 1)) {
						if (outlen) *outlen = (int)(a.zout - a.zout_start);
						return a.zout_start;
					}
					else {
						STBI_FREE(a.zout_start);
						return NULL;
					}
				}

				char *stbi_zlib_decode_malloc(char const *buffer, int len, int *outlen)
				{
					return stbi_zlib_decode_malloc_guesssize(buffer, len, 16384, outlen);
				}

				char *stbi_zlib_decode_malloc_guesssize_headerflag(const char *buffer, int len, int initial_size, int *outlen, int parse_header)
				{
					stbi__zbuf a;
					char *p = (char *)stbi__malloc(initial_size);
					if (p == NULL) return NULL;
					a.zbuffer = (stbi_uc *)buffer;
					a.zbuffer_end = (stbi_uc *)buffer + len;
					if (stbi__do_zlib(&a, p, initial_size, 1, parse_header)) {
						if (outlen) *outlen = (int)(a.zout - a.zout_start);
						return a.zout_start;
					}
					else {
						STBI_FREE(a.zout_start);
						return NULL;
					}
				}

				int stbi_zlib_decode_buffer(char *obuffer, int olen, char const *ibuffer, int ilen)
				{
					stbi__zbuf a;
					a.zbuffer = (stbi_uc *)ibuffer;
					a.zbuffer_end = (stbi_uc *)ibuffer + ilen;
					if (stbi__do_zlib(&a, obuffer, olen, 0, 1))
						return (int)(a.zout - a.zout_start);
					else
						return -1;
				}

				char *stbi_zlib_decode_noheader_malloc(char const *buffer, int len, int *outlen)
				{
					stbi__zbuf a;
					char *p = (char *)stbi__malloc(16384);
					if (p == NULL) return NULL;
					a.zbuffer = (stbi_uc *)buffer;
					a.zbuffer_end = (stbi_uc *)buffer + len;
					if (stbi__do_zlib(&a, p, 16384, 1, 0)) {
						if (outlen) *outlen = (int)(a.zout - a.zout_start);
						return a.zout_start;
					}
					else {
						STBI_FREE(a.zout_start);
						return NULL;
					}
				}

				int stbi_zlib_decode_noheader_buffer(char *obuffer, int olen, const char *ibuffer, int ilen)
				{
					stbi__zbuf a;
					a.zbuffer = (stbi_uc *)ibuffer;
					a.zbuffer_end = (stbi_uc *)ibuffer + ilen;
					if (stbi__do_zlib(&a, obuffer, olen, 0, 0))
						return (int)(a.zout - a.zout_start);
					else
						return -1;
				}
#endif

				// public domain "baseline" PNG decoder   v0.10  Sean Barrett 2006-11-18
				//    simple implementation
				//      - only 8-bit samples
				//      - no CRC checking
				//      - allocates lots of intermediate memory
				//        - avoids problem of streaming data between subsystems
				//        - avoids explicit window management
				//    performance
				//      - uses stb_zlib, a PD zlib implementation with fast huffman decoding

#ifndef STBI_NO_PNG
				typedef struct
				{
					stbi__uint32 length;
					stbi__uint32 type;
				} stbi__pngchunk;

				stbi__pngchunk stbi__get_chunk_header(stbi__context *s)
				{
					stbi__pngchunk c;
					c.length = stbi__get32be(s);
					c.type = stbi__get32be(s);
					return c;
				}

				int stbi__check_png_header(stbi__context *s)
				{
					stbi_uc png_sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
					int i;
					for (i = 0; i < 8; ++i)
					if (stbi__get8(s) != png_sig[i]) return stbi__err("bad png sig", "Not a PNG");
					return 1;
				}

				typedef struct
				{
					stbi__context *s;
					stbi_uc *idata, *expanded, *out;
				} stbi__png;


				enum {
					STBI__F_none = 0,
					STBI__F_sub = 1,
					STBI__F_up = 2,
					STBI__F_avg = 3,
					STBI__F_paeth = 4,
					// synthetic filters used for first scanline to avoid needing a dummy row of 0s
					STBI__F_avg_first,
					STBI__F_paeth_first
				};

				stbi_uc first_row_filter[5] =
				{
					STBI__F_none,
					STBI__F_sub,
					STBI__F_none,
					STBI__F_avg_first,
					STBI__F_paeth_first
				};

				int stbi__paeth(int a, int b, int c)
				{
					int p = a + b - c;
					int pa = abs(p - a);
					int pb = abs(p - b);
					int pc = abs(p - c);
					if (pa <= pb && pa <= pc) return a;
					if (pb <= pc) return b;
					return c;
				}

				stbi_uc stbi__depth_scale_table[9] = { 0, 0xff, 0x55, 0, 0x11, 0, 0, 0, 0x01 };

				// create the png data from post-deflated data
				int stbi__create_png_image_raw(stbi__png *a, stbi_uc *raw, stbi__uint32 raw_len, int out_n, stbi__uint32 x, stbi__uint32 y, int depth, int color)
				{
					stbi__context *s = a->s;
					stbi__uint32 i, j, stride = x*out_n;
					stbi__uint32 img_len, img_width_bytes;
					int k;
					int img_n = s->img_n; // copy it into a local for later

					STBI_ASSERT(out_n == s->img_n || out_n == s->img_n + 1);
					a->out = (stbi_uc *)stbi__malloc(x * y * out_n); // extra bytes to write off the end into
					if (!a->out) return stbi__err("outofmem", "Out of memory");

					img_width_bytes = (((img_n * x * depth) + 7) >> 3);
					img_len = (img_width_bytes + 1) * y;
					if (s->img_x == x && s->img_y == y) {
						if (raw_len != img_len) return stbi__err("not enough pixels", "Corrupt PNG");
					}
					else { // interlaced:
						if (raw_len < img_len) return stbi__err("not enough pixels", "Corrupt PNG");
					}

					for (j = 0; j < y; ++j) {
						stbi_uc *cur = a->out + stride*j;
						stbi_uc *prior = cur - stride;
						int filter = *raw++;
						int filter_bytes = img_n;
						int width = x;
						if (filter > 4)
							return stbi__err("invalid filter", "Corrupt PNG");

						if (depth < 8) {
							STBI_ASSERT(img_width_bytes <= x);
							cur += x*out_n - img_width_bytes; // store output to the rightmost img_len bytes, so we can decode in place
							filter_bytes = 1;
							width = img_width_bytes;
						}

						// if first row, use special filter that doesn't sample previous row
						if (j == 0) filter = first_row_filter[filter];

						// handle first byte explicitly
						for (k = 0; k < filter_bytes; ++k) {
							switch (filter) {
							case STBI__F_none: cur[k] = raw[k]; break;
							case STBI__F_sub: cur[k] = raw[k]; break;
							case STBI__F_up: cur[k] = STBI__BYTECAST(raw[k] + prior[k]); break;
							case STBI__F_avg: cur[k] = STBI__BYTECAST(raw[k] + (prior[k] >> 1)); break;
							case STBI__F_paeth: cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(0, prior[k], 0)); break;
							case STBI__F_avg_first: cur[k] = raw[k]; break;
							case STBI__F_paeth_first: cur[k] = raw[k]; break;
							}
						}

						if (depth == 8) {
							if (img_n != out_n)
								cur[img_n] = 255; // first pixel
							raw += img_n;
							cur += out_n;
							prior += out_n;
						}
						else {
							raw += 1;
							cur += 1;
							prior += 1;
						}

						// this is a little gross, so that we don't switch per-pixel or per-component
						if (depth < 8 || img_n == out_n) {
							int nk = (width - 1)*img_n;
#define CASE(f) \
					case f:     \
							for (k = 0; k < nk; ++k)
							switch (filter) {
								// "none" filter turns into a memcpy here; make that explicit.
							case STBI__F_none:         memcpy(cur, raw, nk); break;
								CASE(STBI__F_sub)          cur[k] = STBI__BYTECAST(raw[k] + cur[k - filter_bytes]); break;
								CASE(STBI__F_up)           cur[k] = STBI__BYTECAST(raw[k] + prior[k]); break;
								CASE(STBI__F_avg)          cur[k] = STBI__BYTECAST(raw[k] + ((prior[k] + cur[k - filter_bytes]) >> 1)); break;
								CASE(STBI__F_paeth)        cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(cur[k - filter_bytes], prior[k], prior[k - filter_bytes])); break;
								CASE(STBI__F_avg_first)    cur[k] = STBI__BYTECAST(raw[k] + (cur[k - filter_bytes] >> 1)); break;
								CASE(STBI__F_paeth_first)  cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(cur[k - filter_bytes], 0, 0)); break;
							}
#undef CASE
							raw += nk;
						}
						else {
							STBI_ASSERT(img_n + 1 == out_n);
#define CASE(f) \
					case f:     \
							for (i = x - 1; i >= 1; --i, cur[img_n] = 255, raw += img_n, cur += out_n, prior += out_n) \
							for (k = 0; k < img_n; ++k)
							switch (filter) {
								CASE(STBI__F_none)         cur[k] = raw[k]; break;
								CASE(STBI__F_sub)          cur[k] = STBI__BYTECAST(raw[k] + cur[k - out_n]); break;
								CASE(STBI__F_up)           cur[k] = STBI__BYTECAST(raw[k] + prior[k]); break;
								CASE(STBI__F_avg)          cur[k] = STBI__BYTECAST(raw[k] + ((prior[k] + cur[k - out_n]) >> 1)); break;
								CASE(STBI__F_paeth)        cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(cur[k - out_n], prior[k], prior[k - out_n])); break;
								CASE(STBI__F_avg_first)    cur[k] = STBI__BYTECAST(raw[k] + (cur[k - out_n] >> 1)); break;
								CASE(STBI__F_paeth_first)  cur[k] = STBI__BYTECAST(raw[k] + stbi__paeth(cur[k - out_n], 0, 0)); break;
							}
#undef CASE
						}
					}

					// we make a separate pass to expand bits to pixels; for performance,
					// this could run two scanlines behind the above code, so it won't
					// intefere with filtering but will still be in the cache.
					if (depth < 8) {
						for (j = 0; j < y; ++j) {
							stbi_uc *cur = a->out + stride*j;
							stbi_uc *in = a->out + stride*j + x*out_n - img_width_bytes;
							// unpack 1/2/4-bit into a 8-bit buffer. allows us to keep the common 8-bit path optimal at minimal cost for 1/2/4-bit
							// png guarante byte alignment, if width is not multiple of 8/4/2 we'll decode dummy trailing data that will be skipped in the later loop
							stbi_uc scale = (color == 0) ? stbi__depth_scale_table[depth] : 1; // scale grayscale values to 0..255 range

							// note that the final byte might overshoot and write more data than desired.
							// we can allocate enough data that this never writes out of memory, but it
							// could also overwrite the next scanline. can it overwrite non-empty data
							// on the next scanline? yes, consider 1-pixel-wide scanlines with 1-bit-per-pixel.
							// so we need to explicitly clamp the final ones

							if (depth == 4) {
								for (k = x*img_n; k >= 2; k -= 2, ++in) {
									*cur++ = scale * ((*in >> 4));
									*cur++ = scale * ((*in) & 0x0f);
								}
								if (k > 0) *cur++ = scale * ((*in >> 4));
							}
							else if (depth == 2) {
								for (k = x*img_n; k >= 4; k -= 4, ++in) {
									*cur++ = scale * ((*in >> 6));
									*cur++ = scale * ((*in >> 4) & 0x03);
									*cur++ = scale * ((*in >> 2) & 0x03);
									*cur++ = scale * ((*in) & 0x03);
								}
								if (k > 0) *cur++ = scale * ((*in >> 6));
								if (k > 1) *cur++ = scale * ((*in >> 4) & 0x03);
								if (k > 2) *cur++ = scale * ((*in >> 2) & 0x03);
							}
							else if (depth == 1) {
								for (k = x*img_n; k >= 8; k -= 8, ++in) {
									*cur++ = scale * ((*in >> 7));
									*cur++ = scale * ((*in >> 6) & 0x01);
									*cur++ = scale * ((*in >> 5) & 0x01);
									*cur++ = scale * ((*in >> 4) & 0x01);
									*cur++ = scale * ((*in >> 3) & 0x01);
									*cur++ = scale * ((*in >> 2) & 0x01);
									*cur++ = scale * ((*in >> 1) & 0x01);
									*cur++ = scale * ((*in) & 0x01);
								}
								if (k > 0) *cur++ = scale * ((*in >> 7));
								if (k > 1) *cur++ = scale * ((*in >> 6) & 0x01);
								if (k > 2) *cur++ = scale * ((*in >> 5) & 0x01);
								if (k > 3) *cur++ = scale * ((*in >> 4) & 0x01);
								if (k > 4) *cur++ = scale * ((*in >> 3) & 0x01);
								if (k > 5) *cur++ = scale * ((*in >> 2) & 0x01);
								if (k > 6) *cur++ = scale * ((*in >> 1) & 0x01);
							}
							if (img_n != out_n) {
								int q;
								// insert alpha = 255
								cur = a->out + stride*j;
								if (img_n == 1) {
									for (q = x - 1; q >= 0; --q) {
										cur[q * 2 + 1] = 255;
										cur[q * 2 + 0] = cur[q];
									}
								}
								else {
									STBI_ASSERT(img_n == 3);
									for (q = x - 1; q >= 0; --q) {
										cur[q * 4 + 3] = 255;
										cur[q * 4 + 2] = cur[q * 3 + 2];
										cur[q * 4 + 1] = cur[q * 3 + 1];
										cur[q * 4 + 0] = cur[q * 3 + 0];
									}
								}
							}
						}
					}

					return 1;
				}

				int stbi__create_png_image(stbi__png *a, stbi_uc *image_data, stbi__uint32 image_data_len, int out_n, int depth, int color, int interlaced)
				{
					stbi_uc *final;
					int p;
					if (!interlaced)
						return stbi__create_png_image_raw(a, image_data, image_data_len, out_n, a->s->img_x, a->s->img_y, depth, color);

					// de-interlacing
					final = (stbi_uc *)stbi__malloc(a->s->img_x * a->s->img_y * out_n);
					for (p = 0; p < 7; ++p) {
						int xorig[] = { 0, 4, 0, 2, 0, 1, 0 };
						int yorig[] = { 0, 0, 4, 0, 2, 0, 1 };
						int xspc[] = { 8, 8, 4, 4, 2, 2, 1 };
						int yspc[] = { 8, 8, 8, 4, 4, 2, 2 };
						int i, j, x, y;
						// pass1_x[4] = 0, pass1_x[5] = 1, pass1_x[12] = 1
						x = (a->s->img_x - xorig[p] + xspc[p] - 1) / xspc[p];
						y = (a->s->img_y - yorig[p] + yspc[p] - 1) / yspc[p];
						if (x && y) {
							stbi__uint32 img_len = ((((a->s->img_n * x * depth) + 7) >> 3) + 1) * y;
							if (!stbi__create_png_image_raw(a, image_data, image_data_len, out_n, x, y, depth, color)) {
								STBI_FREE(final);
								return 0;
							}
							for (j = 0; j < y; ++j) {
								for (i = 0; i < x; ++i) {
									int out_y = j*yspc[p] + yorig[p];
									int out_x = i*xspc[p] + xorig[p];
									memcpy(final + out_y*a->s->img_x*out_n + out_x*out_n,
										a->out + (j*x + i)*out_n, out_n);
								}
							}
							STBI_FREE(a->out);
							image_data += img_len;
							image_data_len -= img_len;
						}
					}
					a->out = final;

					return 1;
				}

				int stbi__compute_transparency(stbi__png *z, stbi_uc tc[3], int out_n)
				{
					stbi__context *s = z->s;
					stbi__uint32 i, pixel_count = s->img_x * s->img_y;
					stbi_uc *p = z->out;

					// compute color-based transparency, assuming we've
					// already got 255 as the alpha value in the output
					STBI_ASSERT(out_n == 2 || out_n == 4);

					if (out_n == 2) {
						for (i = 0; i < pixel_count; ++i) {
							p[1] = (p[0] == tc[0] ? 0 : 255);
							p += 2;
						}
					}
					else {
						for (i = 0; i < pixel_count; ++i) {
							if (p[0] == tc[0] && p[1] == tc[1] && p[2] == tc[2])
								p[3] = 0;
							p += 4;
						}
					}
					return 1;
				}

				int stbi__expand_png_palette(stbi__png *a, stbi_uc *palette, int len, int pal_img_n)
				{
					stbi__uint32 i, pixel_count = a->s->img_x * a->s->img_y;
					stbi_uc *p, *temp_out, *orig = a->out;

					p = (stbi_uc *)stbi__malloc(pixel_count * pal_img_n);
					if (p == NULL) return stbi__err("outofmem", "Out of memory");

					// between here and free(out) below, exitting would leak
					temp_out = p;

					if (pal_img_n == 3) {
						for (i = 0; i < pixel_count; ++i) {
							int n = orig[i] * 4;
							p[0] = palette[n];
							p[1] = palette[n + 1];
							p[2] = palette[n + 2];
							p += 3;
						}
					}
					else {
						for (i = 0; i < pixel_count; ++i) {
							int n = orig[i] * 4;
							p[0] = palette[n];
							p[1] = palette[n + 1];
							p[2] = palette[n + 2];
							p[3] = palette[n + 3];
							p += 4;
						}
					}
					STBI_FREE(a->out);
					a->out = temp_out;

					STBI_NOTUSED(len);

					return 1;
				}

				int stbi__unpremultiply_on_load = 0;
				int stbi__de_iphone_flag = 0;

				void stbi_set_unpremultiply_on_load(int flag_true_if_should_unpremultiply)
				{
					stbi__unpremultiply_on_load = flag_true_if_should_unpremultiply;
				}

				void stbi_convert_iphone_png_to_rgb(int flag_true_if_should_convert)
				{
					stbi__de_iphone_flag = flag_true_if_should_convert;
				}

				void stbi__de_iphone(stbi__png *z)
				{
					stbi__context *s = z->s;
					stbi__uint32 i, pixel_count = s->img_x * s->img_y;
					stbi_uc *p = z->out;

					if (s->img_out_n == 3) {  // convert bgr to rgb
						for (i = 0; i < pixel_count; ++i) {
							stbi_uc t = p[0];
							p[0] = p[2];
							p[2] = t;
							p += 3;
						}
					}
					else {
						STBI_ASSERT(s->img_out_n == 4);
						if (stbi__unpremultiply_on_load) {
							// convert bgr to rgb and unpremultiply
							for (i = 0; i < pixel_count; ++i) {
								stbi_uc a = p[3];
								stbi_uc t = p[0];
								if (a) {
									p[0] = p[2] * 255 / a;
									p[1] = p[1] * 255 / a;
									p[2] = t * 255 / a;
								}
								else {
									p[0] = p[2];
									p[2] = t;
								}
								p += 4;
							}
						}
						else {
							// convert bgr to rgb
							for (i = 0; i < pixel_count; ++i) {
								stbi_uc t = p[0];
								p[0] = p[2];
								p[2] = t;
								p += 4;
							}
						}
					}
				}

#define STBI__PNG_TYPE(a,b,c,d)  (((a) << 24) + ((b) << 16) + ((c) << 8) + (d))

				int stbi__parse_png_file(stbi__png *z, int scan, int req_comp)
				{
					stbi_uc palette[1024], pal_img_n = 0;
					stbi_uc has_trans = 0, tc[3];
					stbi__uint32 ioff = 0, idata_limit = 0, i, pal_len = 0;
					int first = 1, k, interlace = 0, color = 0, depth = 0, is_iphone = 0;
					stbi__context *s = z->s;

					z->expanded = NULL;
					z->idata = NULL;
					z->out = NULL;

					if (!stbi__check_png_header(s)) return 0;

					if (scan == STBI__SCAN_type) return 1;

					for (;;) {
						stbi__pngchunk c = stbi__get_chunk_header(s);
						switch (c.type) {
						case STBI__PNG_TYPE('C', 'g', 'B', 'I'):
							is_iphone = 1;
							stbi__skip(s, c.length);
							break;
						case STBI__PNG_TYPE('I', 'H', 'D', 'R'): {
																	 int comp, filter;
																	 if (!first) return stbi__err("multiple IHDR", "Corrupt PNG");
																	 first = 0;
																	 if (c.length != 13) return stbi__err("bad IHDR len", "Corrupt PNG");
																	 s->img_x = stbi__get32be(s); if (s->img_x > (1 << 24)) return stbi__err("too large", "Very large image (corrupt?)");
																	 s->img_y = stbi__get32be(s); if (s->img_y > (1 << 24)) return stbi__err("too large", "Very large image (corrupt?)");
																	 depth = stbi__get8(s);  if (depth != 1 && depth != 2 && depth != 4 && depth != 8)  return stbi__err("1/2/4/8-bit only", "PNG not supported: 1/2/4/8-bit only");
																	 color = stbi__get8(s);  if (color > 6)         return stbi__err("bad ctype", "Corrupt PNG");
																	 if (color == 3) pal_img_n = 3; else if (color & 1) return stbi__err("bad ctype", "Corrupt PNG");
																	 comp = stbi__get8(s);  if (comp) return stbi__err("bad comp method", "Corrupt PNG");
																	 filter = stbi__get8(s);  if (filter) return stbi__err("bad filter method", "Corrupt PNG");
																	 interlace = stbi__get8(s); if (interlace > 1) return stbi__err("bad interlace method", "Corrupt PNG");
																	 if (!s->img_x || !s->img_y) return stbi__err("0-pixel image", "Corrupt PNG");
																	 if (!pal_img_n) {
																		 s->img_n = (color & 2 ? 3 : 1) + (color & 4 ? 1 : 0);
																		 if ((1 << 30) / s->img_x / s->img_n < s->img_y) return stbi__err("too large", "Image too large to decode");
																		 if (scan == STBI__SCAN_header) return 1;
																	 }
																	 else {
																		 // if paletted, then pal_n is our final components, and
																		 // img_n is # components to decompress/filter.
																		 s->img_n = 1;
																		 if ((1 << 30) / s->img_x / 4 < s->img_y) return stbi__err("too large", "Corrupt PNG");
																		 // if SCAN_header, have to scan to see if we have a tRNS
																	 }
																	 break;
						}

						case STBI__PNG_TYPE('P', 'L', 'T', 'E'):  {
																	  if (first) return stbi__err("first not IHDR", "Corrupt PNG");
																	  if (c.length > 256 * 3) return stbi__err("invalid PLTE", "Corrupt PNG");
																	  pal_len = c.length / 3;
																	  if (pal_len * 3 != c.length) return stbi__err("invalid PLTE", "Corrupt PNG");
																	  for (i = 0; i < pal_len; ++i) {
																		  palette[i * 4 + 0] = stbi__get8(s);
																		  palette[i * 4 + 1] = stbi__get8(s);
																		  palette[i * 4 + 2] = stbi__get8(s);
																		  palette[i * 4 + 3] = 255;
																	  }
																	  break;
						}

						case STBI__PNG_TYPE('t', 'R', 'N', 'S'): {
																	 if (first) return stbi__err("first not IHDR", "Corrupt PNG");
																	 if (z->idata) return stbi__err("tRNS after IDAT", "Corrupt PNG");
																	 if (pal_img_n) {
																		 if (scan == STBI__SCAN_header) { s->img_n = 4; return 1; }
																		 if (pal_len == 0) return stbi__err("tRNS before PLTE", "Corrupt PNG");
																		 if (c.length > pal_len) return stbi__err("bad tRNS len", "Corrupt PNG");
																		 pal_img_n = 4;
																		 for (i = 0; i < c.length; ++i)
																			 palette[i * 4 + 3] = stbi__get8(s);
																	 }
																	 else {
																		 if (!(s->img_n & 1)) return stbi__err("tRNS with alpha", "Corrupt PNG");
																		 if (c.length != (stbi__uint32)s->img_n * 2) return stbi__err("bad tRNS len", "Corrupt PNG");
																		 has_trans = 1;
																		 for (k = 0; k < s->img_n; ++k)
																			 tc[k] = (stbi_uc)(stbi__get16be(s) & 255) * stbi__depth_scale_table[depth]; // non 8-bit images will be larger
																	 }
																	 break;
						}

						case STBI__PNG_TYPE('I', 'D', 'A', 'T'): {
																	 if (first) return stbi__err("first not IHDR", "Corrupt PNG");
																	 if (pal_img_n && !pal_len) return stbi__err("no PLTE", "Corrupt PNG");
																	 if (scan == STBI__SCAN_header) { s->img_n = pal_img_n; return 1; }
																	 if ((int)(ioff + c.length) < (int)ioff) return 0;
																	 if (ioff + c.length > idata_limit) {
																		 stbi_uc *p;
																		 if (idata_limit == 0) idata_limit = c.length > 4096 ? c.length : 4096;
																		 while (ioff + c.length > idata_limit)
																			 idata_limit *= 2;
																		 p = (stbi_uc *)STBI_REALLOC(z->idata, idata_limit); if (p == NULL) return stbi__err("outofmem", "Out of memory");
																		 z->idata = p;
																	 }
																	 if (!stbi__getn(s, z->idata + ioff, c.length)) return stbi__err("outofdata", "Corrupt PNG");
																	 ioff += c.length;
																	 break;
						}

						case STBI__PNG_TYPE('I', 'E', 'N', 'D'): {
																	 stbi__uint32 raw_len, bpl;
																	 if (first) return stbi__err("first not IHDR", "Corrupt PNG");
																	 if (scan != STBI__SCAN_load) return 1;
																	 if (z->idata == NULL) return stbi__err("no IDAT", "Corrupt PNG");
																	 // initial guess for decoded data size to avoid unnecessary reallocs
																	 bpl = (s->img_x * depth + 7) / 8; // bytes per line, per component
																	 raw_len = bpl * s->img_y * s->img_n /* pixels */ + s->img_y /* filter mode per row */;
																	 z->expanded = (stbi_uc *)stbi_zlib_decode_malloc_guesssize_headerflag((char *)z->idata, ioff, raw_len, (int *)&raw_len, !is_iphone);
																	 if (z->expanded == NULL) return 0; // zlib should set error
																	 STBI_FREE(z->idata); z->idata = NULL;
																	 if ((req_comp == s->img_n + 1 && req_comp != 3 && !pal_img_n) || has_trans)
																		 s->img_out_n = s->img_n + 1;
																	 else
																		 s->img_out_n = s->img_n;
																	 if (!stbi__create_png_image(z, z->expanded, raw_len, s->img_out_n, depth, color, interlace)) return 0;
																	 if (has_trans)
																	 if (!stbi__compute_transparency(z, tc, s->img_out_n)) return 0;
																	 if (is_iphone && stbi__de_iphone_flag && s->img_out_n > 2)
																		 stbi__de_iphone(z);
																	 if (pal_img_n) {
																		 // pal_img_n == 3 or 4
																		 s->img_n = pal_img_n; // record the actual colors we had
																		 s->img_out_n = pal_img_n;
																		 if (req_comp >= 3) s->img_out_n = req_comp;
																		 if (!stbi__expand_png_palette(z, palette, pal_len, s->img_out_n))
																			 return 0;
																	 }
																	 STBI_FREE(z->expanded); z->expanded = NULL;
																	 return 1;
						}

						default:
							// if critical, fail
							if (first) return stbi__err("first not IHDR", "Corrupt PNG");
							if ((c.type & (1 << 29)) == 0) {
#ifndef STBI_NO_FAILURE_STRINGS
								// not threadsafe
								char invalid_chunk[] = "XXXX PNG chunk not known";
								invalid_chunk[0] = STBI__BYTECAST(c.type >> 24);
								invalid_chunk[1] = STBI__BYTECAST(c.type >> 16);
								invalid_chunk[2] = STBI__BYTECAST(c.type >> 8);
								invalid_chunk[3] = STBI__BYTECAST(c.type >> 0);
#endif
								return stbi__err(invalid_chunk, "PNG not supported: unknown PNG chunk type");
							}
							stbi__skip(s, c.length);
							break;
						}
						// end of PNG chunk, read and skip CRC
						stbi__get32be(s);
					}
				}

				unsigned char *stbi__do_png(stbi__png *p, int *x, int *y, int *n, int req_comp)
				{
					unsigned char *result = NULL;
					if (req_comp < 0 || req_comp > 4) return stbi__errpuc("bad req_comp", "Internal error");
					if (stbi__parse_png_file(p, STBI__SCAN_load, req_comp)) {
						result = p->out;
						p->out = NULL;
						if (req_comp && req_comp != p->s->img_out_n) {
							result = stbi__convert_format(result, p->s->img_out_n, req_comp, p->s->img_x, p->s->img_y);
							p->s->img_out_n = req_comp;
							if (result == NULL) return result;
						}
						*x = p->s->img_x;
						*y = p->s->img_y;
						if (n) *n = p->s->img_out_n;
					}
					STBI_FREE(p->out);      p->out = NULL;
					STBI_FREE(p->expanded); p->expanded = NULL;
					STBI_FREE(p->idata);    p->idata = NULL;

					return result;
				}

				unsigned char *stbi__png_load(stbi__context *s, int *x, int *y, int *comp, int req_comp)
				{
					stbi__png p;
					p.s = s;
					return stbi__do_png(&p, x, y, comp, req_comp);
				}

				int stbi__png_test(stbi__context *s)
				{
					int r;
					r = stbi__check_png_header(s);
					stbi__rewind(s);
					return r;
				}

				int stbi__png_info_raw(stbi__png *p, int *x, int *y, int *comp)
				{
					if (!stbi__parse_png_file(p, STBI__SCAN_header, 0)) {
						stbi__rewind(p->s);
						return 0;
					}
					if (x) *x = p->s->img_x;
					if (y) *y = p->s->img_y;
					if (comp) *comp = p->s->img_n;
					return 1;
				}

				int stbi__png_info(stbi__context *s, int *x, int *y, int *comp)
				{
					stbi__png p;
					p.s = s;
					return stbi__png_info_raw(&p, x, y, comp);
				}
#endif

				// Microsoft/Windows BMP image

#ifndef STBI_NO_BMP
				int stbi__bmp_test_raw(stbi__context *s)
				{
					int r;
					int sz;
					if (stbi__get8(s) != 'B') return 0;
					if (stbi__get8(s) != 'M') return 0;
					stbi__get32le(s); // discard filesize
					stbi__get16le(s); // discard reserved
					stbi__get16le(s); // discard reserved
					stbi__get32le(s); // discard data offset
					sz = stbi__get32le(s);
					r = (sz == 12 || sz == 40 || sz == 56 || sz == 108 || sz == 124);
					return r;
				}

				int stbi__bmp_test(stbi__context *s)
				{
					int r = stbi__bmp_test_raw(s);
					stbi__rewind(s);
					return r;
				}


				// returns 0..31 for the highest set bit
				int stbi__high_bit(unsigned int z)
				{
					int n = 0;
					if (z == 0) return -1;
					if (z >= 0x10000) n += 16, z >>= 16;
					if (z >= 0x00100) n += 8, z >>= 8;
					if (z >= 0x00010) n += 4, z >>= 4;
					if (z >= 0x00004) n += 2, z >>= 2;
					if (z >= 0x00002) n += 1, z >>= 1;
					return n;
				}

				int stbi__bitcount(unsigned int a)
				{
					a = (a & 0x55555555) + ((a >> 1) & 0x55555555); // max 2
					a = (a & 0x33333333) + ((a >> 2) & 0x33333333); // max 4
					a = (a + (a >> 4)) & 0x0f0f0f0f; // max 8 per 4, now 8 bits
					a = (a + (a >> 8)); // max 16 per 8 bits
					a = (a + (a >> 16)); // max 32 per 8 bits
					return a & 0xff;
				}

				int stbi__shiftsigned(int v, int shift, int bits)
				{
					int result;
					int z = 0;

					if (shift < 0) v <<= -shift;
					else v >>= shift;
					result = v;

					z = bits;
					while (z < 8) {
						result += v >> z;
						z += bits;
					}
					return result;
				}

				stbi_uc *stbi__bmp_load(stbi__context *s, int *x, int *y, int *comp, int req_comp)
				{
					stbi_uc *out;
					unsigned int mr = 0, mg = 0, mb = 0, ma = 0, all_a = 255;
					stbi_uc pal[256][4];
					int psize = 0, i, j, compress = 0, width;
					int bpp, flip_vertically, pad, target, offset, hsz;
					if (stbi__get8(s) != 'B' || stbi__get8(s) != 'M') return stbi__errpuc("not BMP", "Corrupt BMP");
					stbi__get32le(s); // discard filesize
					stbi__get16le(s); // discard reserved
					stbi__get16le(s); // discard reserved
					offset = stbi__get32le(s);
					hsz = stbi__get32le(s);
					if (hsz != 12 && hsz != 40 && hsz != 56 && hsz != 108 && hsz != 124) return stbi__errpuc("unknown BMP", "BMP type not supported: unknown");
					if (hsz == 12) {
						s->img_x = stbi__get16le(s);
						s->img_y = stbi__get16le(s);
					}
					else {
						s->img_x = stbi__get32le(s);
						s->img_y = stbi__get32le(s);
					}
					if (stbi__get16le(s) != 1) return stbi__errpuc("bad BMP", "bad BMP");
					bpp = stbi__get16le(s);
					if (bpp == 1) return stbi__errpuc("monochrome", "BMP type not supported: 1-bit");
					flip_vertically = ((int)s->img_y) > 0;
					s->img_y = abs((int)s->img_y);
					if (hsz == 12) {
						if (bpp < 24)
							psize = (offset - 14 - 24) / 3;
					}
					else {
						compress = stbi__get32le(s);
						if (compress == 1 || compress == 2) return stbi__errpuc("BMP RLE", "BMP type not supported: RLE");
						stbi__get32le(s); // discard sizeof
						stbi__get32le(s); // discard hres
						stbi__get32le(s); // discard vres
						stbi__get32le(s); // discard colorsused
						stbi__get32le(s); // discard max important
						if (hsz == 40 || hsz == 56) {
							if (hsz == 56) {
								stbi__get32le(s);
								stbi__get32le(s);
								stbi__get32le(s);
								stbi__get32le(s);
							}
							if (bpp == 16 || bpp == 32) {
								mr = mg = mb = 0;
								if (compress == 0) {
									if (bpp == 32) {
										mr = 0xffu << 16;
										mg = 0xffu << 8;
										mb = 0xffu << 0;
										ma = 0xffu << 24;
										all_a = 0; // if all_a is 0 at end, then we loaded alpha channel but it was all 0
									}
									else {
										mr = 31u << 10;
										mg = 31u << 5;
										mb = 31u << 0;
									}
								}
								else if (compress == 3) {
									mr = stbi__get32le(s);
									mg = stbi__get32le(s);
									mb = stbi__get32le(s);
									// not documented, but generated by photoshop and handled by mspaint
									if (mr == mg && mg == mb) {
										// ?!?!?
										return stbi__errpuc("bad BMP", "bad BMP");
									}
								}
								else
									return stbi__errpuc("bad BMP", "bad BMP");
							}
						}
						else {
							STBI_ASSERT(hsz == 108 || hsz == 124);
							mr = stbi__get32le(s);
							mg = stbi__get32le(s);
							mb = stbi__get32le(s);
							ma = stbi__get32le(s);
							stbi__get32le(s); // discard color space
							for (i = 0; i < 12; ++i)
								stbi__get32le(s); // discard color space parameters
							if (hsz == 124) {
								stbi__get32le(s); // discard rendering intent
								stbi__get32le(s); // discard offset of profile data
								stbi__get32le(s); // discard size of profile data
								stbi__get32le(s); // discard reserved
							}
						}
						if (bpp < 16)
							psize = (offset - 14 - hsz) >> 2;
					}
					s->img_n = ma ? 4 : 3;
					if (req_comp && req_comp >= 3) // we can directly decode 3 or 4
						target = req_comp;
					else
						target = s->img_n; // if they want monochrome, we'll post-convert
					out = (stbi_uc *)stbi__malloc(target * s->img_x * s->img_y);
					if (!out) return stbi__errpuc("outofmem", "Out of memory");
					if (bpp < 16) {
						int z = 0;
						if (psize == 0 || psize > 256) { STBI_FREE(out); return stbi__errpuc("invalid", "Corrupt BMP"); }
						for (i = 0; i < psize; ++i) {
							pal[i][2] = stbi__get8(s);
							pal[i][1] = stbi__get8(s);
							pal[i][0] = stbi__get8(s);
							if (hsz != 12) stbi__get8(s);
							pal[i][3] = 255;
						}
						stbi__skip(s, offset - 14 - hsz - psize * (hsz == 12 ? 3 : 4));
						if (bpp == 4) width = (s->img_x + 1) >> 1;
						else if (bpp == 8) width = s->img_x;
						else { STBI_FREE(out); return stbi__errpuc("bad bpp", "Corrupt BMP"); }
						pad = (-width) & 3;
						for (j = 0; j < (int)s->img_y; ++j) {
							for (i = 0; i < (int)s->img_x; i += 2) {
								int v = stbi__get8(s), v2 = 0;
								if (bpp == 4) {
									v2 = v & 15;
									v >>= 4;
								}
								out[z++] = pal[v][0];
								out[z++] = pal[v][1];
								out[z++] = pal[v][2];
								if (target == 4) out[z++] = 255;
								if (i + 1 == (int)s->img_x) break;
								v = (bpp == 8) ? stbi__get8(s) : v2;
								out[z++] = pal[v][0];
								out[z++] = pal[v][1];
								out[z++] = pal[v][2];
								if (target == 4) out[z++] = 255;
							}
							stbi__skip(s, pad);
						}
					}
					else {
						int rshift = 0, gshift = 0, bshift = 0, ashift = 0, rcount = 0, gcount = 0, bcount = 0, acount = 0;
						int z = 0;
						int easy = 0;
						stbi__skip(s, offset - 14 - hsz);
						if (bpp == 24) width = 3 * s->img_x;
						else if (bpp == 16) width = 2 * s->img_x;
						else /* bpp = 32 and pad = 0 */ width = 0;
						pad = (-width) & 3;
						if (bpp == 24) {
							easy = 1;
						}
						else if (bpp == 32) {
							if (mb == 0xff && mg == 0xff00 && mr == 0x00ff0000 && ma == 0xff000000)
								easy = 2;
						}
						if (!easy) {
							if (!mr || !mg || !mb) { STBI_FREE(out); return stbi__errpuc("bad masks", "Corrupt BMP"); }
							// right shift amt to put high bit in position #7
							rshift = stbi__high_bit(mr) - 7; rcount = stbi__bitcount(mr);
							gshift = stbi__high_bit(mg) - 7; gcount = stbi__bitcount(mg);
							bshift = stbi__high_bit(mb) - 7; bcount = stbi__bitcount(mb);
							ashift = stbi__high_bit(ma) - 7; acount = stbi__bitcount(ma);
						}
						for (j = 0; j < (int)s->img_y; ++j) {
							if (easy) {
								for (i = 0; i < (int)s->img_x; ++i) {
									unsigned char a;
									out[z + 2] = stbi__get8(s);
									out[z + 1] = stbi__get8(s);
									out[z + 0] = stbi__get8(s);
									z += 3;
									a = (easy == 2 ? stbi__get8(s) : 255);
									all_a |= a;
									if (target == 4) out[z++] = a;
								}
							}
							else {
								for (i = 0; i < (int)s->img_x; ++i) {
									stbi__uint32 v = (bpp == 16 ? (stbi__uint32)stbi__get16le(s) : stbi__get32le(s));
									int a;
									out[z++] = STBI__BYTECAST(stbi__shiftsigned(v & mr, rshift, rcount));
									out[z++] = STBI__BYTECAST(stbi__shiftsigned(v & mg, gshift, gcount));
									out[z++] = STBI__BYTECAST(stbi__shiftsigned(v & mb, bshift, bcount));
									a = (ma ? stbi__shiftsigned(v & ma, ashift, acount) : 255);
									all_a |= a;
									if (target == 4) out[z++] = STBI__BYTECAST(a);
								}
							}
							stbi__skip(s, pad);
						}
					}

					// if alpha channel is all 0s, replace with all 255s
					if (target == 4 && all_a == 0)
					for (i = 4 * s->img_x*s->img_y - 1; i >= 0; i -= 4)
						out[i] = 255;

					if (flip_vertically) {
						stbi_uc t;
						for (j = 0; j < (int)s->img_y >> 1; ++j) {
							stbi_uc *p1 = out + j     *s->img_x*target;
							stbi_uc *p2 = out + (s->img_y - 1 - j)*s->img_x*target;
							for (i = 0; i < (int)s->img_x*target; ++i) {
								t = p1[i], p1[i] = p2[i], p2[i] = t;
							}
						}
					}

					if (req_comp && req_comp != target) {
						out = stbi__convert_format(out, target, req_comp, s->img_x, s->img_y);
						if (out == NULL) return out; // stbi__convert_format frees input on failure
					}

					*x = s->img_x;
					*y = s->img_y;
					if (comp) *comp = s->img_n;
					return out;
				}
#endif

				// Targa Truevision - TGA
				// by Jonathan Dummer
#ifndef STBI_NO_TGA
				int stbi__tga_info(stbi__context *s, int *x, int *y, int *comp)
				{
					int tga_w, tga_h, tga_comp;
					int sz;
					stbi__get8(s);                   // discard Offset
					sz = stbi__get8(s);              // color type
					if (sz > 1) {
						stbi__rewind(s);
						return 0;      // only RGB or indexed allowed
					}
					sz = stbi__get8(s);              // image type
					// only RGB or grey allowed, +/- RLE
					if ((sz != 1) && (sz != 2) && (sz != 3) && (sz != 9) && (sz != 10) && (sz != 11)) return 0;
					stbi__skip(s, 9);
					tga_w = stbi__get16le(s);
					if (tga_w < 1) {
						stbi__rewind(s);
						return 0;   // test width
					}
					tga_h = stbi__get16le(s);
					if (tga_h < 1) {
						stbi__rewind(s);
						return 0;   // test height
					}
					sz = stbi__get8(s);               // bits per pixel
					// only RGB or RGBA or grey allowed
					if ((sz != 8) && (sz != 16) && (sz != 24) && (sz != 32)) {
						stbi__rewind(s);
						return 0;
					}
					tga_comp = sz;
					if (x) *x = tga_w;
					if (y) *y = tga_h;
					if (comp) *comp = tga_comp / 8;
					return 1;                   // seems to have passed everything
				}

				int stbi__tga_test(stbi__context *s)
				{
					int res;
					int sz;
					stbi__get8(s);      //   discard Offset
					sz = stbi__get8(s);   //   color type
					if (sz > 1) return 0;   //   only RGB or indexed allowed
					sz = stbi__get8(s);   //   image type
					if ((sz != 1) && (sz != 2) && (sz != 3) && (sz != 9) && (sz != 10) && (sz != 11)) return 0;   //   only RGB or grey allowed, +/- RLE
					stbi__get16be(s);      //   discard palette start
					stbi__get16be(s);      //   discard palette length
					stbi__get8(s);         //   discard bits per palette color entry
					stbi__get16be(s);      //   discard x origin
					stbi__get16be(s);      //   discard y origin
					if (stbi__get16be(s) < 1) return 0;      //   test width
					if (stbi__get16be(s) < 1) return 0;      //   test height
					sz = stbi__get8(s);   //   bits per pixel
					if ((sz != 8) && (sz != 16) && (sz != 24) && (sz != 32))
						res = 0;
					else
						res = 1;
					stbi__rewind(s);
					return res;
				}

				stbi_uc *stbi__tga_load(stbi__context *s, int *x, int *y, int *comp, int req_comp)
				{
					//   read in the TGA header stuff
					int tga_offset = stbi__get8(s);
					int tga_indexed = stbi__get8(s);
					int tga_image_type = stbi__get8(s);
					int tga_is_RLE = 0;
					int tga_palette_start = stbi__get16le(s);
					int tga_palette_len = stbi__get16le(s);
					int tga_palette_bits = stbi__get8(s);
					int tga_x_origin = stbi__get16le(s);
					int tga_y_origin = stbi__get16le(s);
					int tga_width = stbi__get16le(s);
					int tga_height = stbi__get16le(s);
					int tga_bits_per_pixel = stbi__get8(s);
					int tga_comp = tga_bits_per_pixel / 8;
					int tga_inverted = stbi__get8(s);
					//   image data
					unsigned char *tga_data;
					unsigned char *tga_palette = NULL;
					int i, j;
					unsigned char raw_data[4];
					int RLE_count = 0;
					int RLE_repeating = 0;
					int read_next_pixel = 1;

					//   do a tiny bit of precessing
					if (tga_image_type >= 8)
					{
						tga_image_type -= 8;
						tga_is_RLE = 1;
					}
					/* int tga_alpha_bits = tga_inverted & 15; */
					tga_inverted = 1 - ((tga_inverted >> 5) & 1);

					//   error check
					if ( //(tga_indexed) ||
						(tga_width < 1) || (tga_height < 1) ||
						(tga_image_type < 1) || (tga_image_type > 3) ||
						((tga_bits_per_pixel != 8) && (tga_bits_per_pixel != 16) &&
						(tga_bits_per_pixel != 24) && (tga_bits_per_pixel != 32))
						)
					{
						return NULL; // we don't report this as a bad TGA because we don't even know if it's TGA
					}

					//   If I'm paletted, then I'll use the number of bits from the palette
					if (tga_indexed)
					{
						tga_comp = tga_palette_bits / 8;
					}

					//   tga info
					*x = tga_width;
					*y = tga_height;
					if (comp) *comp = tga_comp;

					tga_data = (unsigned char*)stbi__malloc((size_t)tga_width * tga_height * tga_comp);
					if (!tga_data) return stbi__errpuc("outofmem", "Out of memory");

					// skip to the data's starting position (offset usually = 0)
					stbi__skip(s, tga_offset);

					if (!tga_indexed && !tga_is_RLE) {
						for (i = 0; i < tga_height; ++i) {
							int row = tga_inverted ? tga_height - i - 1 : i;
							stbi_uc *tga_row = tga_data + row*tga_width*tga_comp;
							stbi__getn(s, tga_row, tga_width * tga_comp);
						}
					}
					else  {
						//   do I need to load a palette?
						if (tga_indexed)
						{
							//   any data to skip? (offset usually = 0)
							stbi__skip(s, tga_palette_start);
							//   load the palette
							tga_palette = (unsigned char*)stbi__malloc(tga_palette_len * tga_palette_bits / 8);
							if (!tga_palette) {
								STBI_FREE(tga_data);
								return stbi__errpuc("outofmem", "Out of memory");
							}
							if (!stbi__getn(s, tga_palette, tga_palette_len * tga_palette_bits / 8)) {
								STBI_FREE(tga_data);
								STBI_FREE(tga_palette);
								return stbi__errpuc("bad palette", "Corrupt TGA");
							}
						}
						//   load the data
						for (i = 0; i < tga_width * tga_height; ++i)
						{
							//   if I'm in RLE mode, do I need to get a RLE stbi__pngchunk?
							if (tga_is_RLE)
							{
								if (RLE_count == 0)
								{
									//   yep, get the next byte as a RLE command
									int RLE_cmd = stbi__get8(s);
									RLE_count = 1 + (RLE_cmd & 127);
									RLE_repeating = RLE_cmd >> 7;
									read_next_pixel = 1;
								}
								else if (!RLE_repeating)
								{
									read_next_pixel = 1;
								}
							}
							else
							{
								read_next_pixel = 1;
							}
							//   OK, if I need to read a pixel, do it now
							if (read_next_pixel)
							{
								//   load however much data we did have
								if (tga_indexed)
								{
									//   read in 1 byte, then perform the lookup
									int pal_idx = stbi__get8(s);
									if (pal_idx >= tga_palette_len)
									{
										//   invalid index
										pal_idx = 0;
									}
									pal_idx *= tga_bits_per_pixel / 8;
									for (j = 0; j * 8 < tga_bits_per_pixel; ++j)
									{
										raw_data[j] = tga_palette[pal_idx + j];
									}
								}
								else
								{
									//   read in the data raw
									for (j = 0; j * 8 < tga_bits_per_pixel; ++j)
									{
										raw_data[j] = stbi__get8(s);
									}
								}
								//   clear the reading flag for the next pixel
								read_next_pixel = 0;
							} // end of reading a pixel

							// copy data
							for (j = 0; j < tga_comp; ++j)
								tga_data[i*tga_comp + j] = raw_data[j];

							//   in case we're in RLE mode, keep counting down
							--RLE_count;
						}
						//   do I need to invert the image?
						if (tga_inverted)
						{
							for (j = 0; j * 2 < tga_height; ++j)
							{
								int index1 = j * tga_width * tga_comp;
								int index2 = (tga_height - 1 - j) * tga_width * tga_comp;
								for (i = tga_width * tga_comp; i > 0; --i)
								{
									unsigned char temp = tga_data[index1];
									tga_data[index1] = tga_data[index2];
									tga_data[index2] = temp;
									++index1;
									++index2;
								}
							}
						}
						//   clear my palette, if I had one
						if (tga_palette != NULL)
						{
							STBI_FREE(tga_palette);
						}
					}

					// swap RGB
					if (tga_comp >= 3)
					{
						unsigned char* tga_pixel = tga_data;
						for (i = 0; i < tga_width * tga_height; ++i)
						{
							unsigned char temp = tga_pixel[0];
							tga_pixel[0] = tga_pixel[2];
							tga_pixel[2] = temp;
							tga_pixel += tga_comp;
						}
					}

					// convert to target component count
					if (req_comp && req_comp != tga_comp)
						tga_data = stbi__convert_format(tga_data, tga_comp, req_comp, tga_width, tga_height);

					//   the things I do to get rid of an error message, and yet keep
					//   Microsoft's C compilers happy... [8^(
					tga_palette_start = tga_palette_len = tga_palette_bits =
						tga_x_origin = tga_y_origin = 0;
					//   OK, done
					return tga_data;
				}
#endif

				// *************************************************************************************************
				// Photoshop PSD loader -- PD by Thatcher Ulrich, integration by Nicolas Schulz, tweaked by STB

#ifndef STBI_NO_PSD
				int stbi__psd_test(stbi__context *s)
				{
					int r = (stbi__get32be(s) == 0x38425053);
					stbi__rewind(s);
					return r;
				}

				stbi_uc *stbi__psd_load(stbi__context *s, int *x, int *y, int *comp, int req_comp)
				{
					int   pixelCount;
					int channelCount, compression;
					int channel, i, count, len;
					int bitdepth;
					int w, h;
					stbi_uc *out;

					// Check identifier
					if (stbi__get32be(s) != 0x38425053)   // "8BPS"
						return stbi__errpuc("not PSD", "Corrupt PSD image");

					// Check file type version.
					if (stbi__get16be(s) != 1)
						return stbi__errpuc("wrong version", "Unsupported version of PSD image");

					// Skip 6 reserved bytes.
					stbi__skip(s, 6);

					// Read the number of channels (R, G, B, A, etc).
					channelCount = stbi__get16be(s);
					if (channelCount < 0 || channelCount > 16)
						return stbi__errpuc("wrong channel count", "Unsupported number of channels in PSD image");

					// Read the rows and columns of the image.
					h = stbi__get32be(s);
					w = stbi__get32be(s);

					// Make sure the depth is 8 bits.
					bitdepth = stbi__get16be(s);
					if (bitdepth != 8 && bitdepth != 16)
						return stbi__errpuc("unsupported bit depth", "PSD bit depth is not 8 or 16 bit");

					// Make sure the color mode is RGB.
					// Valid options are:
					//   0: Bitmap
					//   1: Grayscale
					//   2: Indexed color
					//   3: RGB color
					//   4: CMYK color
					//   7: Multichannel
					//   8: Duotone
					//   9: Lab color
					if (stbi__get16be(s) != 3)
						return stbi__errpuc("wrong color format", "PSD is not in RGB color format");

					// Skip the Mode Data.  (It's the palette for indexed color; other info for other modes.)
					stbi__skip(s, stbi__get32be(s));

					// Skip the image resources.  (resolution, pen tool paths, etc)
					stbi__skip(s, stbi__get32be(s));

					// Skip the reserved data.
					stbi__skip(s, stbi__get32be(s));

					// Find out if the data is compressed.
					// Known values:
					//   0: no compression
					//   1: RLE compressed
					compression = stbi__get16be(s);
					if (compression > 1)
						return stbi__errpuc("bad compression", "PSD has an unknown compression format");

					// Create the destination image.
					out = (stbi_uc *)stbi__malloc(4 * w*h);
					if (!out) return stbi__errpuc("outofmem", "Out of memory");
					pixelCount = w*h;

					// Initialize the data to zero.
					//memset( out, 0, pixelCount * 4 );

					// Finally, the image data.
					if (compression) {
						// RLE as used by .PSD and .TIFF
						// Loop until you get the number of unpacked bytes you are expecting:
						//     Read the next source byte into n.
						//     If n is between 0 and 127 inclusive, copy the next n+1 bytes literally.
						//     Else if n is between -127 and -1 inclusive, copy the next byte -n+1 times.
						//     Else if n is 128, noop.
						// Endloop

						// The RLE-compressed data is preceeded by a 2-byte data count for each row in the data,
						// which we're going to just skip.
						stbi__skip(s, h * channelCount * 2);

						// Read the RLE data by channel.
						for (channel = 0; channel < 4; channel++) {
							stbi_uc *p;

							p = out + channel;
							if (channel >= channelCount) {
								// Fill this channel with default data.
								for (i = 0; i < pixelCount; i++, p += 4)
									*p = (channel == 3 ? 255 : 0);
							}
							else {
								// Read the RLE data.
								count = 0;
								while (count < pixelCount) {
									len = stbi__get8(s);
									if (len == 128) {
										// No-op.
									}
									else if (len < 128) {
										// Copy next len+1 bytes literally.
										len++;
										count += len;
										while (len) {
											*p = stbi__get8(s);
											p += 4;
											len--;
										}
									}
									else if (len > 128) {
										stbi_uc   val;
										// Next -len+1 bytes in the dest are replicated from next source byte.
										// (Interpret len as a negative 8-bit int.)
										len ^= 0x0FF;
										len += 2;
										val = stbi__get8(s);
										count += len;
										while (len) {
											*p = val;
											p += 4;
											len--;
										}
									}
								}
							}
						}

					}
					else {
						// We're at the raw image data.  It's each channel in order (Red, Green, Blue, Alpha, ...)
						// where each channel consists of an 8-bit value for each pixel in the image.

						// Read the data by channel.
						for (channel = 0; channel < 4; channel++) {
							stbi_uc *p;

							p = out + channel;
							if (channel >= channelCount) {
								// Fill this channel with default data.
								stbi_uc val = channel == 3 ? 255 : 0;
								for (i = 0; i < pixelCount; i++, p += 4)
									*p = val;
							}
							else {
								// Read the data.
								if (bitdepth == 16) {
									for (i = 0; i < pixelCount; i++, p += 4)
										*p = (stbi_uc)(stbi__get16be(s) >> 8);
								}
								else {
									for (i = 0; i < pixelCount; i++, p += 4)
										*p = stbi__get8(s);
								}
							}
						}
					}

					if (req_comp && req_comp != 4) {
						out = stbi__convert_format(out, 4, req_comp, w, h);
						if (out == NULL) return out; // stbi__convert_format frees input on failure
					}

					if (comp) *comp = 4;
					*y = h;
					*x = w;

					return out;
				}
#endif

				// *************************************************************************************************
				// Softimage PIC loader
				// by Tom Seddon
				//
				// See http://softimage.wiki.softimage.com/index.php/INFO:_PIC_file_format
				// See http://ozviz.wasp.uwa.edu.au/~pbourke/dataformats/softimagepic/

#ifndef STBI_NO_PIC
				int stbi__pic_is4(stbi__context *s, const char *str)
				{
					int i;
					for (i = 0; i < 4; ++i)
					if (stbi__get8(s) != (stbi_uc)str[i])
						return 0;

					return 1;
				}

				int stbi__pic_test_core(stbi__context *s)
				{
					int i;

					if (!stbi__pic_is4(s, "\x53\x80\xF6\x34"))
						return 0;

					for (i = 0; i < 84; ++i)
						stbi__get8(s);

					if (!stbi__pic_is4(s, "PICT"))
						return 0;

					return 1;
				}

				typedef struct
				{
					stbi_uc size, type, channel;
				} stbi__pic_packet;

				stbi_uc *stbi__readval(stbi__context *s, int channel, stbi_uc *dest)
				{
					int mask = 0x80, i;

					for (i = 0; i < 4; ++i, mask >>= 1) {
						if (channel & mask) {
							if (stbi__at_eof(s)) return stbi__errpuc("bad file", "PIC file too short");
							dest[i] = stbi__get8(s);
						}
					}

					return dest;
				}

				void stbi__copyval(int channel, stbi_uc *dest, const stbi_uc *src)
				{
					int mask = 0x80, i;

					for (i = 0; i < 4; ++i, mask >>= 1)
					if (channel&mask)
						dest[i] = src[i];
				}

				stbi_uc *stbi__pic_load_core(stbi__context *s, int width, int height, int *comp, stbi_uc *result)
				{
					int act_comp = 0, num_packets = 0, y, chained;
					stbi__pic_packet packets[10];

					// this will (should...) cater for even some bizarre stuff like having data
					// for the same channel in multiple packets.
					do {
						stbi__pic_packet *packet;

						if (num_packets == sizeof(packets) / sizeof(packets[0]))
							return stbi__errpuc("bad format", "too many packets");

						packet = &packets[num_packets++];

						chained = stbi__get8(s);
						packet->size = stbi__get8(s);
						packet->type = stbi__get8(s);
						packet->channel = stbi__get8(s);

						act_comp |= packet->channel;

						if (stbi__at_eof(s))          return stbi__errpuc("bad file", "file too short (reading packets)");
						if (packet->size != 8)  return stbi__errpuc("bad format", "packet isn't 8bpp");
					} while (chained);

					*comp = (act_comp & 0x10 ? 4 : 3); // has alpha channel?

					for (y = 0; y < height; ++y) {
						int packet_idx;

						for (packet_idx = 0; packet_idx < num_packets; ++packet_idx) {
							stbi__pic_packet *packet = &packets[packet_idx];
							stbi_uc *dest = result + y*width * 4;

							switch (packet->type) {
							default:
								return stbi__errpuc("bad format", "packet has bad compression type");

							case 0: {//uncompressed
										int x;

										for (x = 0; x<width; ++x, dest += 4)
										if (!stbi__readval(s, packet->channel, dest))
											return 0;
										break;
							}

							case 1://Pure RLE
							{
									   int left = width, i;

									   while (left>0) {
										   stbi_uc count, value[4];

										   count = stbi__get8(s);
										   if (stbi__at_eof(s))   return stbi__errpuc("bad file", "file too short (pure read count)");

										   if (count > left)
											   count = (stbi_uc)left;

										   if (!stbi__readval(s, packet->channel, value))  return 0;

										   for (i = 0; i<count; ++i, dest += 4)
											   stbi__copyval(packet->channel, dest, value);
										   left -= count;
									   }
							}
								break;

							case 2: {//Mixed RLE
										int left = width;
										while (left>0) {
											int count = stbi__get8(s), i;
											if (stbi__at_eof(s))  return stbi__errpuc("bad file", "file too short (mixed read count)");

											if (count >= 128) { // Repeated
												stbi_uc value[4];

												if (count == 128)
													count = stbi__get16be(s);
												else
													count -= 127;
												if (count > left)
													return stbi__errpuc("bad file", "scanline overrun");

												if (!stbi__readval(s, packet->channel, value))
													return 0;

												for (i = 0; i<count; ++i, dest += 4)
													stbi__copyval(packet->channel, dest, value);
											}
											else { // Raw
												++count;
												if (count>left) return stbi__errpuc("bad file", "scanline overrun");

												for (i = 0; i < count; ++i, dest += 4)
												if (!stbi__readval(s, packet->channel, dest))
													return 0;
											}
											left -= count;
										}
										break;
							}
							}
						}
					}

					return result;
				}

				stbi_uc *stbi__pic_load(stbi__context *s, int *px, int *py, int *comp, int req_comp)
				{
					stbi_uc *result;
					int i, x, y;

					for (i = 0; i < 92; ++i)
						stbi__get8(s);

					x = stbi__get16be(s);
					y = stbi__get16be(s);
					if (stbi__at_eof(s))  return stbi__errpuc("bad file", "file too short (pic header)");
					if ((1 << 28) / x < y) return stbi__errpuc("too large", "Image too large to decode");

					stbi__get32be(s); //skip `ratio'
					stbi__get16be(s); //skip `fields'
					stbi__get16be(s); //skip `pad'

					// intermediate buffer is RGBA
					result = (stbi_uc *)stbi__malloc(x*y * 4);
					memset(result, 0xff, x*y * 4);

					if (!stbi__pic_load_core(s, x, y, comp, result)) {
						STBI_FREE(result);
						result = 0;
					}
					*px = x;
					*py = y;
					if (req_comp == 0) req_comp = *comp;
					result = stbi__convert_format(result, 4, req_comp, x, y);

					return result;
				}

				int stbi__pic_test(stbi__context *s)
				{
					int r = stbi__pic_test_core(s);
					stbi__rewind(s);
					return r;
				}
#endif

				// *************************************************************************************************
				// GIF loader -- public domain by Jean-Marc Lienher -- simplified/shrunk by stb

#ifndef STBI_NO_GIF
				typedef struct
				{
					stbi__int16 prefix;
					stbi_uc first;
					stbi_uc suffix;
				} stbi__gif_lzw;

				typedef struct
				{
					int w, h;
					stbi_uc *out, *old_out;             // output buffer (always 4 components)
					int flags, bgindex, ratio, transparent, eflags, delay;
					stbi_uc  pal[256][4];
					stbi_uc lpal[256][4];
					stbi__gif_lzw codes[4096];
					stbi_uc *color_table;
					int parse, step;
					int lflags;
					int start_x, start_y;
					int max_x, max_y;
					int cur_x, cur_y;
					int line_size;
				} stbi__gif;

				int stbi__gif_test_raw(stbi__context *s)
				{
					int sz;
					if (stbi__get8(s) != 'G' || stbi__get8(s) != 'I' || stbi__get8(s) != 'F' || stbi__get8(s) != '8') return 0;
					sz = stbi__get8(s);
					if (sz != '9' && sz != '7') return 0;
					if (stbi__get8(s) != 'a') return 0;
					return 1;
				}

				int stbi__gif_test(stbi__context *s)
				{
					int r = stbi__gif_test_raw(s);
					stbi__rewind(s);
					return r;
				}

				void stbi__gif_parse_colortable(stbi__context *s, stbi_uc pal[256][4], int num_entries, int transp)
				{
					int i;
					for (i = 0; i < num_entries; ++i) {
						pal[i][2] = stbi__get8(s);
						pal[i][1] = stbi__get8(s);
						pal[i][0] = stbi__get8(s);
						pal[i][3] = transp == i ? 0 : 255;
					}
				}

				int stbi__gif_header(stbi__context *s, stbi__gif *g, int *comp, int is_info)
				{
					stbi_uc version;
					if (stbi__get8(s) != 'G' || stbi__get8(s) != 'I' || stbi__get8(s) != 'F' || stbi__get8(s) != '8')
						return stbi__err("not GIF", "Corrupt GIF");

					version = stbi__get8(s);
					if (version != '7' && version != '9')    return stbi__err("not GIF", "Corrupt GIF");
					if (stbi__get8(s) != 'a')                return stbi__err("not GIF", "Corrupt GIF");

					stbi__g_failure_reason = "";
					g->w = stbi__get16le(s);
					g->h = stbi__get16le(s);
					g->flags = stbi__get8(s);
					g->bgindex = stbi__get8(s);
					g->ratio = stbi__get8(s);
					g->transparent = -1;

					if (comp != 0) *comp = 4;  // can't actually tell whether it's 3 or 4 until we parse the comments

					if (is_info) return 1;

					if (g->flags & 0x80)
						stbi__gif_parse_colortable(s, g->pal, 2 << (g->flags & 7), -1);

					return 1;
				}

				int stbi__gif_info_raw(stbi__context *s, int *x, int *y, int *comp)
				{
					stbi__gif g;
					if (!stbi__gif_header(s, &g, comp, 1)) {
						stbi__rewind(s);
						return 0;
					}
					if (x) *x = g.w;
					if (y) *y = g.h;
					return 1;
				}

				void stbi__out_gif_code(stbi__gif *g, stbi__uint16 code)
				{
					stbi_uc *p, *c;

					// recurse to decode the prefixes, since the linked-list is backwards,
					// and working backwards through an interleaved image would be nasty
					if (g->codes[code].prefix >= 0)
						stbi__out_gif_code(g, g->codes[code].prefix);

					if (g->cur_y >= g->max_y) return;

					p = &g->out[g->cur_x + g->cur_y];
					c = &g->color_table[g->codes[code].suffix * 4];

					if (c[3] >= 128) {
						p[0] = c[2];
						p[1] = c[1];
						p[2] = c[0];
						p[3] = c[3];
					}
					g->cur_x += 4;

					if (g->cur_x >= g->max_x) {
						g->cur_x = g->start_x;
						g->cur_y += g->step;

						while (g->cur_y >= g->max_y && g->parse > 0) {
							g->step = (1 << g->parse) * g->line_size;
							g->cur_y = g->start_y + (g->step >> 1);
							--g->parse;
						}
					}
				}

				stbi_uc *stbi__process_gif_raster(stbi__context *s, stbi__gif *g)
				{
					stbi_uc lzw_cs;
					stbi__int32 len, init_code;
					stbi__uint32 first;
					stbi__int32 codesize, codemask, avail, oldcode, bits, valid_bits, clear;
					stbi__gif_lzw *p;

					lzw_cs = stbi__get8(s);
					if (lzw_cs > 12) return NULL;
					clear = 1 << lzw_cs;
					first = 1;
					codesize = lzw_cs + 1;
					codemask = (1 << codesize) - 1;
					bits = 0;
					valid_bits = 0;
					for (init_code = 0; init_code < clear; init_code++) {
						g->codes[init_code].prefix = -1;
						g->codes[init_code].first = (stbi_uc)init_code;
						g->codes[init_code].suffix = (stbi_uc)init_code;
					}

					// support no starting clear code
					avail = clear + 2;
					oldcode = -1;

					len = 0;
					for (;;) {
						if (valid_bits < codesize) {
							if (len == 0) {
								len = stbi__get8(s); // start new block
								if (len == 0)
									return g->out;
							}
							--len;
							bits |= (stbi__int32)stbi__get8(s) << valid_bits;
							valid_bits += 8;
						}
						else {
							stbi__int32 code = bits & codemask;
							bits >>= codesize;
							valid_bits -= codesize;
							// @OPTIMIZE: is there some way we can accelerate the non-clear path?
							if (code == clear) {  // clear code
								codesize = lzw_cs + 1;
								codemask = (1 << codesize) - 1;
								avail = clear + 2;
								oldcode = -1;
								first = 0;
							}
							else if (code == clear + 1) { // end of stream code
								stbi__skip(s, len);
								while ((len = stbi__get8(s)) > 0)
									stbi__skip(s, len);
								return g->out;
							}
							else if (code <= avail) {
								if (first) return stbi__errpuc("no clear code", "Corrupt GIF");

								if (oldcode >= 0) {
									p = &g->codes[avail++];
									if (avail > 4096)        return stbi__errpuc("too many codes", "Corrupt GIF");
									p->prefix = (stbi__int16)oldcode;
									p->first = g->codes[oldcode].first;
									p->suffix = (code == avail) ? p->first : g->codes[code].first;
								}
								else if (code == avail)
									return stbi__errpuc("illegal code in raster", "Corrupt GIF");

								stbi__out_gif_code(g, (stbi__uint16)code);

								if ((avail & codemask) == 0 && avail <= 0x0FFF) {
									codesize++;
									codemask = (1 << codesize) - 1;
								}

								oldcode = code;
							}
							else {
								return stbi__errpuc("illegal code in raster", "Corrupt GIF");
							}
						}
					}
				}

				void stbi__fill_gif_background(stbi__gif *g, int x0, int y0, int x1, int y1)
				{
					int x, y;
					stbi_uc *c = g->pal[g->bgindex];
					for (y = y0; y < y1; y += 4 * g->w) {
						for (x = x0; x < x1; x += 4) {
							stbi_uc *p = &g->out[y + x];
							p[0] = c[2];
							p[1] = c[1];
							p[2] = c[0];
							p[3] = 0;
						}
					}
				}

				// this function is designed to support animated gifs, although stb_image doesn't support it
				stbi_uc *stbi__gif_load_next(stbi__context *s, stbi__gif *g, int *comp, int req_comp)
				{
					int i;
					stbi_uc *prev_out = 0;

					if (g->out == 0 && !stbi__gif_header(s, g, comp, 0))
						return 0; // stbi__g_failure_reason set by stbi__gif_header

					prev_out = g->out;
					g->out = (stbi_uc *)stbi__malloc(4 * g->w * g->h);
					if (g->out == 0) return stbi__errpuc("outofmem", "Out of memory");

					switch ((g->eflags & 0x1C) >> 2) {
					case 0: // unspecified (also always used on 1st frame)
						stbi__fill_gif_background(g, 0, 0, 4 * g->w, 4 * g->w * g->h);
						break;
					case 1: // do not dispose
						if (prev_out) memcpy(g->out, prev_out, 4 * g->w * g->h);
						g->old_out = prev_out;
						break;
					case 2: // dispose to background
						if (prev_out) memcpy(g->out, prev_out, 4 * g->w * g->h);
						stbi__fill_gif_background(g, g->start_x, g->start_y, g->max_x, g->max_y);
						break;
					case 3: // dispose to previous
						if (g->old_out) {
							for (i = g->start_y; i < g->max_y; i += 4 * g->w)
								memcpy(&g->out[i + g->start_x], &g->old_out[i + g->start_x], g->max_x - g->start_x);
						}
						break;
					}

					for (;;) {
						switch (stbi__get8(s)) {
						case 0x2C: /* Image Descriptor */
						{
									   int prev_trans = -1;
									   stbi__int32 x, y, w, h;
									   stbi_uc *o;

									   x = stbi__get16le(s);
									   y = stbi__get16le(s);
									   w = stbi__get16le(s);
									   h = stbi__get16le(s);
									   if (((x + w) > (g->w)) || ((y + h) > (g->h)))
										   return stbi__errpuc("bad Image Descriptor", "Corrupt GIF");

									   g->line_size = g->w * 4;
									   g->start_x = x * 4;
									   g->start_y = y * g->line_size;
									   g->max_x = g->start_x + w * 4;
									   g->max_y = g->start_y + h * g->line_size;
									   g->cur_x = g->start_x;
									   g->cur_y = g->start_y;

									   g->lflags = stbi__get8(s);

									   if (g->lflags & 0x40) {
										   g->step = 8 * g->line_size; // first interlaced spacing
										   g->parse = 3;
									   }
									   else {
										   g->step = g->line_size;
										   g->parse = 0;
									   }

									   if (g->lflags & 0x80) {
										   stbi__gif_parse_colortable(s, g->lpal, 2 << (g->lflags & 7), g->eflags & 0x01 ? g->transparent : -1);
										   g->color_table = (stbi_uc *)g->lpal;
									   }
									   else if (g->flags & 0x80) {
										   if (g->transparent >= 0 && (g->eflags & 0x01)) {
											   prev_trans = g->pal[g->transparent][3];
											   g->pal[g->transparent][3] = 0;
										   }
										   g->color_table = (stbi_uc *)g->pal;
									   }
									   else
										   return stbi__errpuc("missing color table", "Corrupt GIF");

									   o = stbi__process_gif_raster(s, g);
									   if (o == NULL) return NULL;

									   if (prev_trans != -1)
										   g->pal[g->transparent][3] = (stbi_uc)prev_trans;

									   return o;
						}

						case 0x21: // Comment Extension.
						{
									   int len;
									   if (stbi__get8(s) == 0xF9) { // Graphic Control Extension.
										   len = stbi__get8(s);
										   if (len == 4) {
											   g->eflags = stbi__get8(s);
											   g->delay = stbi__get16le(s);
											   g->transparent = stbi__get8(s);
										   }
										   else {
											   stbi__skip(s, len);
											   break;
										   }
									   }
									   while ((len = stbi__get8(s)) != 0)
										   stbi__skip(s, len);
									   break;
						}

						case 0x3B: // gif stream termination code
							return (stbi_uc *)s; // using '1' causes warning on some compilers

						default:
							return stbi__errpuc("unknown code", "Corrupt GIF");
						}
					}

					STBI_NOTUSED(req_comp);
				}

				stbi_uc *stbi__gif_load(stbi__context *s, int *x, int *y, int *comp, int req_comp)
				{
					stbi_uc *u = 0;
					stbi__gif g;
					memset(&g, 0, sizeof(g));

					u = stbi__gif_load_next(s, &g, comp, req_comp);
					if (u == (stbi_uc *)s) u = 0;  // end of animated gif marker
					if (u) {
						*x = g.w;
						*y = g.h;
						if (req_comp && req_comp != 4)
							u = stbi__convert_format(u, 4, req_comp, g.w, g.h);
					}
					else if (g.out)
						STBI_FREE(g.out);

					return u;
				}

				int stbi__gif_info(stbi__context *s, int *x, int *y, int *comp)
				{
					return stbi__gif_info_raw(s, x, y, comp);
				}
#endif

				// *************************************************************************************************
				// Radiance RGBE HDR loader
				// originally by Nicolas Schulz
#ifndef STBI_NO_HDR
				int stbi__hdr_test_core(stbi__context *s)
				{
					const char *signature = "#?RADIANCE\n";
					int i;
					for (i = 0; signature[i]; ++i)
					if (stbi__get8(s) != signature[i])
						return 0;
					return 1;
				}

				int stbi__hdr_test(stbi__context* s)
				{
					int r = stbi__hdr_test_core(s);
					stbi__rewind(s);
					return r;
				}

#define STBI__HDR_BUFLEN  1024
				char *stbi__hdr_gettoken(stbi__context *z, char *buffer)
				{
					int len = 0;
					char c = '\0';

					c = (char)stbi__get8(z);

					while (!stbi__at_eof(z) && c != '\n') {
						buffer[len++] = c;
						if (len == STBI__HDR_BUFLEN - 1) {
							// flush to end of line
							while (!stbi__at_eof(z) && stbi__get8(z) != '\n')
								;
							break;
						}
						c = (char)stbi__get8(z);
					}

					buffer[len] = 0;
					return buffer;
				}

				void stbi__hdr_convert(float *output, stbi_uc *input, int req_comp)
				{
					if (input[3] != 0) {
						float f1;
						// Exponent
						f1 = (float)ldexp(1.0f, input[3] - (int)(128 + 8));
						if (req_comp <= 2)
							output[0] = (input[0] + input[1] + input[2]) * f1 / 3;
						else {
							output[0] = input[0] * f1;
							output[1] = input[1] * f1;
							output[2] = input[2] * f1;
						}
						if (req_comp == 2) output[1] = 1;
						if (req_comp == 4) output[3] = 1;
					}
					else {
						switch (req_comp) {
						case 4: output[3] = 1; /* fallthrough */
						case 3: output[0] = output[1] = output[2] = 0;
							break;
						case 2: output[1] = 1; /* fallthrough */
						case 1: output[0] = 0;
							break;
						}
					}
				}

				float *stbi__hdr_load(stbi__context *s, int *x, int *y, int *comp, int req_comp)
				{
					char buffer[STBI__HDR_BUFLEN];
					char *token;
					int valid = 0;
					int width, height;
					stbi_uc *scanline;
					float *hdr_data;
					int len;
					unsigned char count, value;
					int i, j, k, c1, c2, z;


					// Check identifier
					if (strcmp(stbi__hdr_gettoken(s, buffer), "#?RADIANCE") != 0)
						return stbi__errpf("not HDR", "Corrupt HDR image");

					// Parse header
					for (;;) {
						token = stbi__hdr_gettoken(s, buffer);
						if (token[0] == 0) break;
						if (strcmp(token, "FORMAT=32-bit_rle_rgbe") == 0) valid = 1;
					}

					if (!valid)    return stbi__errpf("unsupported format", "Unsupported HDR format");

					// Parse width and height
					// can't use sscanf() if we're not using stdio!
					token = stbi__hdr_gettoken(s, buffer);
					if (strncmp(token, "-Y ", 3))  return stbi__errpf("unsupported data layout", "Unsupported HDR format");
					token += 3;
					height = (int)strtol(token, &token, 10);
					while (*token == ' ') ++token;
					if (strncmp(token, "+X ", 3))  return stbi__errpf("unsupported data layout", "Unsupported HDR format");
					token += 3;
					width = (int)strtol(token, NULL, 10);

					*x = width;
					*y = height;

					if (comp) *comp = 3;
					if (req_comp == 0) req_comp = 3;

					// Read data
					hdr_data = (float *)stbi__malloc(height * width * req_comp * sizeof(float));

					// Load image data
					// image data is stored as some number of sca
					if (width < 8 || width >= 32768) {
						// Read flat data
						for (j = 0; j < height; ++j) {
							for (i = 0; i < width; ++i) {
								stbi_uc rgbe[4];
							main_decode_loop:
								stbi__getn(s, rgbe, 4);
								stbi__hdr_convert(hdr_data + j * width * req_comp + i * req_comp, rgbe, req_comp);
							}
						}
					}
					else {
						// Read RLE-encoded data
						scanline = NULL;

						for (j = 0; j < height; ++j) {
							c1 = stbi__get8(s);
							c2 = stbi__get8(s);
							len = stbi__get8(s);
							if (c1 != 2 || c2 != 2 || (len & 0x80)) {
								// not run-length encoded, so we have to actually use THIS data as a decoded
								// pixel (note this can't be a valid pixel--one of RGB must be >= 128)
								stbi_uc rgbe[4];
								rgbe[0] = (stbi_uc)c1;
								rgbe[1] = (stbi_uc)c2;
								rgbe[2] = (stbi_uc)len;
								rgbe[3] = (stbi_uc)stbi__get8(s);
								stbi__hdr_convert(hdr_data, rgbe, req_comp);
								i = 1;
								j = 0;
								STBI_FREE(scanline);
								goto main_decode_loop; // yes, this makes no sense
							}
							len <<= 8;
							len |= stbi__get8(s);
							if (len != width) { STBI_FREE(hdr_data); STBI_FREE(scanline); return stbi__errpf("invalid decoded scanline length", "corrupt HDR"); }
							if (scanline == NULL) scanline = (stbi_uc *)stbi__malloc(width * 4);

							for (k = 0; k < 4; ++k) {
								i = 0;
								while (i < width) {
									count = stbi__get8(s);
									if (count > 128) {
										// Run
										value = stbi__get8(s);
										count -= 128;
										for (z = 0; z < count; ++z)
											scanline[i++ * 4 + k] = value;
									}
									else {
										// Dump
										for (z = 0; z < count; ++z)
											scanline[i++ * 4 + k] = stbi__get8(s);
									}
								}
							}
							for (i = 0; i < width; ++i)
								stbi__hdr_convert(hdr_data + (j*width + i)*req_comp, scanline + i * 4, req_comp);
						}
						STBI_FREE(scanline);
					}

					return hdr_data;
				}

				int stbi__hdr_info(stbi__context *s, int *x, int *y, int *comp)
				{
					char buffer[STBI__HDR_BUFLEN];
					char *token;
					int valid = 0;

					if (strcmp(stbi__hdr_gettoken(s, buffer), "#?RADIANCE") != 0) {
						stbi__rewind(s);
						return 0;
					}

					for (;;) {
						token = stbi__hdr_gettoken(s, buffer);
						if (token[0] == 0) break;
						if (strcmp(token, "FORMAT=32-bit_rle_rgbe") == 0) valid = 1;
					}

					if (!valid) {
						stbi__rewind(s);
						return 0;
					}
					token = stbi__hdr_gettoken(s, buffer);
					if (strncmp(token, "-Y ", 3)) {
						stbi__rewind(s);
						return 0;
					}
					token += 3;
					*y = (int)strtol(token, &token, 10);
					while (*token == ' ') ++token;
					if (strncmp(token, "+X ", 3)) {
						stbi__rewind(s);
						return 0;
					}
					token += 3;
					*x = (int)strtol(token, NULL, 10);
					*comp = 3;
					return 1;
				}
#endif // STBI_NO_HDR

#ifndef STBI_NO_BMP
				int stbi__bmp_info(stbi__context *s, int *x, int *y, int *comp)
				{
					int hsz;
					if (stbi__get8(s) != 'B' || stbi__get8(s) != 'M') {
						stbi__rewind(s);
						return 0;
					}
					stbi__skip(s, 12);
					hsz = stbi__get32le(s);
					if (hsz != 12 && hsz != 40 && hsz != 56 && hsz != 108 && hsz != 124) {
						stbi__rewind(s);
						return 0;
					}
					if (hsz == 12) {
						*x = stbi__get16le(s);
						*y = stbi__get16le(s);
					}
					else {
						*x = stbi__get32le(s);
						*y = stbi__get32le(s);
					}
					if (stbi__get16le(s) != 1) {
						stbi__rewind(s);
						return 0;
					}
					*comp = stbi__get16le(s) / 8;
					return 1;
				}
#endif

#ifndef STBI_NO_PSD
				int stbi__psd_info(stbi__context *s, int *x, int *y, int *comp)
				{
					int channelCount;
					if (stbi__get32be(s) != 0x38425053) {
						stbi__rewind(s);
						return 0;
					}
					if (stbi__get16be(s) != 1) {
						stbi__rewind(s);
						return 0;
					}
					stbi__skip(s, 6);
					channelCount = stbi__get16be(s);
					if (channelCount < 0 || channelCount > 16) {
						stbi__rewind(s);
						return 0;
					}
					*y = stbi__get32be(s);
					*x = stbi__get32be(s);
					if (stbi__get16be(s) != 8) {
						stbi__rewind(s);
						return 0;
					}
					if (stbi__get16be(s) != 3) {
						stbi__rewind(s);
						return 0;
					}
					*comp = 4;
					return 1;
				}
#endif

#ifndef STBI_NO_PIC
				int stbi__pic_info(stbi__context *s, int *x, int *y, int *comp)
				{
					int act_comp = 0, num_packets = 0, chained;
					stbi__pic_packet packets[10];

					if (!stbi__pic_is4(s, "\x53\x80\xF6\x34")) {
						stbi__rewind(s);
						return 0;
					}

					stbi__skip(s, 88);

					*x = stbi__get16be(s);
					*y = stbi__get16be(s);
					if (stbi__at_eof(s)) {
						stbi__rewind(s);
						return 0;
					}
					if ((*x) != 0 && (1 << 28) / (*x) < (*y)) {
						stbi__rewind(s);
						return 0;
					}

					stbi__skip(s, 8);

					do {
						stbi__pic_packet *packet;

						if (num_packets == sizeof(packets) / sizeof(packets[0]))
							return 0;

						packet = &packets[num_packets++];
						chained = stbi__get8(s);
						packet->size = stbi__get8(s);
						packet->type = stbi__get8(s);
						packet->channel = stbi__get8(s);
						act_comp |= packet->channel;

						if (stbi__at_eof(s)) {
							stbi__rewind(s);
							return 0;
						}
						if (packet->size != 8) {
							stbi__rewind(s);
							return 0;
						}
					} while (chained);

					*comp = (act_comp & 0x10 ? 4 : 3);

					return 1;
				}
#endif

				// *************************************************************************************************
				// Portable Gray Map and Portable Pixel Map loader
				// by Ken Miller
				//
				// PGM: http://netpbm.sourceforge.net/doc/pgm.html
				// PPM: http://netpbm.sourceforge.net/doc/ppm.html
				//
				// Known limitations:
				//    Does not support comments in the header section
				//    Does not support ASCII image data (formats P2 and P3)
				//    Does not support 16-bit-per-channel

#ifndef STBI_NO_PNM

				int      stbi__pnm_test(stbi__context *s)
				{
					char p, t;
					p = (char)stbi__get8(s);
					t = (char)stbi__get8(s);
					if (p != 'P' || (t != '5' && t != '6')) {
						stbi__rewind(s);
						return 0;
					}
					return 1;
				}

				stbi_uc *stbi__pnm_load(stbi__context *s, int *x, int *y, int *comp, int req_comp)
				{
					stbi_uc *out;
					if (!stbi__pnm_info(s, (int *)&s->img_x, (int *)&s->img_y, (int *)&s->img_n))
						return 0;
					*x = s->img_x;
					*y = s->img_y;
					*comp = s->img_n;

					out = (stbi_uc *)stbi__malloc(s->img_n * s->img_x * s->img_y);
					if (!out) return stbi__errpuc("outofmem", "Out of memory");
					stbi__getn(s, out, s->img_n * s->img_x * s->img_y);

					if (req_comp && req_comp != s->img_n) {
						out = stbi__convert_format(out, s->img_n, req_comp, s->img_x, s->img_y);
						if (out == NULL) return out; // stbi__convert_format frees input on failure
					}
					return out;
				}

				int      stbi__pnm_isspace(char c)
				{
					return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
				}

				void     stbi__pnm_skip_whitespace(stbi__context *s, char *c)
				{
					while (!stbi__at_eof(s) && stbi__pnm_isspace(*c))
						*c = (char)stbi__get8(s);
				}

				int      stbi__pnm_isdigit(char c)
				{
					return c >= '0' && c <= '9';
				}

				int      stbi__pnm_getinteger(stbi__context *s, char *c)
				{
					int value = 0;

					while (!stbi__at_eof(s) && stbi__pnm_isdigit(*c)) {
						value = value * 10 + (*c - '0');
						*c = (char)stbi__get8(s);
					}

					return value;
				}

				int      stbi__pnm_info(stbi__context *s, int *x, int *y, int *comp)
				{
					int maxv;
					char c, p, t;

					stbi__rewind(s);

					// Get identifier
					p = (char)stbi__get8(s);
					t = (char)stbi__get8(s);
					if (p != 'P' || (t != '5' && t != '6')) {
						stbi__rewind(s);
						return 0;
					}

					*comp = (t == '6') ? 3 : 1;  // '5' is 1-component .pgm; '6' is 3-component .ppm

					c = (char)stbi__get8(s);
					stbi__pnm_skip_whitespace(s, &c);

					*x = stbi__pnm_getinteger(s, &c); // read width
					stbi__pnm_skip_whitespace(s, &c);

					*y = stbi__pnm_getinteger(s, &c); // read height
					stbi__pnm_skip_whitespace(s, &c);

					maxv = stbi__pnm_getinteger(s, &c);  // read max value

					if (maxv > 255)
						return stbi__err("max value > 255", "PPM image not 8-bit");
					else
						return 1;
				}
#endif

				int stbi__info_main(stbi__context *s, int *x, int *y, int *comp)
				{
#ifndef STBI_NO_JPEG
					if (stbi__jpeg_info(s, x, y, comp)) return 1;
#endif

#ifndef STBI_NO_PNG
					if (stbi__png_info(s, x, y, comp))  return 1;
#endif

#ifndef STBI_NO_GIF
					if (stbi__gif_info(s, x, y, comp))  return 1;
#endif

#ifndef STBI_NO_BMP
					if (stbi__bmp_info(s, x, y, comp))  return 1;
#endif

#ifndef STBI_NO_PSD
					if (stbi__psd_info(s, x, y, comp))  return 1;
#endif

#ifndef STBI_NO_PIC
					if (stbi__pic_info(s, x, y, comp))  return 1;
#endif

#ifndef STBI_NO_PNM
					if (stbi__pnm_info(s, x, y, comp))  return 1;
#endif

#ifndef STBI_NO_HDR
					if (stbi__hdr_info(s, x, y, comp))  return 1;
#endif

					// test tga last because it's a crappy test!
#ifndef STBI_NO_TGA
					if (stbi__tga_info(s, x, y, comp))
						return 1;
#endif
					return stbi__err("unknown image type", "Image not of any known type, or corrupt");
				}

#ifndef STBI_NO_STDIO
				int stbi_info(char const *filename, int *x, int *y, int *comp)
				{
					FILE *f = stbi__fopen(filename, "rb");
					int result;
					if (!f) return stbi__err("can't fopen", "Unable to open file");
					result = stbi_info_from_file(f, x, y, comp);
					fclose(f);
					return result;
				}

				int stbi_info_from_file(FILE *f, int *x, int *y, int *comp)
				{
					int r;
					stbi__context s;
					long pos = ftell(f);
					stbi__start_file(&s, f);
					r = stbi__info_main(&s, x, y, comp);
					fseek(f, pos, SEEK_SET);
					return r;
				}
#endif // !STBI_NO_STDIO

				int stbi_info_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp)
				{
					stbi__context s;
					stbi__start_mem(&s, buffer, len);
					return stbi__info_main(&s, x, y, comp);
				}

				int stbi_info_from_callbacks(stbi_io_callbacks const *c, void *user, int *x, int *y, int *comp)
				{
					stbi__context s;
					stbi__start_callbacks(&s, (stbi_io_callbacks *)c, user);
					return stbi__info_main(&s, x, y, comp);
				}
			} // namespace stbi::decode
		} // namespace stbi
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
				str = str.substr(pos + 1);
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

		std::string rskip_all(std::string str, std::string delim)
		{
			auto pos = str.find(delim);
			if (pos == std::string::npos)
			{
				return str;
			}
			else
			{
				return str.substr(0, pos);
			}
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
			std::string second = (pos != std::string::npos ? s.substr(pos + 1) : std::string());
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
		Path::Path(std::string path, bool isAbsolute)
		{
			if (isAbsolute) abspath_ = path;
			else abspath_ = os::absolute_path(path);
		}

		bool Path::empty() const
		{
			return abspath_.empty();
		}

		bool Path::exist() const
		{
			if (empty()) return false;
			return os::path_exists(abspath_, true);
		}

		bool Path::is_file() const
		{
			return os::is_file(abspath_);
		}

		bool Path::is_dir() const
		{
			return os::is_directory(abspath_);
		}

		std::string Path::abs_path() const
		{
			return abspath_;
		}

		std::string Path::relative_path() const
		{
			std::string cwd = os::current_working_directory() + os::path_delim();
			if (fmt::starts_with(abspath_, cwd))
			{
				return fmt::lstrip(abspath_, cwd);
			}
			return relative_path(cwd);
		}

		std::string Path::relative_path(std::string root) const
		{
			if (!fmt::ends_with(root, os::path_delim())) root += os::path_delim();
			if (fmt::starts_with(abspath_, root))
			{
				return fmt::lstrip(abspath_, root);
			}

			auto rootParts = os::path_split(root);
			auto thisParts = os::path_split(abspath_);
			std::size_t commonParts = 0;
			std::size_t size = (std::min)(rootParts.size(), thisParts.size());
			for (std::size_t i = 0; i < size; ++i)
			{
				if (!os::path_identical(rootParts[i], thisParts[i])) break;
				++commonParts;
			}

			if (commonParts == 0)
			{
				log::detail::zupply_internal_warn("Unable to resolve relative path to: " + root + "! Return absolute path.");
				return abspath_;
			}

			std::vector<std::string> tmp;
			// traverse back from root, add ../ to path
			for (std::size_t pos = rootParts.size(); pos > commonParts; --pos)
			{
				tmp.push_back("..");
			}
			// forward add parts of this path
			for (std::size_t pos = commonParts; pos < thisParts.size(); ++pos)
			{
				tmp.push_back(thisParts[pos]);
			}
			return os::path_join(tmp);
		}

		std::string Path::filename() const
		{
			if (is_file()) return os::path_split_filename(abspath_);
			return std::string();
		}

		Directory::Directory(std::string root, bool recursive)
			:root_(root), recursive_(recursive)
		{
			resolve();
		}

		Directory::Directory(std::string root, std::string pattern, bool recursive)
			: root_(root), recursive_(recursive)
		{
			resolve();
			filter(pattern);
		}

		bool Directory::is_recursive() const
		{
			return recursive_;
		}

		std::string Directory::root() const
		{
			return root_.abs_path();
		}

		std::vector<Path> Directory::to_list() const
		{
			return paths_;
		}

		void Directory::resolve()
		{
			std::deque<std::string> workList;
			if (root_.is_dir())
			{
				workList.push_back(root_.abs_path());
			}

			while (!workList.empty())
			{
				std::vector<std::string> list = os::list_directory(workList.front());
				workList.pop_front();
				for (auto i = list.begin(); i != list.end(); ++i)
				{
					if (os::is_file(*i))
					{
						paths_.push_back(Path(*i));
					}
					else if (os::is_directory(*i))
					{
						paths_.push_back(Path(*i));
						if (recursive_) workList.push_back(*i);	// add subdir work list
					}
					else
					{
						// this should not happen unless file/dir modified during runtime
						// ignore the error
					}
				}
			}
		}

		void Directory::filter(std::string pattern)
		{
			std::vector<Path> filtered;
			for (auto entry : paths_)
			{
				if (entry.is_file())
				{
					std::string filename = os::path_split_filename(entry.abs_path());
					if (fmt::wild_card_match(filename.c_str(), pattern.c_str()))
					{
						filtered.push_back(entry);
					}
				}
			}
			paths_ = filtered;	// replace all
		}

		void Directory::reset()
		{
			resolve();
		}

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

		bool FileEditor::open(const char* filename, bool truncateOrNot, int retryTimes, int retryInterval)
		{
			this->close();
			// use absolute path
			filename_ = os::absolute_path(filename);
			// try open
			return this->try_open(retryTimes, retryInterval, truncateOrNot);
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

		std::size_t FileReader::count_lines()
		{
			// store current read location
			std::streampos readPtrBackup = istream_.tellg();
			istream_.seekg(std::ios_base::beg);

			const int bufSize = 1024 * 1024;	// using 1MB buffer
			std::vector<char> buf(bufSize);

			std::size_t ct = 0;
			std::streamsize nbuf = 0;
			char last = 0;
			do
			{
				istream_.read(&buf.front(), bufSize);
				nbuf = istream_.gcount();
				for (auto i = 0; i < nbuf; i++)
				{
					last = buf[i];
					if (last == '\n')
					{
						++ct;
					}
				}
			} while (nbuf > 0);

			if (last != '\n') ++ct;

			// restore read position
			istream_.clear();
			istream_.seekg(readPtrBackup);

			return ct;
		}

		std::string FileReader::next_line(bool trimWhitespaces)
		{
			std::string line;

			if (!istream_.good() || !istream_.is_open())
			{
				return line;
			}

			if (!istream_.eof())
			{
				std::getline(istream_, line);
				if (trimWhitespaces) return fmt::trim(line);
				if (line.back() == '\r')
				{
					// LFCR \r\n problem
					line.pop_back();
				}
			}
			return line;
		}

		int FileReader::goto_line(int n)
		{
			if (!istream_.good() || !istream_.is_open())
				return -1;

			istream_.seekg(std::ios::beg);

			if (n < 0)
			{
				throw ArgException("Jumping to a negtive position!");
			}

			if (n == 0)
			{
				return 0;
			}

			int i = 0;
			for (i = 0; i < n - 1; ++i)
			{

				istream_.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');

				if (istream_.eof())
				{
					log::detail::zupply_internal_warn("Reached end of file, line: " + std::to_string(i + 1));
					break;
				}
			}

			return i + 1;
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
			static const std::string kNativePathDelimWindows = "\\";
			static const std::string kNativePathDelimPosix = "/";
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

		int is_atty()
		{
#if ZUPPLY_OS_WINDOWS
			return _isatty(_fileno(stdout));
#elif ZUPPLY_OS_UNIX
			return isatty(fileno(stdout));
#endif
			return 0;
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
			fstream_open(dstf, dst, std::ios::binary | std::ios::trunc);
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
			if (os::is_directory(path))
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
			if (!is_directory(path)) return true;
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
			return (is_directory(path) == false);
#endif
		}

		bool remove_file(std::string path)
		{
#if ZUPPLY_OS_WINDOWS
			_wremove(utf8_to_wstring(path).c_str());
#else
			unlink(path.c_str());
#endif
			return (is_file(path) == false);
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

		std::string path_delim()
		{
#if ZUPPLY_OS_WINDOWS
			return consts::kNativePathDelimWindows;
#else // posix
			return consts::kNativePathDelimPosix;
#endif
		}

		std::string current_working_directory()
		{
#if ZUPPLY_OS_WINDOWS
			wchar_t *buffer = nullptr;
			if ((buffer = _wgetcwd(nullptr, 0)) == nullptr)
			{
				// failed
				log::detail::zupply_internal_warn("Unable to get current working directory!");
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
				log::detail::zupply_internal_warn("Unable to get current working directory!");
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
				log::detail::zupply_internal_warn("Unable to get current working directory!");
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

		bool path_identical(std::string first, std::string second, bool forceCaseSensitve)
		{
#if ZUPPLY_OS_WINDOWS
			if (!forceCaseSensitve)
			{
				return fmt::to_lower_ascii(first) == fmt::to_lower_ascii(second);
			}
			return first == second;
#else
			return first == second;
#endif
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
				log::detail::zupply_internal_warn("Unable to get absolute path for: " + reletivePath + "! Return original.");
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
					// try recover manually
					std::string dirtyPath;
					if (fmt::starts_with(reletivePath, "/"))
					{
						// already an absolute path
						dirtyPath = reletivePath;
					}
					else
					{
						dirtyPath = path_join({ current_working_directory(), reletivePath });
					}
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
					std::string tmp = path_join(ret);
					if (!fmt::starts_with(tmp, "/")) tmp = "/" + tmp;
					return tmp;
				}
				//still failed
				log::detail::zupply_internal_warn("Unable to get absolute path for: " + reletivePath + "! Return original.");
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

			while (!is_directory(tmp))
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
			return is_directory(path);
		}

		std::vector<std::string> list_directory(std::string root)
		{
			std::vector<std::string> ret;
			root = os::absolute_path(root);
			if (os::is_file(root))
			{
				// it's a file, not dir
				log::detail::zupply_internal_warn(root + " is a file, not a directory!");
				return ret;
			}
			if (!os::is_directory(root)) return ret;	// not a dir

#if ZUPPLY_OS_WINDOWS
			std::wstring wroot = os::utf8_to_wstring(root);
			wchar_t dirPath[1024];
			wsprintfW(dirPath, L"%s\\*", wroot.c_str());
			WIN32_FIND_DATAW f;
			HANDLE h = FindFirstFileW(dirPath, &f);
			if (h == INVALID_HANDLE_VALUE) { return ret; }

			do
			{
				const wchar_t *name = f.cFileName;
				if (lstrcmpW(name, L".") == 0 || lstrcmpW(name, L"..") == 0) { continue; }
				std::wstring path = wroot + L"\\" + name;
				ret.push_back(os::wstring_to_utf8(path));
			} while (FindNextFileW(h, &f));
			FindClose(h);
			return ret;
#else
			DIR *dir;
			struct dirent *entry;

			if (!(dir = opendir(root.c_str())))
				return ret;
			if (!(entry = readdir(dir)))
				return ret;

			do {
				if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
					continue;
				std::string path = root + "/" + entry->d_name;
				ret.push_back(path);
			} while ((entry = readdir(dir)));
			closedir(dir);
			return ret;
#endif
			return ret;
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
			os::ifstream_open(stream_, filename, std::ios::in | std::ios::binary);
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
				line_ = fmt::rskip_all(line_, "#");
				line_ = fmt::rskip_all(line_, ";");
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
					ls = parse_section(std::string(line_.substr(1, line_.length() - 2)), ls);
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
			min_(0), max_(1), once_(false), count_(0),
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

		Value ArgOption::get_value()
		{
			return val_;
		}

		int ArgOption::get_count()
		{
			return count_;
		}

		std::string ArgOption::get_help()
		{
			// target: '-i, --input=FILE		this is an example input(default: abc.txt)'
			const std::size_t alignment = 26;
			std::string ret;

			// place holders
			if (shortKey_ == -1 && longKey_.empty() && min_ > 0)
			{
				ret = "<" + type_ + ">";
			}
			else
			{
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
			}

			if (ret.size() < alignment)
			{
				while (ret.size() < alignment)
					ret.push_back(' ');
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

		std::string ArgOption::get_short_help()
		{
			// [-a=<DOUBLE>] [--long=<INT> [<INT>]]...
			std::string ret = "-";
			if (shortKey_ != -1)
			{
				ret.push_back(shortKey_);
			}
			else if (!longKey_.empty())
			{
				ret += "-" + longKey_;
			}
			else
			{
				ret = "<" + type_ + ">";
				return ret;
			}

			// type info
			std::string st = "<" + type_ + ">";
			if (min_ > 0)
			{
				if (required_) ret.push_back('=');
				else ret.push_back(' ');
				ret += st;
				for (int i = 2; i < min_; ++i)
				{
					ret.push_back(' ');
					ret += st;
				}
				if (max_ == -1 || max_ > min_)
				{
					ret += " {" + st + "}...";
				}
			}

			if (!required_)
			{
				ret = "[" + ret + "]";
			}
			return ret;
		}

		ArgParser::ArgParser() : info_({ "program", "?" })
		{
			add_opt_internal(-1, "").set_max(0);	// reserve for help
			add_opt_internal(-1, "").set_max(0);	// reserve for version
		}

		void ArgParser::add_info(std::string info)
		{
			info_.push_back(info);
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

		ArgOption& ArgParser::add_opt_internal(char shortKey, std::string longKey)
		{
			options_.push_back(ArgOption(shortKey, longKey));
			if (shortKey != -1 || (!longKey.empty()))
			{
				register_keys(shortKey, longKey, options_.size() - 1);
			}
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
				.call([this]{std::cout << this->get_help() << std::endl; std::exit(0); })
				.set_min(0).set_max(0);
		}

		void ArgParser::add_opt_version(char shortKey, std::string longKey, std::string version, std::string help)
		{
			info_[1] = version;			// info_[1] reserved for version string
			auto& opt = options_[1];	// options_[1] reserved for version option
			register_keys(shortKey, longKey, 1);
			opt.shortKey_ = shortKey;
			opt.longKey_ = longKey;
			opt.set_help(help)
				.call([this]{std::cout << this->version() << std::endl; std::exit(0); })
				.set_min(0).set_max(0);
		}

		ArgOption& ArgParser::add_opt_flag(char shortKey, std::string longKey, std::string help, bool* dst)
		{
			auto& opt = add_opt(shortKey, longKey).set_help(help).set_min(0).set_max(0);
			if (nullptr != dst)
			{
				opt.call([dst]{*dst = true; }, [dst]{*dst = false; });
			}
			return opt;
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

			// 3. placeholders
			for (auto o = options_.begin(); o != options_.end(); ++o)
			{
				if (o->shortKey_ == -1 && o->longKey_.empty())
				{
					int n = o->max_;
					while (n > 0 && args_.size() > 0)
					{
						o->val_ = o->val_.str() + " " + args_[0].str();
						++o->count_;
						--n;
						args_.erase(args_.begin());
					}
					o->val_ = fmt::trim(o->val_.str());
					if (args_.empty()) break;
				}
			}

			// 4. callbacks
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


			// 5. check errors
			for (auto o : options_)
			{
				if (o.required_ && (o.count_ < 1))
				{
					errors_.push_back("[ArgParser Error]: Required option not found: " + o.get_short_help());
				}
				if (o.count_ > 1 && o.min_ > o.size_)
				{
					errors_.push_back("[ArgParser Error]:" + o.get_short_help() + " need at least " + std::to_string(o.min_) + " arguments.");
				}
				if (o.count_ > 1 && o.once_)
				{
					errors_.push_back("[ArgParser Error]:" + o.get_short_help() + " limited to be called only once.");
				}
			}
		}

		std::string ArgParser::get_help()
		{
			std::string ret("Usage: ");
			ret += info_[0];
			std::string usageLine;

			// single line options list
			for (auto opt : options_)
			{
				if (!opt.required_ && opt.max_ == 0 && opt.shortKey_ != -1)
				{
					// put optional short flags together [-abcdefg...]
					usageLine.push_back(opt.shortKey_);
				}
			}
			if (!usageLine.empty())
			{
				usageLine = " [-" + usageLine + "]";
			}

			// options that take arguments
			for (auto opt : options_)
			{
				if (!opt.required_ && opt.max_ == 0) continue;
				std::string tmp = " " + opt.get_short_help();
				if (!tmp.empty()) usageLine += tmp;
			}

			ret += " " + usageLine + "\n";

			for (std::size_t i = 2; i < info_.size(); ++i)
			{
				ret += "  " + info_[i] + "\n";
			}

			// required options first
			ret += "\n  Required options:\n";
			for (auto opt : options_)
			{
				if (opt.required_)
				{
					std::string tmpLine = "  " + opt.get_help() + "\n";
					if (fmt::trim(tmpLine).empty()) continue;
					ret += tmpLine;
				}
			}
			// optional options
			ret += "\n  Optional options:\n";
			for (auto opt : options_)
			{
				if (!opt.required_)
				{
					std::string tmpLine = "  " + opt.get_help() + "\n";
					if (fmt::trim(tmpLine).empty()) continue;
					ret += tmpLine;
				}
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
		ProgBar::ProgBar(unsigned range, std::string info)
			: ss_(nullptr), info_(info)
		{
			range_ = range;
			pos_ = 0;
			running_ = false;
			timer_.reset();
			sprintf(rate_, "%.1f/sec", 0.0);
			start();
		}

		ProgBar::~ProgBar()
		{
			stop();
		}

		void ProgBar::step(unsigned size)
		{
			pos_ += size;
			if (pos_ >= range_)
			{
				stop();
			}
			calc_rate(size);
			timer_.reset();
		}

		void ProgBar::calc_rate(unsigned size)
		{
			double interval = timer_.elapsed_sec_double();
			if (interval < 1e-12) interval = 1e-12;
			double rate = size / interval;
			if (rate > 1073741824)
			{
				sprintf(rate_, "%.1e/s", rate);
			}
			else if (rate > 1048576)
			{
				sprintf(rate_, "%.1fM/sec", rate / 1048576);
			}
			else if (rate > 1024)
			{
				sprintf(rate_, "%.1fK/sec", rate / 1024);
			}
			else if (rate > 0.1)
			{
				sprintf(rate_, "%.1f/sec", rate);
			}
			else if (rate * 60 > 1)
			{
				sprintf(rate_, "%.1f/min", rate * 60);
			}
			else if (rate * 3600 > 1)
			{
				sprintf(rate_, "%.1f/sec", rate * 3600);
			}
			else
			{
				sprintf(rate_, "%.1e/s", rate);
			}
		}

		void ProgBar::start()
		{
			if (os::is_atty())
			{
				oldCout_ = std::cout.rdbuf(buffer_.rdbuf());
				oldCerr_ = std::cerr.rdbuf(buffer_.rdbuf());
				ss_.rdbuf(oldCout_);
				//ss_ << std::endl;
				running_ = true;
				worker_ = std::thread([this]{ this->bg_work(); });
			}
		}

		void ProgBar::stop()
		{
			if (running_)
			{
				running_ = false;
				worker_.join();	// join worker thread
				draw();
				std::cout.rdbuf(oldCout_);
				std::cerr.rdbuf(oldCerr_);
				std::cout << std::endl;
			}
		}

		void ProgBar::draw()
		{
			Size sz = os::console_size();
			// clear line
			ss_ << "\r";
			for (int i = 1; i < sz.width; ++i)
			{
				ss_ << " ";
			}
			ss_ << "\r";
			// flush stored messages
			std::string buf = buffer_.str();
			if (!buf.empty())
			{
				ss_ << buf;
				if (buf.back() != '\n')
				{
					ss_ << "\n";
				}
				try
				{
					buffer_.str(std::string());
				}
				catch (...)
				{
					// suppress the exception as a hack
				}
			}

			const int reserved = 21;	// leave some space for info
			int available = sz.width - reserved - static_cast<int>(info_.size()) - 2;
			if (available > 10)
			{
				int cnt = 0;
				int len = static_cast<int>(pos_ / static_cast<double>(range_)* available) - 1;
				ss_ << info_ << "[";
				for (int i = 0; i < len; ++i)
				{
					ss_ << "=";
					++cnt;
				}
				if (len >= 0)
				{
					ss_ << ">";
					++cnt;
				}
				for (int i = cnt; i < available; ++i)
				{
					ss_ << " ";
				}
				ss_ << "] ";
				float perc = 100.f * pos_ / range_;
				ss_ << std::fixed << std::setprecision(1) << perc;
				ss_ << "% (";
				ss_ << std::string(rate_);
				ss_ << ")";
				ss_.flush();
			}

		}

		void ProgBar::bg_work()
		{
			while (running_)
			{
				draw();
				time::sleep(66);
			}
		}

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
							logger->detach_all_sinks();
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

		void Logger::detach_all_sinks()
		{
			sinks_.clear();
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
					psink = log::get_sink(sinkname);
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
