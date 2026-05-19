#include "JYNMainMenu.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Gameplay/JYNGameMode.h"

void UJYNMainMenu::NativeConstruct()
{
	Super::NativeConstruct();

	if (PlayButton)
	{
		PlayButton->OnClicked.AddDynamic(this, &UJYNMainMenu::OnPlayClicked);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UJYNMainMenu::OnQuitClicked);
	}
}

void UJYNMainMenu::OnPlayClicked()
{
	if (UWorld* World = GetWorld())
	{
		if (AJYNGameMode* GameMode = Cast<AJYNGameMode>(World->GetAuthGameMode()))
		{
			GameMode->StartRun();
		}
	}
}

void UJYNMainMenu::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(GetWorld(), nullptr, EQuitPreference::Quit, true);
}
