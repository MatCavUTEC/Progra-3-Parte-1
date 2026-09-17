# Plan por etapas — Circuito de Escape

Plan de trabajo del proyecto 1 ([enunciado](proyecto_1_2026_2.pdf)). Al completar una etapa se
marca aquí con su commit de merge. La justificación de cada decisión está en [design.md](design.md).

## Avance

- [x] **Etapa 0** — `chore/build-setup` — completa (merge `9802e78`)
- [x] **Etapa 1** — `feature/position-cells` — completa (tag `etapa-1`)
- [x] **Etapa 2** — `feature/grid` — completa (tag `etapa-2`)
- [x] **Etapa 3** — `feature/game-rules` — completa (tag `etapa-3`)
- [x] **Etapa 4** — `feature/algorithms` — completa (tag `etapa-4`)
- [ ] **Etapa 5** — `feature/environment-core`
- [ ] **Etapa 6** — `feature/cell-interactions`
- [ ] **Etapa 7** — `feature/controllers`
- [ ] **Etapa 8** — `feature/scenarios`
- [ ] **Etapa 9** — `feature/console-ui`
- [ ] **Etapa 10** — `feature/auto-simulation`
- [ ] **Etapa 11** — `docs/delivery`

Correspondencia con el plan sugerido del enunciado (§11): semana 1 → etapas 0–2, semana 2 → 3–6,
semana 3 → 7 (y 4), semana 4 → 8–11.

## Flujo de trabajo

- Repositorio de prueba trabajado en solitario: sin pull requests ni coordinación con el grupo.
- Una rama por etapa, creada desde `main`, con commits pequeños.
- Compilar y correr `ctest` antes de cada commit.
- Al terminar la etapa: merge `--no-ff` a `main`, borrar la rama y marcarla en este archivo.
- No hacer push a `main` desde el asistente.

## Decisiones confirmadas

1. **Constructor del entorno:** `NavigationEnvironment(grid, start, GameRules)`, en lugar de la
   firma de §5.7 con energía inicial y límite de turnos sueltos. La desviación está documentada en
   `design.md`.
2. **Valores de las reglas:** `GameRules` es la única fuente. La función que arma el tablero copia
   a cada celda sus valores (`RoughTerrain::energyCost`, `ResourceCell::reward`, `Battery::energy`,
   penalizaciones de `Trap`). El entorno toma de `GameRules` el costo de entrada normal, el de
   `wait`, el del intento inválido, la energía inicial y el límite de turnos.
3. **Consumibles:** recursos y baterías se marcan con `collected` / `consumed`; no se reemplazan por
   `Empty`.
4. **Mapas:** archivos `.txt` en `assets/maps/`, con la convención ASCII de §5.8 y `@` como inicio
   del agente. `app/` y las pruebas leen el archivo; la conversión de texto a tablero es una función
   pura del motor.
5. **Cabeceras adicionales:** `position.hpp`, `events.hpp` y `algorithms.hpp`.
6. **Interfaz:** `app/console_ui.{hpp,cpp}` como biblioteca enlazada a FTXUI, con su propia prueba
   `tests/console_ui_test.cpp`. Es la única prueba que usa FTXUI.
7. **`reset(seed)`:** restaura el estado inicial y guarda la semilla. El azar está en `RandomPolicy`.
8. **Contradicciones del enunciado:** se sigue §5.7 (`IController` con `std::span`; `NavigationEvent`
   con los seis eventos, incluido `TrapTriggeredEvent`).
9. **Perfiles de dificultad (§5.5):** la tabla quedó leída así:

   | Perfil | Energía | Turnos | Costo normal / terreno / wait-inválido | Recurso | Batería | Trampa (energía / puntaje) |
   |---|---|---|---|---|---|---|
   | Easy | 80 | 240 | 1 / 2 / 1 | +15 | +5 | −1 / 0 |
   | Standard | 60 | 180 | 1 / 2 / 1 | +10 | +3 | −2 / −1 |
   | Hard | 40 | 140 | 1 / 3 / 1 | +8 | +2 | −3 / −2 |

## Etapas

### Etapa 0 — `chore/build-setup` ✅ completa

Secciones: §5.8 (dependencia), §10.1, §10.2.

Hecho:
- FTXUI v7.0.3 con `FetchContent`, enlazado solo a `navigation_game`; el core y las pruebas no
  dependen de FTXUI. Sin conexión se usa `-DFETCHCONTENT_SOURCE_DIR_FTXUI=<ruta>`.
- `.gitignore` con `build/`, `cmake-build-*/` y `.vs/`.
- Las pruebas anulan `NDEBUG`, así que `assert` también verifica en Release.
- `docs/design.md` con las decisiones confirmadas y `README.md` con la compilación.
- Verificado: configuración desde una carpeta vacía y `ctest` 4/4 en Debug y Release.

### Etapa 1 — `feature/position-cells` ✅ completa

Secciones: §5.2, §5.5 (celdas), §5.7 (tipos básicos y celdas), §6.3.

Hecho:
- `position.hpp` / `src/position.cpp`: `Position` (con `==` por defecto), `Action`, `neighbor()`
  (devuelve `std::nullopt` si la fila o la columna saldría negativa y lanza
  `std::invalid_argument` ante una acción fuera del enum) y `toString()` con formato `(fila,columna)`.
- `cells.hpp`: las siete celdas y el `std::variant` `Cell`.
- `CellTraits`: plantilla general; especialización total para `Wall` (`traversable = false`);
  especialización parcial para `ResourceCell<Reward>` (`collectible = true`).
- `src/cells.cpp`: `isTraversable` e `isCollectible` sobre un `Cell`.
- Pruebas (`grid_test.cpp`): igualdad de posiciones, vecinos en el interior, bordes y esquinas,
  `neighbor` sin conocer el tamaño del tablero, acción inválida, `toString`, valores por defecto de
  las celdas, `static_assert` sobre los rasgos y rasgos a través de `Cell`.
- Verificado: `ctest` 4/4, sin advertencias con `-Wall -Wextra -Wpedantic -Wconversion -Wshadow`,
  y las pruebas fallan al introducir errores a propósito.
- Referencia de estilo: `TypeShape`, `ContainerTraits`, `sameValue`.

### Etapa 2 — `feature/grid` ✅ completa

Secciones: §5.1, §5.7 (cuadrícula), §6.2, §8 (acceso y bordes).

Hecho:
- `grid.hpp`: `Grid<CellType, Rows, Columns>` sobre `std::array`, con `static_assert` contra cero
  filas o columnas, `contains`, `at` en const y no const (lanza `std::out_of_range`), `rows()`,
  `columns()`, `value_type` e iteradores const y no const (`begin`, `end`, `cbegin`, `cend`).
- En lugar de `operator()(fila, columna)` se usa `at(Position)`, la "operación equivalente" que
  admite §6.2 y la que usa el enunciado; el motivo está en `design.md`.
- Pruebas (`grid_test.cpp`) con tableros de 3 × 4 y de 2 × 2: dimensiones en tiempo de compilación,
  `contains`, tablero vacío al construir, acceso por posición, las cuatro esquinas, posiciones
  fuera del tablero, versión const, recorrido por filas, modificación mediante iteradores e
  iteradores const.
- Verificado: `ctest` 4/4, sin advertencias con `-Wall -Wextra -Wpedantic -Wconversion -Wshadow`,
  las pruebas fallan al invertir el índice a columnas y al aflojar `contains`, y un tablero de cero
  filas no compila.
- Referencia de estilo: `Matrix`, `FixedBuffer`.

### Etapa 3 — `feature/game-rules` ✅ completa

Secciones: §5.5 (perfiles), §5.1 (carga del tablero).

Hecho:
- `game_rules.hpp` / `src/game_rules.cpp`: `Difficulty`, `GameRules` con los diez valores
  configurables, `rulesFor()` con los perfiles `easy`, `standard` y `hard`, y `parseDifficulty()`,
  que devuelve `std::optional` y distingue mayúsculas.
- `scenario.hpp` / `src/scenario.cpp`: `Scenario<Rows, Columns>` (tablero y posición inicial),
  `parseScenario()` y `cellFromSymbol()`, que copia a cada celda los valores de las reglas.
- El costo de `wait` y el del intento inválido son dos campos distintos, con el mismo valor en los
  tres perfiles; §5.4 los trata por separado.
- Pruebas: los tres perfiles, el valor por defecto igual a `standard` y `parseDifficulty`
  (`interactions_test.cpp`); todos los símbolos, los valores según el perfil, el inicio en
  cualquier posición, siete formatos inválidos y un mapa con dos salidas (`grid_test.cpp`).
- Verificado: `ctest` 4/4, sin advertencias con `-Wall -Wextra -Wpedantic -Wconversion -Wshadow`,
  y las pruebas fallan al cambiar un valor de perfil, al aceptar mapas sin inicio y al construir
  una celda sin consultar las reglas.

### Etapa 4 — `feature/algorithms` ✅ completa

Secciones: §6.1, §6.4.

Hecho:
- `algorithms.hpp`: `countMatching` y `minimumBy` sobre rangos por iteradores, `findPosition` sobre
  el tablero, `appendEvents` (paquete variádico con fold expression) y `Overloaded` con su guía de
  deducción.
- En lugar de `findFirst` quedó `findPosition`, que devuelve la posición y no la celda, que es lo
  que necesita el entorno. `totalCost` y `anyTrue` se descartaron por no tener usuario.
- Pruebas (`tests/algorithms_test.cpp`, registrado como `AlgorithmsTest`): rangos vacíos,
  `std::vector`, `std::list` y los iteradores de `Grid`; empates en `minimumBy`; orden de
  `appendEvents` y paquete vacío; `Overloaded` con las siete alternativas de `Cell`.
- Verificado: `ctest` 5/5, sin advertencias con `-Wall -Wextra -Wpedantic -Wconversion -Wshadow`,
  y las pruebas fallan al invertir la condición de `countMatching`, al romper el desempate de
  `minimumBy` y al cambiar fila por columna en `findPosition`.
- Referencia de estilo: `countMatching`, `copyMatching`, `minimumBy`, `allTrue`, `makeContainer`,
  `Overloaded`.

### Etapa 5 — `feature/environment-core`

Secciones: §4, §5.3, §5.4, §5.7 (estado observable y entorno), §7.

- `events.hpp`: `Observation`, los seis eventos, `NavigationEvent`, `EndReason` y `StepResult`.
- `environment.hpp`: `NavigationEnvironment<Rows, Columns>`.
  - El constructor valida las precondiciones (una sola salida con `countMatching`, ubicación de la
    meta con `findFirst`) y lanza `std::invalid_argument`.
  - `state`, `availableActions`, `isFinished`, `grid` y `reset`.
  - `step()` sin efectos de celda todavía: turno, intento inválido, `wait`, costo de entrada,
    condiciones de término con precedencia (`goalReached` → `noEnergy` → `turnLimit`) y
    `std::logic_error` si la partida ya terminó.
- Pruebas (`environment_test.cpp`): movimiento libre, contra un muro y fuera del tablero; costos de
  movimiento normal, terreno elevado, `wait` e intento inválido; acciones disponibles junto a un
  muro, en una esquina y después del término; término por energía y por turnos; precedencia;
  precondiciones del constructor.

### Etapa 6 — `feature/cell-interactions`

Secciones: §5.5 (efectos y orden obligatorio), §6.4.

- Efectos con `std::visit` y `Overloaded`:
  - recurso: solo la primera vez;
  - batería: solo la primera vez, limitada a la energía máxima, aunque el costo de entrada haya
    dejado la energía en 0;
  - trampa: en cada entrada, con ambas penalizaciones;
  - salida: victoria solo si queda energía.
- Eventos en el mismo orden en que ocurren los cambios.
- Pruebas (`interactions_test.cpp`): cada caso anterior, llegada a la salida y procesamiento de cada
  tipo de evento.

### Etapa 7 — `feature/controllers`

Secciones: §5.6, §5.7 (controladores y políticas), §6.5, §8.

- `controllers.hpp`: `IController`, el concept `NavigationPolicy` y `PolicyController<Policy>`.
- Políticas: `RandomPolicy` (`std::mt19937` con semilla), `HeuristicPolicy` (reduce la distancia
  Manhattan, con desempate fijo) y `HumanPolicy` (acción pendiente que fija la interfaz).
  Implementación en `src/controllers.cpp`.
- Controladores guardados como `std::unique_ptr<IController>`.
- Pruebas (`controllers_test.cpp`): la política aleatoria elige acciones legales y repite la
  secuencia con la misma semilla; la heurística reduce la distancia; cambio de controlador en
  ejecución.
- Prueba negativa de compilación documentada en `design.md` (opcional: registrarla en CTest como
  fallo esperado).
- Referencia de estilo: `RuleSet` / `RuleModel`.

### Etapa 8 — `feature/scenarios`

Secciones: §5.1 (escenario 20 × 30), §10.2, §13.

- `assets/maps/scenario_01.txt` y `scenario_02.txt`: 20 × 30, borde de muros, al menos dos rutas
  parciales alternativas, legibles.
- La ruta a `assets/maps/` llega a la aplicación y a las pruebas desde CMake.
- Pruebas (`tests/scenarios_test.cpp`, nuevo): dimensiones, una sola salida, inicio válido y una
  secuencia de acciones fija que llega a la meta con `standard` (solución comprobada).

### Etapa 9 — `feature/console-ui`

Secciones: §5.8, §8 (pruebas de consola).

- `app/console_ui.{hpp,cpp}`: `UiCommand`, `RenderMode` y `ConsoleUI`.
  - `translate`: W, A, S, D, E, H, Q y flechas, sin distinguir mayúsculas.
  - `render`: barra de estado, reglas de coordenadas con keycaps, celdas de 2 columnas de ancho y
    pie con el último evento y la ayuda breve.
  - `help`: ayuda completa.
- `app/main.cpp`: `ScreenInteractive` con `CatchEvent`, y opciones `--difficulty`, `--ascii` y
  `--map`. Una tecla desconocida muestra un mensaje sin llamar a `step`.
- Pruebas (`tests/console_ui_test.cpp`): traducción de comandos válidos, rechazo de un comando
  desconocido sin modificar el entorno, símbolos emoji y ASCII de todas las celdas y del agente, y
  renderizado de 20 filas con 30 celdas cada una.

### Etapa 10 — `feature/auto-simulation`

Secciones: §7, §10.2 (simulación reproducible).

- Modo `--auto random|heuristic --seed N`: juega sin interfaz interactiva y muestra el resumen
  (completada o no, turnos, energía restante, recursos y puntaje).
- Prueba: la misma semilla sobre el mismo escenario produce el mismo resultado.

### Etapa 11 — `docs/delivery`

Secciones: §6.6, §10.2, §10.3.

- `README.md`: integrantes, requisitos, compilación, pruebas, ejecución, controles, perfiles con
  sus valores efectivos, ejemplos, elección de contenedores e instrucciones para PowerShell / cmd,
  Linux y macOS.
- `docs/design.md`: tabla completa de ubicación de los temas obligatorios.
- `docs/contributions.md`: responsabilidades y aportes.
- Ensayo desde un clon limpio siguiendo solo el README.
- Tag anotado `proyecto-1-entrega`: confirmar antes de crearlo.

## Técnicas nuevas respecto a las tareas de referencia

El enunciado exige estas técnicas, que no aparecen en `referencia/`: `std::span`, `std::optional`,
`std::visit` (con una lambda genérica `[](const auto& value)` y `decltype(value)`, desde la
Etapa 1), `<random>`, el `operator==` por defecto, los inicializadores designados
(`GameRules{.initialEnergy = 80, ...}`, desde la Etapa 3), `FetchContent` y el renderizado de FTXUI
dentro de una prueba. Todo lo demás se mantiene en el nivel de las tareas:
- los iteradores de `Grid` son los de `std::array`, como en `FixedBuffer`;
- los concepts usan `requires`, como `RuleFor`;
- el adaptador de políticas sigue el patrón de `RuleModel`.

## Ajustes respecto al plan original

- La conversión de texto a tablero pasó de la Etapa 2 a la 3, porque según la decisión 2 copia
  valores de `GameRules`.
- La conversión de texto a tablero no quedó en `cells.hpp` sino en `scenario.hpp`, porque necesita
  `Grid` y `GameRules` y además devuelve la posición inicial del agente.
- El rasgo de la especialización parcial es `collectible` en lugar de `consumable`. Las baterías
  también se consumen y cada efecto marca su propia celda, así que `consumable` no tendría uso;
  `collectible` permite contar los recursos del mapa (§6.3 exige usar el resultado).
