#include "App.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <utility>

#include "TextNormalizer.h"

using Clock = std::chrono::high_resolution_clock;

// Formatea un double con 4 decimales (para los tiempos en ms).
static std::string fmt(double v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.4f", v);
    return buf;
}

// Paleta de colores de la interfaz.
namespace col {
const sf::Color kBg        (24, 26, 32);
const sf::Color kPanel     (32, 35, 43);
const sf::Color kText      (220, 223, 228);
const sf::Color kMuted     (140, 146, 158);
const sf::Color kAccent    (88, 166, 255);
const sf::Color kHighlight (70, 110, 180);   // fondo de las ocurrencias
} // namespace col

// ---------------------------------------------------------------------------
// Construccion: normaliza el texto y construye el Suffix Tree (mide el tiempo).
// ---------------------------------------------------------------------------
App::App(std::string rawText, std::string sourceLabel)
    : source_(std::move(sourceLabel)) {
    text_ = textnorm::normalize(rawText);

    const auto t0 = Clock::now();
    tree_.buildUkkonen(text_);
    buildMs_ = std::chrono::duration<double, std::milli>(Clock::now() - t0).count();

    std::cerr << "[app] origen=" << source_
              << "  chars=" << text_.size()
              << "  nodos=" << tree_.nodeCount()
              << "  construccion=" << buildMs_ << " ms\n";
}

// ---------------------------------------------------------------------------
// Carga una fuente monoespaciada (intenta varias rutas tipicas de macOS).
// ---------------------------------------------------------------------------
bool App::loadFont() {
    const char* candidates[] = {
        "data/font.ttf",                                  // fuente propia si existe
        "/System/Library/Fonts/Supplemental/Courier New.ttf",
        "/System/Library/Fonts/Menlo.ttc",
        "/System/Library/Fonts/Monaco.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",   // ultimo recurso (no monoespaciada)
    };
    for (const char* path : candidates) {
        if (font_.loadFromFile(path)) {
            std::cerr << "[app] fuente: " << path << "\n";
            return true;
        }
    }
    std::cerr << "[app] ADVERTENCIA: no se pudo cargar ninguna fuente.\n";
    return false;
}

// ---------------------------------------------------------------------------
// Parte el texto normalizado en lineas de 'cols_' caracteres. Con corte fijo
// por columnas, la posicion p en el texto mapea exactamente a la fila p/cols_
// y la columna p%cols_, lo que hara trivial el resaltado de ocurrencias.
// ---------------------------------------------------------------------------
void App::wrapText() {
    lines_.clear();
    const int n = static_cast<int>(text_.size());
    if (cols_ < 1) cols_ = 1;
    for (int i = 0; i < n; i += cols_)
        lines_.push_back(text_.substr(i, cols_));
    if (lines_.empty()) lines_.push_back("");
}

// ---------------------------------------------------------------------------
// Geometria
// ---------------------------------------------------------------------------
sf::FloatRect App::searchBar() const {
    const auto sz = window_.getSize();
    const float x = 20.f, y = 64.f;
    const float w = static_cast<float>(sz.x) - x - kRightPanelW - 40.f;
    return { x, y, w, 30.f };
}

sf::FloatRect App::docPanel() const {
    const auto sz = window_.getSize();
    const sf::FloatRect sb = searchBar();
    const float y = sb.top + sb.height + 10.f;
    return { sb.left, y, sb.width, static_cast<float>(sz.y) - y - 20.f };
}

sf::FloatRect App::metricsPanel() const {
    const auto sz = window_.getSize();
    const float x = static_cast<float>(sz.x) - kRightPanelW - 20.f;
    const float y = 64.f;
    return { x, y, kRightPanelW, static_cast<float>(sz.y) - y - 20.f };
}

int App::visibleLines() const {
    return std::max(1, static_cast<int>(docPanel().height / lineHeight_));
}

int App::maxScroll() const {
    return std::max(0, static_cast<int>(lines_.size()) - visibleLines());
}

// ---------------------------------------------------------------------------
// Eventos: cerrar y scroll (flechas, RePag/AvPag, rueda del mouse).
// ---------------------------------------------------------------------------
void App::handleEvents() {
    sf::Event e{};
    while (window_.pollEvent(e)) {
        if (e.type == sf::Event::Closed)
            window_.close();
        else if (e.type == sf::Event::TextEntered) {
            const sf::Uint32 u = e.text.unicode;
            if (u == 8) {                                   // backspace
                if (!rawQuery_.empty()) rawQuery_.pop_back();
            } else if (u == 13 || u == 10) {                // enter -> buscar
                runSearch();
            } else if (u >= 32 && u < 127) {                // caracter imprimible
                rawQuery_.push_back(static_cast<char>(u));
            }
        }
        else if (e.type == sf::Event::KeyPressed) {
            if (e.key.code == sf::Keyboard::Escape) window_.close();
            else if (e.key.code == sf::Keyboard::Down)  ++scroll_;
            else if (e.key.code == sf::Keyboard::Up)    --scroll_;
            else if (e.key.code == sf::Keyboard::PageDown) scroll_ += visibleLines();
            else if (e.key.code == sf::Keyboard::PageUp)   scroll_ -= visibleLines();
            else if (e.key.code == sf::Keyboard::Home)  scroll_ = 0;
            else if (e.key.code == sf::Keyboard::End)   scroll_ = maxScroll();
        } else if (e.type == sf::Event::MouseWheelScrolled) {
            scroll_ -= static_cast<int>(e.mouseWheelScroll.delta * 3);
        } else if (e.type == sf::Event::Resized) {
            // Reajustar la vista y el envuelto al nuevo tamano.
            window_.setView(sf::View(sf::FloatRect(0, 0,
                static_cast<float>(e.size.width), static_cast<float>(e.size.height))));
            cols_ = std::max(1, static_cast<int>(docPanel().width / charWidth_));
            wrapText();
        }
        scroll_ = std::clamp(scroll_, 0, maxScroll());
    }
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------
void App::drawHeader() {
    if (!fontLoaded_) return;
    sf::Text t("", font_, 18);
    t.setFillColor(col::kAccent);
    t.setStyle(sf::Text::Bold);
    t.setPosition(20.f, 14.f);
    t.setString("Suffix Tree (Ukkonen) - Buscador indexado de documentos");
    window_.draw(t);

    sf::Text info("", font_, 13);
    info.setFillColor(col::kMuted);
    info.setPosition(20.f, 42.f);
    info.setString("origen: " + source_ +
                   "   |   chars: " + std::to_string(text_.size()) +
                   "   |   nodos: " + std::to_string(tree_.nodeCount()) +
                   "   |   construccion: " + std::to_string(buildMs_) + " ms" +
                   "   |   [flechas/rueda] scroll   [Esc] salir");
    window_.draw(info);
}

void App::drawDocument() {
    const sf::FloatRect p = docPanel();

    sf::RectangleShape bg({ p.width, p.height });
    bg.setPosition(p.left, p.top);
    bg.setFillColor(col::kPanel);
    window_.draw(bg);

    if (!fontLoaded_) return;

    const int first = scroll_;
    const int last  = std::min(static_cast<int>(lines_.size()), first + visibleLines());

    drawHighlights(p, first, last);

    sf::Text line("", font_, charSize_);
    line.setFillColor(col::kText);
    for (int i = first; i < last; ++i) {
        line.setString(lines_[i]);
        line.setPosition(p.left + 6.f, p.top + 4.f + (i - first) * lineHeight_);
        window_.draw(line);
    }
}

// Dibuja un rectangulo de fondo detras de cada ocurrencia visible. Como una
// ocurrencia [pos, pos+m) puede cruzar el corte de linea, se emite un segmento
// por cada linea que toca.
void App::drawHighlights(const sf::FloatRect& p, int first, int last) {
    if (!hasResult_ || occ_.empty()) return;
    const int m = static_cast<int>(query_.size());
    if (m <= 0) return;

    const int firstChar = first * cols_;        // primer caracter visible
    const int lastChar  = last  * cols_;        // limite (exclusivo)

    sf::RectangleShape hl;
    hl.setFillColor(col::kHighlight);

    for (int pos : occ_) {
        const int endPos = pos + m;             // rango [pos, endPos)
        if (endPos <= firstChar || pos >= lastChar) continue;   // fuera de vista

        const int lineA = pos / cols_;
        const int lineB = (endPos - 1) / cols_;
        for (int ln = lineA; ln <= lineB; ++ln) {
            if (ln < first || ln >= last) continue;
            const int lineStart = ln * cols_;
            const int segStart = std::max(pos, lineStart) - lineStart;            // columna inicio
            const int segEnd   = std::min(endPos, lineStart + cols_) - lineStart; // columna fin (excl)
            const int width    = segEnd - segStart;
            if (width <= 0) continue;
            hl.setSize({ width * charWidth_, lineHeight_ });
            hl.setPosition(p.left + 6.f + segStart * charWidth_,
                           p.top + 4.f + (ln - first) * lineHeight_);
            window_.draw(hl);
        }
    }
}

// ---------------------------------------------------------------------------
// Ejecuta la consulta: busca en el Suffix Tree y en la ingenua (para comparar),
// midiendo ambos tiempos. El patron se normaliza igual que el texto para que
// "Banana" coincida con "banana".
// ---------------------------------------------------------------------------
void App::runSearch() {
    query_ = textnorm::normalize(rawQuery_);
    if (query_.empty()) { hasResult_ = false; return; }

    auto t0 = Clock::now();
    stResult_ = tree_.search(query_);
    stSearchMs_ = std::chrono::duration<double, std::milli>(Clock::now() - t0).count();

    t0 = Clock::now();
    nvResult_ = naive::search(text_, query_);
    nvSearchMs_ = std::chrono::duration<double, std::milli>(Clock::now() - t0).count();

    occ_ = tree_.findOccurrences(query_);
    std::sort(occ_.begin(), occ_.end());
    hasResult_ = true;

    // Saltar a la primera ocurrencia para que quede a la vista.
    if (!occ_.empty())
        scroll_ = std::clamp(occ_.front() / cols_ - 2, 0, maxScroll());
}

void App::drawSearchBar() {
    const sf::FloatRect r = searchBar();
    sf::RectangleShape box({ r.width, r.height });
    box.setPosition(r.left, r.top);
    box.setFillColor(col::kPanel);
    box.setOutlineColor(col::kAccent);
    box.setOutlineThickness(1.f);
    window_.draw(box);
    if (!fontLoaded_) return;

    sf::Text t("", font_, 15);
    t.setPosition(r.left + 8.f, r.top + 5.f);
    if (rawQuery_.empty()) {
        t.setFillColor(col::kMuted);
        t.setString("Buscar: escribe un patron y presiona Enter...");
    } else {
        t.setFillColor(col::kText);
        t.setString("Buscar: " + rawQuery_ + "_");   // cursor simple
    }
    window_.draw(t);
}

void App::drawMetrics() {
    const sf::FloatRect p = metricsPanel();
    sf::RectangleShape bg({ p.width, p.height });
    bg.setPosition(p.left, p.top);
    bg.setFillColor(col::kPanel);
    window_.draw(bg);
    if (!fontLoaded_) return;

    float y = p.top + 10.f;
    const float x = p.left + 12.f;
    auto put = [&](const std::string& s, sf::Color c,
                   unsigned size = 14, bool bold = false) {
        sf::Text t(s, font_, size);
        t.setFillColor(c);
        if (bold) t.setStyle(sf::Text::Bold);
        t.setPosition(x, y);
        window_.draw(t);
        y += size + 6.f;
    };

    put("CONSULTA", col::kAccent, 15, true);
    if (!hasResult_) {
        put("(sin busqueda todavia)", col::kMuted);
        return;
    }

    put("patron: \"" + query_ + "\"  (m=" + std::to_string(query_.size()) + ")", col::kText);
    put("coincidencias: " + std::to_string(stResult_.count), col::kText);
    y += 6.f;

    put("-- Suffix Tree --", col::kAccent, 14, true);
    put("nodos visitados: " + std::to_string(stResult_.nodesVisited), col::kText);
    put("comparaciones:   " + std::to_string(stResult_.charsCompared), col::kText);
    put("tiempo busqueda: " + fmt(stSearchMs_) + " ms", col::kText);
    y += 6.f;

    put("-- Busqueda ingenua --", col::kAccent, 14, true);
    put("comparaciones:   " + std::to_string(nvResult_.charsCompared), col::kText);
    put("tiempo busqueda: " + fmt(nvSearchMs_) + " ms", col::kText);
    y += 6.f;

    put("-- Comparacion --", col::kAccent, 14, true);
    const double ratio = stResult_.charsCompared > 0
        ? static_cast<double>(nvResult_.charsCompared) / stResult_.charsCompared : 0.0;
    put("ingenua/arbol (comparaciones): " + fmt(ratio) + "x", col::kText);
    const bool ok = (stResult_.count == nvResult_.count);
    put(ok ? "validacion: OK (coinciden)" : "validacion: DIFIEREN",
        ok ? sf::Color(120, 200, 120) : sf::Color(220, 120, 120));
    y += 6.f;

    put("posiciones (" + std::to_string(occ_.size()) + "):", col::kAccent, 14, true);
    std::string pos;
    for (size_t i = 0; i < occ_.size() && i < 12; ++i)
        pos += std::to_string(occ_[i]) + " ";
    if (occ_.size() > 12) pos += "...";
    put(pos, col::kMuted);
}

void App::render() {
    window_.clear(col::kBg);
    drawHeader();
    drawSearchBar();
    drawDocument();
    drawMetrics();
    window_.display();
}

// ---------------------------------------------------------------------------
// Bucle principal
// ---------------------------------------------------------------------------
void App::run() {
    window_.create(sf::VideoMode(1280, 800),
                   "Proyecto 2 - Suffix Tree", sf::Style::Default);
    window_.setFramerateLimit(60);

    fontLoaded_ = loadFont();
    if (fontLoaded_) {
        const sf::Glyph g = font_.getGlyph(L'M', charSize_, false);
        charWidth_  = (g.advance > 1.f) ? g.advance : charWidth_;
        lineHeight_ = font_.getLineSpacing(charSize_);
        if (lineHeight_ < charSize_) lineHeight_ = charSize_ * 1.4f;
    }

    cols_ = std::max(1, static_cast<int>(docPanel().width / charWidth_));
    wrapText();

    while (window_.isOpen()) {
        handleEvents();
        render();
    }
}
