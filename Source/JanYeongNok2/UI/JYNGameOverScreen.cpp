#include "JYNGameOverScreen.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Gameplay/JYNGameMode.h"

void UJYNGameOverScreen::NativeConstruct()
{
	Super::NativeConstruct();

	// 버튼 클릭 이벤트 바인딩
	if (RestartButton)
	{
		RestartButton->OnClicked.AddDynamic(this, &UJYNGameOverScreen::OnRestartClicked);
	}

	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.AddDynamic(this, &UJYNGameOverScreen::OnMainMenuClicked);
	}
}

void UJYNGameOverScreen::SetScore(int32 FinalScore)
{
	if (ScoreValueText)
	{
		ScoreValueText->SetText(FText::AsNumber(FinalScore));
	}
}

void UJYNGameOverScreen::OnRestartClicked()
{
	// 메인메뉴 건너뛰고 바로 게임 시작하도록 플래그 설정
	AJYNGameMode::bSkipMainMenuOnRestart = true;

	// 일시정지 해제 후 레벨 재시작
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		PC->SetPause(false);
		PC->RestartLevel();
	}
}

void UJYNGameOverScreen::OnMainMenuClicked()
{
	// 레벨을 재시작하면 BeginPlay에서 메인메뉴가 자동으로 뜸 (죽은 적/캐릭터 정리됨)
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		PC->SetPause(false);
		PC->RestartLevel();
	}
}
