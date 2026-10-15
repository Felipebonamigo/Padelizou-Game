#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PadelHudWidget.generated.h"

class UBorder;
class UTextBlock;

// Placar de TV mínimo, montado em C++ (sem Blueprint): placar, mensagem da partida, log de eventos, modo de golpe e a
// tela de fim de partida.
UCLASS()
class PADELIZOUGAME_API UPadelHudWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;

    void Atualizar(const FText& Placar, const FText& Mensagem, const FText& Eventos, const FText& Modo);
    void MostrarFim(bool bMostrar, const FText& Resumo);
    /// O indicador de quando apertar (Manual): texto grande no centro de baixo; vazio esconde.
    void MostrarIndicador(const FText& Texto, const FLinearColor& Cor);

private:
    UPROPERTY(Transient) TObjectPtr<UTextBlock> TextoDoPlacar;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> TextoDaMensagem;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> TextoDosEventos;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> TextoDoModo;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> TextoDoFim;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> TextoDoIndicador;
    UPROPERTY(Transient) TObjectPtr<UBorder> TelaDoFim;
};
