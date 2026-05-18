#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JYNProjectileBase.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

/**
 * 암기 투사체 베이스
 * - 유도 타겟이 있으면 HomingAcceleration으로 추적
 * - 유도 타겟이 없거나 원뿔 밖이면 직선 비행
 * - 적 적중 시 데미지 + 시전자 내공 소량 회복
 */
UCLASS(abstract)
class AJYNProjectileBase : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile")
	float Damage = 10.0f;

	/** 적중 시 시전자 내공 회복량 */
	UPROPERTY(EditAnywhere, Category="Projectile", meta=(ClampMin=0.0f))
	float NaegongRegainOnHit = 2.0f;

	/** 투사체 수명 (초) */
	UPROPERTY(EditAnywhere, Category="Projectile", meta=(ClampMin=0.1f))
	float LifeSpanSeconds = 3.0f;

protected:
	/** 유도 타겟 (없으면 직선 비행) */
	UPROPERTY(BlueprintReadOnly, Category="Projectile")
	TWeakObjectPtr<AActor> HomingTarget;

	/** 투사체를 발사한 캐릭터 */
	UPROPERTY(BlueprintReadOnly, Category="Projectile")
	TWeakObjectPtr<AActor> Instigator_JYN;

public:
	AJYNProjectileBase();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	/** 발사 시 호출: 방향 설정 + 유도 타겟 지정 */
	UFUNCTION(BlueprintCallable, Category="Projectile")
	void Launch(const FVector& Direction, AActor* InHomingTarget, AActor* InInstigator);

protected:
	UFUNCTION()
	void OnSphereHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	/** BP에서 적중 이펙트/사운드 구현 */
	UFUNCTION(BlueprintImplementableEvent, Category="Projectile", meta=(DisplayName="OnHitEnemy"))
	void BP_OnHitEnemy(AActor* HitEnemy);

	UFUNCTION(BlueprintImplementableEvent, Category="Projectile", meta=(DisplayName="OnHitWorld"))
	void BP_OnHitWorld();

private:
	void HandleHitActor(AActor* OtherActor, const FVector& HitDirection);
};
