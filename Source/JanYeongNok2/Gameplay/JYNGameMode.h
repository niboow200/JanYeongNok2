#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "JYNGameMode.generated.h"

class AJYNEnemyBase;
class UJYNPlayerHUD;
class UJYNLevelUpScreen;
struct FJYNMugongCardInfo;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScoreChanged, int32, NewScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWaveChanged, int32, NewWave);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMugongCardRequested, int32, PlayerLevel);

/**
 * 잔영록 GameMode
 * - 시간 기반 웨이브 스폰
 * - 점수 누적
 * - 레벨업 시 화면 정지 + 무공 카드 선택 UI 호출
 * - 런 시작/종료 관리
 */
UCLASS()
class AJYNGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	// ── HUD ──────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category="UI")
	TSubclassOf<UJYNPlayerHUD> HUDWidgetClass;

	UPROPERTY()
	TObjectPtr<UJYNPlayerHUD> HUDWidgetInstance;

	// ── 레벨업 카드 ─────────────────────────────────────────
	/** 레벨업 카드 선택 위젯 클래스 (BP에서 WBP_LevelUpScreen 설정) */
	UPROPERTY(EditAnywhere, Category="UI")
	TSubclassOf<UJYNLevelUpScreen> LevelUpWidgetClass;

	UPROPERTY()
	TObjectPtr<UJYNLevelUpScreen> LevelUpWidgetInstance;

	// ── 스폰 설정 ──────────────────────────────────────────
	/** 스폰할 적 클래스 목록 (BP에서 설정) */
	UPROPERTY(EditAnywhere, Category="Spawning")
	TArray<TSubclassOf<AJYNEnemyBase>> EnemyClasses;

	/** 웨이브당 기본 스폰 수 */
	UPROPERTY(EditAnywhere, Category="Spawning", meta=(ClampMin=1))
	int32 BaseEnemiesPerWave = 3;

	/** 웨이브마다 추가되는 적 수 */
	UPROPERTY(EditAnywhere, Category="Spawning", meta=(ClampMin=0))
	int32 EnemiesPerWaveIncrease = 1;

	/** 웨이브 간격 (초) */
	UPROPERTY(EditAnywhere, Category="Spawning", meta=(ClampMin=1.0f, Units="s"))
	float WaveInterval = 15.0f;

	/** 플레이어 주변 스폰 최소 거리 */
	UPROPERTY(EditAnywhere, Category="Spawning", meta=(Units="cm"))
	float SpawnMinDistance = 800.0f;

	/** 플레이어 주변 스폰 최대 거리 */
	UPROPERTY(EditAnywhere, Category="Spawning", meta=(Units="cm"))
	float SpawnMaxDistance = 1400.0f;

	// ── 점수 ──────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category="Score")
	int32 ScorePerKill = 10;

	// ── 이벤트 ──────────────────────────────────────────
	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnScoreChanged OnScoreChanged;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnWaveChanged OnWaveChanged;

	/** 레벨업 시 브로드캐스트 → BP HUD가 카드 UI 표시 */
	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnMugongCardRequested OnMugongCardRequested;

	// ── 상태 ──────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, Category="State")
	int32 CurrentScore = 0;

	UPROPERTY(BlueprintReadOnly, Category="State")
	int32 CurrentWave = 0;

	UPROPERTY(BlueprintReadOnly, Category="State")
	bool bRunActive = false;

private:
	FTimerHandle WaveTimerHandle;

	/** OnPlayerLevelUp에서 생성한 카드 목록 캐시 (CardIndex로 효과 적용 시 참조) */
	TArray<FJYNMugongCardInfo> PendingMugongCards;

public:
	AJYNGameMode();

protected:
	virtual void BeginPlay() override;

public:
	/** 런 시작 */
	UFUNCTION(BlueprintCallable, Category="Run")
	void StartRun();

	/** 런 종료 (플레이어 사망) */
	UFUNCTION(BlueprintCallable, Category="Run")
	void EndRun();

	/** 적 사망 시 호출 (EnemyBase의 OnEnemyDied에 바인딩) */
	UFUNCTION()
	void OnEnemyKilled(AJYNEnemyBase* Enemy);

	/** 레벨업 시 호출 (ExperienceComponent의 OnLevelUp에 바인딩) */
	UFUNCTION()
	void OnPlayerLevelUp(int32 NewLevel);

	/** 무공 카드 선택 완료 → 게임 재개 */
	UFUNCTION(BlueprintCallable, Category="Mugong")
	void OnMugongCardSelected();

private:
	UFUNCTION()
	void OnMugongCardPickedInternal(int32 CardIndex);

	TArray<FJYNMugongCardInfo> GenerateMugongCards(int32 Level);

protected:
	void SpawnWave();
	void AddScore(int32 Amount);

	APawn* GetPlayerPawn() const;
};
