#include "taco_wrapper/taco_backend.hpp"
#include "taco_wrapper/generator.hpp"
#include "taco_wrapper/executor.hpp"
#include "taco_wrapper/comparator.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>


bool TacoBackend::generate_kernel(const vector<string>& mutated_kernel_file_names, const fs::path& output_dir) {
    // Call your existing executor.cpp function
    for (auto &mutated_file_name : mutated_kernel_file_names) {
        fs::path p(mutated_file_name);
        fs::path taco_kernel_file = output_dir / (p.stem());
        // cout << "taco_kernel_file: " << taco_kernel_file << endl;
        fs::create_directories(taco_kernel_file);
        // std::cout << taco_kernel_file << std::endl;
        tsKernel tskernel;
        tskernel.loadJson(mutated_file_name);
        taco_wrapper::generate_taco_kernel(tskernel, taco_kernel_file);
    }
    
    return true;
}

bool TacoBackend::execute_kernel(const string& kernelPath, const string& outputDir) {
    // Call your existing executor.cpp function
    return taco_wrapper::run_kernel(kernelPath);
}

bool TacoBackend::compare_results(const string& refDir, const string& testDir) {
    // Call your existing comparator.cpp function
    return taco_wrapper::compare_outputs(refDir, testDir);
}

// Plugin entry points
extern "C" FuzzBackend* create_backend() {
    return new TacoBackend();
}

extern "C" void destroy_backend(FuzzBackend* backend) {
    delete backend;
}
