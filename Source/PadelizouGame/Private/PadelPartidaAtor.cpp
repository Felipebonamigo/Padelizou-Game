#include "PadelPartidaAtor.h"

#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Coordenadas.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputKeyEventArgs.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "PadelHudWidget.h"
#include "PadelPaineis.h"
#include "PadelSim/Quadra.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogPadel, Log, All);

namespace
{
constexpr double M = PadelCoordenadas::MetroEmCm;

const TCHAR* NomeDoGolpe(padel::TipoDeGolpe T)
{
    switch (T)
    {
    case padel::TipoDeGolpe::Saque: return TEXT("saque");
    case padel::TipoDeGolpe::Normal: return TEXT("drive");
    case padel::TipoDeGolpe::Ataque: return TEXT("ataque");
    case padel::TipoDeGolpe::Defesa: return TEXT("defesa");
    case padel::TipoDeGolpe::Lob: return TEXT("lob");
    case padel::TipoDeGolpe::Smash: return TEXT("smash");
    case padel::TipoDeGolpe::Bandeja: return TEXT("bandeja");
    case padel::TipoDeGolpe::Vibora: return TEXT("víbora");
    case padel::TipoDeGolpe::Erro: return TEXT("erro");
    case padel::TipoDeGolpe::Chiquita: return TEXT("chiquita");
    case padel::TipoDeGolpe::Contrapared: return TEXT("contrapared");
    case padel::TipoDeGolpe::SmashPor3: return TEXT("smash por 3");
    case padel::TipoDeGolpe::SmashPor4: return TEXT("smash por 4");
    }
    return TEXT("?");
}

const TCHAR* NomeDaAssistencia(padel::Assistencia A)
{
    switch (A)
    {
    case padel::Assistencia::Pro: return TEXT("Pro");
    case padel::Assistencia::Padrao: return TEXT("Padrão");
    case padel::Assistencia::Casual: return TEXT("Casual");
    }
    return TEXT("?");
}

FString Utf8(const std::string& S) { return FString(UTF8_TO_TCHAR(S.c_str())); }
FString Utf8(const char* S) { return FString(UTF8_TO_TCHAR(S)); }

const FLinearColor CorDoTime0(0.08f, 0.30f, 0.95f);
const FLinearColor CorDoTime1(0.95f, 0.18f, 0.12f);
} // namespace

APadelPartidaAtor::APadelPartidaAtor()
{
    PrimaryActorTick.bCanEverTick = true;
    Raiz = CreateDefaultSubobject<USceneComponent>(TEXT("Raiz"));
    SetRootComponent(Raiz);
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraDeTV"));
    Camera->SetupAttachment(Raiz);
    // Lente equivalente a 35 mm: FOV horizontal de ~54°.
    Camera->SetFieldOfView(54.f);
}

UStaticMeshComponent* APadelPartidaAtor::NovaForma(UStaticMesh* Malha, UMaterialInterface* Material, const FLinearColor& Cor,
                                                   const FVector& Centro, const FVector& Escala, const FRotator& Giro)
{
    UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
    C->SetStaticMesh(Malha);
    C->SetMobility(EComponentMobility::Movable);
    C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    C->SetupAttachment(Raiz);
    C->RegisterComponent();
    C->SetRelativeLocation(Centro);
    C->SetRelativeRotation(Giro);
    C->SetRelativeScale3D(Escala);
    if (Material)
    {
        UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Material, this);
        Mid->SetVectorParameterValue(TEXT("Cor"), Cor);
        C->SetMaterial(0, Mid);
    }
    return C;
}

void APadelPartidaAtor::ConstruirQuadra()
{
    const FLinearColor Chao(0.16f, 0.17f, 0.19f), Linha(0.85f, 0.85f, 0.85f), Rede(0.04f, 0.04f, 0.04f), Grade(0.07f, 0.07f, 0.08f);
    NovaForma(MalhaPlano, MaterialCor, Chao, FVector::ZeroVector, FVector(padel::quadra::Comprimento, padel::quadra::Largura, 1.0));

    // Linhas: 5 cm de largura, coladas ao chão. Faixa(X, Y, comprimento em X, comprimento em Y), tudo em cm do Unreal.
    const double L = 5.0;
    auto Faixa = [&](double X, double Y, double CompX, double CompY)
    {
        NovaForma(MalhaCubo, MaterialCor, Linha, FVector(X, Y, 0.3), FVector(CompX / 100.0, CompY / 100.0, 0.005));
    };
    const double Meio = padel::quadra::MeioComprimento * M, Metade = padel::quadra::MeiaLargura * M;
    const double Saque = padel::quadra::LinhaDeSaque * M;
    Faixa(Meio - L / 2, 0, L, 2 * Metade);    // fundo da casa
    Faixa(-Meio + L / 2, 0, L, 2 * Metade);   // fundo do visitante
    Faixa(0, Metade - L / 2, 2 * Meio, L);    // laterais
    Faixa(0, -Metade + L / 2, 2 * Meio, L);
    Faixa(Saque, 0, L, 2 * Metade);           // linhas de saque (6,95 m da rede)
    Faixa(-Saque, 0, L, 2 * Metade);
    Faixa(0, 0, 2 * Saque, L);                // linha central entre as de saque

    // Rede (0,90 m).
    NovaForma(MalhaCubo, MaterialCor, Rede, FVector(0, 0, padel::quadra::AlturaDaRede * M * 0.5),
              FVector(0.02, 2 * Metade / 100.0, padel::quadra::AlturaDaRede));

    // Paredes: tudo de quadra::Paineis(), vidro semitransparente e grade cinza escuro.
    int Vidros = 0, Grades = 0;
    for (const padel::quadra::Painel& P : padel::quadra::Paineis())
    {
        const FPadelCaixa C = PadelPaineis::CaixaDoPainel(P);
        const bool bVidro = P.Material == padel::Superficie::Vidro;
        (bVidro ? Vidros : Grades)++;
        NovaForma(MalhaCubo, bVidro ? MaterialVidro : MaterialCor, bVidro ? FLinearColor(0.55f, 0.65f, 0.7f) : Grade, C.Centro,
                  C.MeiaExtensao * 2.0 / 100.0);
    }
    UE_LOG(LogPadel, Log, TEXT("Quadra: %d painéis de vidro e %d de grade, desenhados de quadra::Paineis()"), Vidros, Grades);
}

void APadelPartidaAtor::ConstruirJogadores()
{
    for (int32 I = 0; I < 4; ++I)
    {
        const FLinearColor Cor = (I / 2 == 0) ? CorDoTime0 : CorDoTime1;
        Corpos.Add(NovaForma(MalhaCilindro, MaterialCor, Cor, FVector(0, 0, 67.5), FVector(0.5, 0.5, 1.35)));
        Cabecas.Add(NovaForma(MalhaEsfera, MaterialCor, Cor * 1.3f, FVector(0, 0, 157.5), FVector(0.45, 0.45, 0.45)));
        Narizes.Add(NovaForma(MalhaCubo, MaterialCor, FLinearColor::White, FVector(0, 0, 155), FVector(0.14, 0.10, 0.10)));
        Raquetes.Add(NovaForma(MalhaCilindro, MaterialCor, FLinearColor(0.95f, 0.95f, 0.95f), FVector(0, 0, 100), FVector(0.34, 0.34, 0.02)));
    }
    // Anel no chão do jogador local.
    Anel = NovaForma(MalhaCilindro, MaterialCor, FLinearColor(1.f, 0.85f, 0.1f), FVector(0, 0, 1), FVector(1.1, 1.1, 0.02));
}

void APadelPartidaAtor::ConstruirBola()
{
    const double D = padel::Bola::Raio * 2.0 * EscalaVisualDaBola;   // diâmetro em m == escala da esfera de 1 m
    BolaMalha = NovaForma(MalhaEsfera, MaterialCor, FLinearColor(1.f, 0.9f, 0.05f), FVector(0, 0, 100), FVector(D, D, D));
}

void APadelPartidaAtor::LerLinhaDeComando()
{
    const TCHAR* Linha = FCommandLine::Get();
    bHumanoSimulado = bHumanoSimulado || FParse::Param(Linha, TEXT("PadelHumanoSimulado"));
    bCapturas = FParse::Param(Linha, TEXT("PadelCapturas"));
    bSairNoFim = FParse::Param(Linha, TEXT("PadelSairNoFim"));
    bTesteDeEntrada = FParse::Param(Linha, TEXT("PadelTesteDeEntrada"));
    FString Modo;
    if (FParse::Value(Linha, TEXT("PadelModo="), Modo) && Modo.Equals(TEXT("auto"), ESearchCase::IgnoreCase))
    {
        ModoDaProximaPartida = padel::ModoDeGolpe::Automatico;
    }
    FParse::Value(Linha, TEXT("PadelSemente="), Semente);
    FString Assist;
    if (FParse::Value(Linha, TEXT("PadelAssistencia="), Assist))
    {
        if (Assist.Equals(TEXT("padrao"), ESearchCase::IgnoreCase)) AssistenciaAtual = padel::Assistencia::Padrao;
        else if (Assist.Equals(TEXT("casual"), ESearchCase::IgnoreCase)) AssistenciaAtual = padel::Assistencia::Casual;
    }
}

void APadelPartidaAtor::IniciarPartida()
{
    OpcoesAtuais = padel::OpcoesDaPartida{};
    OpcoesAtuais.Dificuldade = padel::Dificuldade::Medio;
    // Formato curto do design: set até 4 games e super tie-break a 10.
    OpcoesAtuais.GamesPorSet = 4;
    OpcoesAtuais.PontosDoTieBreak = 10;
    OpcoesAtuais.Humanos = {true, false, false, false};
    OpcoesAtuais.ModoDeGolpe = ModoDaProximaPartida;
    // Manual = quatro botões; o Automático só existe com dois botões (a Partida recusa a outra combinação).
    OpcoesAtuais.Controle = ModoDaProximaPartida == padel::ModoDeGolpe::Manual ? padel::Controle::QuatroBotoes : padel::Controle::DoisBotoes;
    OpcoesAtuais.Assistencia = AssistenciaAtual;
    if (Semente != 0) OpcoesAtuais.Semente = static_cast<uint32_t>(Semente);

    Sim = MakeUnique<FPadelSimulacao>(OpcoesAtuais);
    Sim->Partida().AoEvento = [this](const padel::EventoDaPartida& E) { AoEventoDaPartida(E); };

    HumanoSim.Reset();
    if (bHumanoSimulado)
    {
        AleatorioDoHumano = MakeUnique<padel::Aleatorio>(Semente != 0 ? static_cast<uint32_t>(Semente) + 7u : 7u);
        HumanoSim = MakeUnique<padel::HumanoSimulado>(0, padel::PerfilDeHumano::Avancado, *AleatorioDoHumano, true, OpcoesAtuais.Controle);
    }

    Eventos.Reset();
    bCapturouRally = bCapturouFim = false;
    FimDesde = -1.0;
    NomeDoModoParaArquivo = ModoDaProximaPartida == padel::ModoDeGolpe::Manual ? TEXT("manual") : TEXT("automatico");
    if (Hud) Hud->MostrarFim(false, FText::GetEmpty());
    UE_LOG(LogPadel, Display, TEXT("PADEL_INICIO modo=%s assistencia=%s humano_simulado=%d semente=%d"), *NomeDoModoParaArquivo,
           NomeDaAssistencia(AssistenciaAtual), bHumanoSimulado ? 1 : 0, Semente);
}

void APadelPartidaAtor::PrepararEntrada(APlayerController* PC)
{
    Contexto = NewObject<UInputMappingContext>(this);
    auto NovaAcao = [this](EInputActionValueType Tipo)
    {
        UInputAction* A = NewObject<UInputAction>(this);
        A->ValueType = Tipo;
        return A;
    };
    AcaoMover = NovaAcao(EInputActionValueType::Axis2D);
    AcaoA = NovaAcao(EInputActionValueType::Boolean);
    AcaoB = NovaAcao(EInputActionValueType::Boolean);
    AcaoX = NovaAcao(EInputActionValueType::Boolean);
    AcaoY = NovaAcao(EInputActionValueType::Boolean);
    AcaoRB = NovaAcao(EInputActionValueType::Boolean);
    AcaoJogarDeNovo = NovaAcao(EInputActionValueType::Boolean);
    AcaoTrocarModo = NovaAcao(EInputActionValueType::Boolean);
    AcaoTrocarAssistencia = NovaAcao(EInputActionValueType::Boolean);

    // Mover: analógico esquerdo / WASD. X = direita do jogador, Y = rumo à rede (W).
    Contexto->MapKey(AcaoMover, EKeys::Gamepad_Left2D);
    Contexto->MapKey(AcaoMover, EKeys::D);
    Contexto->MapKey(AcaoMover, EKeys::A).Modifiers.Add(NewObject<UInputModifierNegate>(Contexto));
    {
        UInputModifierSwizzleAxis* TrocaW = NewObject<UInputModifierSwizzleAxis>(Contexto);
        TrocaW->Order = EInputAxisSwizzle::YXZ;
        Contexto->MapKey(AcaoMover, EKeys::W).Modifiers.Add(TrocaW);

        UInputModifierSwizzleAxis* TrocaS = NewObject<UInputModifierSwizzleAxis>(Contexto);
        TrocaS->Order = EInputAxisSwizzle::YXZ;
        FEnhancedActionKeyMapping& S = Contexto->MapKey(AcaoMover, EKeys::S);
        S.Modifiers.Add(TrocaS);
        S.Modifiers.Add(NewObject<UInputModifierNegate>(Contexto));
    }
    Contexto->MapKey(AcaoA, EKeys::SpaceBar);   // A
    Contexto->MapKey(AcaoA, EKeys::Gamepad_FaceButton_Bottom);
    Contexto->MapKey(AcaoB, EKeys::LeftShift);   // B (lob)
    Contexto->MapKey(AcaoB, EKeys::Gamepad_FaceButton_Right);
    Contexto->MapKey(AcaoX, EKeys::Q);   // X (cortado)
    Contexto->MapKey(AcaoX, EKeys::Gamepad_FaceButton_Left);
    Contexto->MapKey(AcaoY, EKeys::E);   // Y (ataque)
    Contexto->MapKey(AcaoY, EKeys::Gamepad_FaceButton_Top);
    Contexto->MapKey(AcaoRB, EKeys::LeftControl);   // RB (modificador)
    Contexto->MapKey(AcaoRB, EKeys::Gamepad_RightShoulder);
    Contexto->MapKey(AcaoJogarDeNovo, EKeys::R);
    Contexto->MapKey(AcaoJogarDeNovo, EKeys::Gamepad_Special_Right);
    Contexto->MapKey(AcaoTrocarModo, EKeys::F2);
    Contexto->MapKey(AcaoTrocarAssistencia, EKeys::F3);

    if (ULocalPlayer* LP = PC->GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            Sub->AddMappingContext(Contexto, 0);
        }
    }

    EnableInput(PC);
    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
    {
        EIC->BindActionValueLambda(AcaoMover, ETriggerEvent::Triggered, [this](const FInputActionValue& V) { Leitura.Mover = V.Get<FVector2D>(); });
        EIC->BindActionValueLambda(AcaoMover, ETriggerEvent::Completed, [this](const FInputActionValue&) { Leitura.Mover = FVector2D::ZeroVector; });
        EIC->BindActionValueLambda(AcaoMover, ETriggerEvent::Canceled, [this](const FInputActionValue&) { Leitura.Mover = FVector2D::ZeroVector; });

        EIC->BindActionValueLambda(AcaoA, ETriggerEvent::Started, [this](const FInputActionValue&) { bApertouA = true; Leitura.AcaoSegurada = true; });
        EIC->BindActionValueLambda(AcaoA, ETriggerEvent::Completed, [this](const FInputActionValue&) { Leitura.AcaoSegurada = false; });
        EIC->BindActionValueLambda(AcaoB, ETriggerEvent::Started, [this](const FInputActionValue&) { bApertouB = true; });
        EIC->BindActionValueLambda(AcaoX, ETriggerEvent::Started, [this](const FInputActionValue&) { bApertouX = true; });
        EIC->BindActionValueLambda(AcaoY, ETriggerEvent::Started, [this](const FInputActionValue&) { bApertouY = true; Leitura.AtaqueSegurado = true; });
        EIC->BindActionValueLambda(AcaoY, ETriggerEvent::Completed, [this](const FInputActionValue&) { Leitura.AtaqueSegurado = false; });
        EIC->BindActionValueLambda(AcaoRB, ETriggerEvent::Started, [this](const FInputActionValue&) { Leitura.ModificadorSegurado = true; });
        EIC->BindActionValueLambda(AcaoRB, ETriggerEvent::Completed, [this](const FInputActionValue&) { Leitura.ModificadorSegurado = false; });
        EIC->BindActionValueLambda(AcaoJogarDeNovo, ETriggerEvent::Started, [this](const FInputActionValue&) { bPedidoDeNovaPartida = true; });
        EIC->BindActionValueLambda(AcaoTrocarModo, ETriggerEvent::Started, [this](const FInputActionValue&)
        {
            // O modo é das opções da Partida, que são const: vale na próxima partida.
            ModoDaProximaPartida = ModoDaProximaPartida == padel::ModoDeGolpe::Manual ? padel::ModoDeGolpe::Automatico : padel::ModoDeGolpe::Manual;
        });
        EIC->BindActionValueLambda(AcaoTrocarAssistencia, ETriggerEvent::Started, [this](const FInputActionValue&)
        {
            // Assistência muda a qualquer hora, direto no jogador.
            AssistenciaAtual = static_cast<padel::Assistencia>((static_cast<int>(AssistenciaAtual) + 1) % 3);
            if (Sim) Sim->Partida().Jogadores[0].Assistencia = AssistenciaAtual;
        });
    }
    else
    {
        UE_LOG(LogPadel, Warning, TEXT("InputComponent não é um UEnhancedInputComponent: sem controle"));
    }
}

void APadelPartidaAtor::BeginPlay()
{
    Super::BeginPlay();
    MalhaCubo = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    MalhaCilindro = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    MalhaEsfera = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    MalhaPlano = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
    MaterialCor = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiais/M_Cor.M_Cor"));
    MaterialVidro = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiais/M_Vidro.M_Vidro"));

    LerLinhaDeComando();
    ConstruirQuadra();
    ConstruirJogadores();
    ConstruirBola();
    IniciarPartida();
}

void APadelPartidaAtor::EndPlay(const EEndPlayReason::Type Motivo)
{
    if (QuadrosMedidos > 0)
    {
        UE_LOG(LogPadel, Display, TEXT("PADEL_MEDIDA tick do ator: %lld quadros, média %.3f ms, máximo %.3f ms"), QuadrosMedidos,
               SomaDeMs / static_cast<double>(QuadrosMedidos), MaiorMs);
    }
    Super::EndPlay(Motivo);
}

void APadelPartidaAtor::AoEventoDaPartida(const padel::EventoDaPartida& E)
{
    using T = padel::TipoDeEventoDaPartida;
    FString Texto;
    switch (E.Tipo)
    {
    case T::Golpe:
        Texto = FString::Printf(TEXT("Jogador %d: %s"), E.Jogador, E.Golpe ? NomeDoGolpe(*E.Golpe) : TEXT("golpe"));
        break;
    case T::Quique: Texto = TEXT("Quique"); break;
    case T::Parede: Texto = TEXT("Parede"); break;
    case T::Ponto:
        Texto = FString::Printf(TEXT("PONTO time %d%s%s"), E.Time, E.Motivo ? TEXT(": ") : TEXT(""),
                                E.Motivo ? *Utf8(padel::Partida::TextoDoMotivo(*E.Motivo)) : TEXT(""));
        UE_LOG(LogPadel, Display, TEXT("PADEL_EVENTO %s | %s"), *Texto, *UltimoPlacar.Replace(TEXT("\n"), TEXT(" / ")));
        break;
    case T::Falta: Texto = TEXT("Falta"); break;
    case T::Let: Texto = TEXT("Let"); break;
    case T::Game:
        Texto = FString::Printf(TEXT("GAME time %d"), E.Time);
        UE_LOG(LogPadel, Display, TEXT("PADEL_EVENTO %s"), *Texto);
        break;
    case T::Set:
        Texto = FString::Printf(TEXT("SET time %d"), E.Time);
        UE_LOG(LogPadel, Display, TEXT("PADEL_EVENTO %s"), *Texto);
        break;
    case T::Partida:
        Texto = FString::Printf(TEXT("PARTIDA time %d"), E.Time);
        UE_LOG(LogPadel, Display, TEXT("PADEL_EVENTO %s"), *Texto);
        break;
    default: return;
    }
    Eventos.Add({Texto, RelogioDeTela + 2.0});   // fica na tela por 2 s
    if (Eventos.Num() > 8) Eventos.RemoveAt(0);
}

FText APadelPartidaAtor::TextoDoPlacar() const
{
    const padel::Placar& P = Sim->Partida().Placar;
    const int Sacando = P.SacadorAtual().Time;
    auto Linha = [&](int T, const TCHAR* Nome)
    {
        return FString::Printf(TEXT("%s %s    SETS %d   GAMES %d   %s"), Sacando == T ? TEXT("●") : TEXT(" "), Nome, P.Sets[T], P.Games[T],
                               *Utf8(P.TextoDosPontos(T)));
    };
    FString S = Linha(0, TEXT("CASA ")) + TEXT("\n") + Linha(1, TEXT("RIVAL"));
    if (P.EmTieBreak) S += TEXT("\nSUPER TIE-BREAK");
    return FText::FromString(S);
}

FText APadelPartidaAtor::TextoDosEventos() const
{
    FString S;
    for (const FLinhaDeEvento& L : Eventos)
    {
        if (!S.IsEmpty()) S += TEXT("\n");
        S += L.Texto;
    }
    return FText::FromString(S);
}

void APadelPartidaAtor::AtualizarHud()
{
    if (!Hud || !Sim) return;
    const padel::Partida& Pa = Sim->Partida();
    const FString Placar = TextoDoPlacar().ToString();
    const FString Mensagem = Pa.Mensagem ? Utf8(Pa.Mensagem->Texto) : FString();
    const FString Evs = TextoDosEventos().ToString();
    const bool bManual = Pa.Opcoes.ModoDeGolpe == padel::ModoDeGolpe::Manual;
    const FString Modo = FString::Printf(
        TEXT("Modo de golpe: %s | próxima partida: %s (F2) | Assistência: %s (F3) | R / Start: jogar de novo | A/Espaço  B/Shift  X/Q  Y/E  RB/Ctrl  WASD"),
        bManual ? TEXT("Manual (4 botões)") : TEXT("Automático (2 botões)"),
        ModoDaProximaPartida == padel::ModoDeGolpe::Manual ? TEXT("Manual") : TEXT("Automático"),
        NomeDaAssistencia(Pa.Jogadores[0].Assistencia));
    if (Placar != UltimoPlacar || Mensagem != UltimaMensagem || Evs != UltimoEventos || Modo != UltimoModo)
    {
        UltimoPlacar = Placar;
        UltimaMensagem = Mensagem;
        UltimoEventos = Evs;
        UltimoModo = Modo;
        Hud->Atualizar(FText::FromString(Placar), FText::FromString(Mensagem), FText::FromString(Evs), FText::FromString(Modo));
    }
}

void APadelPartidaAtor::AtualizarJogadores()
{
    const padel::Partida& Pa = Sim->Partida();
    for (int32 I = 0; I < 4; ++I)
    {
        const padel::Jogador& J = Pa.Jogadores[I];
        const FVector P = PadelCoordenadas::ParaUnreal(J.X, J.Y, 0.f);
        const float Frente = J.Time == 0 ? 180.f : 0.f;   // o time da casa (y > 0) olha para -X do Unreal, a rede
        const FVector F = FRotator(0.f, Frente, 0.f).RotateVector(FVector::XAxisVector);
        Corpos[I]->SetRelativeLocation(P + FVector(0, 0, 67.5));
        Cabecas[I]->SetRelativeLocation(P + FVector(0, 0, 157.5));
        Narizes[I]->SetRelativeLocationAndRotation(P + F * 22.0 + FVector(0, 0, 155), FRotator(0.f, Frente, 0.f));
        // Raquete: um disco na mão; no balanço varre de trás para a frente do lado da raquete.
        const float Fase = J.Balancando() ? FMath::Clamp(J.TempoNoBalanco() / padel::Jogador::DuracaoDoBalanco, 0.f, 1.f) : -1.f;
        const float Psi = (Fase < 0.f ? 90.f : 120.f - 100.f * Fase) * static_cast<float>(J.LadoDaRaquete());
        const FVector Dir = FRotator(0.f, Frente + Psi, 0.f).RotateVector(FVector::XAxisVector);
        Raquetes[I]->SetRelativeLocationAndRotation(P + Dir * 55.0 + FVector(0, 0, 100), FRotator(90.f, Frente + Psi, 0.f));
        if (I == 0 && Anel) Anel->SetRelativeLocation(P + FVector(0, 0, 1));
    }
    if (BolaMalha)
    {
        const padel::Bola& B = Pa.Bola;
        BolaMalha->SetRelativeLocation(PadelCoordenadas::ParaUnreal(B.X, B.Y, B.Z));
    }
}

void APadelPartidaAtor::AtualizarCamera(float DeltaSeconds)
{
    // Atrás do time da casa (y > 0): 17,5 m de recuo da linha de fundo e 7,5 m de altura, olhando o centro da quadra.
    // Segue a bola só no eixo lateral, com atraso suave.
    const double X = (padel::quadra::MeioComprimento + 17.5) * M;
    const double Z = 7.5 * M;
    const double BolaY = PadelCoordenadas::ParaUnreal(Sim->Partida().Bola.X, 0.f, 0.f).Y;
    const double Alvo = BolaY * 0.55;
    if (!bCameraInicializada)
    {
        CameraY = Alvo;
        bCameraInicializada = true;
    }
    constexpr double TempoDeAtraso = 0.45;
    CameraY += (Alvo - CameraY) * (1.0 - FMath::Exp(-static_cast<double>(DeltaSeconds) / TempoDeAtraso));
    const FVector Pos(X, CameraY, Z);
    const FRotator Olhar = (FVector::ZeroVector - Pos).Rotation();
    Camera->SetWorldLocationAndRotation(Pos, Olhar);
}

void APadelPartidaAtor::TratarCapturas()
{
    if (!Sim) return;
    const padel::Partida& Pa = Sim->Partida();
    auto Capturar = [this](const TCHAR* Nome)
    {
        const FString Arquivo = FPaths::ConvertRelativePathToFull(
            FPaths::ProjectDir() / TEXT("docs/imagens") / FString::Printf(TEXT("%s-%s.png"), Nome, *NomeDoModoParaArquivo));
        FScreenshotRequest::RequestScreenshot(Arquivo, true, false);
        UE_LOG(LogPadel, Display, TEXT("PADEL_CAPTURA %s"), *Arquivo);
    };
    if (bCapturas && !bCapturouRally && Pa.Estado == padel::EstadoDaPartida::Rally && Pa.RallyAtual >= 5)
    {
        bCapturouRally = true;
        Capturar(TEXT("rally"));
    }
    if (Pa.Acabou())
    {
        if (FimDesde < 0.0)
        {
            FimDesde = RelogioDeTela;
            if (Hud) Hud->MostrarFim(true, FText::FromString(Utf8(Pa.Placar.Resumo()) + TEXT("\n\nR / Start: jogar de novo")));
            UE_LOG(LogPadel, Display, TEXT("PADEL_FIM placar: %s | tempo de jogo %.1f s"),
                   *Utf8(Pa.Placar.Resumo()).Replace(TEXT("\n"), TEXT(" / ")), Pa.TempoDeJogo);
        }
        if (bCapturas && !bCapturouFim && RelogioDeTela - FimDesde > 1.0)
        {
            bCapturouFim = true;
            Capturar(TEXT("fim"));
        }
        if (bSairNoFim && RelogioDeTela - FimDesde > 3.0)
        {
            if (APlayerController* PC = GetWorld()->GetFirstPlayerController()) PC->ConsoleCommand(TEXT("quit"));
        }
    }
}


void APadelPartidaAtor::RodarTesteDeEntrada(APlayerController* PC)
{
    // Injeta teclas no PlayerInput (o mesmo caminho do teclado de verdade) e deixa o log dizer o que o Enhanced Input
    // entregou. Roteiro: W+D, solta; Espaço (A), solta; Shift (B), solta; E (Y) segura e solta; Ctrl (RB); Q (X).
    struct FPasso { double Em; FKey Tecla; EInputEvent Evento; };
    static const FPasso Roteiro[] = {
        {1.0, EKeys::W, IE_Pressed}, {1.0, EKeys::D, IE_Pressed}, {1.4, EKeys::W, IE_Released}, {1.4, EKeys::D, IE_Released},
        {1.8, EKeys::SpaceBar, IE_Pressed}, {2.0, EKeys::SpaceBar, IE_Released},
        {2.4, EKeys::LeftShift, IE_Pressed}, {2.5, EKeys::LeftShift, IE_Released},
        {2.8, EKeys::E, IE_Pressed}, {3.2, EKeys::E, IE_Released},
        {3.6, EKeys::LeftControl, IE_Pressed}, {3.9, EKeys::LeftControl, IE_Released},
        {4.2, EKeys::Q, IE_Pressed}, {4.3, EKeys::Q, IE_Released},
    };
    if (!PC || !PC->PlayerInput) return;
    while (EtapaDoTesteDeEntrada < UE_ARRAY_COUNT(Roteiro) && RelogioDeTela >= Roteiro[EtapaDoTesteDeEntrada].Em)
    {
        const FPasso& P = Roteiro[EtapaDoTesteDeEntrada++];
        PC->PlayerInput->InputKey(FInputKeyEventArgs(nullptr, FInputDeviceId::CreateFromInternalId(0), P.Tecla, P.Evento, 1.0f, false, FPlatformTime::Cycles64()));
        UE_LOG(LogPadel, Display, TEXT("PADEL_TESTE injetou %s %s"), *P.Tecla.ToString(), P.Evento == IE_Pressed ? TEXT("apertada") : TEXT("solta"));
    }
    if (EtapaDoTesteDeEntrada >= UE_ARRAY_COUNT(Roteiro) && RelogioDeTela > 5.0) PC->ConsoleCommand(TEXT("quit"));
}

void APadelPartidaAtor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const double T0 = FPlatformTime::Seconds();
    RelogioDeTela += DeltaSeconds;
    Eventos.RemoveAll([this](const FLinhaDeEvento& L) { return L.Expira <= RelogioDeTela; });

    if (!bEntradaPronta)
    {
        if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
        {
            PC->SetViewTarget(this);
            PC->SetInputMode(FInputModeGameOnly());
            PrepararEntrada(PC);
            Hud = CreateWidget<UPadelHudWidget>(PC);
            Hud->AddToViewport();
            bEntradaPronta = true;
        }
    }
    if (!Sim) return;
    if (bTesteDeEntrada) RodarTesteDeEntrada(GetWorld()->GetFirstPlayerController());

    if (bPedidoDeNovaPartida)
    {
        bPedidoDeNovaPartida = false;
        IniciarPartida();
    }

    padel::Entrada Entrada;
    if (bHumanoSimulado && HumanoSim)
    {
        Entrada = HumanoSim->Decidir(padel::EstadoVisivel::De(Sim->Partida()), DeltaSeconds);
    }
    else
    {
        FLeituraDoControle L = Leitura;
        L.AcaoApertou = bApertouA;
        L.LobApertou = bApertouB;
        L.CortadoApertou = bApertouX;
        L.AtaqueApertou = bApertouY;
        Entrada = PadelControle::MontarEntrada(L);
    }
    if (bTesteDeEntrada && !bHumanoSimulado)
    {
        if (bApertouA || bApertouB || bApertouX || bApertouY || Entrada.Dx != 0.f || Entrada.Dy != 0.f || Leitura.AtaqueSegurado || Leitura.ModificadorSegurado)
        {
            UE_LOG(LogPadel, Display, TEXT("PADEL_ENTRADA Dx=%.2f Dy=%.2f A=%d Aseg=%d B=%d X=%d Y=%d Yseg=%d RB=%d"), Entrada.Dx, Entrada.Dy,
                   Entrada.AcaoPressionada, Entrada.AcaoSegurada, Entrada.LobPressionada, Entrada.CortadoPressionado, Entrada.AtaquePressionado,
                   Entrada.AtaqueSegurado, Entrada.ModificadorSegurado);
        }
    }
    bApertouA = bApertouB = bApertouX = bApertouY = false;

    if (!Sim->Partida().Acabou())
    {
        // Quadro lento nunca vira uma rajada de segundos de simulação.
        Sim->Quadro(FMath::Min<double>(DeltaSeconds, 0.25), Entrada);
    }
    AtualizarJogadores();
    AtualizarCamera(DeltaSeconds);
    AtualizarHud();
    TratarCapturas();

    // Medição do Tick inteiro do ator (ignora os primeiros quadros, que pagam compilação de shader e carga).
    if (++QuadrosVistos > 180)
    {
        const double Ms = (FPlatformTime::Seconds() - T0) * 1000.0;
        SomaDeMs += Ms;
        MaiorMs = FMath::Max(MaiorMs, Ms);
        ++QuadrosMedidos;
    }
}
