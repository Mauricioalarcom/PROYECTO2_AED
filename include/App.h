#ifndef APP_H
#define APP_H

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

#include "DocumentLoader.h"
#include "NaiveSearch.h"
#include "SuffixTree.h"

// ===========================================================================
// Aplicacion visual (SFML) del buscador indexado de documentos.
//
// Se usa una fuente MONOESPACIADA a proposito: con ancho de glifo constante, el
// mapeo posicion-en-texto -> (fila, columna) es exacto y el resaltado de
// ocurrencias se reduce a dibujar rectangulos por columna.
// ===========================================================================
class App {
public:
    App() = default;
    App(std::string rawText, std::vector<int> rawPageStarts, std::string sourceLabel);
    void run();
    void requestLoadFromPath(const std::string& path);

private:
    enum class Screen { Import, Viewer };

    // ---- datos / nucleo ----
    Screen       screen_ = Screen::Import;
    std::string  source_;
    std::string  text_;
    SuffixTree   tree_;
    double       buildMs_    = 0.0;
    bool         hasDocument_ = false;

    // ---- tracking de paginas (solo para PDF) ----
    std::vector<int> pageStartsNorm_;
    int              totalPages_ = 0;

    int getPage(int normPos) const;

    // ---- ventana / recursos ----
    sf::RenderWindow window_;
    sf::Font         font_;
    bool             fontLoaded_  = false;
    unsigned int     charSize_    = 16;
    float            charWidth_   = 9.f;
    float            lineHeight_  = 20.f;
    sf::Vector2u     lastWindowSize_{};

    // ---- pantalla de importacion ----
    std::string importPath_;
    std::string statusMessage_;

    // ---- layout del documento ----
    int                      cols_   = 80;
    std::vector<std::string> lines_;
    int                      scroll_ = 0;

    // ---- estado de la busqueda ----
    std::string           rawQuery_;
    std::string           query_;
    bool                  hasResult_   = false;
    SuffixTree::SearchResult stResult_;
    double                stSearchMs_  = 0.0;
    naive::Result         nvResult_;
    double                nvSearchMs_  = 0.0;
    std::vector<int>      occ_;

    // ---- ciclo de vida / render ----
    void setDocument(docload::LoadResult doc, std::string sourceLabel);
    void clearSearchState();
    void refreshLayout();
    bool loadFont();
    void wrapText();
    void handleEvents();
    void runSearch();
    void render();
    void drawBackground();
    void drawImportScreen();
    void drawViewerScreen();
    void drawHeader();
    void drawSearchBar();
    void drawDocument();
    void drawHighlights(const sf::FloatRect& panel, int first, int last);
    void drawMetrics();
    float drawTreePath(float x, float y);

    // ---- geometria ----
    static constexpr float kRightPanelW = 380.f;
    static constexpr float kLeftMargin  = 24.f;
    static constexpr float kTopBarH     = 52.f;
    sf::FloatRect searchBar()            const;
    sf::FloatRect docPanel()             const;
    sf::FloatRect metricsPanel()         const;
    sf::FloatRect importCard()           const;
    sf::FloatRect importField()          const;
    sf::FloatRect importPrimaryButton()  const;
    sf::FloatRect importSecondaryButton() const;
    sf::FloatRect viewerButton()         const;
    int  visibleLines() const;
    int  maxScroll()    const;

    // ---- utilidades ----
    static std::string trim(const std::string& s);
    static std::string baseName(const std::string& path);
    static std::string openPdfDialog();
    static bool hit(const sf::FloatRect& r, sf::Vector2f p);
};

#endif // APP_H
