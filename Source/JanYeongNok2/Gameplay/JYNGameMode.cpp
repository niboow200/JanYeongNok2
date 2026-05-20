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

		AJYNEnemyBase* Enemy = GetOrSpawnEnemy(EnemyClass, SpawnLocation);
		if (Enemy)
		{
			// AddUniqueDynamic — 풀에서 재사용 시 중복 바인딩 방지
			Enemy->OnEnemyDied.AddUniqueDynamic(this, &AJYNGameMode::OnEnemyKilled);
		}
	}
}

AJYNEnemyBase* AJYNGameMode::GetOrSpawnEnemy(TSubclassOf<AJYNEnemyBase> EnemyClass, const FVector& Location)
{
	if (!EnemyClass) return nullptr;

	// 풀에서 같은 클래스의 비활성 적 찾기
	for (AJYNEnemyBase* Pooled : EnemyPool)
	{
		if (Pooled && Pooled->IsHidden() && Pooled->GetClass() == EnemyClass)
		{
			Pooled->ResetForPool(Location);
			return Pooled;
		}
	}

	// 풀에 없으면 새로 spawn 후 풀에 등록
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AJYNEnemyBase* NewEnemy = GetWorld()->SpawnActor<AJYNEnemyBase>(
		EnemyClass, FTransform(FRotator::ZeroRotator, Location), Params);

	if (NewEnemy)
	{
		EnemyPool.Add(NewEnemy);
	}
	return NewEnemy;
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
	// ⚠️ static 사용 금지 - Live Coding이 static 초기값을 갱신하지 못함
	using CardEntry = TTuple<FString, FString, EJYNMugongCardType>;
	const TArray<CardEntry> CardPool = {
		// HP
		{ TEXT("철신공"),       TEXT("최대 HP 20 증가"),                EJYNMugongCardType::IronBody      },
		{ TEXT("회복신공"),     TEXT("HP 초당 회복량 1 증가"),          EJYNMugongCardType::HPRegen       },
		{ TEXT("대환단"),       TEXT("현재 HP 30 즉시 회복"),           EJYNMugongCardType::GreatPill     },
		// 내공
		{ TEXT("내공 증진"),    TEXT("최대 내공 20 증가"),              EJYNMugongCardType::NaegongMax    },
		{ TEXT("내공 재생"),    TEXT("내공 초당 회복량 2 증가"),        EJYNMugongCardType::NaegongRegen  },
		{ TEXT("내공 흡수 강화"), TEXT("피격 시 내공 흡수 효율 25% 증가"), EJYNMugongCardType::NaegongAbsorb },
		// 암기 (통합)
		{ TEXT("암기술"),       TEXT("암기 능력 강화 (피해→간격→반각→투사 순환)"), EJYNMugongCardType::Amgi },
		// 표창 (통합 — 활성화 → 개수/가속/데미지 순환)
		{ TEXT("표창"),         TEXT("회전 표창"),                    EJYNMugongCardType::Pyochang      },
		// 경공
		{ TEXT("신행술"),       TEXT("이동 속도 10% 증가"),             EJYNMugongCardType::Agility       },
		{ TEXT("무형지기"),     TEXT("경공 쿨타임 0.3초 감소"),         EJYNMugongCardType::NoForm        },
	};

	// 랜덤 셔플 후 3장 선택 (표창은 통합 카드 한 장이라 별도 필터링 불필요)
	TArray<int32> Indices;
	for (int32 i = 0; i < CardPool.Num(); i++) Indices.Add(i);
	for (int32 i = Indices.Num() - 1; i > 0; i--)
	{
		int32 j = FMath::RandRange(0, i);
		Indices.Swap(i, j);
	}

	// 플레이어 캐릭터 (암기술 next-stage description용)
	AJYNPlayerCharacter* Player = Cast<AJYNPlayerCharacter>(GetPlayerPawn());

	TArray<FJYNMugongCardInfo> Result;
	for (int32 k = 0; k < FMath::Min(3, Indices.Num()); k++)
	{
		const CardEntry& Entry = CardPool[Indices[k]];
		FJYNMugongCardInfo Card;
		Card.CardName    = FText::FromString(Entry.Get<0>());
		Card.Description = FText::FromString(Entry.Get<1>());
		Card.Grade       = FMath::Min(Level / 3, 3); // 레벨에 따라 등급 상승
		Card.CardType    = Entry.Get<2>();

		// 암기술 카드는 현재 스택 단계에 따라 description 동적 변경
		if (Card.CardType == EJYNMugongCardType::Amgi && Player)
		{
			Card.Description = FText::FromString(Player->GetNextAmgiDescription());
		}

		// 표창 카드 description 동적 변경 (활성화 / 개수+ / 가속+ / 데미지+)
		if (Card.CardType == EJYNMugongCardType::Pyochang && Player)
		{
			Card.Description = FText::FromString(Player->GetNextPyochangDescription());
		}

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
