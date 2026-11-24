#include <string>
#include <vector>

inline bool ends_with(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// Replaces the first occurrence only.
inline bool replace(std::string& s, const std::string& from, const std::string& to) {
    size_t pos = s.find(from);
    if (pos == std::string::npos) { return false; }
    s.replace(pos, from.size(), to);
    return true;
}

inline std::string implode(const std::vector<std::string>& parts, char delim) {
    std::string out;
    for (size_t i = 0; i < parts.size(); i++) {
        if (i) { out += delim; }
        out += parts[i];
    }
    return out;
}

// Same results as a getline loop: a trailing delimiter adds no empty element.
inline std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> out;
    size_t pos = 0;
    while (pos < s.size()) {
        size_t next = s.find(delim, pos);
        if (next == std::string::npos) { out.push_back(s.substr(pos)); break; }
        out.push_back(s.substr(pos, next - pos));
        pos = next + 1;
    }
    return out;
}
