// As funções sem libm (padel::mat — seno, cosseno, atan2, exp, log, potência) contra o gabarito
// (Gabarito/matematica.csv, de GabaritoDaMatematica.cs): mesmos bits, entrada a entrada. NaN vale NaN (o bit do sinal
// e a carga do NaN não importam). E o portão: nenhum arquivo do motor chama a libm.
#include "PadelSim/Matematica.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#ifndef PASTA_DO_GABARITO
#error "defina PASTA_DO_GABARITO (o CMakeLists.txt define)"
#endif
#ifndef PASTA_DO_MOTOR
#error "defina PASTA_DO_MOTOR (o CMakeLists.txt define)"
#endif

namespace
{
float DeBits(uint32_t b)
{
    float f;
    std::memcpy(&f, &b, sizeof f);
    return f;
}
uint32_t Bits(float f)
{
    uint32_t b;
    std::memcpy(&b, &f, sizeof b);
    return b;
}
bool EhNaN(uint32_t b) { return (b & 0x7f800000u) == 0x7f800000u && (b & 0x007fffffu) != 0; }
} // namespace

int main()
{
    int falhas = 0, verificacoes = 0;
    std::ifstream arq(std::string(PASTA_DO_GABARITO) + "/matematica.csv");
    if (!arq) { std::printf("FALHA: não abriu matematica.csv\n"); return 1; }
    std::string s;
    bool cabecalho = true;
    while (std::getline(arq, s))
    {
        if (s.empty() || s[0] == '#') continue;
        if (cabecalho) { cabecalho = false; continue; }
        std::stringstream ss(s);
        std::string funcao, ta, tb, tr;
        std::getline(ss, funcao, ',');
        std::getline(ss, ta, ',');
        std::getline(ss, tb, ',');
        std::getline(ss, tr, ',');
        float a = DeBits(static_cast<uint32_t>(std::strtoul(ta.c_str(), nullptr, 10)));
        float b = DeBits(static_cast<uint32_t>(std::strtoul(tb.c_str(), nullptr, 10)));
        uint32_t esperado = static_cast<uint32_t>(std::strtoul(tr.c_str(), nullptr, 10));
        float r;
        if (funcao == "seno") r = padel::mat::Seno(a);
        else if (funcao == "cosseno") r = padel::mat::Cosseno(a);
        else if (funcao == "atan2") r = padel::mat::ArcoTangente2(a, b);
        else if (funcao == "exp") r = padel::mat::Exp(a);
        else if (funcao == "log") r = padel::mat::Log(a);
        else if (funcao == "potencia") r = padel::mat::Potencia(a, b);
        else if (funcao == "potenciaInteira") r = padel::mat::PotenciaInteira(a, static_cast<int>(b));
        else { std::printf("FALHA: função %s desconhecida\n", funcao.c_str()); return 1; }
        ++verificacoes;
        bool ok = Bits(r) == esperado || (EhNaN(Bits(r)) && EhNaN(esperado));
        if (!ok && ++falhas <= 20)
            std::printf("FALHA: %s(%.9g, %.9g) = %.9g (0x%08x), esperado %.9g (0x%08x)\n", funcao.c_str(), a, b, r, Bits(r), DeBits(esperado), esperado);
    }

    // O portão: o motor não chama a libm (o mesmo do Padel.Core: MatematicaDeterministicaTests).
    const char* arquivos[] = {"Aleatorio.cpp", "Arbitro.cpp", "Bola.cpp", "Golpes.cpp", "GolpesEspeciais.cpp", "HumanoSimulado.cpp",
                              "Jogadores.cpp", "Partida.cpp", "PassoFixo.cpp", "Placar.cpp", "Quadra.cpp"};
    const std::regex libm(R"(\b(std::)?(sin|cos|tan|asin|acos|atan|atan2|exp|exp2|log|log2|log10|pow|cbrt|sinh|cosh|tanh|fma|sinf|cosf|tanf|atanf|atan2f|expf|logf|powf|fmaf)\s*\()");
    for (const char* nome : arquivos)
    {
        std::ifstream f(std::string(PASTA_DO_MOTOR) + "/" + nome);
        if (!f) { std::printf("FALHA: não abriu %s\n", nome); ++falhas; continue; }
        std::string linha;
        int n = 0;
        while (std::getline(f, linha))
        {
            ++n;
            auto comentario = linha.find("//");
            std::string codigo = comentario == std::string::npos ? linha : linha.substr(0, comentario);
            ++verificacoes;
            if (std::regex_search(codigo, libm) && ++falhas <= 40) std::printf("FALHA: libm no motor, %s:%d: %s\n", nome, n, linha.c_str());
        }
    }
    std::printf("Matemática: %d verificações, %d falha(s).\n", verificacoes, falhas);
    return falhas == 0 ? 0 : 1;
}
