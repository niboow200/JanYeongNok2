#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JYNPyochangOrbit.generated.h"

class USphereComponent;
class UNiagaraSystem;
class UNiagaraComponent;

/**
 * 표창 회전 액터
 * - 플레이어 캐릭터 child로 attach
 * - SphereComponent들이 collision만 담당 (visual mesh 없음)
 * - 각 Sphere에 Niagara가 attach되어 sprite/trail로 시각화
 */
UCLASS()
class AJYNPyochangOrbit : public AActor
{
	GENERATED_BODY()

public:
	AJYNPyochangOrbit();

	/** 현재 표창 개수 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pyochang", meta=(ClampMin=1, ClampMax=12))
	int32 PyochangCount = 3;

	/** 회전 반경 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pyochang", meta=(ClampMin=50.0f, Units="cm"))
	float OrbitRadius = 200.0f;

	/** 회전 속도 (도/초) — 카드로 증가 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pyochang", meta=(Units="deg"))
	float RotationSpeed = 120.0f;

	/** 표창 1개당 데미지 — 카드로 증가 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pyochang")
	float Damage = 4.0f;

	/** 같은 적을 연속 타격 방지 쿨다운 */
	UPROPERTY(EditAnywhere, Category="Pyochang", meta=(ClampMin=0.1f, Units="s"))
	float HitCooldownPerTarget = 0.4f;

	/** 표창 sphere 충돌 반지름 */
	UPROPERTY(EditAnywhere, Category="Pyochang", meta=(ClampMin=10.0f, Units="cm"))
	float PyochangRadius = 25.0f;

	/** 시각화 Niagara System (BP에서 NS_PyochangOrbit 설정) */
	UPROPERTY(EditAnywhere, Category="Pyochang")
	TObjectPtr<UNiagaraSystem> PyochangVFX;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	/** 표창 개수 변경 시 호출 — 자식 컴포넌트 재생성 */
	UFUNCTION(BlueprintCallable, Category="Pyochang")
	void RebuildPyochangs();

	/** 외부 데미지 멀티플라이어 적용 (PlayerCharacter의 ProjectileDamageMultiplier 등) */
	UFUNCTION(BlueprintCallable, Category="Pyochang")
	void SetDamageMultiplier(float Multiplier);

private:
	/** Sphere collision components (회전체) */
	UPROPERTY()
	TArray<TObjectPtr<USphereComponent>> PyochangSpheres;

	/** 각 sphere에 attach된 Niagara VFX (visual) */
	UPROPERTY()
	TArray<TObjectPtr<UNiagaraComponent>> PyochangNiagaras;

	float CurrentRotation = 0.0f;
	float CachedDamageMultiplier = 1.0f;

	/** target → last hit time map */
	TMap<TWeakObjectPtr<AActor>, float> LastHitTimes;

	UFUNCTION()
	void OnPyochangBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	void ClearPyochangs();
};
