#include "JYNEnemyBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Gameplay/JYNXPSoul.h"
#include "Character/JYNPlayerCharacter.h"
#include "Engine/World.h"
#include "AIController.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "TimerManager.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"

AJYNEnemyBase::AJYNEnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;  // 추격 + 공격 (StateTree 대체)

	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 480.0f, 0.0f);

	// Profile을 Custom으로 변경해야 채널 응답 변경이 적용됨
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Custom"));
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// Animation URO - 화면 밖에서는 ABP 평가 안 함 (성능 최적화)
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	}
}

void AJYNEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	// 명시적으로 살아있는 상태로 초기화 (ABP가 첫 틱부터 IsDead() 호출하므로 안전)
	bIsDead = false;
	bIsHitStunned = false;
	CurrentHP = MaxHP;
	DefaultMaxWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;

	// BP/CDO 설정 이후에 강제로 Custom + Pawn Overlap
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Custom"));
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// 캡슐 Overlap 이벤트 바인딩 — Begin/End 양쪽
	GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(
		this, &AJYNEnemyBase::OnCapsuleBeginOverlap);
	GetCapsuleComponent()->OnComponentEndOverlap.AddDynamic(
		this, &AJYNEnemyBase::OnCapsuleEndOverlap);
}

void AJYNEnemyBase::OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (bIsDead || !OtherActor) return;

	AJYNPlayerCharacter* Player = Cast<AJYNPlayerCharacter>(OtherActor);
	if (!Player) return;

	OverlappingPlayer = Player;

	// 즉시 1회 데미지 + cooldown 후 다시 적용되는 repeat timer
	ApplyTouchDamageToPlayer();
	GetWorld()->GetTimerManager().SetTimer(
		TouchDamageTimerHandle, this, &AJYNEnemyBase::ApplyTouchDamageToPlayer,
		TouchDamageCooldown, true);
}

void AJYNEnemyBase::OnCapsuleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// 플레이어가 멀어지면 timer 중단
	if (OtherActor == OverlappingPlayer.Get())
	{
		OverlappingPlayer.Reset();
		GetWorld()->GetTimerManager().ClearTimer(TouchDamageTimerHandle);
	}
}

void AJYNEnemyBase::ApplyTouchDamageToPlayer()
{
	if (bIsDead || !OverlappingPlayer.IsValid()) return;

	AJYNPlayerCharacter* Player = OverlappingPlayer.Get();
	const FVector HitDir = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	Player->TakeDamageJYN(TouchDamage, HitDir);
}

// ── 추격 + 공격 (StateTree 대체) ────────────────────────

void AJYNEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 사망/피격 스턴 중에는 AI 정지
	if (bIsDead || bIsHitStunned) return;

	// 플레이어 검색
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn) return;

	AJYNPlayerCharacter* Player = Cast<AJYNPlayerCharacter>(PlayerPawn);
	if (!Player) return;

	const FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
	const float Distance2D = ToPlayer.Size2D();

	if (Distance2D > AttackRange)
	{
		// 추격 — 단순 직선 (CharacterMovement가 지형/언덕 자동 처리)
		const FVector Direction = ToPlayer.GetSafeNormal2D();
		AddMovementInput(Direction, 1.0f);
	}
	else
	{
		// 공격 사정거리 내 — 쿨타임 충족 시 공격
		const float Now = GetWorld()->GetTimeSeconds();
		if (Now - LastAttackTime >= AttackCooldown)
		{
			LastAttackTime = Now;
			PerformAttack(Player);
		}
	}
}

void AJYNEnemyBase::PerformAttack(AJYNPlayerCharacter* Player)
{
	if (!Player) return;

	// 공격 중 이동 멈춤
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
	}

	// 공격 몽타주 재생 (ABP의 DefaultSlot에서 자동 표시)
	if (AttackMontage)
	{
		if (USkeletalMeshComponent* MeshComp = GetMesh())
		{
			if (UAnimInstance* AnimInst = MeshComp->GetAnimInstance())
			{
				AnimInst->Montage_Play(AttackMontage);
			}
		}
	}

	// 플레이어에게 데미지
	const FVector HitDir = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	Player->TakeDamageJYN(AttackDamage, HitDir);
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

	// DeathDestroyDelay 후 풀로 비활성화 반환 (Destroy 안 함 — 재사용)
	FTimerHandle DeactivateTimer;
	GetWorld()->GetTimerManager().SetTimer(
		DeactivateTimer, this, &AJYNEnemyBase::DeactivateForPool, DeathDestroyDelay, false);
}

// ── Object Pool 지원 ──────────────────────────────────────

void AJYNEnemyBase::DeactivateForPool()
{
	// 모든 활동 중단 + 화면에서 숨김 (Destroy 대신)
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->DisableMovement();
		MoveComp->StopMovementImmediately();
	}

	// 모든 타이머 정리
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	OverlappingPlayer.Reset();
}

void AJYNEnemyBase::ResetForPool(const FVector& NewLocation)
{
	// 1) 상태 플래그 먼저 초기화 (ABP가 Tick에서 IsDead()/IsHitStunned() 호출하므로)
	bIsDead = false;
	bIsHitStunned = false;
	CurrentHP = MaxHP;
	LastAttackTime = -999.0f;
	OverlappingPlayer.Reset();

	// 2) 위치 + Movement 설정
	SetActorLocation(NewLocation);
	SetActorEnableCollision(true);
	SetActorTickEnabled(true);

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Walking);
		MoveComp->MaxWalkSpeed = DefaultMaxWalkSpeed;
	}

	// 3) ABP 완전 재생성 (Hidden 상태에서 → 사용자가 stale pose 못 봄)
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (UAnimInstance* AnimInst = MeshComp->GetAnimInstance())
		{
			AnimInst->StopAllMontages(0.0f);
		}

		UClass* AnimClass = MeshComp->GetAnimClass();
		if (AnimClass)
		{
			MeshComp->SetAnimInstanceClass(nullptr);
			MeshComp->SetAnimInstanceClass(AnimClass);
		}

		// 강제로 한 프레임 tick하여 새 pose(Idle)로 갱신 후 visible 처리
		MeshComp->TickAnimation(0.0f, false);
		MeshComp->RefreshBoneTransforms();
	}

	// 4) AI Controller 재생성 + PathFollowing 강제 비활성화
	if (!GetController())
	{
		SpawnDefaultController();
	}
	if (AAIController* AIC = Cast<AAIController>(GetController()))
	{
		if (UPathFollowingComponent* PathFC = AIC->FindComponentByClass<UPathFollowingComponent>())
		{
			PathFC->Deactivate();
		}
	}

	// 5) 마지막에 visible 처리 (새 pose가 이미 적용된 상태)
	SetActorHiddenInGame(false);
}
