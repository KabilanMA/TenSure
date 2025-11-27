#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <iostream>

#include <nlohmann/json.hpp>
#include <fstream>

using namespace std;
using json = nlohmann::json;

enum TensorFormat {
    tsSparse = 0,
    tsDense = 1
};

inline string to_string(TensorFormat tf) {
    switch(tf) {
        case TensorFormat::tsDense:  return "Dense";
        case TensorFormat::tsSparse: return "Sparse";
    }
    return "Unknown";
}

inline vector<string> to_string(const vector<TensorFormat>& tfs) {
    vector<string> tfs_vector = {};
    
    for (auto &tf : tfs)
    {
        switch (tf)
        {
        case TensorFormat::tsDense:
            tfs_vector.push_back("Dense");
            break;
        case TensorFormat::tsSparse:
            tfs_vector.push_back("Sparse");
            break;
        default:
            break;
        }
    }

    return tfs_vector;
}

inline TensorFormat parseTensorFormat(const string &s) {
    if (s == "Dense")  return TensorFormat::tsDense;
    if (s == "Sparse") return TensorFormat::tsSparse;
    throw runtime_error("Unknown TensorFormat: " + s);
}

inline vector<TensorFormat> parseTensorFormat(const vector<string>& tfs_vec)
{
    vector<TensorFormat> tfs;
    for (auto &tf : tfs_vec)
    {
        if (tf == "Dense")
        {
            tfs.push_back(TensorFormat::tsDense);
        } else if (tf == "Sparse")
        {
            tfs.push_back(TensorFormat::tsSparse);
        } else {
            throw runtime_error("Unknown TensorFormat: " + tf);
        }
    }

    return tfs;
}

inline bool is_equal(const vector<TensorFormat>& fmt1, const vector<TensorFormat>& fmt2)
{
    if (fmt1.size() != fmt2.size())
        return false;
    
    for (size_t i = 0; i < fmt1.size(); i++)
    {
        if (fmt1[i] != fmt2[i])
            return false;
    }
    return true;
}

enum MutationOperator {
    SPARSITY = 0,
};

typedef struct tsTensor
{
    char name;
    string str_repr;
    vector<char> idxs;
    vector<int> shape;
    vector<TensorFormat> storageFormat;
} tsTensor;

typedef struct tsTensorData
{
    char tensorName;
    vector<vector<int>> coordinate;
    vector<double> data;
    string tfmt;

    // Insert: update if coord exists, otherwise push new coord+value
    void insert(const std::vector<int>& coord, double value)
    {
        // find existing coordinate (linear search)
        auto it = std::find_if(coordinate.begin(), coordinate.end(),
            [&](const vector<int>& c){
                return c == coord;
            });
        
        if (it != coordinate.end()) {
            // update the value at the same index
            size_t idx = std::distance(coordinate.begin(), it);
            data[idx] = value;
        } else {
            // append new coordinate and value
            coordinate.push_back(coord);
            data.push_back(value);
        }
    }

    // helper: get value at coordinate (throws if not found)
    double get(const vector<int>& coord) const {
        auto it = find(coordinate.begin(), coordinate.end(), coord);
        if (it == coordinate.end()) {
            throw out_of_range("Coordinate not found");
        }

        size_t idx = distance(coordinate.begin(), it);
        return data[idx];
    }

    size_t size() const {
        return data.size(); 
    }

    void clear() {
        coordinate.clear();
        data.clear();
    }

} tsTensorData;

typedef struct tsComputation {
    string expressions;
} tsComputation;

typedef struct tsKernel
{
    vector<tsTensor> tensors;
    map<string, string> dataFileNames;
    vector<tsComputation> computations;

    void saveJson(const string& file_name)
    {
        json j;

        // Serialize tensors
        j["tensors"] = json::array();
        for (auto &tensor : tensors)
        {
            json t;
            t["name"] = string(1, tensor.name);
            t["shape"] = tensor.shape;
            t["str_repr"] = tensor.str_repr;
            t["idxs"] = tensor.idxs;
            t["storageFormat"] = to_string(tensor.storageFormat);
            
            // Loopup file path using the tensor's name
            auto it = dataFileNames.find(string(1, tensor.name));
            t["dataFile"] = (it != dataFileNames.end()) ? it->second : "";
            
            j["tensors"].push_back(t);
        }

        // Serialize computations
        j["computations"] = json::array();
        for (auto &c : computations)
        {
            json comp;
            comp["expression"] = c.expressions;
            j["computations"].push_back(comp);
        }

        // Write JSON to file in pretty format
        ofstream out(file_name);
        out << j.dump(4);
        // cout << j.dump(4) << endl;
    }

    void loadJson(const string& file_name)
    {
        ifstream in(file_name);
        if (!in.is_open())
        {
            cerr << "Error: Unable to open files " << file_name << endl;
            return;
        }

        json j;
        in >> j;

        tensors.clear();
        dataFileNames.clear();
        computations.clear();

        // Deserialize tensors
        for (auto &t : j["tensors"])
        {
            tsTensor tensor;
            tensor.name = t["name"].get<string>()[0];
            tensor.shape = t["shape"].get<vector<int>>();
            tensor.idxs = t["idxs"].get<vector<char>>();
            tensor.str_repr = t["str_repr"].get<string>();
            tensor.storageFormat = parseTensorFormat(t["storageFormat"].get<vector<string>>());

            tensors.push_back(tensor);

            string name = t["name"].get<string>();
            string dataFile = t["dataFile"].get<string>();
            if (!dataFile.empty())
                dataFileNames[name] = dataFile;
        }

        // Deserialize computations
        for (auto &c : j["computations"])
        {
            tsComputation comp;
            comp.expressions = c["expression"].get<string>();
            computations.push_back(comp);
        }
    }
} tsKernel;
