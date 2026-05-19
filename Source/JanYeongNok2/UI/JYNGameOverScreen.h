#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JYNGameOverScreen.generated.h"

class UButton;
class UTextBlock;

/**
 * 게임 오버 화면
 * - 최종 점수 표시
 * - 재시작/메인메뉴 버튼
 */
UCLASS()
class UJYNGameOverScreen : public UUserWidget
{
	GENERATED_BODY()

	// ── BindWidget ────────────────────────────────────────
protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> GameOverText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> ScoreValueText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton>    RestartButton;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton>    MainMenuButton;

	// ── 공개 API ─────────────────────────────────────────
public:
	/** 최종 점수 설정 */
	UFUNCTION(BlueprintCallable, Category="GameOver")
	void SetScore(int32 FinalScore);

protected:
	virtual void NativeConstruct() override;

private:
	UFUNCTION() void OnRestartClicked();
	UFUNCTION() void OnMainMenuClicked();
};
