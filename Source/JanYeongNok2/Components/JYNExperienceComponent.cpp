#include "JYNExperienceComponent.h"

UJYNExperienceComponent::UJYNExperienceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UJYNExperienceComponent::AddXP(float Amount)
{
	if (Amount <= 0.0f) return;

	CurrentXP += Amount;
	OnXPChanged.Broadcast(CurrentXP, GetXPRequired());
	CheckLevelUp();
}

float UJYNExperienceComponent::GetXPRequired() const
{
	return BaseXPRequired * FMath::Pow(XPScalingFactor, CurrentLevel - 1);
}

float UJYNExperienceComponent::GetXPRatio() const
{
	const float Required = GetXPRequired();
	return Required > 0.0f ? FMath::Clamp(CurrentXP / Required, 0.0f, 1.0f) : 0.0f;
}

void UJYNExperienceComponent::CheckLevelUp()
{
	const float Required = GetXPRequired();
	if (CurrentXP >= Required)
	{
		CurrentXP -= Required;
		CurrentLevel++;
		OnLevelUp.Broadcast(CurrentLevel);

		// 남은 XP로 연속 레벨업 체크
		CheckLevelUp();
	}
}
