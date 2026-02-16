#include "sparsifier_wrapper/sparsifier_backend.hpp"

bool SparsifierBackend::generate_kernel(const vector<string>& mutated_kernel_file_names, const fs::path& output_dir) {
    cout << "GENERATE Sparsifier KERNELS" << endl;
    
    return true;
}

int SparsifierBackend::execute_kernel(const fs::path& kernelPath, const fs::path& outputDir) {
    cout << "EXECUTE Sparsifier KERNEL" << endl;
    return 0;
}

bool SparsifierBackend::compare_results(const string& refDir, const string& testDir) {
    cout << "COMPARE Sparsifier RESULTS" << endl;
    return true;
}

// Plugin entry points
extern "C" FuzzBackend* create_backend() {
    return new SparsifierBackend();
}

extern "C" void destroy_backend(FuzzBackend* backend) {
    delete backend;
}

