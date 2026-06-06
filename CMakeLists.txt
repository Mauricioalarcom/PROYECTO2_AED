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
# Libreria con el nucleo del proyecto (reutilizable por main y por benchmark)
# ---------------------------------------------------------------------------
add_library(core STATIC
        src/SuffixTree.cpp
        src/NaiveSearch.cpp
        src/TextNormalizer.cpp
        src/DocumentLoader.cpp
)
target_include_directories(core PUBLIC ${CMAKE_SOURCE_DIR}/include)

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

# Copia los datos de ejemplo junto al ejecutable
file(COPY ${CMAKE_SOURCE_DIR}/data DESTINATION ${CMAKE_BINARY_DIR})
