#include "NaiveSearch.h"

namespace naive {

static Result run(const std::string& text, const std::string& pattern, bool keepPositions) {
    Result r;
    const int n = static_cast<int>(text.size());
    const int m = static_cast<int>(pattern.size());
    if (m == 0 || m > n) return r;

    for (int i = 0; i + m <= n; ++i) {
        int j = 0;
        while (j < m) {
            ++r.charsCompared;
            if (text[i + j] != pattern[j]) break;
            ++j;
        }
        if (j == m) {
            ++r.count;
            if (keepPositions) r.positions.push_back(i);
        }
    }
    r.found = r.count > 0;
    return r;
}

Result search(const std::string& text, const std::string& pattern) {
    return run(text, pattern, true);
}

Result count(const std::string& text, const std::string& pattern) {
    return run(text, pattern, false);
}

} // namespace naive
