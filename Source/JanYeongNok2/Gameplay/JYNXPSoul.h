#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JYNXPSoul.generated.h"

class USphereComponent;

/**
 * 적 사망 시 드롭되는 XP Soul 픽업
 * 플레이어가 겹치면 ExperienceComponent에 XP 추가 후 자동 파괴
 * BP에서 이펙트/사운드/메시 추가
 */
UCLASS()
class AJYNXPSoul : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> CollisionSphere;

public:
	/** 이 Soul이 주는 XP량 (EnemyBase.XPReward에서 설정 후 Spawn) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="XP")
	float XPAmount = 10.0f;

	/** 자동 흡수 거리 (플레이어가 이 거리 안에 있으면 자석처럼 이동) */
	UPROPERTY(EditAnywhere, Category="XP", meta=(ClampMin=0.0f, Units="cm"))
	float MagnetRange = 300.0f;

	/** 자석 이동 속도 */
	UPROPERTY(EditAnywhere, Category="XP", meta=(ClampMin=0.0f, Units="cm/s"))
	float MagnetSpeed = 600.0f;

public:
	AJYNXPSoul();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

protected:
	/** BP에서 수집 이펙트 구현 */
	UFUNCTION(BlueprintImplementableEvent, Category="XP", meta=(DisplayName="OnCollected"))
	void BP_OnCollected();

private:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	void Collect(AActor* Player);

	bool bCollected = false;
	bool bMagnetLatched = false;  // 한 번 magnet 시작하면 끝까지 따라옴 (sticky)
};
