#include "JYNPlayerHUD.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Character/JYNPlayerCharacter.h"
#include "Components/JYNNaegongComponent.h"
#include "Components/JYNExperienceComponent.h"
#include "Gameplay/JYNGameMode.h"

void UJYNPlayerHUD::NativeConstruct()
{
	Super::NativeConstruct();

	// BindWidgetOptional이 연결 못 한 위젯은 이름으로 폴백 탐색
	if (!HPBar)       HPBar       = Cast<UProgressBar>(GetWidgetFromName(TEXT("HPBar")));
	if (!NaegongBar)  NaegongBar  = Cast<UProgressBar>(GetWidgetFromName(TEXT("NaegongBar")));
	if (!XPBar)       XPBar       = Cast<UProgressBar>(GetWidgetFromName(TEXT("XPBar")));
	if (!LevelText)   LevelText   = Cast<UTextBlock>(GetWidgetFromName(TEXT("LevelText")));
	if (!ScoreText)   ScoreText   = Cast<UTextBlock>(GetWidgetFromName(TEXT("ScoreText")));
	if (!DashCooldownIndicator) DashCooldownIndicator = Cast<UImage>(GetWidgetFromName(TEXT("DashCooldownIndicator")));

	// 원형 인디케이터 — Image의 brush material을 dynamic instance로 변환
	if (DashCooldownIndicator)
	{
		DashCooldownMID = DashCooldownIndicator->GetDynamicMaterial();
	}

	// 플레이어와 GameMode 캐시
	if (APlayerController* PC = GetOwningPlayer())
	{
		CachedPlayer = Cast<AJYNPlayerCharacter>(PC->GetPawn());
	}
	CachedGameMode = Cast<AJYNGameMode>(UGameplayStatics::GetGameMode(this));

	// 초기값 즉시 적용
	UpdateHUDValues();
}

void UJYNPlayerHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 플레이어가 아직 빙의 안 됐을 경우 재시도
	if (!CachedPlayer)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			CachedPlayer = Cast<AJYNPlayerCharacter>(PC->GetPawn());
		}
	}

	UpdateHUDValues();
}

void UJYNPlayerHUD::UpdateHUDValues()
{
	if (!CachedPlayer) return;

	// ── HP ──────────────────────────────────────────────────
	if (HPBar)
	{
		HPBar->SetPercent(CachedPlayer->GetHPRatio());
	}

	// ── 내공 ─────────────────────────────────────────────────
	if (NaegongBar && CachedPlayer->NaegongComponent)
	{
		NaegongBar->SetPercent(CachedPlayer->NaegongComponent->GetNaegongRatio());
	}

	// ── 경험치 ───────────────────────────────────────────────
	if (XPBar && CachedPlayer->ExperienceComponent)
	{
		XPBar->SetPercent(CachedPlayer->ExperienceComponent->GetXPRatio());
	}

	// ── 레벨 ─────────────────────────────────────────────────
	if (LevelText && CachedPlayer->ExperienceComponent)
	{
		const int32 Lv = CachedPlayer->ExperienceComponent->CurrentLevel;
		LevelText->SetText(FText::Format(FText::FromString(TEXT("Lv.{0}")), FText::AsNumber(Lv)));
	}

	// ── 점수 ─────────────────────────────────────────────────
	if (ScoreText)
	{
		if (!CachedGameMode)
		{
			CachedGameMode = Cast<AJYNGameMode>(UGameplayStatics::GetGameMode(this));
		}
		if (CachedGameMode)
		{
			ScoreText->SetText(FText::AsNumber(CachedGameMode->CurrentScore));
		}
	}

	// ── 대쉬 쿨다운 인디케이터 ──────────────────────────────
	if (DashCooldownMID)
	{
		// Progress 0.0(방금 사용) → 1.0(준비완료)
		DashCooldownMID->SetScalarParameterValue(TEXT("Progress"), CachedPlayer->GetDashCooldownProgress());
	}
}
