
#include <nlohmann/json.hpp>
#include <signal.h>
#include <thread>
#include <chrono>
#include <filesystem>
#include <string>

#include "tensure/random_gen.hpp"
#include "taco_wrapper/generator.hpp"

#include "taco.h"

using namespace taco;
using namespace std;
namespace fs = std::filesystem;


static inline bool g_terminate = false;

// Utility: produce timestamped filename
std::string timestamp_str() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto t = system_clock::to_time_t(now);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y%m%d-%H%M%S", std::localtime(&t));
    return std::string(buf);
}

void signal_handler(int signum) {
    std::cerr << "Signal " << signum << " received. Will terminate after current iteration.\n";
    g_terminate = true;
}

int main(int argc, char * argv[])
{

    // auto [tensors, einsum] = generate_random_einsum(2, 3);
    // vector<string> datafile_names = generate_random_tensor_data(tensors, "./data", "");
    // generate_kernel(tensors, {einsum}, datafile_names, "./data/kernel.json");

    // vector<string> mutated_file_names = mutate_equivalent_kernel("./data/kernel.json", MutationOperator::SPARSITY, 100);

    // for (string &mutated_file_name : mutated_file_names)
    // {
    //     tsKernel kernel;
    //     kernel.loadJson(mutated_file_name);
    //     std::cout << taco_wrapper::generate_program(kernel) << std::endl;
        
    // }

    // Setup
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Configurable parameters
    uint64_t seed = 42;
    size_t max_iterations = 1000000;
    fs::path out_root = "fuzz_output";
    fs::path fail_dir = out_root / "failures";
    fs::path corpus_dir = out_root / "corpus";
    fs::path data_root = out_root / "data";

    if (const char* env = getenv("FUZZ_SEED")) seed = std::stoull(env);
    if (const char* env2 = getenv("FUZZ_ITERS")) max_iterations = std::stoull(env2);

    mt19937 rng(seed);
    cout << "Starting fuzz loop with seed=" << seed << " up to " << max_iterations << " iterations\n";

    // Create dirs
    fs::create_directories(out_root);
    fs::create_directories(corpus_dir);
    fs::create_directories(fail_dir);
    fs::create_directories(data_root);

    const uint64_t executor_timeout_ms = 30'000;

    for (int iter = 0; iter < max_iterations && !g_terminate; ++iter)
    {
        try {
            std::string iter_id = "iter_" + std::to_string(iter) + "_" + timestamp_str();
            fs::path iter_dir = corpus_dir / iter_dir;
            fs::path iter_data_dir = data_root / iter_id;
            fs::create_directories(iter_dir);
            fs::create_directories(iter_data_dir);

            // 1) Generate random kernel specification (einsum equations + tensor meta)
            auto [tensors, einsum] = generate_random_einsum(2,6);

            // 2) Generate and store data for tensors
            vector<string> datafile_names = generate_random_tensor_data(tensors, iter_data_dir, "");

            // 3) Save kernel JSON (atomic write)
            fs::path kernel_json_path = iter_data_dir / "kernel.json";
            if (generate_kernel(tensors, {einsum}, datafile_names, kernel_json_path)){

            } else {
                continue;
            }

            // 4) Run reference executor (trusted) to produce expected outputs
            // fs::path ref_out_dir = iter_data_dir / "ref_out";
            // fs::create_directories(ref_out_dir);
            // int ref_ret = run_with_timeout([&](){
            //     return run_reference_executor()
            // }, executor_timeout_ms);

            // 5) Run target (the compiler/runtime we are fuzzing)

            // 6) Compare outputs

            // 7) Save passing case to corpus for future shrinking/replay

            if (iter % 100 == 0)
            {
                cout << "Iteration " << iter << " OK. Saved to " << iter_dir << endl;
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Exception in iteration " << iter << ": " << e.what() << std::endl;
            // Optionally archive the case with an error message
            // Note: spec might not be available here - keep robust logging
        }
    }

    std::cout << "Fuzzing loop finished (terminated=" << g_terminate << ")\n";
    return 0;
}