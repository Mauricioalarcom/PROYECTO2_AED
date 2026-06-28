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
// Parte 2.1: ventana, fuente, render del documento con scroll.
// Parte 2.2 (este commit): barra de busqueda, ejecucion de la consulta y panel
// de metricas con la comparacion Suffix Tree vs busqueda ingenua.
// Los siguientes commits agregan el resaltado de ocurrencias y la ruta en el
// arbol.
//
// Se usa una fuente MONOESPACIADA a proposito: con ancho de glifo constante, el
// mapeo posicion-en-texto -> (fila, columna) es exacto y el resaltado de
// ocurrencias se reduce a dibujar rectangulos por columna.
// ===========================================================================
class App {
public:
    App() = default;
    // rawPageStarts: offsets en el texto crudo donde empieza cada pagina del PDF;
    // vacio para archivos .txt o texto embebido (sin informacion de pagina).
    App(std::string rawText, std::vector<int> rawPageStarts, std::string sourceLabel);
    void run();
    void requestLoadFromPath(const std::string& path);

private:
    enum class Screen { Import, Viewer };

    // ---- datos / nucleo ----
    Screen       screen_ = Screen::Import;
    std::string source_;
    std::string text_;       // texto normalizado: lo que se indexa Y lo que se muestra
    SuffixTree  tree_;
    double      buildMs_ = 0.0;
    bool        hasDocument_ = false;

    // ---- tracking de paginas (solo para PDF) ----
    std::vector<int> pageStartsNorm_;  // offset en text_ donde empieza cada pagina
    int              totalPages_ = 0;

    // Devuelve el numero de pagina (1-based) de la posicion normPos en text_.
    // Retorna -1 si no hay informacion de pagina.
    int getPage(int normPos) const;

    // ---- ventana / recursos ----
    sf::RenderWindow window_;
    sf::Font     font_;
    bool         fontLoaded_ = false;
    unsigned int charSize_   = 16;
    float        charWidth_  = 9.f;   // avance de glifo (constante en monoespaciada)
    float        lineHeight_ = 20.f;
    sf::Vector2u lastWindowSize_{};

    // ---- layout del documento ----
    int                      cols_ = 80;    // columnas por linea (segun ancho del panel)
    std::vector<std::string> lines_;        // texto partido en lineas para mostrar
    int                      scroll_ = 0;   // indice de la primera linea visible

    // ---- pantalla de importacion ----
    std::string importPath_;
    std::string statusMessage_;

    // ---- estado de la busqueda ----
    std::string  rawQuery_;                 // patron tal como lo escribe el usuario
    std::string  query_;                    // patron normalizado (el que se busca)
    bool         hasResult_ = false;
    SuffixTree::SearchResult stResult_;     // resultado + metricas del Suffix Tree
    double       stSearchMs_ = 0.0;
    naive::Result nvResult_;                // resultado + metricas de la ingenua
    double       nvSearchMs_ = 0.0;
    std::vector<int> occ_;                  // posiciones de las ocurrencias

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
    // Dibuja la ruta del patron en el arbol como una cadena vertical de aristas;
    // devuelve la 'y' donde termino para seguir dibujando debajo.
    float drawTreePath(float x, float y);

    // ---- geometria ----
    static constexpr float kRightPanelW = 380.f;  // ancho del panel de metricas
    static constexpr float kLeftMargin = 24.f;
    static constexpr float kTopBarH = 52.f;
    sf::FloatRect searchBar()   const;
    sf::FloatRect docPanel()    const;
    sf::FloatRect metricsPanel() const;
    sf::FloatRect importCard() const;
    sf::FloatRect importField() const;
    sf::FloatRect importPrimaryButton() const;
    sf::FloatRect importSecondaryButton() const;
    sf::FloatRect viewerButton() const;
    int  visibleLines() const;
    int  maxScroll() const;

    // ---- utilidades ----
    static std::string trim(const std::string& s);
    static std::string baseName(const std::string& path);
    static std::string openPdfDialog();
    static bool hit(const sf::FloatRect& r, sf::Vector2f p);
};

#endif // APP_H
