#pragma once
#include "backends/backend_interface.hpp"
#include "sparsifier_wrapper/generator.hpp"
#include "sparsifier_wrapper/executor.hpp"
#include "sparsifier_wrapper/comparator.hpp"

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

struct SparsifierBackend : public FuzzBackend {
    bool generate_kernel(const vector<string>& mutated_kernel_file_names, const fs::path& output_dir) override;

    int execute_kernel(const fs::path& kernelPath, const fs::path& outputDir) override;

    bool compare_results(const string& refDir,
                         const string& testDir) override;
};

// Plugin entry points
extern "C" FuzzBackend* create_backend();
extern "C" void destroy_backend(FuzzBackend* backend);
