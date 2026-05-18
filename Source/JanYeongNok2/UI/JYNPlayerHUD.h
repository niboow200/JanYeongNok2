#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JYNPlayerHUD.generated.h"

class UProgressBar;
class UTextBlock;
class AJYNPlayerCharacter;
class AJYNGameMode;

/**
 * 잔영록 플레이어 HUD 위젯
 * - BindWidget으로 WBP의 위젯과 자동 연결
 * - NativeTick에서 플레이어 컴포넌트를 읽어 매 프레임 갱신
 *
 * WBP에서 위젯 이름이 아래 변수명과 정확히 일치해야 함:
 *   HPBar, NaegongBar, XPBar, LevelText, ScoreText
 */
UCLASS(abstract)
class UJYNPlayerHUD : public UUserWidget
{
	GENERATED_BODY()

	// ── BindWidgetOptional: WBP에서 동일 이름 위젯과 자동 연결 ──────────────────
	// 이름이 정확히 일치해야 바인딩됨:  HPBar, NaegongBar, XPBar, LevelText, ScoreText
protected:
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UProgressBar> HPBar;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UProgressBar> NaegongBar;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UProgressBar> XPBar;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ScoreText;

	// ── 캐시 ──────────────────────────────────────────────────
private:
	UPROPERTY()
	TObjectPtr<AJYNPlayerCharacter> CachedPlayer;

	UPROPERTY()
	TObjectPtr<AJYNGameMode> CachedGameMode;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void UpdateHUDValues();
};
