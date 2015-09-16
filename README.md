# Zupply - A light-weight portable C++ 11 library for Researches and Demos

![Build Status](https://travis-ci.org/ZhreShold/zupply.svg)

## Introduction
**Zupply** is a light-weight, cross-platform, easy to use C++11 library packed with fundamental functions/classes best
for reaserches/small projects/demos.

#### Why Zupply
- Tired of repetitive coding on reading/writing files? Logging messages? Loading configurations?
- Feel desperate transferring code to another platform because you hard-coded in platform specific ways?
- Reluctant to use Boost because it's too heavy-weight?
- Hate setting up environments on a clean computer without any develop library which is required to be linked by many programs?
- Just want to build a small demo, why bother libraries such as OpenCV just for reading/writing images?

- ##### If you agree at least two of them, zupply will be the right tool.

#### Features
- Zero dependency, only C++ 11 standard
- Designed to be easily included in projects, no need to link
- Pure and clean, everything wrapped in namespace zz, almost no pollution if you don't expose the namespace
- Targeting Linux/Windows/Mac OS X/Partial Unix based OSes, meanwhile providing unified experience coding on each platform

#### What's included
- Command line argument parser
- INI/CFG configuration file parser
- Easy to use Timer/DateTime class to measure time and date
- Fast sync/async logger with rich information and highly configurable
- Cross-platform functions to handle file-systems, such as create directory, check file existance, etc...
- Various formatting functions to trim/split/replace strings
- Thread safe data structures for specific purposes
- A lot more

#### What's under construction
- Generate list of files/sub-directories with wildcard matching(simple but very useful)
- Image IO functions: to read/write JPEG/PNG/BMP/TGA and GIF probably
- Serializer/Deserializer: for dump/read objects to/from string directly, binary should also be supported
- Progress bar class that is easy to use

## Usage
##### zupply is designed to be as easy to integrate as possible, thus you can:
- Copy zupply.hpp and zupply.cpp into your project
- Start writing code
```c++
#include "zupply.hpp"
using namespace zz; // optional using namespace zz for ease

// write your own code
int main(int argc, char** argv)
{
    auto logger = log::get_logger("default");
    logger->info("Welcome to zupply!");
    return 0;
}
```
- Build and run

##### Note: you will need a compiler which support C++11 features, the following compilers/libraries are tested
- vc++12(Visual Studio 2013) or newer
```
# create visual studio project require cmake
cd build & create_visual_studio_2013_project.bat
```
- g++ 4.8.1 or newer(link pthread because gcc require it!!)
```
# with cmake
cd build
cmake .
make
# or manual build without cmake
cd build
g++ -std=c++11 -pthread ../unittest/unittest.cpp ../src/zupply.cpp -lpthread -o unittest
```
- Clang++ 3.3 or newer
```
# using cmake is identical to gcc
# or manually build with clang++
cd build
clang++ -std=c++11 ../unittest/unittest.cpp ../src/zupply.cpp -o unittest
```

## Documentation
Full [documentation](http://zhreshold.github.io/zupply/) supplied.

## Tutorials
For tutorials, please check Zupply [Wiki](https://github.com/ZhreShold/zupply/wiki)!

## License
Zupply uses very permissive [MIT](https://opensource.org/licenses/MIT) license.