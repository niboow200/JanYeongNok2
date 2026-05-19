#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "JYNEnemyBase.generated.h"

class AJYNXPSoul;
class UAnimMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDied, AJYNEnemyBase*, Enemy);

/**
 * 잔영록 적 캐릭터 베이스
 * HP, 피격, 사망, XP Soul 드롭 담당
 * 사망 애니메이션은 AnimBlueprint(ABP_JYNWolf)에서 bIsDead로 제어
 */
UCLASS(abstract)
class AJYNEnemyBase : public ACharacter
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float MaxHP = 30.0f;

	UPROPERTY(BlueprintReadOnly, Category="Stats")
	float CurrentHP = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stats")
	float XPReward = 10.0f;

	UPROPERTY(EditAnywhere, Category="Stats")
	TSubclassOf<AJYNXPSoul> XPSoulClass;

	UPROPERTY(EditAnywhere, Category="Stats", meta=(ClampMin=0.0f, ClampMax=1.0f))
	float KnockbackResistance = 0.0f;

	/** 플레이어와 접촉(overlap) 시 입히는 데미지 */
	UPROPERTY(EditAnywhere, Category="Stats", meta=(ClampMin=0.0f))
	float TouchDamage = 10.0f;

	/** 접촉 데미지 재적용 쿨다운 (overlap 중에도 이 간격으로 데미지) */
	UPROPERTY(EditAnywhere, Category="Stats", meta=(ClampMin=0.1f, Units="s"))
	float TouchDamageCooldown = 0.6f;

	/** 근접 공격 애니메이션 몽타주 (BP에서 설정) */
	UPROPERTY(EditAnywhere, Category="Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	/** 피격 애니메이션 몽타주 (BP에서 AM_Wolf_GetHit 설정) */
	UPROPERTY(EditAnywhere, Category="Animation")
	TObjectPtr<UAnimMontage> HitMontage;

	/** 피격 시 이동 정지 지속 시간 (초) */
	UPROPERTY(EditAnywhere, Category="Animation", meta=(ClampMin=0.0f, Units="s"))
	float HitStunDuration = 0.4f;

	/** 사망 후 액터 파괴까지 대기 시간 (사망 애니 길이에 맞게 BP에서 설정) */
	UPROPERTY(EditAnywhere, Category="Animation", meta=(ClampMin=0.0f))
	float DeathDestroyDelay = 3.5f;

	UPROPERTY(BlueprintAssignable, Category="Events")
	FOnEnemyDied OnEnemyDied;

public:
	AJYNEnemyBase();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	UFUNCTION(BlueprintCallable, Category="Damage")
	float TakeDamageJYN(float Damage, const FVector& HitDirection);

	/** ABP EventGraph에서 매 틱 IsDead() 호출로 bIsDead 갱신 */
	UFUNCTION(BlueprintPure, Category="Stats")
	bool IsDead() const { return bIsDead; }

	/** Hit stun 상태 — StateTree에서 이동/공격 차단 시 사용 가능 */
	UFUNCTION(BlueprintPure, Category="Stats")
	bool IsHitStunned() const { return bIsHitStunned; }

	UFUNCTION(BlueprintPure, Category="Stats")
	float GetHPRatio() const { return MaxHP > 0.0f ? CurrentHP / MaxHP : 0.0f; }

protected:
	void Die();

	UFUNCTION(BlueprintImplementableEvent, Category="Events", meta=(DisplayName="OnDied"))
	void BP_OnDied();

	UFUNCTION(BlueprintImplementableEvent, Category="Events", meta=(DisplayName="OnDamaged"))
	void BP_OnDamaged(float RemainingHP);

private:
	bool bIsDead = false;
	bool bIsHitStunned = false;
	float DefaultMaxWalkSpeed = 600.0f;
	float LastTouchDamageTime = -999.0f;
	FTimerHandle HitStunTimerHandle;

	void EndHitStun();

	UFUNCTION()
	void OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);
};
