#include "JYNEnemyBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Gameplay/JYNXPSoul.h"
#include "Character/JYNPlayerCharacter.h"
#include "Engine/World.h"
#include "AIController.h"
#include "Animation/AnimMontage.h"
#include "TimerManager.h"
#include "Components/CapsuleComponent.h"

AJYNEnemyBase::AJYNEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;  // 접촉 데미지 cooldown 체크용

	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 480.0f, 0.0f);

	// Profile을 Custom으로 변경해야 채널 응답 변경이 적용됨
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Custom"));
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AJYNEnemyBase::BeginPlay()
{
	Super::BeginPlay();
	CurrentHP = MaxHP;
	DefaultMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;

	// BP/CDO 설정 이후에 강제로 Custom + Pawn Overlap (BP가 'Pawn' 프리셋을 덮어쓰는 문제 해결)
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Custom"));
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// 캡슐 Overlap 이벤트 바인딩 — 플레이어와 접촉 시 데미지
	GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(
		this, &AJYNEnemyBase::OnCapsuleBeginOverlap);
}

void AJYNEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsDead) return;

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastTouchDamageTime < TouchDamageCooldown) return;

	// Overlap 지속 중에도 cooldown마다 데미지 갱신
	TArray<AActor*> Overlapping;
	GetCapsuleComponent()->GetOverlappingActors(Overlapping, AJYNPlayerCharacter::StaticClass());
	for (AActor* Other : Overlapping)
	{
		if (AJYNPlayerCharacter* Player = Cast<AJYNPlayerCharacter>(Other))
		{
			LastTouchDamageTime = Now;
			const FVector HitDir = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
			Player->TakeDamageJYN(TouchDamage, HitDir);
			break;
		}
	}
}

void AJYNEnemyBase::OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (bIsDead || !OtherActor) return;

	AJYNPlayerCharacter* Player = Cast<AJYNPlayerCharacter>(OtherActor);
	if (!Player) return;

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastTouchDamageTime < TouchDamageCooldown) return;

	LastTouchDamageTime = Now;
	const FVector HitDir = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	Player->TakeDamageJYN(TouchDamage, HitDir);
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
		return ActualDamage;
	}

	// 피격 상태 마크 — ABP가 매 틱 IsHitStunned()로 읽어서 GetHit State로 전환
	bIsHitStunned = true;

	// AI 이동 중단 + 속도 0으로
	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		AICon->StopMovement();
	}
	GetCharacterMovement()->MaxWalkSpeed = 0.0f;

	// HitStunDuration 후 복구
	GetWorld()->GetTimerManager().SetTimer(
		HitStunTimerHandle, this, &AJYNEnemyBase::EndHitStun, HitStunDuration, false);

	return ActualDamage;
}

void AJYNEnemyBase::EndHitStun()
{
	if (bIsDead) return;
	bIsHitStunned = false;
	GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed;
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
