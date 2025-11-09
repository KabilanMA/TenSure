#pragma once

#include "tensure/formats.hpp"
#include "tensure/utils.hpp"
#include "taco.h"
#include <string>

namespace taco_wrapper {
using namespace std;

typedef struct TacoTensor {
    string name;
    vector<char> idxs;
    vector<int> shape;
    vector<TensorFormat> fmt;
    string dataFilename;

    // Constructor
    // TacoTensor(char n, const vector<char>& indices, const vector<int> dimensions, const vector<TensorFormat>& storageFormat) : name(n), idxs(indices), shape(dimensions), fmt(storageFormat) {}

    TacoTensor() {}

    // generate initilization string
    string initilization_string(string tab_space)
    {   
        ostringstream oss;
        oss << tab_space << "Tensor<double> " << name << "(\"" << name << "\", {" << join(shape, ",") << "}, Format({";
        string dataFormat = "";
        for (int i = 0; i < fmt.size(); i++)
        {
            dataFormat += to_string(fmt[i]);
            if (i!=fmt.size()-1)
            {
                dataFormat += ",";
            }
        }
        oss << dataFormat << "}));\n";

        if (dataFilename != "-")
        {
            oss << tab_space << name << " = " << "read(\"" << dataFilename << "\", Format({" << dataFormat << "}));\n";
            oss << tab_space << name << ".pack();\n\n";
        }
        
        return oss.str();
    }
} TacoTensor;

bool generate_taco_kernel(const tsKernel& kernel, const fs::path& outFile);
string generate_program(const tsKernel &kernel_info, const string& results_file);

}