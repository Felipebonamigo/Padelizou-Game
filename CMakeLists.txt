# O PadelSim fora do Unreal: compila a física pura e roda os testes contra o gabarito do Padel.Core.
# No Unreal, os mesmos arquivos de Source/PadelSim entram como módulo (PadelSim.Build.cs).
cmake_minimum_required(VERSION 3.20)
project(PadelSim CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(PadelSim STATIC
    Source/PadelSim/Private/Aleatorio.cpp
    Source/PadelSim/Private/Arbitro.cpp
    Source/PadelSim/Private/Bola.cpp
    Source/PadelSim/Private/Golpes.cpp
    Source/PadelSim/Private/GolpesEspeciais.cpp
    Source/PadelSim/Private/HumanoSimulado.cpp
    Source/PadelSim/Private/Jogadores.cpp
    Source/PadelSim/Private/Matematica.cpp
    Source/PadelSim/Private/Partida.cpp
    Source/PadelSim/Private/PassoFixo.cpp
    Source/PadelSim/Private/Placar.cpp
    Source/PadelSim/Private/Quadra.cpp)
target_include_directories(PadelSim PUBLIC Source/PadelSim/Public)
# Determinismo: nada de contrair a*b+c num FMA nem de matemática "rápida" — o resultado tem de bater com o Padel.Core.
if(MSVC)
    target_compile_options(PadelSim PRIVATE /fp:precise /W4 /WX)
else()
    target_compile_options(PadelSim PRIVATE -ffp-contract=off -fno-fast-math -Wall -Wextra -Werror)
endif()

enable_testing()
# Um executável por área, todos contra o gabarito do Padel.Core.
foreach(teste TestesDaMatematica TestesDoPadelSim TestesDosGolpesEspeciais TestesDaPartida TestesDoPassoFixo TestesDoPlacarEDoArbitro)
    add_executable(${teste} Testes/PadelSim/${teste}.cpp)
    target_link_libraries(${teste} PRIVATE PadelSim)
    target_compile_definitions(${teste} PRIVATE PASTA_DO_GABARITO="${CMAKE_CURRENT_SOURCE_DIR}/Testes/PadelSim/Gabarito"
                                              PASTA_DO_MOTOR="${CMAKE_CURRENT_SOURCE_DIR}/Source/PadelSim/Private")
    add_test(NAME ${teste} COMMAND ${teste})
endforeach()
