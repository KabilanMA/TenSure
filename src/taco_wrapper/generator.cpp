#include "taco_wrapper/generator.hpp"

namespace taco_wrapper {

TacoTensor toTacoTensor(const tsTensor& t, const string& dataFilename)
{
    TacoTensor tacoT;
    tacoT.name = std::string(1, t.name);
    tacoT.shape = t.shape;
    tacoT.idxs = t.idxs;
    tacoT.fmt = t.storageFormat;
    tacoT.dataFilename = dataFilename;

    return tacoT;
}

bool generate_taco_kernel(const tsKernel& kernel, const fs::path& out_file, std::vector<fs::path> results_file) {
    try {
        // generate TACO program string
        fs::create_directories(out_file);
        string program_code = generate_program(kernel, results_file);

        // atomic write
        string tmp_name = out_file / ((out_file.parent_path().stem().string()) + ".tmp");
        ofstream ofs(tmp_name);
        ofs << program_code;
        ofs.close();
        fs::rename(tmp_name, (out_file / ((out_file.parent_path().stem().string()) + ".cpp")).string()); // atomic replacement
    } catch (const exception& e) {
        cerr << "TacoBackend::generate_kernel failed: " << e.what() << endl;
        return false;
    }

    return true;
}

string generate_program(const tsKernel &kernel_info, std::vector<fs::path> results_file)
{
    int tab_space_count = 4;
    std::string space = "";
    for (size_t i = 0; i < tab_space_count; i++)
    {
            space += " ";
    }
    ostringstream oss;
    oss << "#include <iostream>\n" 
        << "#include <fstream>\n"
        << "#include <chrono>\n" 
        << "#include <sstream>\n"
        << "#include <vector>\n" 
        << "#include <string>\n"
        << "#include <stdexcept>\n" 
        << "#include \"taco.h\"\n\n"
        << "using namespace taco;\n\n"
        << "int read_taco_file(std::string file_name, Tensor<double>& T)\n"
            <<"{\n\t"
            << "std::ifstream file(file_name);\n\t"
            << "if (!file.is_open()) {\n\t\t"
                << "throw std::runtime_error(\"Failed to open file: \" + file_name);\n\t"
            << "}\n\n\t"
            << "std::string line;\n\t"
            << "while (std::getline(file, line)) {\n\t\t"
                << "if (line.empty() || line[0] == '#') continue;\n\n\t\t"
                << "std::istringstream iss(line);\n\t\t"
                << "std::vector<double> tokens;\n\t\t"
                << "double tmp;\n\n\t"
                << "while (iss >> tmp) {\n\t\t\t"
                    << "tokens.push_back(tmp);\n\t\t"
                << "}\n\n\t\t"
                << "if (tokens.size() < 2) {\n\t\t\t"
                    << "throw std::runtime_error(\"Malformed line: \" + line);\n\t\t"
                << "}\n\n\t\t"
                << "std::vector<int> coord;\n\t\t"
                << "coord.reserve(tokens.size() - 1);\n\n\t\t"
                << "for (size_t i =0; i < tokens.size() -  1; i++) {\n\t\t\t"
                    << "coord.push_back(static_cast<int>(tokens[i]));\n\t\t"
                << "}\n\t\t"
                << "T.insert(coord, tokens.back());\n\t"
            << "}\n\t"
            << "return 0;\n"
        << "}\n\n"
        << "int main() {\n";
    
    set<char> indexVar;
    vector<string> tensor_init = {};
    for(size_t i = 0; i < kernel_info.tensors.size(); i++)
    {
        const tsTensor &tensor = kernel_info.tensors[i];
        string tensorDataFilename = kernel_info.dataFileNames.at(std::string(1, tensor.name));
        for (auto &id : tensor.idxs)
            indexVar.insert(id);
        TacoTensor tacoTensor = toTacoTensor(tensor, tensorDataFilename);
        tensor_init.push_back(tacoTensor.initilization_string(space));
        // std::cout << tacoTensor.initilization_string(4) << std::endl;
    }
    oss << space << "IndexVar " << join(indexVar) << ";\n\n";

    for (auto &tensor_vals : tensor_init)
    {
        oss << tensor_vals;
    }

    for (auto &expression : kernel_info.computations)
    {
        oss << space << expression.expressions << ";\n\n";
    }
    oss << space << "auto compile_start_time = std::chrono::high_resolution_clock::now();\n";
    oss << space << kernel_info.tensors[0].name << ".compile();\n";
    oss << space << kernel_info.tensors[0].name << ".assemble();\n";
    oss << space << "auto compile_end_time = std::chrono::high_resolution_clock::now();\n";
    oss << space << kernel_info.tensors[0].name << ".compute();\n\n";
    oss << space << "auto end_time = std::chrono::high_resolution_clock::now();\n";
    oss << space << "std::chrono::duration<double, std::milli> compile_elapsed_ms = compile_end_time - compile_start_time;\n";
    oss << space << "std::chrono::duration<double, std::milli> compute_elapsed_ms = end_time - compile_end_time;\n";

    fs::path time_file = results_file[0];
    time_file.replace_extension(".txt");
    oss << space << "std::ofstream time_file(\"" << time_file.string() << "\");\n";
    oss << space << "time_file << \"Compilation time: \" << compile_elapsed_ms.count() << \" ms\"\n\"\";\n";
    oss << space << "time_file << \"Computation time: \" << compute_elapsed_ms.count() << \" ms\"\n\"\";\n";
    oss << space << "time_file.close();\n";

    for (auto &results_file_path : results_file) {
        fs::path abs_results_file_path = std::filesystem::absolute(std::filesystem::current_path() / results_file_path);
        oss << space << "write(\"" << abs_results_file_path.string() << "\", " << kernel_info.tensors[0].name << ");\n";
    }
    oss << "\n" << space << "return 0;\n";

    oss << "}";
    return oss.str();
}

}