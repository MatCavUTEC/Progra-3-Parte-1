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

### Templates propios

`algorithms.hpp` reúne los templates que usa el motor, en lugar de repetir bucles en cada capa:

- `countMatching(first, last, predicate)`: cuenta elementos de cualquier rango. Se usa con los
  iteradores de `Grid` (una sola salida, total de recursos) y con contenedores de acciones o
  posiciones, que es la evidencia que pide §6.1 de un template usado con varios contenedores.
- `minimumBy(first, last, cost)`: devuelve el elemento de menor costo, o `std::nullopt` si el rango
  está vacío. Con costos iguales gana el primero, para que la política heurística sea reproducible.
- `findPosition(grid, predicate)`: recibe el tablero, no un par de iteradores, porque debe traducir
  el índice del recorrido a una `Position`; internamente usa los iteradores del tablero.
- `appendEvents(destino, eventos...)`: publica varios eventos con una fold expression sobre el
  operador coma.
- `Overloaded`: combina lambdas para `std::visit`.

Quedaron fuera `totalCost` y `anyTrue`, que el plan daba como alternativas: ningún punto del motor
los necesita y §6.1 pide que los templates se usen.

### Resolución de un turno

`step` sigue el orden obligatorio de §5.5: incrementar el turno; si la acción es inválida, cobrar
`rejectedMoveCost` y pasar al término; si es `wait`, cobrar `waitCost` sin activar ninguna celda;
si el movimiento es válido, actualizar la posición, cobrar el costo de entrada y aplicar el efecto
de la celda; y comprobar el término con la precedencia `goalReached`, `noEnergy`, `turnLimit`.

Tres detalles que el enunciado deja abiertos:

- **Orden de los eventos en un intento inválido.** §5.5 menciona el descuento antes del evento de
  rechazo. Aquí se emite `MovementRejectedEvent` y después `EnergyChangedEvent`, igual que un
  movimiento válido emite `MovedEvent` y luego su costo: primero qué pasó, después cuánto costó.
- **`EnergyChangedEvent` solo se emite si la energía cambió.** Los costos son configurables y
  pueden valer cero, y un evento que informa "de 60 a 60" no describe ningún cambio.
- **`MovedEvent::energyCost` informa el costo configurado de la celda**, no lo que se descontó
  después de recortar la energía a cero. Ese recorte se ve en `EnergyChangedEvent`.

La energía máxima de la observación es la energía inicial del perfil, como indica la tabla de §5.5,
y toda modificación se recorta al intervalo entre cero y ese máximo.

`availableActions` devuelve `wait` más las direcciones que producirían un desplazamiento válido, y
una lista vacía cuando la partida terminó: si `step` ya no se puede llamar, no hay acción legal.

### Efectos de las celdas

`applyCellEffect` visita la celda de destino con `std::visit` y `Overloaded`, sobre una referencia
modificable, porque el efecto marca la propia celda:

- recurso: la primera vez marca `collected`, suma su recompensa al puntaje, incrementa el contador
  de recursos y emite `ResourceCollectedEvent`;
- batería: la primera vez marca `consumed` y recarga con `changeEnergy`, que recorta al máximo;
- trampa: en cada entrada emite `TrapTriggeredEvent`, resta energía y resta puntaje, que puede
  quedar negativo;
- espacio libre, muro, terreno elevado y salida: sin efecto. El costo del terreno elevado ya se
  cobró al entrar, y la victoria se decide al comprobar el término.

Los cuatro casos sin efecto se escriben igual que los demás, en lugar de un `[](auto&) {}` que los
cubra a todos: así, agregar un tipo de celda produce un error de compilación en vez de un efecto
que falta en silencio.

### Controladores y políticas

§6.5 pide explicar cuatro cosas de este diseño:

- **Qué verifica el concept.** `NavigationPolicy<Policy>` exige que
  `policy.selectAction(observation, actions)` exista y devuelva exactamente `Action`, con la
  observación por referencia const y las acciones legales como `std::span<const Action>`.
- **Qué clase es genérica.** `PolicyController<Policy>`, que guarda la política por valor y la
  adapta a la interfaz virtual. Es el mismo patrón de `RuleSet` / `RuleModel` de la tarea 4.
- **Dónde ocurre el despacho dinámico.** En `IController::selectAction`, que la aplicación llama a
  través de un `std::unique_ptr<IController>`. Cambiar de controlador en ejecución es cambiar el
  puntero.
- **Por qué no hay condicionales por tipo.** El entorno nunca pregunta qué controlador lo maneja:
  recibe una `Action` y la resuelve igual venga de quien venga. No hay `typeid` ni `dynamic_cast`
  en ninguna capa.

`RandomPolicy` posee su propio `std::mt19937` con semilla, que es lo que hace reproducible una
simulación. `HeuristicPolicy` descarta `wait` y elige el movimiento legal que deja la menor
distancia Manhattan a la salida, con `minimumBy`; solo espera si no hay ningún movimiento posible.
No busca el camino óptimo y puede quedar oscilando frente a un callejón, que es lo que §5.6
admite. `HumanPolicy` guarda la acción que le entrega la interfaz y la consume al usarla: pedir dos
decisiones sin una nueva pulsación es un error de precondición.

### Prueba negativa de compilación

`tests/compile_fail/bad_policy.cpp` intenta adaptar una política cuyo `selectAction` devuelve
`void`. El objetivo `bad_policy` está fuera de la compilación normal (`EXCLUDE_FROM_ALL`) y CTest
lo construye en la prueba `BadPolicyDoesNotCompile`, que tiene `WILL_FAIL`: la prueba pasa
justamente porque no compila. El diagnóstico de GCC 15.2 empieza así:

```
error: template constraint failure for 'template<class Policy>
       requires NavigationPolicy<Policy> class PolicyController'
note: constraints not satisfied
note: required for the satisfaction of 'NavigationPolicy<Policy>' [with Policy = BadPolicy]
```

### Recursos y baterías consumidos

Se marcan con `collected` / `consumed` en lugar de reemplazar la celda por `Empty` (§5.7 admite
ambas opciones). Una celda marcada se comporta como espacio libre y se dibuja como tal.

### Mapas

Los escenarios de 20 × 30 están en `assets/maps/` como texto: un carácter por celda con la
convención ASCII de §5.8, y `@` para la posición inicial del agente (esa celda es espacio libre).
`app/` y las pruebas leen el archivo. La conversión de las líneas a `Grid` es una función pura del
motor que lanza `std::invalid_argument` si el formato no es válido.

`parseScenario` (`include/circuit_escape/scenario.hpp`) devuelve un `Scenario`: el tablero y la
posición inicial, que es lo que necesita el constructor del entorno. Vive en su propia cabecera
porque necesita `Grid` y `GameRules`, dependencias que `cells.hpp` no tiene por qué arrastrar.
Cada celda recibe sus valores desde las reglas recibidas, de acuerdo con "Valores de las reglas".

El parser valida solo el formato: cantidad de filas, longitud de cada fila, símbolos conocidos y
exactamente un `@`. Que exista una sola salida es precondición del entorno (§5.7), así que un mapa
con dos salidas se convierte sin error y lo rechaza `NavigationEnvironment`.

Pendiente para la interfaz (Etapa 9): la tabla de símbolos está hoy solo en `cellFromSymbol`. El
modo ASCII necesita el mapeo inverso, de celda a carácter, así que conviene compartir una única
tabla en lugar de escribir la correspondencia dos veces.

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
| Templates de función con iteradores (§6.1) | `countMatching` y `minimumBy` (rangos por iteradores) y `findPosition` (recibe el tablero) en `include/circuit_escape/algorithms.hpp` |
| Especialización total y parcial (§6.3) | `CellTraits<Wall>` (total) y `CellTraits<ResourceCell<Reward>>` (parcial) en `include/circuit_escape/cells.hpp`; usadas por `isTraversable` e `isCollectible` en `src/cells.cpp` |
| Paquete variádico y fold expression (§6.4) | `appendEvents` (fold sobre el operador coma) y `Overloaded` en `include/circuit_escape/algorithms.hpp`; `Overloaded` se usa con `std::visit` en `environment.hpp`, para el costo de entrada y los efectos de celda |
| Concept, interfaz virtual y adaptador genérico (§6.5) | `NavigationPolicy`, `IController` y `PolicyController` en `include/circuit_escape/controllers.hpp`; políticas en `src/controllers.cpp` |
| Biblioteca estándar (§6.6) | `std::array` en `grid.hpp`; `std::variant` y `std::visit` en `cells.hpp` y `environment.hpp`; `std::optional` en `position.hpp`, `algorithms.hpp` y `game_rules.hpp`; `std::vector` para eventos y acciones |
| Prueba negativa de compilación (§8) | `tests/compile_fail/bad_policy.cpp`, registrada en CTest como `BadPolicyDoesNotCompile` con `WILL_FAIL` |
