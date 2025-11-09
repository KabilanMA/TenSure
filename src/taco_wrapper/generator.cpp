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

bool generate_taco_kernel(const tsKernel& kernel, const fs::path& outFile) {
    try {
        // generate TACO program string
        fs::create_directories(outFile);
        string results_file = outFile / "result.tns";
        string program_code = generate_program(kernel, results_file);

        // atomic write
        string tmp_name = outFile / ((outFile.parent_path().stem().string()) + ".tmp");
        ofstream ofs(tmp_name);
        ofs << program_code;
        ofs.close();
        fs::rename(tmp_name, (outFile / ((outFile.parent_path().stem().string()) + ".cpp")).string()); // atomic replacement
    } catch (const exception& e) {
        cerr << "TacoBackend::generate_kernel failed: " << e.what() << endl;
        return false;
    }

    return true;
}

string generate_program(const tsKernel &kernel_info, const string &results_file)
{
    int tab_space_count = 4;
    std::string space = "";
    for (size_t i = 0; i < tab_space_count; i++)
    {
            space += " ";
    }
    ostringstream oss;
    oss << "#include <iostream>\n#include \"taco.h\"\n\nusing namespace taco;\n\nint main() {\n";
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

    oss << space << kernel_info.tensors[0].name << ".compile();\n";
    oss << space << kernel_info.tensors[0].name << ".assemble();\n";
    oss << space << kernel_info.tensors[0].name << ".compute();\n\n";

    oss << space << "write(\"" << results_file << "\", " << kernel_info.tensors[0].name << ");\n\n";

    oss << space << "return 0;\n";

    oss << "}";
    return oss.str();
}

}