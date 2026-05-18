#include "JYNLevelUpScreen.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UJYNLevelUpScreen::NativeConstruct()
{
	Super::NativeConstruct();

	// BindWidgetOptional 미연결 위젯 이름으로 폴백
	if (!Card1Button) Card1Button = Cast<UButton>(GetWidgetFromName(TEXT("Card1Button")));
	if (!Card2Button) Card2Button = Cast<UButton>(GetWidgetFromName(TEXT("Card2Button")));
	if (!Card3Button) Card3Button = Cast<UButton>(GetWidgetFromName(TEXT("Card3Button")));
	if (!Card1Name)   Card1Name   = Cast<UTextBlock>(GetWidgetFromName(TEXT("Card1Name")));
	if (!Card2Name)   Card2Name   = Cast<UTextBlock>(GetWidgetFromName(TEXT("Card2Name")));
	if (!Card3Name)   Card3Name   = Cast<UTextBlock>(GetWidgetFromName(TEXT("Card3Name")));
	if (!Card1Desc)   Card1Desc   = Cast<UTextBlock>(GetWidgetFromName(TEXT("Card1Desc")));
	if (!Card2Desc)   Card2Desc   = Cast<UTextBlock>(GetWidgetFromName(TEXT("Card2Desc")));
	if (!Card3Desc)   Card3Desc   = Cast<UTextBlock>(GetWidgetFromName(TEXT("Card3Desc")));
	if (!LevelTitle)  LevelTitle  = Cast<UTextBlock>(GetWidgetFromName(TEXT("LevelTitle")));

	if (Card1Button) Card1Button->OnClicked.AddDynamic(this, &UJYNLevelUpScreen::OnCard1Clicked);
	if (Card2Button) Card2Button->OnClicked.AddDynamic(this, &UJYNLevelUpScreen::OnCard2Clicked);
	if (Card3Button) Card3Button->OnClicked.AddDynamic(this, &UJYNLevelUpScreen::OnCard3Clicked);
}

void UJYNLevelUpScreen::SetupCards(int32 NewLevel, const TArray<FJYNMugongCardInfo>& Cards)
{
	CurrentCards = Cards;

	if (LevelTitle)
	{
		LevelTitle->SetText(FText::Format(
			FText::FromString(TEXT("레벨 {0} 돌파 — 무공 선택")),
			FText::AsNumber(NewLevel)));
	}

	if (Cards.Num() >= 1) SetCardText(Card1Name, Card1Desc, Cards[0]);
	if (Cards.Num() >= 2) SetCardText(Card2Name, Card2Desc, Cards[1]);
	if (Cards.Num() >= 3) SetCardText(Card3Name, Card3Desc, Cards[2]);
}

void UJYNLevelUpScreen::SetCardText(UTextBlock* NameBlock, UTextBlock* DescBlock, const FJYNMugongCardInfo& Info)
{
	if (NameBlock) NameBlock->SetText(Info.CardName);
	if (DescBlock) DescBlock->SetText(Info.Description);
}

void UJYNLevelUpScreen::OnCard1Clicked() { PickCard(0); }
void UJYNLevelUpScreen::OnCard2Clicked() { PickCard(1); }
void UJYNLevelUpScreen::OnCard3Clicked() { PickCard(2); }

void UJYNLevelUpScreen::PickCard(int32 Index)
{
	OnCardPicked.Broadcast(Index);
	RemoveFromParent();
}
