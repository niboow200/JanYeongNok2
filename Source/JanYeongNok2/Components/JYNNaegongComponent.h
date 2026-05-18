#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JYNNaegongComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNaegongChanged, float, Current, float, Max);

/**
 * 내공(內功) 컴포넌트
 * - 피격 시 HP보다 먼저 흡수
 * - 임계점(ThresholdRatio) 이상이면 무공 자동 강화
 * - 시간 자동 회복 + 평타 적중 소량 회복
 * - UI에 직접 표시하지 않음 (임계점 여부는 무공 효과로만 표현)
 */
UCLASS(ClassGroup=(JYN), meta=(BlueprintSpawnableComponent))
class UJYNNaegongComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Naegong")
	float MaxNaegong = 50.0f;

	UPROPERTY(BlueprintReadOnly, Category="Naegong")
	float CurrentNaegong = 50.0f;

	/** 초당 자동 회복량 */
	UPROPERTY(EditAnywhere, Category="Naegong", meta=(ClampMin=0.0f))
	float RegenPerSecond = 2.0f;

	/** 피격 흡수 효율 배율 (1.0 = 1:1, 1.25 = 25% 더 효율적으로 흡수)
	 *  AbsorbMultiplier 만큼 피해를 흡수하되, 내공 소모는 흡수량 / AbsorbMultiplier */
	UPROPERTY(BlueprintReadOnly, Category="Naegong")
	float AbsorbMultiplier = 1.0f;

	/** 내공 변경 시 브로드캐스트 (HUD 갱신용) */
	UPROPERTY(BlueprintAssignable, Category="Naegong")
	FOnNaegongChanged OnNaegongChanged;

public:
	UJYNNaegongComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	/**
	 * 피격 시 내공이 먼저 흡수. 내공 초과분(HP로 가야 할 데미지)을 반환.
	 * 내공이 충분하면 반환값 = 0
	 */
	UFUNCTION(BlueprintCallable, Category="Naegong")
	float AbsorbDamage(float Damage);

	/** 평타 적중 시 소량 내공 회복 */
	UFUNCTION(BlueprintCallable, Category="Naegong")
	void RegainOnHit(float Amount);

	UFUNCTION(BlueprintPure, Category="Naegong")
	float GetNaegongRatio() const { return MaxNaegong > 0.0f ? CurrentNaegong / MaxNaegong : 0.0f; }

private:
	void SetNaegong(float NewValue);
};
