#include "App.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <limits>
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
const sf::Color kBg        (17, 19, 26);
const sf::Color kPanel     (28, 32, 42);
const sf::Color kPanel2    (24, 27, 36);
const sf::Color kText      (232, 235, 242);
const sf::Color kMuted     (145, 152, 166);
const sf::Color kAccent    (92, 173, 255);
const sf::Color kAccent2   (128, 214, 165);
const sf::Color kDanger    (233, 122, 122);
const sf::Color kHighlight (72, 108, 176);   // fondo de las ocurrencias
} // namespace col

// ---------------------------------------------------------------------------
// Construccion: normaliza el texto y construye el Suffix Tree (mide el tiempo).
// Para PDF, normaliza pagina por pagina para conservar los offsets de pagina
// en el texto normalizado.
// ---------------------------------------------------------------------------
App::App(std::string rawText, std::vector<int> rawPageStarts, std::string sourceLabel)
    : App() {
    setDocument({std::move(rawText), std::move(rawPageStarts)}, std::move(sourceLabel));
}

void App::requestLoadFromPath(const std::string& path) {
    const std::string trimmed = trim(path);
    if (trimmed.empty()) {
        statusMessage_ = "Ingresa una ruta valida o arrastra un PDF.";
        screen_ = Screen::Import;
        return;
    }

    try {
        setDocument(docload::loadFull(trimmed), trimmed);
        screen_ = Screen::Viewer;
        statusMessage_.clear();
        importPath_ = trimmed;
    } catch (const std::exception& e) {
        statusMessage_ = e.what();
        screen_ = hasDocument_ ? Screen::Viewer : Screen::Import;
    }
}

void App::setDocument(docload::LoadResult doc, std::string sourceLabel) {
    source_ = std::move(sourceLabel);
    text_.clear();
    pageStartsNorm_.clear();
    totalPages_ = 0;
    hasResult_ = false;
    rawQuery_.clear();
    query_.clear();
    occ_.clear();
    stResult_ = {};
    nvResult_ = {};
    stSearchMs_ = 0.0;
    nvSearchMs_ = 0.0;
    scroll_ = 0;

    if (doc.pageStarts.empty()) {
        text_ = textnorm::normalize(doc.text);
    } else {
        totalPages_ = static_cast<int>(doc.pageStarts.size());
        pageStartsNorm_.reserve(totalPages_);
        for (int p = 0; p < totalPages_; ++p) {
            const int rawStart = doc.pageStarts[p];
            const int rawEnd = (p + 1 < totalPages_)
                ? doc.pageStarts[p + 1]
                : static_cast<int>(doc.text.size());
            const std::string normPage = textnorm::normalize(doc.text.substr(rawStart, rawEnd - rawStart));
            const int sep = (!text_.empty() && !normPage.empty()) ? 1 : 0;
            pageStartsNorm_.push_back(static_cast<int>(text_.size()) + sep);
            if (sep) text_ += ' ';
            text_ += normPage;
        }
    }

    const auto t0 = Clock::now();
    tree_.buildUkkonen(text_);
    buildMs_ = std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
    hasDocument_ = true;

    std::cerr << "[app] origen=" << source_
              << "  chars=" << text_.size()
              << "  nodos=" << tree_.nodeCount()
              << "  paginas=" << totalPages_
              << "  construccion=" << buildMs_ << " ms\n";

    refreshLayout();
}

void App::clearSearchState() {
    rawQuery_.clear();
    query_.clear();
    occ_.clear();
    hasResult_ = false;
    stResult_ = {};
    nvResult_ = {};
    stSearchMs_ = 0.0;
    nvSearchMs_ = 0.0;
    scroll_ = 0;
}

// Busqueda binaria: ultima pagina cuyo offset normalizado <= normPos.
int App::getPage(int normPos) const {
    if (pageStartsNorm_.empty()) return -1;
    int lo = 0, hi = static_cast<int>(pageStartsNorm_.size()) - 1;
    while (lo < hi) {
        const int mid = (lo + hi + 1) / 2;
        if (pageStartsNorm_[mid] <= normPos) lo = mid;
        else hi = mid - 1;
    }
    return lo + 1;  // pagina 1-based
}

// ---------------------------------------------------------------------------
// Carga una fuente monoespaciada (intenta varias rutas en Linux y macOS).
// ---------------------------------------------------------------------------
bool App::loadFont() {
    const char* candidates[] = {
        "data/font.ttf",                                  // fuente propia si existe
        // Rutas comunes en Linux
        "/usr/share/fonts/TTF/VeraMono.ttf",
        "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
        "/usr/share/fonts/truetype/ubuntu/UbuntuMono-R.ttf",
        "/usr/share/fonts/Adwaita/AdwaitaMono-Regular.ttf",
        "/usr/share/fonts/noto/NotoSansMono-Regular.ttf",
        // Rutas de macOS (fallback)
        "/System/Library/Fonts/Supplemental/Courier New.ttf",
        "/System/Library/Fonts/Menlo.ttc",
        "/System/Library/Fonts/Monaco.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",   // ultimo recurso (no monoespaciada)
    };
    for (const char* path : candidates) {
        if (font_.loadFromFile(path)) {
            std::cerr << "[app] fuente cargada: " << path << "\n";
            return true;
        }
    }
    std::cerr << "[app] ADVERTENCIA: no se pudo cargar ninguna fuente.\n";
    return false;
}

std::string App::trim(const std::string& s) {
    const auto first = s.find_first_not_of(" \t\n\r\f\v");
    if (first == std::string::npos) return "";
    const auto last = s.find_last_not_of(" \t\n\r\f\v");
    return s.substr(first, last - first + 1);
}

std::string App::baseName(const std::string& path) {
    if (path.empty()) return "(sin documento)";
    const std::filesystem::path p(path);
    const auto name = p.filename();
    if (!name.empty()) return name.string();
    return path;
}

std::string App::openPdfDialog() {
    const char* cmd =
        "zenity --file-selection --title=\"Selecciona un PDF\" "
        "--file-filter=\"PDF files | *.pdf\" "
        "--file-filter=\"Text files | *.txt\" 2>/dev/null";

    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "";

    std::string out;
    char buffer[512];
    while (std::fgets(buffer, sizeof(buffer), pipe)) out += buffer;
    pclose(pipe);
    return trim(out);
}

bool App::hit(const sf::FloatRect& r, sf::Vector2f p) {
    return r.contains(p);
}

void App::refreshLayout() {
    const auto sz = window_.getSize();
    if (sz.x == 0 || sz.y == 0) {
        cols_ = std::max(1, cols_);
        wrapText();
        return;
    }

    cols_ = std::max(1, static_cast<int>(docPanel().width / charWidth_));
    wrapText();
    scroll_ = std::clamp(scroll_, 0, maxScroll());
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
    if (lines_.empty()) lines_.emplace_back("");
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

sf::FloatRect App::importCard() const {
    const auto sz = window_.getSize();
    const float w = std::min(840.f, static_cast<float>(sz.x) - 80.f);
    const float h = 320.f;
    return { (static_cast<float>(sz.x) - w) * 0.5f, (static_cast<float>(sz.y) - h) * 0.45f, w, h };
}

sf::FloatRect App::importField() const {
    const sf::FloatRect c = importCard();
    return { c.left + 32.f, c.top + 120.f, c.width - 64.f, 44.f };
}

sf::FloatRect App::importPrimaryButton() const {
    const sf::FloatRect f = importField();
    return { f.left, f.top + 62.f, 190.f, 42.f };
}

sf::FloatRect App::importSecondaryButton() const {
    const sf::FloatRect f = importField();
    return { f.left + 206.f, f.top + 62.f, 250.f, 42.f };
}

sf::FloatRect App::viewerButton() const {
    const auto sz = window_.getSize();
    return { static_cast<float>(sz.x) - 176.f, 10.f, 148.f, 32.f };
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
            if (screen_ == Screen::Import) {
                if (u == 8) {
                    if (!importPath_.empty()) importPath_.pop_back();
                } else if (u == 13 || u == 10) {
                    requestLoadFromPath(importPath_);
                } else if (u >= 32 && u < 127) {
                    importPath_.push_back(static_cast<char>(u));
                }
            } else if (screen_ == Screen::Viewer) {
                if (u == 8) {
                    if (!rawQuery_.empty()) rawQuery_.pop_back();
                } else if (u == 13 || u == 10) {
                    runSearch();
                } else if (u >= 32 && u < 127) {
                    rawQuery_.push_back(static_cast<char>(u));
                }
            }
        }
        else if (e.type == sf::Event::KeyPressed) {
            if (screen_ == Screen::Import) {
                if (e.key.code == sf::Keyboard::Escape) window_.close();
                else if (e.key.code == sf::Keyboard::Enter || e.key.code == sf::Keyboard::Return)
                    requestLoadFromPath(importPath_);
                else if (e.key.code == sf::Keyboard::F2) {
                    const std::string picked = openPdfDialog();
                    if (!picked.empty()) {
                        importPath_ = picked;
                        requestLoadFromPath(importPath_);
                    }
                }
            } else {
                if (e.key.code == sf::Keyboard::Escape) window_.close();
                else if (e.key.code == sf::Keyboard::F2) {
                    screen_ = Screen::Import;
                    statusMessage_ = "Arrastra un PDF, pega una ruta o usa el selector.";
                    importPath_ = source_;
                }
                else if (e.key.code == sf::Keyboard::Down)  ++scroll_;
                else if (e.key.code == sf::Keyboard::Up)    --scroll_;
                else if (e.key.code == sf::Keyboard::PageDown) scroll_ += visibleLines();
                else if (e.key.code == sf::Keyboard::PageUp)   scroll_ -= visibleLines();
                else if (e.key.code == sf::Keyboard::Home)  scroll_ = 0;
                else if (e.key.code == sf::Keyboard::End)   scroll_ = maxScroll();
            }
        } else if (e.type == sf::Event::MouseWheelScrolled && screen_ == Screen::Viewer) {
            scroll_ -= static_cast<int>(e.mouseWheelScroll.delta * 3);
        } else if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left) {
            const sf::Vector2f p(static_cast<float>(e.mouseButton.x), static_cast<float>(e.mouseButton.y));
            if (screen_ == Screen::Import) {
                if (hit(importPrimaryButton(), p)) {
                    requestLoadFromPath(importPath_);
                } else if (hit(importSecondaryButton(), p)) {
                    const std::string picked = openPdfDialog();
                    if (!picked.empty()) {
                        importPath_ = picked;
                        requestLoadFromPath(importPath_);
                    } else {
                        statusMessage_ = "No se selecciono ningun archivo.";
                    }
                }
            } else if (screen_ == Screen::Viewer && hit(viewerButton(), p)) {
                screen_ = Screen::Import;
                statusMessage_ = "Arrastra un PDF, pega una ruta o usa el selector.";
                importPath_ = source_;
            }
        } else if (e.type == sf::Event::Resized) {
            window_.setView(sf::View(sf::FloatRect(0, 0,
                static_cast<float>(e.size.width), static_cast<float>(e.size.height))));
            if (screen_ == Screen::Viewer)
                refreshLayout();
        }

        if (screen_ == Screen::Viewer)
            scroll_ = std::clamp(scroll_, 0, maxScroll());
    }
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------
void App::drawBackground() {
    window_.clear(col::kBg);

    sf::RectangleShape strip({ static_cast<float>(window_.getSize().x), 4.f });
    strip.setFillColor(col::kAccent);
    strip.setPosition(0.f, 0.f);
    window_.draw(strip);
}

void App::drawHeader() {
    if (!fontLoaded_) return;
    const sf::FloatRect btnRect = viewerButton();

    sf::RectangleShape banner({ static_cast<float>(window_.getSize().x), kTopBarH });
    banner.setPosition(0.f, 0.f);
    banner.setFillColor(col::kPanel2);
    window_.draw(banner);

    sf::Text t("", font_, 18);
    t.setFillColor(col::kText);
    t.setStyle(sf::Text::Bold);
    t.setPosition(kLeftMargin, 12.f);
    t.setString("Suffix Tree (Ukkonen) - Buscador indexado de documentos");
    window_.draw(t);

    sf::Text info("", font_, 13);
    info.setFillColor(col::kMuted);
    info.setPosition(kLeftMargin, 36.f);
    info.setString("origen: " + baseName(source_) +
                   "   |   chars: " + std::to_string(text_.size()) +
                   "   |   nodos: " + std::to_string(tree_.nodeCount()) +
                   "   |   construccion: " + fmt(buildMs_) + " ms" +
                   "   |   F2: importar otro PDF");
    window_.draw(info);

    sf::RectangleShape button({ btnRect.width, btnRect.height });
    button.setPosition(btnRect.left, btnRect.top);
    button.setFillColor(col::kPanel);
    button.setOutlineColor(col::kAccent);
    button.setOutlineThickness(1.f);
    window_.draw(button);

    sf::Text bt("", font_, 13);
    bt.setFillColor(col::kText);
    bt.setStyle(sf::Text::Bold);
    bt.setPosition(btnRect.left + 16.f, btnRect.top + 7.f);
    bt.setString("Importar PDF");
    window_.draw(bt);
}

void App::drawDocument() {
    const sf::FloatRect p = docPanel();

    sf::RectangleShape bg({ p.width, p.height });
    bg.setPosition(p.left, p.top);
    bg.setFillColor(col::kPanel);
    bg.setOutlineColor(sf::Color(46, 51, 64));
    bg.setOutlineThickness(1.f);
    window_.draw(bg);

    if (!fontLoaded_) return;

    const int first = scroll_;
    const int last  = std::min(static_cast<int>(lines_.size()), first + visibleLines());

    drawHighlights(p, first, last);

    sf::Text line("", font_, charSize_);
    line.setFillColor(col::kText);
    for (int i = first; i < last; ++i) {
        line.setString(lines_[i]);
            line.setPosition(p.left + 6.f, p.top + 4.f + static_cast<float>(i - first) * lineHeight_);
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
            hl.setSize({ static_cast<float>(width) * charWidth_, lineHeight_ });
            hl.setPosition(p.left + 6.f + static_cast<float>(segStart) * charWidth_,
                           p.top + 4.f + static_cast<float>(ln - first) * lineHeight_);
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
    box.setFillColor(col::kPanel2);
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
    bg.setOutlineColor(sf::Color(46, 51, 64));
    bg.setOutlineThickness(1.f);
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
        y += static_cast<float>(size) + 6.f;
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
        ok ? col::kAccent2 : col::kDanger);
    y += 6.f;

    // Paginas (solo PDF): lista de paginas distintas donde aparece el patron.
    if (totalPages_ > 0 && !occ_.empty()) {
        std::string pgStr;
        int prevPg = -1, shown = 0;
        for (int p : occ_) {          // occ_ esta ordenado
            const int pg = getPage(p);
            if (pg != prevPg) {
                if (shown > 0) pgStr += ' ';
                pgStr += std::to_string(pg);
                prevPg = pg;
                if (++shown >= 10 && occ_.size() > static_cast<size_t>(shown))
                    { pgStr += " ..."; break; }
            }
        }
        put("paginas: " + pgStr, col::kText);
    }

    put("posiciones (" + std::to_string(occ_.size()) + "):", col::kAccent, 14, true);
    std::string pos;
    for (size_t i = 0; i < occ_.size() && i < 10; ++i) {
        const int pg = getPage(occ_[i]);
        if (pg > 0)
            pos += "p" + std::to_string(pg) + ":" + std::to_string(occ_[i]) + " ";
        else
            pos += std::to_string(occ_[i]) + " ";
    }
    if (occ_.size() > 10) pos += "...";
    put(pos, col::kMuted);

    y += 8.f;
    drawTreePath(x, y);
}

void App::drawImportScreen() {
    drawBackground();

    if (!fontLoaded_) return;

    const sf::FloatRect card = importCard();
    sf::RectangleShape panel({ card.width, card.height });
    panel.setPosition(card.left, card.top);
    panel.setFillColor(col::kPanel);
    panel.setOutlineColor(sf::Color(50, 58, 72));
    panel.setOutlineThickness(1.f);
    window_.draw(panel);

    sf::RectangleShape accent({ card.width, 4.f });
    accent.setPosition(card.left, card.top);
    accent.setFillColor(col::kAccent);
    window_.draw(accent);

    sf::Text title("Importar documento PDF", font_, 26);
    title.setFillColor(col::kText);
    title.setStyle(sf::Text::Bold);
    title.setPosition(card.left + 32.f, card.top + 24.f);
    window_.draw(title);

    sf::Text subtitle("Pega una ruta, escribe un archivo local o abre el selector del sistema.", font_, 14);
    subtitle.setFillColor(col::kMuted);
    subtitle.setPosition(card.left + 32.f, card.top + 62.f);
    window_.draw(subtitle);

    sf::Text label("Ruta del archivo:", font_, 14);
    label.setFillColor(col::kAccent);
    label.setStyle(sf::Text::Bold);
    label.setPosition(card.left + 32.f, card.top + 96.f);
    window_.draw(label);

    const sf::FloatRect field = importField();
    sf::RectangleShape input({ field.width, field.height });
    input.setPosition(field.left, field.top);
    input.setFillColor(col::kPanel2);
    input.setOutlineColor(col::kAccent);
    input.setOutlineThickness(1.f);
    window_.draw(input);

    sf::Text pathText("", font_, 15);
    pathText.setFillColor(importPath_.empty() ? col::kMuted : col::kText);
    pathText.setPosition(field.left + 12.f, field.top + 10.f);
    pathText.setString(importPath_.empty() ? "Escribe una ruta o usa el selector..."
                                           : importPath_ + "_");
    window_.draw(pathText);

    auto drawBtn = [&](const sf::FloatRect& r, const std::string& txt,
                       sf::Color fill, sf::Color outline, sf::Color textColor) {
        sf::RectangleShape b({ r.width, r.height });
        b.setPosition(r.left, r.top);
        b.setFillColor(fill);
        b.setOutlineColor(outline);
        b.setOutlineThickness(1.f);
        window_.draw(b);
        sf::Text t(txt, font_, 14);
        t.setFillColor(textColor);
        t.setStyle(sf::Text::Bold);
        t.setPosition(r.left + 16.f, r.top + 11.f);
        window_.draw(t);
    };

    drawBtn(importPrimaryButton(), "Cargar PDF/TXT", col::kAccent, col::kAccent, sf::Color::White);
    drawBtn(importSecondaryButton(), "Abrir selector del sistema", col::kPanel2, col::kAccent2, col::kText);

    sf::Text hint("Enter: cargar | F2: selector | Esc: salir", font_, 13);
    hint.setFillColor(col::kMuted);
    hint.setPosition(card.left + 32.f, card.top + 200.f);
    window_.draw(hint);

    if (!statusMessage_.empty()) {
        sf::Text status(statusMessage_, font_, 14);
        status.setFillColor(col::kDanger);
        status.setPosition(card.left + 32.f, card.top + 232.f);
        window_.draw(status);
    }
}

void App::drawViewerScreen() {
    drawBackground();
    drawHeader();
    drawSearchBar();
    drawDocument();
    drawMetrics();
}

// Cadena vertical: raiz -> "etiqueta" -> [nodo] -> ... Evidencia los nodos y
// aristas recorridos por el patron (la "ruta de busqueda" del enunciado).
float App::drawTreePath(float x, float y) {
    auto put = [&](const std::string& s, sf::Color c,
                   unsigned size = 14, bool bold = false) {
        sf::Text t(s, font_, size);
        t.setFillColor(c);
        if (bold) t.setStyle(sf::Text::Bold);
        t.setPosition(x, y);
        window_.draw(t);
        y += static_cast<float>(size) + 6.f;
    };

            put("-- Ruta en el arbol --", col::kAccent, 14, true);
    if (!stResult_.found || stResult_.path.empty()) {
        put("patron ausente (sin ruta)", col::kMuted);
        return y;
    }

    const std::vector<Node*>& path = stResult_.path;
    put("raiz", col::kText);

    const int maxSteps = 8;
    for (int k = 1; k < (int)path.size(); ++k) {
        if (k > maxSteps) {
            put("  ... (" + std::to_string(path.size() - 1) + " aristas en total)", col::kMuted);
            break;
        }
        std::string lbl = tree_.edgeLabel(path[k]);
        if (lbl.size() > 16) lbl = lbl.substr(0, 16) + "..";
        put("  | \"" + lbl + "\"", col::kMuted);

        const int si = path[k]->suffixIndex;
        std::string nodeStr = "  [n" + std::to_string(k) + "]";
        if (si >= 0) nodeStr += "  hoja@" + std::to_string(si);
        put(nodeStr, col::kText);
    }
    return y;
}

void App::render() {
    if (screen_ == Screen::Import) drawImportScreen();
    else drawViewerScreen();
    window_.display();
}

// ---------------------------------------------------------------------------
// Bucle principal
// ---------------------------------------------------------------------------
void App::run() {
    window_.create(sf::VideoMode(1980, 1260),
                   "Proyecto 2 - Suffix Tree", sf::Style::Default);
    window_.setFramerateLimit(60);
    window_.setVerticalSyncEnabled(true);

    lastWindowSize_ = window_.getSize();

    fontLoaded_ = loadFont();
    if (fontLoaded_) {
        const sf::Glyph g = font_.getGlyph(L'M', charSize_, false);
        charWidth_  = (g.advance > 1.f) ? g.advance : charWidth_;
        lineHeight_ = font_.getLineSpacing(charSize_);
        if (lineHeight_ < static_cast<float>(charSize_)) lineHeight_ = static_cast<float>(charSize_) * 1.4f;
    }

    refreshLayout();
    if (!hasDocument_) {
        screen_ = Screen::Import;
        statusMessage_ = "Arrastra un PDF, pega una ruta o usa el selector del sistema.";
    } else {
        screen_ = Screen::Viewer;
    }

    while (window_.isOpen()) {
        handleEvents();
        const auto sz = window_.getSize();
        if (sz != lastWindowSize_) {
            lastWindowSize_ = sz;
            if (screen_ == Screen::Viewer)
                refreshLayout();
        }
        render();
    }
}
