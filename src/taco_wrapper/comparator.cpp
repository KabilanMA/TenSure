#include "taco_wrapper/comparator.hpp"

namespace taco_wrapper {

using namespace std;

struct VecHash {
    size_t operator()(const std::vector<int>& v) const noexcept {
        size_t h = 0;
        for (int x : v) {
            h = h * 1315423911u + std::hash<int>()(x);
        }
        return h;
    }
};

bool compare_outputs(const std::string& ref_output,
                     const std::string& kernel_output,
                     double tol)
{
    auto read_tensor = [&](const std::string& path) {
        std::unordered_map<std::vector<int>, double, VecHash> data;

        std::ifstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open " + path);
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;

            std::istringstream iss(line);
            std::vector<std::string> toks;
            std::string tok;

            while (iss >> tok) {
                toks.push_back(tok);
            }

            if (toks.size() < 2)
                continue; // ignore garbage lines

            double val = std::stod(toks.back());
            if (val == 0.0)
                continue; // skip zeros entirely

            std::vector<int> coords;
            coords.reserve(toks.size() - 1);
            for (size_t i = 0; i + 1 < toks.size(); ++i) {
                coords.push_back(std::stoi(toks[i]));
            }

            data.emplace(std::move(coords), val);
        }

        return data;
    };

    auto ref = read_tensor(ref_output);
    auto out = read_tensor(kernel_output);

    if (ref.size() != out.size())
        return false;

    for (auto& [coords, val_ref] : ref) {
        auto it = out.find(coords);
        if (it == out.end())
            return false;

        if (std::fabs(it->second - val_ref) > tol)
            return false;
    }

    return true;
}

}