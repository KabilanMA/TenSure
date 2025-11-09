#pragma once

#include <string>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <chrono>
#include <thread>

namespace taco_wrapper
{
using namespace std;

bool run_kernel(const string& kernelPath);
}
