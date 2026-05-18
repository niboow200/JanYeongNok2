#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JYNExperienceComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnXPChanged, float, Current, float, Required);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLevelUp, int32, NewLevel);

/**
 * 경험치 & 레벨업 컴포넌트
 * - 적 처치 시 XP 획득
 * - 레벨업 조건 충족 시 OnLevelUp 브로드캐스트
 * - GameMode가 OnLevelUp을 받아 화면 정지 + 무공 카드 선택 UI 호출
 */
UCLASS(ClassGroup=(JYN), meta=(BlueprintSpawnableComponent))
class UJYNExperienceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category="Experience")
	int32 CurrentLevel = 1;

	UPROPERTY(BlueprintReadOnly, Category="Experience")
	float CurrentXP = 0.0f;

	/** 레벨업에 필요한 기본 XP */
	UPROPERTY(EditAnywhere, Category="Experience", meta=(ClampMin=1.0f))
	float BaseXPRequired = 20.0f;

	/** 레벨업마다 필요 XP 증가 배율 */
	UPROPERTY(EditAnywhere, Category="Experience", meta=(ClampMin=1.0f))
	float XPScalingFactor = 1.2f;

	UPROPERTY(BlueprintAssignable, Category="Experience")
	FOnXPChanged OnXPChanged;

	/** 레벨업 → GameMode가 구독해서 카드 선택 UI 띄움 */
	UPROPERTY(BlueprintAssignable, Category="Experience")
	FOnLevelUp OnLevelUp;

public:
	UJYNExperienceComponent();

public:
	/** XP 추가. XP Soul 픽업 시 호출 */
	UFUNCTION(BlueprintCallable, Category="Experience")
	void AddXP(float Amount);

	UFUNCTION(BlueprintPure, Category="Experience")
	float GetXPRequired() const;

	UFUNCTION(BlueprintPure, Category="Experience")
	float GetXPRatio() const;

private:
	void CheckLevelUp();
};
