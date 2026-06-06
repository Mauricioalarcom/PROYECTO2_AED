#include "TextNormalizer.h"

#include <cctype>

namespace textnorm {

std::string normalize(const std::string& input, const Options& opt) {
    std::string out;
    out.reserve(input.size());

    bool pendingSpace = false;          // hay un espacio "colapsado" por emitir
    bool emittedAny   = false;

    for (unsigned char uc : input) {
        char c = static_cast<char>(uc);

        // Nunca dejar pasar el terminal reservado del Suffix Tree.
        if (c == '\x01') continue;

        const bool isSpace = std::isspace(uc) != 0;
        if (isSpace) {
            if (opt.collapseSpaces) {
                pendingSpace = emittedAny;   // marcar, se emite con el proximo char
            } else {
                out.push_back(' ');
            }
            continue;
        }

        // Decidir si el caracter se conserva.
        const bool isAlpha = std::isalpha(uc) != 0;
        const bool isDigit = std::isdigit(uc) != 0;
        bool keep = isAlpha;
        if (isDigit && opt.keepDigits) keep = true;
        if (!isAlpha && !isDigit && opt.keepPunct && std::isprint(uc)) keep = true;
        if (!keep) {
            // Caracter descartado: en modo colapso cuenta como separador.
            if (opt.collapseSpaces) pendingSpace = emittedAny;
            continue;
        }

        if (pendingSpace) { out.push_back(' '); pendingSpace = false; }
        out.push_back(opt.toLowercase ? static_cast<char>(std::tolower(uc)) : c);
        emittedAny = true;
    }

    if (opt.trim) {
        const auto notSpace = [](char ch) { return ch != ' '; };
        while (!out.empty() && !notSpace(out.front())) out.erase(out.begin());
        while (!out.empty() && !notSpace(out.back()))  out.pop_back();
    }

    return out;
}

} // namespace textnorm
