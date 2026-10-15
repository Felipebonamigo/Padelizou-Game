#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PadelGameMode.generated.h"

// Sem pawn: a câmera é a do APadelPartidaAtor e o controle é lido por ele.
UCLASS()
class PADELIZOUGAME_API APadelGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    APadelGameMode();
};
