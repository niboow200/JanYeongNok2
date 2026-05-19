#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JYNMainMenu.generated.h"

class UButton;
class UTextBlock;

/**
 * 메인 메뉴
 * - 게임 시작 → GameMode.StartRun() 호출
 * - 게임 종료 → QuitGame
 */
UCLASS()
class UJYNMainMenu : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> PlayButton;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> QuitButton;

	virtual void NativeConstruct() override;

private:
	UFUNCTION() void OnPlayClicked();
	UFUNCTION() void OnQuitClicked();
};
