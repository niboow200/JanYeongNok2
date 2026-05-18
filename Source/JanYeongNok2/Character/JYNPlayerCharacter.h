#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "JYNPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class UInputMappingContext;
class UInputAction;
class UJYNNaegongComponent;
class UJYNExperienceComponent;
class AJYNProjectileBase;
struct FInputActionValue;

// JYNLevelUpScreen.h에서 정의된 enum 전방 선언
enum class EJYNMugongCardType : uint8;

/**
 * 잔영록 플레이어 캐릭터 (사천당가 협객)
 * - 탑다운 WASD 이동, Shift 경공(무적)
 * - 자동 공격: AutoAttackInterval마다 범위 내 최근접 적 유도 발사
 * - NaegongComponent: 피격 흡수, 임계점 강화
 * - ExperienceComponent: XP, 레벨업 → 무공 카드 선택 트리거
 */
UCLASS(abstract)
class AJYNPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCameraComponent> Camera;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UJYNNaegongComponent> NaegongComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UJYNExperienceComponent> ExperienceComponent;

	// ── 입력 ──────────────────────────────────────────────
protected:
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> DashAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> PauseAction;

	// ── HP ──────────────────────────────────────────────
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats")
	float MaxHP = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category="Stats")
	float CurrentHP = 100.0f;

	// ── 누적 업그레이드 스탯 ──────────────────────────────
public:
	/** 투사체 피해 배율 (독침 강화 누적) */
	UPROPERTY(BlueprintReadOnly, Category="Stats")
	float ProjectileDamageMultiplier = 1.0f;

	/** 추가 투사 수 (광풍세우 누적) */
	UPROPERTY(BlueprintReadOnly, Category="Stats")
	int32 ExtraProjectileCount = 0;

	// ── 자동 공격 ──────────────────────────────────────────
protected:
	/** 발사할 투사체 클래스 (BP에서 설정) */
	UPROPERTY(EditAnywhere, Category="AutoAttack")
	TSubclassOf<AJYNProjectileBase> ProjectileClass;

	/** 자동 공격 간격 (초) */
	UPROPERTY(EditAnywhere, Category="AutoAttack", meta=(ClampMin=0.1f, Units="s"))
	float AutoAttackInterval = 0.5f;

	/** 자동 공격 사거리 */
	UPROPERTY(EditAnywhere, Category="AutoAttack", meta=(ClampMin=100.0f, Units="cm"))
	float AutoAttackRange = 800.0f;

	/** 유도 원뿔 반각 (이 각도 이내 적만 유도, 이 밖이면 직선 발사) */
	UPROPERTY(EditAnywhere, Category="AutoAttack", meta=(ClampMin=0.0f, ClampMax=90.0f, Units="deg"))
	float HomingHalfAngle = 45.0f;

	/** 투사체 스폰 오프셋 (캐릭터 전방) */
	UPROPERTY(EditAnywhere, Category="AutoAttack", meta=(ClampMin=0.0f, Units="cm"))
	float ProjectileSpawnOffset = 60.0f;

	// ── 경공(대시) — 이전 프로젝트 방식: MaxWalkSpeed 부스트 ──────────────
	/** 경공 지속 시간 */
	UPROPERTY(EditAnywhere, Category="Dash", meta=(ClampMin=0.05f, Units="s"))
	float QinggongDuration = 0.12f;

	/** 경공 후 속도 복구 시간 */
	UPROPERTY(EditAnywhere, Category="Dash", meta=(ClampMin=0.0f, Units="s"))
	float QinggongRecoveryDuration = 0.35f;

	/** 경공 중 최대 이동 속도 */
	UPROPERTY(EditAnywhere, Category="Dash", meta=(ClampMin=600.0f, Units="cm/s"))
	float QinggongMaxWalkSpeed = 1800.0f;

	/** 경공 중 최대 가속도 */
	UPROPERTY(EditAnywhere, Category="Dash", meta=(ClampMin=1000.0f))
	float QinggongMaxAcceleration = 16000.0f;

	/** 경공 중 제동력 (낮을수록 미끄러짐) */
	UPROPERTY(EditAnywhere, Category="Dash", meta=(ClampMin=0.0f))
	float QinggongBrakingDeceleration = 80.0f;

	/** 경공 중 지면 마찰 (낮을수록 미끄러짐) */
	UPROPERTY(EditAnywhere, Category="Dash", meta=(ClampMin=0.0f))
	float QinggongGroundFriction = 0.3f;

	/** 경공 쿨타임 */
	UPROPERTY(EditAnywhere, Category="Dash", meta=(ClampMin=0.0f, Units="s"))
	float DashCooldown = 1.0f;

	/** 대쉬 잔상 Niagara 시스템 */
	UPROPERTY(EditAnywhere, Category="Dash")
	TObjectPtr<UNiagaraSystem> DashTrailSystem;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> DashTrailComponent;

	// ── 피격 무적 ─────────────────────────────────────────
	/** 피격 후 무적 지속 시간 (여러 적 동시 공격 방지) */
	UPROPERTY(EditAnywhere, Category="Stats", meta=(ClampMin=0.0f, Units="s"))
	float HitInvincibilityDuration = 0.6f;

	// ── 내부 상태 ──────────────────────────────────────────
public:
	UPROPERTY(BlueprintReadOnly, Category="Dash")
	bool bIsDashing = false;

private:
	bool bIsInvincible = false;
	bool bIsDead = false;
	bool bIsQinggongOnCooldown = false;
	bool bIsRecovering = false;
	float RecoveryElapsed = 0.0f;

	// 이동 기본값 (BeginPlay에서 저장 후 경공 종료 시 복구)
	float DefaultMaxWalkSpeed = 500.0f;
	float DefaultMaxAcceleration = 2048.0f;
	float DefaultBrakingDeceleration = 2000.0f;
	float DefaultGroundFriction = 8.0f;

	FVector DashDirection = FVector::ZeroVector; // 대쉬 중 입력 유지용

	FTimerHandle AutoAttackTimerHandle;
	FTimerHandle QinggongDurationTimer;
	FTimerHandle QinggongCooldownTimer;
	FTimerHandle InvincibilityTimerHandle; // 피격/경공 무적 공용 타이머

	// ── 생명주기 ──────────────────────────────────────────
public:
	AJYNPlayerCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void NotifyControllerChanged() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// ── 입력 핸들러 ──────────────────────────────────────────
protected:
	void OnMove(const FInputActionValue& Value);
	void OnDash(const FInputActionValue& Value);
	void OnPause(const FInputActionValue& Value);

	// ── 자동 공격 ──────────────────────────────────────────
	void PerformAutoAttack();
	AActor* FindNearestEnemyInRange() const;
	/** 최대 MaxCount명의 사거리 내 적 반환 (거리 오름차순) */
	TArray<AActor*> FindEnemiesInRange(int32 MaxCount) const;
	/** AutoAttackInterval 변경 후 타이머 재설정 */
	void UpdateAutoAttackTimer();

	// ── 경공 ──────────────────────────────────────────────
	void EndQinggong();
	void EndQinggongCooldown();
	void ApplyDashMovementBoost(const FVector& Dir);
	void UpdateRecovery(float DeltaTime);

	// ── 무적 ──────────────────────────────────────────────
	/** Duration 동안 무적 설정. 이미 걸려 있으면 더 긴 쪽 유지 */
	void SetInvincible(float Duration);
	void ClearInvincibility();

	// ── 데미지 / 사망 ──────────────────────────────────────
public:
	/**
	 * 데미지 처리 진입점.
	 * 내공 흡수 → HP 감소 → 사망 체크
	 */
	UFUNCTION(BlueprintCallable, Category="Damage")
	void TakeDamageJYN(float Damage, const FVector& HitDirection);

	/** 투사체 적중 시 내공 회복 (JYNProjectileBase에서 호출) */
	void RegainNaegongOnHit(float Amount);

	// ── 레벨업 효과 ─────────────────────────────────────────
	/** 레벨업마다 자동 적용 (MaxHP +5, 경공 쿨 -0.05s, 이동속도 +10) */
	void ApplyLevelUpBonus(int32 NewLevel);

	/** 선택한 무공 카드 효과 적용 */
	void ApplyMugongCard(EJYNMugongCardType CardType);

	// ── BP 이벤트 ──────────────────────────────────────────
	UFUNCTION(BlueprintImplementableEvent, Category="Events", meta=(DisplayName="OnDamaged"))
	void BP_OnDamaged(float RemainingHP, float MaxHPVal);

	UFUNCTION(BlueprintImplementableEvent, Category="Events", meta=(DisplayName="OnDied"))
	void BP_OnDied();

	UFUNCTION(BlueprintImplementableEvent, Category="Events", meta=(DisplayName="OnDashStarted"))
	void BP_OnDashStarted();

	// ── Getter ──────────────────────────────────────────
public:
	UFUNCTION(BlueprintPure, Category="Stats")
	float GetCurrentHP() const { return CurrentHP; }

	UFUNCTION(BlueprintPure, Category="Stats")
	float GetMaxHP() const { return MaxHP; }

	UFUNCTION(BlueprintPure, Category="Stats")
	float GetHPRatio() const { return MaxHP > 0.0f ? CurrentHP / MaxHP : 0.0f; }

	UFUNCTION(BlueprintPure, Category="Dash")
	bool CanDash() const { return !bIsDashing && !bIsQinggongOnCooldown; }
};
