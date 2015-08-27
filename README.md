# Zupply - A light-weight portable C++ 11 library for researches and demos

## Introduction
**Zupply** is a light-weight, cross-platform, easy to use C++11 library packed with fundamental functions/classes 
for reaserches/small projects/demos.

#### Why Zupply
- Tired of repetitive coding on reading/writing files? Logging messages? Loading configurations?
- Feel desperate transfering code to another platform because you hard-coded in platform specific ways?
- Currently using Boost which is good, but want a small and portable one after all.
- Just want to build a small demo, why bother libraries such as OpenCV just for reading/writing images?

- ##### If you agree at least two of them, zupply will be the right tool.

#### Features
- Zero dependency, only C++ 11 starndard
- Designed to be easily included in projects, no need to link
- Pure and clean, everything wrapped in namespace zz, almost no polution if you don't expose the namespace
- Targeting linux/windows/Mac OS X, meanwhile providing unified experience coding on each platform

#### What's included
- Command line argument parser
- INI/CFG configuration file parser
- Easy to use Timer/Date class to measure time and date
- Fast sync/async logger with rich information and highly configurable
- Cross-platform functions to handle filesystems, such as create directory, check file existance, etc...
- Various formatting functions to trim/split/replace strings
- Thread safe data structures for specific purposes
- A lot more

#### What's under construction
- Image IO functions: to read/write JPEG/PNG/BMP/TGA and GIF probably
- Serializer/Deserializer: for dump/read objects to/from string directly, binary should also be supported
- Progress bar class that is easy to use

## Usage
##### zupply is designed to be as easy to integrate as possible, thus you can:
- Copy zupply.hpp and zupply.cpp into your project
- Start writing code
```c++
#include "zupply.hpp"
// optional using namespace zz
using namespace zz;

// write your own code
int main(int argc, char** argv)
{
    auto logger = log::get_logger("default");
    logger->info("Welcome to zupply!");
    return 0;
}
```
- Build and run

##### Note: you will need a compiler which support C++11 features, the following compilers are tested
- MSVC 2013/2015
- G++ 4.9.x and newer
- Clang++ 3.5 or newer

## Wiki
###### So much to write, try hard :P
#### Contents
1. [Date](#date) and [Timer](#timer)
2. [Logger](#logger)
3. [Directory](#directory) and [File](#file)
4. [Formatter](#formatter)
5. [Argument Parser](#argument-parser)
6. [INI or CFG Parser](#configure-parser)

#### Date
##### Example code
```c++
// show current date in local time zone
zz::time::Date date;
std::cout << "Local time(Pacific) " << date.to_string() << std::endl;
// convert to UTC time
date.to_utc_time();
std::cout << "UTC time " << date.to_string() << std::endl;
// customize to_string format
auto str = date.to_string("%m/%d %a %H:%M:%S.%frac");
std::cout << "With format [%m / %d %a %H:%M : %S.%frac] : " << str << std::endl;
str = date.to_string("%c");
std::cout << "With format %c(standard, local specific): " << str << std::endl;
// another way is using static function
std::cout << "Call UTC directly " << zz::time::Date::utc_time().to_string() << std::endl;
```
#####Output:
```
Local time(Pacific) 15-08-26 16:48:03.520
UTC time 15-08-26 23:48:03.520
With format [%m / %d %a %H:%M : %S.%frac] : 08/26 Wed 23:48:03.520
With format %c(standard, local specific): 08/26/15 23:48:03
Call UTC directly 15-08-26 23:48:03.521
```
##### Supported format specifier 
|Specifier|       Description      |
|---------|------------------------|
|  %frac  | fraction of time less than a second, 3 digits                   |
|    %y   | writes last 2 digits of year as a decimal number (range [00,99])|
|    %m   | writes month as a decimal number (range [01,12])                |
|    %d   | writes day of the month as a decimal number (range [01,31])     |
|    %a   | writes abbreviated weekday name, e.g. Fri (locale dependent)    |
|    %H   | writes hour as a decimal number, 24 hour clock (range [00-23])  |
|    %M   | writes minute as a decimal number (range [00,59])               |
|    %S   | writes second as a decimal number (range [00,60])               |
|   ...   | many more options, check [std::put_time](#http://en.cppreference.com/w/cpp/io/manip/put_time) documentation|
|   %%    | use %% to skip %                                                |

#### Timer
##### Example Code
```c++
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
```
#####Output
```
Summation: 499998500001 elapsed time: [2 ms]
Subtraction: 0 elapsed time: [3002 us]
sec: 0
msec: 3
usec: 3002
nsec: 3002300
```
##### Supported format specifier 
|Specifier|       Description      |
|---------|------------------------|
|  %sec  | quantize in second resolution|
|  %ms   | quantize in millisecond resolution|
|  %us   | quantize in microsecond resolution|
|  %ns   | quantize in nanosecond resolution|
#### Logger

#### Directory

#### File

#### Formatter

#### Argument Parser

#### Configure Parser
