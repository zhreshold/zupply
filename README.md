# zupply
Light-weight portable C++ 11 library for researches and demos

## Introduction
**Zupply** is a light-weight, cross-platform, easy to use C++11 library packed with fundamental functions/classes 
for reaserches/small projects/demos.

#### Why zupply
- Tired of repetitive coding on reading/writing files? Logging messages? Loading configurations?
- Feel desperate transfering code to another platform because you hard-coded in platform specific ways?
- Currently using Boost which is good, but want a small and portable one after all.
- Just want to build a small demo, why bother libraries such as OpenCV just for reading/writing images?

##### If you agree at least two of them, zupply will be the right tool.

#### What's included in zupply
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
- Serialize/Deserialize the majority of data structures

## Usage
##### zupply is designed to be as easy to integrate as possible, thus you can:
- Copy zupply.hpp and zupply.cpp into your project
- Start writing code
```
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
1. [Date](#Date) and [Timer](#Timer)
2. [Logger](#Logger)
3. [Directory](#Directory)
4. [File](#File)
5. [Formatter](#Formatter)
6. [Argument Parser](#Argument-Parser)
7. [INI or CFG Parser](#Configure-Parser)

#### Date

#### Timer

#### Logger

#### Directory

#### File

#### Formatter

#### Argument Parser

#### Configure Parser
