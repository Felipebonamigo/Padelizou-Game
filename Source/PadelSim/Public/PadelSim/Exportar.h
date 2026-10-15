#pragma once

// No Unreal, o UBT define PADELSIM_API como dllexport/dllimport do módulo PadelSim (para PadelizouGame e os testes
// enxergarem o motor) e o DLLEXPORT/DLLIMPORT vêm de HAL/Platform.h. Fora dele (CMake, CI) a macro some.
// Toda classe ou função livre nova do motor leva a macro.
#ifdef PADELSIM_API
#include "HAL/Platform.h"
#else
#define PADELSIM_API
#endif
