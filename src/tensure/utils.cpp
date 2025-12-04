#include "tensure/utils.hpp"

ostream& operator<<(ostream& os, const tsTensor& tensor) 
{
    os << "Tensor: " << tensor.str_repr << "\nShape: " << "[" << join(tensor.shape, ", ") << "]\nFormats: " << "[" << join(to_string(tensor.storageFormat), ", ") << "]";
    return os;
}

ostream& operator<<(ostream& os, const tsKernel& kernel) 
{
    for (auto &tensor : kernel.tensors)
    {
        os << tensor << "\n";

        std::string name(1, tensor.name);
        auto it = kernel.dataFileNames.find(name);

        if (it != kernel.dataFileNames.end())
            os << "dataFileName: " << it->second << "\n";
        else
            os << "dataFileName: [not found]\n";

    }

    os << "Computations: ";
    for (auto &computation : kernel.computations)
    {
        os << "\n\t" << computation.expressions;
    }
    return os;
}

// bool is_exist(vector<vector<string>>)

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

string join(const set<char>& chars, const string delimitter)
{
    string s;
    for (auto it = chars.begin(); it != chars.end(); ++it)
    {
        s += *it;
        if (next(it) != chars.end())
            s += delimitter;
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
    for (auto& s : t.storageFormat) out << to_string(s) << " ";
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
    for (size_t i = 0; i < fmtCount; i++)
    {
        string s;
        in >> s;
        t.storageFormat[i] = parseTensorFormat(s);
    }

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
            {"storageFormat", to_string(t.storageFormat)},
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
        string s = t["name"].get<string>();
        desc.name = s.empty() ? '\0' : s[0];
        desc.shape = t["shape"].get<vector<int>>();
        desc.storageFormat = parseTensorFormat(t["storageFormat"].get<vector<string>>());
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

bool generate_ref_kernel(const vector<tsTensor>& tensors, const vector<string>& computations, const vector<string>& dataFileNames, string file_name)
{
    if (tensors.size()-1 != dataFileNames.size()) return false;
    // cout << "SDSDS 1" << endl;
    tsKernel kernel;
    for (size_t i = 0; i < tensors.size(); i++)
    {
        // cout << "SDSDS 2" << i << endl;
        auto &tensor = tensors[i];
        kernel.tensors.push_back(tensor);
        if (i != 0)
            kernel.dataFileNames.insert({string(1,tensor.name),dataFileNames[i-1]});
        else
            kernel.dataFileNames.insert({string(1, tensor.name), "-"});
    }

    for (const auto& computation : computations)
    {
        // cout << "SDSDS 3" << computation << endl;
        tsComputation comp;
        comp.expressions = computation;
        kernel.computations.push_back(comp);
    }

    // Atomic write
    string tmp_name = file_name + ".tmp";
    // cout << "SDSDS 5" << endl;
    try
    {
        kernel.saveJson(tmp_name); // write to a temporary file first
        filesystem::rename(tmp_name, file_name); // atomic replacement
    }
    catch(const std::exception& e)
    {
        cerr << "generate_kernel failed: " << e.what() << std::endl;
        LOG_ERROR((std::ostringstream{} << "generate_kernel failed: " << e.what()).str());
        // Cleanupo temp if partially written
        error_code ec;
        filesystem::remove(tmp_name, ec);
        return false;
    }
    
    return true;
}

void generate_all_formats(int rank, vector<vector<string>>& out, vector<string>& current)
{
    if ((int)current.size() == rank)
    {
        out.push_back(current);
        return;
    }

    // insert Dense
    current.push_back("Dense");
    generate_all_formats(rank, out, current);
    current.pop_back();

    // insert Sparse
    current.push_back("Sparse");
    generate_all_formats(rank, out, current);
    current.pop_back();
}

vector<vector<string>> generate_all_formats(int rank)
{
    vector<vector<string>> all;
    vector<string> current;
    generate_all_formats(rank, all, current);
    return all;
}

struct VecHash {
    size_t operator()(const std::vector<int>& v) const noexcept {
        size_t h = 0;
        for (int x : v) {
            h = h * 1315423911u + std::hash<int>()(x);
        }
        return h;
    }
};

static bool ends_with(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() &&
           s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool compare_outputs(const string& ref_output, const string& kernel_output, double tol)
{
    auto read_tns = [&](const string& path) {
        unordered_map<vector<int>, double, VecHash> data;

        ifstream file(path);
        if (!file.is_open()) {
            throw runtime_error("Cannot open " + path);
        }

        string line;
        while (getline(file, line)) {
            if (line.empty()) continue;

            istringstream iss(line);
            vector<string> toks;
            string tok;

            while (iss >> tok) {
                toks.push_back(tok);
            }

            if (toks.size() < 2)
                continue; // ignore garbage lines

            double val = std::stod(toks.back());
            if (val == 0.0)
                continue; // skip zeros entirely

            vector<int> coords;
            coords.reserve(toks.size() - 1);
            for (size_t i = 0; i + 1 < toks.size(); ++i) {
                coords.push_back(std::stoi(toks[i]));
            }

            data.emplace(move(coords), val);
        }
        return data;
    };

    auto read_mtx = [&](const string& path) {
        unordered_map<vector<int>, double, VecHash> data;

        ifstream file(path);
        if (!file.is_open()) {
            throw runtime_error("Cannot open " + path);
        }

        string line;
        // Skip comments and header lines
        while(getline(file, line)) {
            if (line.empty()) continue;
            if (line[0] == '%') continue;
            break;
        }

        // First non-comment line: M N NNZ
        {
            istringstream iss(line);
            vector<string> toks;
            string tok;

            while (iss >> tok) toks.push_back(tok);
            if (toks.size() < 3)
                throw runtime_error("Invalid MTX header: " + path);
        }

        // Read triplets
        while (getline(file, line)) {
            if (line.empty()) continue;

            istringstream iss(line);
            vector<string> toks;
            string tok;
            while (iss >> tok) toks.push_back(tok);

            if (toks.size() < 3)
                continue;

            int i = std::stoi(toks[0]);
            int j = std::stoi(toks[1]);
            double val = std::stod(toks[2]);

            if (val == 0.0)
                continue;
            
            vector<int> coords = {i, j};
            data.emplace(move(coords), val);
        }

        return data;
    };

    auto read_ttx = [&](const string& path) {
        unordered_map<vector<int>, double, VecHash> data;

        ifstream file(path);
        if (!file.is_open()) {
            throw runtime_error("Cannot open " + path);
        }

        string line;
        while(getline(file, line)) {
            if (line.empty()) continue;

            istringstream iss(line);
            vector<string> toks;
            string tok;

            while (iss >> tok) {
                toks.push_back(tok);
            }

            if (toks.size() < 2)
                continue;
            
            double val = std::stod(toks.back());
            if (val == 0.0)
                continue;
            
            vector<int> coords;
            coords.reserve(toks.size() - 1);

            for (size_t i = 0; i + 1 < toks.size(); ++i) {
                coords.push_back(std::stoi(toks[i]));
            }

            data.emplace(move(coords), val);
        }

        return data;
    };

    auto read_tensor = [&](const string& path) {
        if (ends_with(path, ".tns")) 
            return read_tns(path);
        if (ends_with(path, ".mtx")) 
            return read_mtx(path);
        if (ends_with(path, ".ttx")) 
            return read_ttx(path);
        
        throw runtime_error("Unsupported tensor format: " + path);
    };

    auto ref = read_tensor(ref_output);
    auto out = read_tensor(kernel_output);

    if (ref.size() != out.size())
        return false;

    for (auto& [coords, val_ref] : ref) {
        auto it = out.find(coords);
        if (it == out.end())
            return false;

        if (fabs(it->second - val_ref) > tol)
            return false;
    }

    return true;
}



// Helper struct to hold parsed tensor data
struct TensorInfo {
    string name;
    string full_expression; // For error logging
    vector<char> indices;
    bool isValidStructure;
};

// --- Helper: Remove all spaces from string ---
string remove_spaces(string s) {
    s.erase(remove_if(s.begin(), s.end(), ::isspace), s.end());
    return s;
}

// --- Helper: Split string by delimiter ---
vector<string> split(const string &s, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

// --- Helper: Parse a single tensor string "A(i,j)" ---
TensorInfo parse_tensor(string token) {
    TensorInfo info;
    info.full_expression = token;
    info.isValidStructure = false;

    size_t openParen = token.find('(');
    size_t closeParen = token.find(')');

    // Check 1: Must have opening and closing parenthesis
    if (openParen == string::npos || closeParen == string::npos || closeParen < openParen) {
        return info;
    }

    // Check 2: Name must exist before parenthesis
    info.name = token.substr(0, openParen);
    if (info.name.empty()) return info;

    // Check 3: Extract indices string
    string content = token.substr(openParen + 1, closeParen - openParen - 1);
    
    // Split indices by comma
    // Handle empty case "A()" -> Scalar
    if (!content.empty()) {
        vector<string> idx_tokens = split(content, ',');
        for (const string& idx_s : idx_tokens) {
            // In this specific format, we expect single char indices like 'i'
            // If you accept "idx1", you need to change vector<char> to vector<string>
            if (idx_s.length() != 1) { 
                // Invalid: Index is not a single char
                return info; 
            }
            info.indices.push_back(idx_s[0]);
        }
    }

    info.isValidStructure = true;
    return info;
}

// --- MAIN VALIDATION FUNCTION ---
bool is_valid_einsum_equation(string equation) {
    // 1. Clean String
    string clean_eq = remove_spaces(equation);

    // 2. Split into LHS (Output) and RHS (Inputs)
    size_t eqPos = clean_eq.find('=');
    if (eqPos == string::npos) {
        cerr << "[Error] Missing '=' sign." << endl;
        return false;
    }

    string lhs_str = clean_eq.substr(0, eqPos);
    string rhs_str = clean_eq.substr(eqPos + 1);

    if (lhs_str.empty() || rhs_str.empty()) {
        cerr << "[Error] Empty LHS or RHS." << endl;
        return false;
    }

    // 3. Parse Output Tensor
    TensorInfo outputTensor = parse_tensor(lhs_str);
    if (!outputTensor.isValidStructure) {
        cerr << "[Error] Invalid Output Tensor Syntax: " << lhs_str << endl;
        return false;
    }

    // 4. Parse Input Tensors (Separated by '*')
    vector<string> input_tokens = split(rhs_str, '*');
    vector<TensorInfo> inputTensors;
    set<char> all_input_indices;

    for (const string& t_str : input_tokens) {
        TensorInfo t = parse_tensor(t_str);
        if (!t.isValidStructure) {
            cerr << "[Error] Invalid Input Tensor Syntax: " << t_str << endl;
            return false;
        }
        
        // Check for Repeated Indices within a single tensor (e.g., B(i,i))
        // Standard Einsum allows this (trace), but based on your previous request
        // you might want to forbid it. Remove this block if trace is allowed.
        set<char> unique_check(t.indices.begin(), t.indices.end());
        if (unique_check.size() != t.indices.size()) {
            cerr << "[Error] Repeated index in tensor: " << t.name << endl;
            return false;
        }

        // Add to list
        inputTensors.push_back(t);
        for (char c : t.indices) all_input_indices.insert(c);
    }

    // 5. Semantic Check: Output Subset Rule
    // Every index in LHS must exist in RHS.
    for (char c : outputTensor.indices) {
        if (all_input_indices.find(c) == all_input_indices.end()) {
            cerr << "[Error] Output index '" << c << "' not found in inputs." << endl;
            return false;
        }
    }

    // 6. Semantic Check: Output Unique Indices
    // LHS should not have repeats: A(i,i) = ... is usually invalid for assignment
    set<char> out_unique(outputTensor.indices.begin(), outputTensor.indices.end());
    if (out_unique.size() != outputTensor.indices.size()) {
        cerr << "[Error] Output tensor has repeated indices." << endl;
        return false;
    }

    return true;
}
