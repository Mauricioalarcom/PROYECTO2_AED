#include "App.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <utility>

#include "TextNormalizer.h"

using Clock = std::chrono::high_resolution_clock;

// Paleta de colores de la interfaz.
namespace col {
const sf::Color kBg        (24, 26, 32);
const sf::Color kPanel     (32, 35, 43);
const sf::Color kText      (220, 223, 228);
const sf::Color kMuted     (140, 146, 158);
const sf::Color kAccent    (88, 166, 255);
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
sf::FloatRect App::docPanel() const {
    const auto sz = window_.getSize();
    const float x = 20.f, y = 70.f;
    return { x, y, static_cast<float>(sz.x) - 2 * x, static_cast<float>(sz.y) - y - 20.f };
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

    sf::Text line("", font_, charSize_);
    line.setFillColor(col::kText);
    for (int i = first; i < last; ++i) {
        line.setString(lines_[i]);
        line.setPosition(p.left + 6.f, p.top + 4.f + (i - first) * lineHeight_);
        window_.draw(line);
    }
}

void App::render() {
    window_.clear(col::kBg);
    drawHeader();
    drawDocument();
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
