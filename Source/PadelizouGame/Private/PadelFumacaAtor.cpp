#include "PadelFumacaAtor.h"

#include "Components/StaticMeshComponent.h"
#include "Coordenadas.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

#include <vector>

DEFINE_LOG_CATEGORY_STATIC(LogFumaca, Log, All);

namespace
{
constexpr double PassoDaFumaca = 1.0 / 60.0;
constexpr double RaioDaBolaEmCm = padel::Bola::Raio * PadelCoordenadas::MetroEmCm;
} // namespace

APadelFumacaAtor::APadelFumacaAtor()
{
    PrimaryActorTick.bCanEverTick = true;

    Esfera = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Esfera"));
    SetRootComponent(Esfera);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Malha(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (Malha.Succeeded())
    {
        Esfera->SetStaticMesh(Malha.Object);
    }
    // A esfera básica tem 100 cm de diâmetro; o diâmetro real da bola é 6,7 cm. Aumentada 10x para ser vista de longe.
    Esfera->SetWorldScale3D(FVector(RaioDaBolaEmCm * 2.0 * 10.0 / 100.0));
    Esfera->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APadelFumacaAtor::BeginPlay()
{
    Super::BeginPlay();
    Lancar();
}

void APadelFumacaAtor::Lancar()
{
    Bola.Reiniciar();
    Bola.Posicionar(0.f, 9.f, 1.f);
    padel::Velocidade V;
    V.Vy = -10.f;   // rumo à rede (y = 0)
    V.Vz = 5.f;
    Bola.Lancar(V);
    Acumulado = 0.0;
    Tempo = 0.0;
    ProximoLog = 0.0;
    Quadros = 0;
    SetActorLocation(PadelCoordenadas::ParaUnreal(Bola.X, Bola.Y, Bola.Z));
}

void APadelFumacaAtor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    Acumulado += DeltaSeconds;
    std::vector<padel::EventoDaBola> Eventos;
    while (Acumulado >= PassoDaFumaca)
    {
        Acumulado -= PassoDaFumaca;
        Eventos.clear();
        Bola.Avancar(static_cast<float>(PassoDaFumaca), Eventos);
        Tempo += PassoDaFumaca;
        ++Quadros;
    }
    SetActorLocation(PadelCoordenadas::ParaUnreal(Bola.X, Bola.Y, Bola.Z));

    if (Tempo >= ProximoLog)
    {
        const FVector P = GetActorLocation();
        UE_LOG(LogFumaca, Display, TEXT("t=%.2f s, quadros=%d, UE(cm)=(%.1f, %.1f, %.1f), PadelSim(m)=(%.3f, %.3f, %.3f), EmJogo=%d"),
               Tempo, Quadros, P.X, P.Y, P.Z, Bola.X, Bola.Y, Bola.Z, Bola.EmJogo ? 1 : 0);
        ProximoLog += 0.5;
    }
    if (!Bola.EmJogo || Tempo > 6.0)
    {
        Lancar();   // repete o lançamento para a fumaça poder ser vista o quanto se quiser
    }
}
