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
        oss << tab_space << "Tensor<double> " << name << "(\"A\", {" << join(shape, ",") << "}, {";
        string dataFormat = "";
        for (int i = 0; i < fmt.size(); i++)
        {
            dataFormat += to_string(fmt[i]);
            if (i!=fmt.size()-1)
            {
                dataFormat += ",";
            }
        }
        oss << dataFormat << "});\n";

        oss << tab_space << name << " = " << "read(\"" << dataFilename << "\", {" << dataFormat << "});\n\n";
        
        return oss.str();
    }
} TacoTensor;

string generate_program(const tsKernel &kernel_info);

}