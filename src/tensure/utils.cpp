#include "tensure/utils.hpp"

ostream& operator<<(ostream& os, const tsTensor& tensor) 
{
    os << "Tensor: " << tensor.str_repr << "\nShape: " << "[" << join(tensor.shape, ", ") << "]\nFormats: " << "[" << join(tensor.storageFormat, ", ") << "]";
    return os;
}

vector<char> find_idxs(vector<tsTensor> ts_tensors)
{
    vector<char> idxs;
    for (auto &ts_tensor : ts_tensors)
    {
        for (auto &idx : ts_tensor.idxs)
        {
            bool exists = find(idxs.begin(), idxs.end(), idx) != idxs.end();
            if (!exists)
                idxs.push_back(idx);
        }
    }
    return idxs;
}

string join(const vector<string>& idxs, const string delimitter) {
    string s;
    for (size_t i = 0; i < idxs.size(); ++i) {
        s += idxs[i];
        if (i + 1 < idxs.size()) s += delimitter;
    }
    return s;
}

string join(const vector<int>& idxs, const string delimitter) {
    string s;
    for (size_t i = 0; i < idxs.size(); ++i) {
        s += to_string(idxs[i]);
        if (i + 1 < idxs.size()) s += delimitter;
    }
    return s;
}

string join(const vector<char>& idxs, const string delimitter) {
    string s;
    for (size_t i = 0; i < idxs.size(); ++i) {
        s += idxs[i];
        if (i + 1 < idxs.size()) s += delimitter;
    }
    return s;
}


void saveTensorData(const tsTensor& t, const std::string& filename) 
{
    ofstream out(filename);
    if (!out) throw runtime_error("Failed to open file for writing");

    out << t.name << "\n";
    out << t.str_repr << "\n";

    // Save idxs
    out << t.idxs.size() << " ";
    for (char c : t.idxs) out << c << " ";
    out << "\n";

    // Save storage format
    out << t.storageFormat.size() << " ";
    for (auto& s : t.storageFormat) out << s << " ";
    out << "\n";
}

tsTensor loadTensorData(const string& filename) 
{
    ifstream in(filename);
    if (!in) throw runtime_error("Failed to open file for reading");

    tsTensor t;
    in >> t.name;
    in.ignore(); // eat newline
    getline(in, t.str_repr);

    size_t idxCount;
    in >> idxCount;
    t.idxs.resize(idxCount);
    for (size_t i = 0; i < idxCount; i++) in >> t.idxs[i];

    size_t fmtCount;
    in >> fmtCount;
    t.storageFormat.resize(fmtCount);
    for (size_t i = 0; i < fmtCount; i++) in >> t.storageFormat[i];

    return t;
}

void saveKernelJson(const string& filename, const vector<tsTensor>& tensors, const vector<tsComputation>& computations)
{
    nlohmann::json j;

    // Add tensors
    for (const auto& t: tensors) 
    {
        j["tensors"].push_back({
            {"name", t.name},
            {"str_repr", t.str_repr},
            {"idxs", t.idxs},
            {"shape", t.shape},
            {"storageFormat", t.storageFormat},
            {"dataFile", (string(1,t.name) + ".tns")}
        });
    }

    // Add computations
    for (const auto& c : computations)
    {
        j["computations"].push_back({
            {"expression", c.expressions}
        });
    }

    // Write to file
    std::ofstream fout(filename);
    if (!fout.is_open()) {
        throw std::runtime_error("Cannot open file to save JSON: " + filename);
    }
    fout << j.dump(4);
    fout.close();
}

void loadKernelJson(const string& filename, map<char, tsTensor>& tensorsMap, vector<tsComputation>& computations)
{
    ifstream fin(filename);
    if (!fin.is_open()) {
        throw runtime_error("Cannot open file to read JSON: " + filename);
    }

    nlohmann::json j;
    fin >> j;
    fin.close();

    // Parse tensors
    for (auto& t : j["tensors"])
    {
        tsTensor desc;
        std::string s = t["name"].get<std::string>();
        desc.name = s.empty() ? '\0' : s[0];
        desc.shape = t["shape"].get<std::vector<int>>();
        desc.storageFormat = t["storageFormat"].get<std::vector<std::string>>();
        tensorsMap[desc.name] = desc;
    }

    // Parse computations
    computations.clear();
    for (auto& c : j["computations"])
    {
        computations.push_back({c["expression"]});
    }
}


void ensure_directory_exists(const std::string& path) 
{
    try {
        if (!fs::exists(path)) {
            fs::create_directories(path);
            std::cout << "Created directory: " << path << std::endl;
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error creating directory: " << e.what() << std::endl;
    }
}

bool generate_kernel(vector<tsTensor>& tensors, vector<string> computations, vector<string> dataFileNames, string file_name)
{
    if (tensors.size() != dataFileNames.size()) return false;
    tsKernel kernel;
    for (size_t i = 0; i < tensors.size(); i++)
    {
        auto &tensor = tensors[i];
        kernel.tensors.push_back(tensor);
        kernel.dataFileNames.insert({string(1,tensor.name),dataFileNames[i]});
    }

    for (auto& computation : computations)
    {
        tsComputation comp;
        comp.expressions = computation;
        kernel.computations.push_back(comp);
    }

    kernel.saveJson(file_name);
    return true;
}