#include "JYNEnemyBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Gameplay/JYNXPSoul.h"
#include "Engine/World.h"

AJYNEnemyBase::AJYNEnemyBase()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 480.0f, 0.0f);
}

void AJYNEnemyBase::BeginPlay()
{
	Super::BeginPlay();
	CurrentHP = MaxHP;
}

float AJYNEnemyBase::TakeDamageJYN(float Damage, const FVector& HitDirection)
{
	if (bIsDead || Damage <= 0.0f) return 0.0f;

	const float ActualDamage = FMath::Max(0.0f, Damage);
	CurrentHP = FMath::Max(0.0f, CurrentHP - ActualDamage);

	if (KnockbackResistance < 1.0f)
	{
		const float KnockbackForce = 400.0f * (1.0f - KnockbackResistance);
		FVector LaunchDir = HitDirection;
		LaunchDir.Z = 0.0f;
		LaunchCharacter(LaunchDir.GetSafeNormal() * KnockbackForce, true, true);
	}

	BP_OnDamaged(CurrentHP);

	if (CurrentHP <= 0.0f)
	{
		Die();
	}

	return ActualDamage;
}

void AJYNEnemyBase::Die()
{
	if (bIsDead) return;
	bIsDead = true; // ABP가 매 틱 IsDead()로 읽어서 Death State로 전환

	DetachFromControllerPendingDestroy();
	SetActorEnableCollision(false);
	GetCharacterMovement()->DisableMovement();

	// XP Soul 드롭
	if (XPSoulClass)
	{
		FVector SoulLocation = GetActorLocation();
		SoulLocation.Z += 50.0f;

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AJYNXPSoul* Soul = GetWorld()->SpawnActor<AJYNXPSoul>(XPSoulClass, FTransform(SoulLocation), Params))
		{
			Soul->XPAmount = XPReward;
		}
	}

	OnEnemyDied.Broadcast(this);
	BP_OnDied();

	// DeathDestroyDelay 후 파괴 (ABP에서 사망 애니 재생되는 시간)
	SetLifeSpan(DeathDestroyDelay);
}
