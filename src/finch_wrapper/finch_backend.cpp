#include "finch_wrapper/finch_backend.hpp"

bool FinchBackend::generate_kernel(const vector<string>& mutated_kernel_file_names, const fs::path& output_dir) {
    cout << "GENERATE FINCH KERNELS" << endl;
    
    return true;
}

int FinchBackend::execute_kernel(const fs::path& kernelPath, const fs::path& outputDir) {
    cout << "EXECUTE FINCH KERNEL" << endl;
    return 0;
}

bool FinchBackend::compare_results(const string& refDir, const string& testDir) {
    cout << "COMPARE FINCH RESULTS" << endl;
    return true;
}

// Plugin entry points
extern "C" FuzzBackend* create_backend() {
    return new FinchBackend();
}

extern "C" void destroy_backend(FuzzBackend* backend) {
    delete backend;
}

