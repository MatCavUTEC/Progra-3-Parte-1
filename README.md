# Progra-3-Parte-1
Primera parte del Proyecto grupal en Programación 3

## Compilación y pruebas

Requisitos: CMake 3.20 o superior, un compilador con soporte de C++20, Ninja y Git. Durante la
configuración CMake descarga FTXUI v7.0.3.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Sin conexión: clone FTXUI v7.0.3 en una carpeta local y agregue
`-DFETCHCONTENT_SOURCE_DIR_FTXUI=<ruta>` al primer comando.
