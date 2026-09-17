cmake_minimum_required(VERSION 3.20)
project(circuit_escape CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# FTXUI fijado a una versión concreta (§5.8). Para compilar sin red se puede usar
# una copia local: -DFETCHCONTENT_SOURCE_DIR_FTXUI=<ruta a FTXUI v7.0.3>
include(FetchContent)
set(FTXUI_ENABLE_INSTALL OFF CACHE BOOL "Generar reglas de instalación de FTXUI")
set(FTXUI_QUIET ON CACHE BOOL "Ocultar mensajes de configuración de FTXUI")
FetchContent_Declare(ftxui
        GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
        GIT_TAG v7.0.3
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(ftxui)

# Biblioteca central (Core): no depende de FTXUI ni hace entrada/salida
add_library(circuit_escape_core
        include/circuit_escape/grid.hpp
        include/circuit_escape/cells.hpp
        include/circuit_escape/environment.hpp
        include/circuit_escape/controllers.hpp
        include/circuit_escape/game_rules.hpp
        src/environment.cpp
        src/controllers.cpp
)
target_include_directories(circuit_escape_core PUBLIC include)

# Aplicación ejecutable principal (app/main.cpp): única parte que usa FTXUI
add_executable(navigation_game
        app/main.cpp
)
target_link_libraries(navigation_game PRIVATE circuit_escape_core ftxui::ftxui)

# Pruebas Unitarias explícitas (tests/)
enable_testing()

add_executable(grid_test tests/grid_test.cpp)
target_link_libraries(grid_test PRIVATE circuit_escape_core)
add_test(NAME GridTest COMMAND grid_test)

add_executable(environment_test tests/environment_test.cpp)
target_link_libraries(environment_test PRIVATE circuit_escape_core)
add_test(NAME EnvironmentTest COMMAND environment_test)

add_executable(interactions_test tests/interactions_test.cpp)
target_link_libraries(interactions_test PRIVATE circuit_escape_core)
add_test(NAME InteractionsTest COMMAND interactions_test)

add_executable(controllers_test tests/controllers_test.cpp)
target_link_libraries(controllers_test PRIVATE circuit_escape_core)
add_test(NAME ControllersTest COMMAND controllers_test)
