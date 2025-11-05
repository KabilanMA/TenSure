#include "taco_wrapper/taco_backend.hpp"
#include "taco_wrapper/generator.hpp"
#include "taco_wrapper/executor.hpp"
#include "taco_wrapper/comparator.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>


bool generate_taco_kernel(const tsKernel& kernel, const string& outFile) {
    try {
        // generate TACO program string
        string program_code = taco_wrapper::generate_program(kernel);

        // atomic write
        string tmp_name = outFile + ".tmp";
        ofstream ofs(tmp_name);
        ofs << program_code;
        ofs.close();
        fs::rename(tmp_name, (outFile + ".cpp")); // atomic replacement
    } catch (const exception& e) {
        cerr << "TacoBackend::generate_kernel failed: " << e.what() << endl;
        return false;
    }

    return true;
}

bool TacoBackend::generate_kernel(const vector<string>& mutated_kernel_file_names, const fs::path& output_dir) {
    // Call your existing executor.cpp function
    for (auto &mutated_file_name : mutated_kernel_file_names) {
        fs::path p(mutated_file_name);
        string taco_kernel_file = output_dir / (p.stem().string());
        // std::cout << taco_kernel_file << std::endl;
        tsKernel tskernel;
        tskernel.loadJson(mutated_file_name);
        generate_taco_kernel(tskernel, taco_kernel_file);
    }
    
    return true;
}

bool TacoBackend::execute_kernel(const string& kernelPath, const string& outputDir) {
    // Call your existing executor.cpp function
    return taco_wrapper::run_kernel(kernelPath, outputDir);
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
