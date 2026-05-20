#include "JYNNaegongComponent.h"

UJYNNaegongComponent::UJYNNaegongComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UJYNNaegongComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentNaegong = MaxNaegong;
}

void UJYNNaegongComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 시간 자동 회복
	if (CurrentNaegong < MaxNaegong)
	{
		SetNaegong(CurrentNaegong + RegenPerSecond * DeltaTime);
	}
}

float UJYNNaegongComponent::AbsorbDamage(float Damage)
{
	if (Damage <= 0.0f) return 0.0f;

	// AbsorbMultiplier만큼 더 효율적으로 흡수
	// 예: Multiplier=1.25이면 내공 1당 1.25 피해 흡수, 내공 소모는 흡수량/1.25
	const float MaxAbsorbable = CurrentNaegong * AbsorbMultiplier;
	const float Absorbed      = FMath::Min(MaxAbsorbable, Damage);
	const float NaegongCost   = Absorbed / AbsorbMultiplier;
	SetNaegong(CurrentNaegong - NaegongCost);

	return Damage - Absorbed;
}

void UJYNNaegongComponent::RegainOnHit(float Amount)
{
	if (Amount <= 0.0f) return;
	SetNaegong(FMath::Min(MaxNaegong, CurrentNaegong + Amount));
}

float UJYNNaegongComponent::UseNaegong(float Amount)
{
	if (Amount <= 0.0f) return 0.0f;
	const float ActualUsed = FMath::Min(Amount, CurrentNaegong);
	SetNaegong(CurrentNaegong - ActualUsed);
	return ActualUsed;
}

void UJYNNaegongComponent::SetNaegong(float NewValue)
{
	CurrentNaegong = FMath::Clamp(NewValue, 0.0f, MaxNaegong);
	OnNaegongChanged.Broadcast(CurrentNaegong, MaxNaegong);
}
