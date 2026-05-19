#include "JYNGameMode.h"
#include "AI/JYNEnemyBase.h"
#include "Character/JYNPlayerCharacter.h"
#include "Components/JYNExperienceComponent.h"
#include "UI/JYNPlayerHUD.h"
#include "UI/JYNLevelUpScreen.h"
#include "UI/JYNGameOverScreen.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Blueprint/UserWidget.h"

// static 플래그 정의 (PIE 세션 동안 유지)
bool AJYNGameMode::bSkipMainMenuOnRestart = false;

AJYNGameMode::AJYNGameMode()
{
}

void AJYNGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 커서 표시 + 게임/UI 혼합 입력 모드
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		PC->bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
	}

	// 재도전 클릭으로 들어왔으면 메인메뉴 건너뛰고 바로 게임 시작
	if (bSkipMainMenuOnRestart)
	{
		bSkipMainMenuOnRestart = false;  // 일회성 플래그, 즉시 리셋

		// HUD 생성 (StartRun이 처리하지만 명시적으로 일시정지 안 함)
		StartRun();
	}
	else
	{
		ShowMainMenu();
	}
}

void AJYNGameMode::ShowMainMenu()
{
	UE_LOG(LogTemp, Warning, TEXT("ShowMainMenu() called"));
	UE_LOG(LogTemp, Warning, TEXT("MainMenuWidgetClass: %s"), MainMenuWidgetClass ? TEXT("Valid") : TEXT("NULL"));

	// 메인 메뉴 위젯 생성
	if (MainMenuWidgetClass)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			UE_LOG(LogTemp, Warning, TEXT("PlayerController found"));
			if (!MainMenuWidgetInstance)
			{
				MainMenuWidgetInstance = CreateWidget<UUserWidget>(PC, MainMenuWidgetClass);
				if (MainMenuWidgetInstance)
				{
					UE_LOG(LogTemp, Warning, TEXT("MainMenu Widget created successfully"));
					MainMenuWidgetInstance->AddToViewport(100);
					UE_LOG(LogTemp, Warning, TEXT("MainMenu Widget added to viewport"));
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("Failed to create MainMenu Widget"));
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("PlayerController not found"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("MainMenuWidgetClass is NULL - Set it in BP_JYNGameMode"));
	}

	// 게임 일시정지
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		PC->SetPause(true);
		UE_LOG(LogTemp, Warning, TEXT("Game paused"));
	}
}

void AJYNGameMode::StartRun()
{
	if (bRunActive) return;

	bRunActive = true;
	CurrentScore = 0;
	CurrentWave = 0;

	// 메인 메뉴 제거
	if (MainMenuWidgetInstance)
	{
		MainMenuWidgetInstance->RemoveFromParent();
		MainMenuWidgetInstance = nullptr;
	}

	// 게임 재개
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		PC->SetPause(false);
	}

	// HUD 생성 및 뷰포트 추가 (아직 없으면)
	if (!HUDWidgetInstance && HUDWidgetClass)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			HUDWidgetInstance = CreateWidget<UJYNPlayerHUD>(PC, HUDWidgetClass);
			if (HUDWidgetInstance)
			{
				HUDWidgetInstance->AddToViewport();
			}
		}
	}

	// 플레이어의 ExperienceComponent에 레벨업 이벤트 바인딩
	if (AJYNPlayerCharacter* Player = Cast<AJYNPlayerCharacter>(GetPlayerPawn()))
	{
		if (Player->ExperienceComponent)
		{
			Player->ExperienceComponent->OnLevelUp.AddDynamic(this, &AJYNGameMode::OnPlayerLevelUp);
		}
	}

	// 웨이브 타이머 시작
	GetWorld()->GetTimerManager().SetTimer(
		WaveTimerHandle,
		this,
		&AJYNGameMode::SpawnWave,
		WaveInterval,
		true,
		1.0f  // 런 시작 1초 후 첫 웨이브
	);
}

void AJYNGameMode::EndRun()
{
	if (!bRunActive) return;

	bRunActive = false;
	GetWorld()->GetTimerManager().ClearTimer(WaveTimerHandle);

	// 게임 오버 화면 표시
	if (GameOverWidgetClass)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
		{
			GameOverWidgetInstance = CreateWidget<UJYNGameOverScreen>(PC, GameOverWidgetClass);
			if (GameOverWidgetInstance)
			{
				GameOverWidgetInstance->SetScore(CurrentScore);
				GameOverWidgetInstance->AddToViewport();
			}
		}
	}

	// 게임 일시정지
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		PC->SetPause(true);
	}
}

void AJYNGameMode::SpawnWave()
{
	if (!bRunActive || EnemyClasses.IsEmpty()) return;

	CurrentWave++;
	OnWaveChanged.Broadcast(CurrentWave);

	APawn* Player = GetPlayerPawn();
	if (!Player) return;

	const FVector PlayerLocation = Player->GetActorLocation();
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetNavigationSystem(GetWorld());

	const int32 SpawnCount = BaseEnemiesPerWave + (CurrentWave - 1) * EnemiesPerWaveIncrease;

	for (int32 i = 0; i < SpawnCount; i++)
	{
		// 랜덤 적 클래스 선택
		const int32 ClassIdx = FMath::RandRange(0, EnemyClasses.Num() - 1);
		TSubclassOf<AJYNEnemyBase> EnemyClass = EnemyClasses[ClassIdx];
		if (!EnemyClass) continue;

		// 플레이어 주변 랜덤 위치 (NavMesh 위)
		FNavLocation SpawnNavLocation;
		const FVector RandDir = FMath::VRand();
		const float RandDist = FMath::RandRange(SpawnMinDistance, SpawnMaxDistance);
		const FVector DesiredLocation = PlayerLocation + FVector(RandDir.X, RandDir.Y, 0.0f) * RandDist;

		FVector SpawnLocation = DesiredLocation;
		if (NavSys)
		{
			if (NavSys->GetRandomReachablePointInRadius(DesiredLocation, 300.0f, SpawnNavLocation))
			{
				SpawnLocation = SpawnNavLocation.Location;
			}
		}

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AJYNEnemyBase* Enemy = GetWorld()->SpawnActor<AJYNEnemyBase>(
			EnemyClass,
			FTransform(FRotator::ZeroRotator, SpawnLocation),
			Params
		);

		if (Enemy)
		{
			Enemy->OnEnemyDied.AddDynamic(this, &AJYNGameMode::OnEnemyKilled);
		}
	}
}

void AJYNGameMode::OnEnemyKilled(AJYNEnemyBase* Enemy)
{
	AddScore(ScorePerKill);
}

void AJYNGameMode::OnPlayerLevelUp(int32 NewLevel)
{
	UGameplayStatics::SetGamePaused(GetWorld(), true);

	if (!LevelUpWidgetClass) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC) return;

	LevelUpWidgetInstance = CreateWidget<UJYNLevelUpScreen>(PC, LevelUpWidgetClass);
	if (!LevelUpWidgetInstance) return;

	// 카드 3장 생성 후 캐시 (선택 시 효과 적용에 사용)
	PendingMugongCards = GenerateMugongCards(NewLevel);

	// 레벨업 자동 스탯 보너스 적용
	if (AJYNPlayerCharacter* Player = Cast<AJYNPlayerCharacter>(GetPlayerPawn()))
	{
		Player->ApplyLevelUpBonus(NewLevel);
	}

	LevelUpWidgetInstance->OnCardPicked.AddDynamic(this, &AJYNGameMode::OnMugongCardPickedInternal);
	// AddToViewport 먼저 → NativeConstruct 실행 → BindWidget 연결 완료
	// SetupCards는 반드시 AddToViewport 이후에 호출해야 TextBlock이 유효함
	LevelUpWidgetInstance->AddToViewport(10); // HUD보다 위
	LevelUpWidgetInstance->SetupCards(NewLevel, PendingMugongCards);

	// 카드 선택 중 UI 전용 입력 모드 (커서 클릭이 위젯에 전달되도록)
	FInputModeUIOnly UIMode;
	UIMode.SetWidgetToFocus(LevelUpWidgetInstance->TakeWidget());
	PC->SetInputMode(UIMode);

	OnMugongCardRequested.Broadcast(NewLevel);
}

void AJYNGameMode::OnMugongCardPickedInternal(int32 CardIndex)
{
	// 선택한 카드의 효과를 플레이어에게 적용
	if (PendingMugongCards.IsValidIndex(CardIndex))
	{
		if (AJYNPlayerCharacter* Player = Cast<AJYNPlayerCharacter>(GetPlayerPawn()))
		{
			Player->ApplyMugongCard(PendingMugongCards[CardIndex].CardType);
		}
	}
	PendingMugongCards.Empty();

	OnMugongCardSelected();
}

void AJYNGameMode::OnMugongCardSelected()
{
	UGameplayStatics::SetGamePaused(GetWorld(), false);
	LevelUpWidgetInstance = nullptr;

	// 카드 선택 완료 후 게임+UI 혼합 입력 모드 복귀
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
	}
}

TArray<FJYNMugongCardInfo> AJYNGameMode::GenerateMugongCards(int32 Level)
{
	// 카드 풀: {이름, 설명, CardType}
	using CardEntry = TTuple<FString, FString, EJYNMugongCardType>;
	static const TArray<CardEntry> CardPool = {
		{ TEXT("암기 속도 강화"), TEXT("자동 공격 간격 20% 감소"),      EJYNMugongCardType::AmkiSpeedUp   },
		{ TEXT("내공 흡수 강화"), TEXT("피격 시 내공 흡수 효율 25% 증가"), EJYNMugongCardType::NaegongAbsorb },
		{ TEXT("철신공"),         TEXT("최대 HP 20 증가"),               EJYNMugongCardType::IronBody       },
		{ TEXT("신행술"),         TEXT("이동 속도 10% 증가"),            EJYNMugongCardType::Agility        },
		{ TEXT("독침 강화"),      TEXT("암기 피해 15% 증가"),            EJYNMugongCardType::PoisonFang     },
		{ TEXT("내공 재생"),      TEXT("내공 초당 회복량 2 증가"),       EJYNMugongCardType::NaegongRegen   },
		{ TEXT("무형지기"),       TEXT("경공 쿨타임 0.3초 감소"),        EJYNMugongCardType::NoForm         },
		{ TEXT("추혼탄"),         TEXT("암기 유도 반각 20% 확대"),       EJYNMugongCardType::ChasingBullet  },
		{ TEXT("대환단"),         TEXT("현재 HP 30 즉시 회복"),          EJYNMugongCardType::GreatPill      },
		{ TEXT("광풍세우"),       TEXT("암기 투사 수 +1"),               EJYNMugongCardType::StormRain      },
	};

	// 랜덤 셔플 후 3장 선택
	TArray<int32> Indices;
	for (int32 i = 0; i < CardPool.Num(); i++) Indices.Add(i);
	for (int32 i = Indices.Num() - 1; i > 0; i--)
	{
		int32 j = FMath::RandRange(0, i);
		Indices.Swap(i, j);
	}

	TArray<FJYNMugongCardInfo> Result;
	for (int32 k = 0; k < FMath::Min(3, Indices.Num()); k++)
	{
		const CardEntry& Entry = CardPool[Indices[k]];
		FJYNMugongCardInfo Card;
		Card.CardName    = FText::FromString(Entry.Get<0>());
		Card.Description = FText::FromString(Entry.Get<1>());
		Card.Grade       = FMath::Min(Level / 3, 3); // 레벨에 따라 등급 상승
		Card.CardType    = Entry.Get<2>();
		Result.Add(Card);
	}
	return Result;
}

void AJYNGameMode::AddScore(int32 Amount)
{
	CurrentScore += Amount;
	OnScoreChanged.Broadcast(CurrentScore);
}

APawn* AJYNGameMode::GetPlayerPawn() const
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	return PC ? PC->GetPawn() : nullptr;
}
