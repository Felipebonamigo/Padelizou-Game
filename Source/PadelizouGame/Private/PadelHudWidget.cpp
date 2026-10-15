#include "PadelHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

namespace
{
UTextBlock* NovoTexto(UWidgetTree* Arvore, int32 Tamanho, ETextJustify::Type Justificar = ETextJustify::Left)
{
    UTextBlock* T = Arvore->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    T->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", Tamanho));
    T->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    T->SetShadowOffset(FVector2D(1.5, 1.5));
    T->SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.9f));
    T->SetJustification(Justificar);
    return T;
}

UBorder* NovaCaixa(UWidgetTree* Arvore, UWidget* Filho, float Opacidade)
{
    UBorder* B = Arvore->ConstructWidget<UBorder>(UBorder::StaticClass());
    B->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, Opacidade));
    B->SetPadding(FMargin(16.f, 10.f));
    B->SetContent(Filho);
    return B;
}

void Ancorar(UCanvasPanelSlot* S, FVector2D Ancora, FVector2D Alinhamento, FVector2D Posicao)
{
    S->SetAnchors(FAnchors(Ancora.X, Ancora.Y));
    S->SetAlignment(Alinhamento);
    S->SetPosition(Posicao);
    S->SetAutoSize(true);
}
} // namespace

bool UPadelHudWidget::Initialize()
{
    const bool bOk = Super::Initialize();
    if (!bOk || WidgetTree->RootWidget) return bOk;

    UCanvasPanel* Raiz = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Raiz"));
    WidgetTree->RootWidget = Raiz;

    TextoDoPlacar = NovoTexto(WidgetTree, 26);
    Ancorar(Cast<UCanvasPanelSlot>(Raiz->AddChild(NovaCaixa(WidgetTree, TextoDoPlacar, 0.65f))), FVector2D(0, 0), FVector2D(0, 0), FVector2D(24, 24));

    TextoDosEventos = NovoTexto(WidgetTree, 18, ETextJustify::Right);
    Ancorar(Cast<UCanvasPanelSlot>(Raiz->AddChild(NovaCaixa(WidgetTree, TextoDosEventos, 0.4f))), FVector2D(1, 0), FVector2D(1, 0), FVector2D(-24, 24));

    TextoDaMensagem = NovoTexto(WidgetTree, 32, ETextJustify::Center);
    Ancorar(Cast<UCanvasPanelSlot>(Raiz->AddChild(TextoDaMensagem)), FVector2D(0.5, 1), FVector2D(0.5, 1), FVector2D(0, -90));

    TextoDoIndicador = NovoTexto(WidgetTree, 44, ETextJustify::Center);
    Ancorar(Cast<UCanvasPanelSlot>(Raiz->AddChild(TextoDoIndicador)), FVector2D(0.5, 1), FVector2D(0.5, 1), FVector2D(0, -160));
    TextoDoIndicador->SetVisibility(ESlateVisibility::Collapsed);

    TextoDoModo = NovoTexto(WidgetTree, 16);
    Ancorar(Cast<UCanvasPanelSlot>(Raiz->AddChild(NovaCaixa(WidgetTree, TextoDoModo, 0.4f))), FVector2D(0, 1), FVector2D(0, 1), FVector2D(24, -24));

    TextoDoFim = NovoTexto(WidgetTree, 34, ETextJustify::Center);
    TelaDoFim = NovaCaixa(WidgetTree, TextoDoFim, 0.8f);
    TelaDoFim->SetPadding(FMargin(48.f, 32.f));
    Ancorar(Cast<UCanvasPanelSlot>(Raiz->AddChild(TelaDoFim)), FVector2D(0.5, 0.5), FVector2D(0.5, 0.5), FVector2D(0, 0));
    TelaDoFim->SetVisibility(ESlateVisibility::Collapsed);
    return bOk;
}

void UPadelHudWidget::Atualizar(const FText& Placar, const FText& Mensagem, const FText& Eventos, const FText& Modo)
{
    if (TextoDoPlacar) TextoDoPlacar->SetText(Placar);
    if (TextoDaMensagem) TextoDaMensagem->SetText(Mensagem);
    if (TextoDosEventos) TextoDosEventos->SetText(Eventos);
    if (TextoDoModo) TextoDoModo->SetText(Modo);
}

void UPadelHudWidget::MostrarIndicador(const FText& Texto, const FLinearColor& Cor)
{
    if (!TextoDoIndicador) return;
    TextoDoIndicador->SetVisibility(Texto.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    TextoDoIndicador->SetText(Texto);
    TextoDoIndicador->SetColorAndOpacity(FSlateColor(Cor));
}

void UPadelHudWidget::MostrarFim(bool bMostrar, const FText& Resumo)
{
    if (!TelaDoFim) return;
    TelaDoFim->SetVisibility(bMostrar ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    if (bMostrar && TextoDoFim) TextoDoFim->SetText(Resumo);
}
