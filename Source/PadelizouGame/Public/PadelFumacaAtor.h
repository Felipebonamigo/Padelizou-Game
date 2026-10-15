#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PadelSim/Bola.h"
#include "PadelFumacaAtor.generated.h"

class UStaticMeshComponent;

// Fumaça da etapa 1: uma esfera que segue a padel::Bola a 60 quadros por segundo, lançada de (0, 9, 1) rumo à rede.
// Só serve para ver o eixo e a escala; a partida de verdade é o APadelPartidaAtor (pedido 0002).
UCLASS()
class PADELIZOUGAME_API APadelFumacaAtor : public AActor
{
    GENERATED_BODY()

public:
    APadelFumacaAtor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    void Lancar();

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Esfera;

    padel::Bola Bola;
    double Acumulado = 0.0;
    double Tempo = 0.0;
    double ProximoLog = 0.0;
    int32 Quadros = 0;
};
