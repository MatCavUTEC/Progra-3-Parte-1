// Prueba negativa de compilación (§8). Este archivo NO debe compilar: sirve para
// mostrar el diagnóstico cuando una política no satisface el concept
// NavigationPolicy. CTest lo construye con la propiedad WILL_FAIL.

#include <span>

#include "circuit_escape/controllers.hpp"

struct BadPolicy {
    // Devuelve void: el concept exige que selectAction devuelva Action
    void selectAction(const Observation&, std::span<const Action>) {}
};

int main() {
    PolicyController<BadPolicy> controller{BadPolicy{}};
    return 0;
}
