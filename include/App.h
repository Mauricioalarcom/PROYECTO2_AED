#ifndef APP_H
#define APP_H

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

#include "SuffixTree.h"

// ===========================================================================
// Aplicacion visual (SFML) del buscador indexado de documentos.
//
// Parte 2.1 (este commit): ventana, carga de fuente monoespaciada, normalizado
// y construccion del Suffix Tree, y render del texto del documento con scroll.
// Los siguientes commits agregan la barra de busqueda, el resaltado de
// ocurrencias, la ruta en el arbol y la comparacion contra la busqueda ingenua.
//
// Se usa una fuente MONOESPACIADA a proposito: con ancho de glifo constante, el
// mapeo posicion-en-texto -> (fila, columna) es exacto y el resaltado de
// ocurrencias se reduce a dibujar rectangulos por columna.
// ===========================================================================
class App {
public:
    App(std::string rawText, std::string sourceLabel);
    void run();

private:
    // ---- datos / nucleo ----
    std::string source_;     // etiqueta del origen (nombre de archivo)
    std::string text_;       // texto normalizado: lo que se indexa Y lo que se muestra
    SuffixTree  tree_;
    double      buildMs_ = 0.0;

    // ---- ventana / recursos ----
    sf::RenderWindow window_;
    sf::Font     font_;
    bool         fontLoaded_ = false;
    unsigned int charSize_   = 16;
    float        charWidth_  = 9.f;   // avance de glifo (constante en monoespaciada)
    float        lineHeight_ = 20.f;

    // ---- layout del documento ----
    int                      cols_ = 80;    // columnas por linea (segun ancho del panel)
    std::vector<std::string> lines_;        // texto partido en lineas para mostrar
    int                      scroll_ = 0;   // indice de la primera linea visible

    // ---- ciclo de vida / render ----
    bool loadFont();
    void wrapText();
    void handleEvents();
    void render();
    void drawHeader();
    void drawDocument();

    // ---- geometria ----
    sf::FloatRect docPanel() const;
    int  visibleLines() const;
    int  maxScroll() const;
};

#endif // APP_H
