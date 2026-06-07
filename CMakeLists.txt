cmake_minimum_required(VERSION 3.28)
project(PROYECTO2_AED CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# SFML 2.5.x (instalado con Homebrew: `brew install sfml@2`)
# Solo se usa para la VISUALIZACION. El nucleo algoritmico (Suffix Tree con
# Ukkonen) esta implementado desde cero en src/SuffixTree.cpp.
# ---------------------------------------------------------------------------
if(APPLE)
    set(SFML_DIR "/opt/homebrew/opt/sfml@2/lib/cmake/SFML")
endif()
find_package(SFML 2.5.1 COMPONENTS system window graphics REQUIRED)
message(STATUS "SFML encontrado: ${SFML_VERSION}")

# ---------------------------------------------------------------------------
# poppler-cpp (OPCIONAL) para extraer texto de PDFs con texto seleccionable.
# Si no esta instalado, el proyecto compila igual pero solo soporta .txt.
# Instalar en macOS: `brew install poppler pkg-config`
# ---------------------------------------------------------------------------
find_package(PkgConfig QUIET)
set(HAVE_POPPLER FALSE)
if(PkgConfig_FOUND)
    pkg_check_modules(POPPLER_CPP QUIET poppler-cpp)
    if(POPPLER_CPP_FOUND)
        set(HAVE_POPPLER TRUE)
        message(STATUS "poppler-cpp encontrado: ${POPPLER_CPP_VERSION} (soporte PDF habilitado)")
    endif()
endif()
if(NOT HAVE_POPPLER)
    message(STATUS "poppler-cpp NO encontrado (solo .txt). Instala: brew install poppler pkg-config")
endif()

# ---------------------------------------------------------------------------
# Libreria con el nucleo del proyecto (reutilizable por main y por benchmark)
# ---------------------------------------------------------------------------
add_library(core STATIC
        src/SuffixTree.cpp
        src/NaiveSearch.cpp
        src/TextNormalizer.cpp
        src/DocumentLoader.cpp
)
target_include_directories(core PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Conectar poppler-cpp al nucleo (DocumentLoader.cpp lo usa para PDF).
if(HAVE_POPPLER)
    target_compile_definitions(core PUBLIC HAVE_POPPLER=1)
    target_include_directories(core PRIVATE ${POPPLER_CPP_INCLUDE_DIRS})
    target_link_libraries(core PUBLIC ${POPPLER_CPP_LIBRARIES})
    target_link_directories(core PUBLIC ${POPPLER_CPP_LIBRARY_DIRS})
endif()

# ---------------------------------------------------------------------------
# Ejecutable principal
# (Parte 1: demo de consola que valida el nucleo contra la solucion ingenua.
#  Parte 2: se conectara la visualizacion SFML.)
# ---------------------------------------------------------------------------
add_executable(${PROJECT_NAME}
        src/main.cpp
        src/App.cpp)        # App.cpp usa SFML; el nucleo (core) queda libre de SFML
target_link_libraries(${PROJECT_NAME} PRIVATE
        core
        sfml-system sfml-window sfml-graphics)

# ---------------------------------------------------------------------------
# Ejecutable de benchmark (punto 6): Suffix Tree vs busqueda ingenua en varios
# tamanos. Enlaza solo el nucleo (no necesita SFML).
# ---------------------------------------------------------------------------
add_executable(benchmark src/benchmark.cpp)
target_link_libraries(benchmark PRIVATE core)

# Copia los datos de ejemplo junto al ejecutable
file(COPY ${CMAKE_SOURCE_DIR}/data DESTINATION ${CMAKE_BINARY_DIR})
