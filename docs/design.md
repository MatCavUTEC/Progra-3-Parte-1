# Diseño — Circuito de Escape

Este documento registra las decisiones de diseño y dónde se aplica cada tema obligatorio del
enunciado ([proyecto_1_2026_2.pdf](proyecto_1_2026_2.pdf)). Se actualiza en cada etapa.

## Capas y dependencias

| Capa | Ubicación | Depende de |
|---|---|---|
| Motor: tablero, celdas, reglas, entorno y controladores | `include/circuit_escape/`, `src/` | Solo la biblioteca estándar |
| Interfaz de consola y ciclo de la partida | `app/` | Motor y FTXUI |
| Pruebas | `tests/` | Motor (la prueba de la interfaz también usa FTXUI) |

- El motor no incluye cabeceras de FTXUI ni hace entrada/salida (§5.8), así que sus pruebas
  corren sin consola.
- FTXUI se descarga con `FetchContent`, fijado en el tag `v7.0.3`. Sin red se puede usar una
  copia local con `-DFETCHCONTENT_SOURCE_DIR_FTXUI=<ruta>`.
- Las pruebas usan `assert` y anulan `NDEBUG` antes de incluir `<cassert>`, de modo que también
  verifican cuando se compila en Release.

## Decisiones

### Constructor de `NavigationEnvironment` (desviación de §5.7)

§5.7 propone:

```cpp
NavigationEnvironment(grid_type initialGrid, Position start,
                      int initialEnergy, std::size_t turnLimit);
```

Este proyecto usa:

```cpp
NavigationEnvironment(grid_type initialGrid, Position start, GameRules rules);
```

Motivo: §5.5 recomienda reunir costos y recompensas en `GameRules` y que el entorno aplique las
reglas que recibe sin conocer el nombre del nivel. El entorno necesita esas reglas de todos modos
(costo de entrada normal, de `wait` y de intento inválido), y `GameRules` ya incluye la energía
inicial y el límite de turnos. Recibirlos además como parámetros sueltos duplicaría esos datos y
permitiría valores contradictorios.

El comportamiento exigido no cambia. Se mantienen las precondiciones de §5.7, y si alguna no se
cumple el constructor lanza `std::invalid_argument`:

- `start` y la salida pertenecen al tablero;
- la posición inicial es transitable;
- existe exactamente una salida;
- la energía inicial y el límite de turnos de `rules` son positivos.

### Valores de las reglas

`GameRules` es la única fuente de los valores numéricos (§5.5). Al construir el tablero a partir
del texto de un mapa, cada celda recibe sus valores desde las reglas: `RoughTerrain::energyCost`,
`ResourceCell::reward`, `Battery::energy` y las penalizaciones de `Trap`. El entorno lee el efecto
desde la celda y toma de `GameRules` el costo de entrada normal, el de `wait`, el del intento
inválido, la energía inicial y el límite de turnos. Así las celdas conservan la forma de §5.7 y el
entorno no tiene ramas según el nivel.

### Rasgos de celdas (`CellTraits`)

`CellTraits` describe lo que depende solo del tipo de celda (§6.3):

- `traversable`: falso solo para `Wall`, mediante una especialización total. Decide si un
  movimiento se rechaza, qué acciones están disponibles y si la posición inicial es válida.
- `collectible`: verdadero para toda la familia `ResourceCell<Reward>`, mediante una
  especialización parcial, sea cual sea el tipo de la recompensa. Sirve para contar los recursos
  del mapa, por ejemplo el total de la barra de estado ("Recursos 2/3"). No cambia al recoger el
  recurso, porque depende del tipo y no del estado.

A diferencia del ejemplo de §6.3, los rasgos no incluyen `energyCost`: los números vienen de
`GameRules` (ver "Valores de las reglas"). Tampoco se usa un rasgo `consumable`: cada efecto marca
su propia celda, así que ese rasgo no tendría uso. `isTraversable` e `isCollectible`
(`src/cells.cpp`) aplican los rasgos a un `Cell` consultando su tipo activo con `std::visit`.

### Acceso a las celdas del tablero

`Grid` expone `at(Position)` en versión const y no const, que valida ambas coordenadas y lanza
`std::out_of_range`. Esa es la "operación equivalente" que admite §6.2 en lugar de un
`operator()(fila, columna)`: es la forma que usa el propio enunciado en §5.7 y en el ejemplo de
renderizado, así que un segundo accesor quedaría sin uso. `contains` responde lo mismo sin lanzar,
y ambos consultan un único límite privado (`isInside`) para que no puedan discrepar.

### Recursos y baterías consumidos

Se marcan con `collected` / `consumed` en lugar de reemplazar la celda por `Empty` (§5.7 admite
ambas opciones). Una celda marcada se comporta como espacio libre y se dibuja como tal.

### Mapas

Los escenarios de 20 × 30 están en `assets/maps/` como texto: un carácter por celda con la
convención ASCII de §5.8, y `@` para la posición inicial del agente (esa celda es espacio libre).
`app/` y las pruebas leen el archivo. La conversión de las líneas a `Grid` es una función pura del
motor que lanza `std::invalid_argument` si el formato no es válido.

### Cabeceras adicionales

Además de las cinco de §10.1:

- `position.hpp`: `Position`, `Action`, `neighbor` y `toString`.
- `events.hpp`: `Observation`, los eventos, `EndReason` y `StepResult`.
- `algorithms.hpp`: templates de función propios (§6.1), `Overloaded` y la fold expression (§6.4).

### Interfaz de consola

`ConsoleUI` vive en `app/console_ui.hpp` y `app/console_ui.cpp`, compilado como biblioteca enlazada
a FTXUI que usan `navigation_game` y `tests/console_ui_test.cpp`. Es la única prueba que depende de
FTXUI, porque §8 exige probar la traducción de comandos y el renderizado.

### `reset(seed)`

El entorno no toma decisiones aleatorias: `reset` restaura el tablero y el agente a su estado
inicial y guarda la semilla recibida. El azar está en `RandomPolicy`, que posee su propio generador
con semilla configurable (§9).

### Contratos con dos versiones en el enunciado

Se sigue §5.7:

- `IController::selectAction(const Observation&, std::span<const Action>)`, en lugar de
  `Controller<State>` con `std::vector` (§6.5).
- `NavigationEvent` con los seis eventos, incluido `TrapTriggeredEvent`, que el ejemplo de §6.4
  omite.

## Ubicación de los temas obligatorios

| Tema (sección del enunciado) | Dónde se aplica |
|---|---|
| `Grid<Cell, Rows, Columns>` con `std::array` e iteradores (§5.1, §6.2) | `include/circuit_escape/grid.hpp` |
| Templates de función con iteradores (§6.1) | Pendiente |
| Especialización total y parcial (§6.3) | `CellTraits<Wall>` (total) y `CellTraits<ResourceCell<Reward>>` (parcial) en `include/circuit_escape/cells.hpp`; usadas por `isTraversable` e `isCollectible` en `src/cells.cpp` |
| Paquete variádico y fold expression (§6.4) | Pendiente |
| Concept, interfaz virtual y adaptador genérico (§6.5) | Pendiente |
| Biblioteca estándar (§6.6) | Pendiente |
| Prueba negativa de compilación (§8) | Pendiente |
