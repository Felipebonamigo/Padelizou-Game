#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PadelControle.h"
#include "PadelSimulacao.h"
#include "PadelSim/HumanoSimulado.h"
#include "PadelPartidaAtor.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UMaterialInterface;
class UPadelHudWidget;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
struct FInputActionValue;

// A casca da partida (pedido 0002): dono de um padel::Partida e do relógio de 120 Hz (FPadelSimulacao). Desenha a
// quadra cinza, quatro cápsulas e a bola, põe a câmera de TV, lê o controle (Enhanced Input) e o placar. Nenhuma regra
// aqui: física, golpes e placar vêm do PadelSim.
UCLASS()
class PADELIZOUGAME_API APadelPartidaAtor : public AActor
{
    GENERATED_BODY()

public:
    APadelPartidaAtor();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Motivo) override;

    /// Multiplica o diâmetro da bola (6,7 cm de verdade). 1 = o real; 2,5 porque a 27 m da câmera a real tem ~3 px.
    UPROPERTY(EditAnywhere, Category = "Padel")
    float EscalaVisualDaBola = 2.5f;

    /// Quanto a câmera acompanha a bola de lado (0 = parada; 0,55 = o x da bola vezes 0,55). O pedido 0002 pedia o
    /// acompanhamento, mas no playtest "a quadra se mexendo" incomodou: parada por padrão.
    UPROPERTY(EditAnywhere, Category = "Padel")
    float SeguimentoLateralDaCamera = 0.f;

    /// Em vez do controle, quem joga o jogador 0 é o padel::HumanoSimulado (para provar a partida sem mãos).
    UPROPERTY(EditAnywhere, Category = "Padel")
    bool bHumanoSimulado = false;

private:
    // Construção
    UStaticMeshComponent* NovaForma(UStaticMesh* Malha, UMaterialInterface* Material, const FLinearColor& Cor, const FVector& Centro, const FVector& Escala, const FRotator& Giro = FRotator::ZeroRotator);
    void ConstruirQuadra();
    void ConstruirJogadores();
    void ConstruirBola();
    void PrepararEntrada(class APlayerController* PC);
    void IniciarPartida();

    // Por quadro
    void LerLinhaDeComando();
    void AtualizarJogadores();
    void AtualizarCamera(float DeltaSeconds);
    void AtualizarHud();
    void AtualizarIndicador();
    void AoEventoDaPartida(const padel::EventoDaPartida& Evento);
    void TratarCapturas();
    void RodarTesteDeEntrada(class APlayerController* PC);
    FText TextoDoPlacar() const;
    FText TextoDosEventos() const;

    // Entrada (Enhanced Input, criada em tempo de execução)
    UPROPERTY(Transient) TObjectPtr<UInputMappingContext> Contexto;
    UPROPERTY(Transient) TObjectPtr<UInputAction> AcaoMover;
    UPROPERTY(Transient) TObjectPtr<UInputAction> AcaoA;
    UPROPERTY(Transient) TObjectPtr<UInputAction> AcaoB;
    UPROPERTY(Transient) TObjectPtr<UInputAction> AcaoX;
    UPROPERTY(Transient) TObjectPtr<UInputAction> AcaoY;
    UPROPERTY(Transient) TObjectPtr<UInputAction> AcaoRB;
    UPROPERTY(Transient) TObjectPtr<UInputAction> AcaoJogarDeNovo;
    UPROPERTY(Transient) TObjectPtr<UInputAction> AcaoTrocarModo;
    UPROPERTY(Transient) TObjectPtr<UInputAction> AcaoTrocarAssistencia;
    UPROPERTY(Transient) TObjectPtr<UInputAction> AcaoTrocarVelocidade;
    FLeituraDoControle Leitura;      // o que está apertado agora
    bool bApertouA = false, bApertouB = false, bApertouX = false, bApertouY = false;   // arestas até o próximo quadro
    bool bPedidoDeNovaPartida = false;

    // Visual
    UPROPERTY(Transient) TObjectPtr<USceneComponent> Raiz;
    UPROPERTY(Transient) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(Transient) TObjectPtr<UStaticMesh> MalhaCubo;
    UPROPERTY(Transient) TObjectPtr<UStaticMesh> MalhaCilindro;
    UPROPERTY(Transient) TObjectPtr<UStaticMesh> MalhaEsfera;
    UPROPERTY(Transient) TObjectPtr<UStaticMesh> MalhaPlano;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> MaterialCor;
    UPROPERTY(Transient) TObjectPtr<UMaterialInterface> MaterialVidro;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Corpos;     // 4 cápsulas (time*2 + índice)
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Cabecas;   // meia esfera de cima da cápsula
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Bases;     // meia esfera de baixo
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Raquetes;
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Narizes;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Anel;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> BolaMalha;
    UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Marcador;   // o indicador de contato, no chão
    FString UltimoIndicador;
    UPROPERTY(Transient) TObjectPtr<UPadelHudWidget> Hud;
    double CameraY = 0.0;
    bool bCameraInicializada = false;
    bool bEntradaPronta = false;
    bool bQuadraConstruida = false;

    // Simulação
    TUniquePtr<FPadelSimulacao> Sim;
    padel::OpcoesDaPartida OpcoesAtuais;
    // Automático (a bola é batida sozinha, o jogador só anda e escolhe a direção) até o timing do Manual estar bom.
    padel::ModoDeGolpe ModoDaProximaPartida = padel::ModoDeGolpe::Automatico;
    padel::Assistencia AssistenciaAtual = padel::Assistencia::Casual;   // o Pro (timing de ±0 ms) é duro demais para começar
    int32 IndiceDaVelocidade = 0;   // 0 = 100 %, 1 = 50 %, 2 = 25 % (câmera lenta para analisar o jogo)
    TUniquePtr<padel::Aleatorio> AleatorioDoHumano;
    TUniquePtr<padel::HumanoSimulado> HumanoSim;
    int32 Semente = 0;

    // Eventos na tela
    struct FLinhaDeEvento { FString Texto; double Expira; };
    TArray<FLinhaDeEvento> Eventos;
    double RelogioDeTela = 0.0;
    FString UltimoPlacar, UltimaMensagem, UltimoEventos, UltimoModo;   // só atualiza o HUD quando o texto muda

    // Medição e capturas (linha de comando)
    bool bCapturas = false, bSairNoFim = false, bTesteDeEntrada = false;
    int32 EtapaDoTesteDeEntrada = 0;
    bool bCapturouRally = false, bCapturouFim = false, bCapturouIndicador = false;
    double FimDesde = -1.0;
    double SomaDeMs = 0.0, MaiorMs = 0.0;
    int64 QuadrosMedidos = 0, QuadrosVistos = 0;
    FString NomeDoModoParaArquivo;
};
