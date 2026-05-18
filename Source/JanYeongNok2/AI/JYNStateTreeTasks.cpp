#include "JYNStateTreeTasks.h"
#include "StateTreeExecutionContext.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Character/JYNPlayerCharacter.h"
#include "JYNEnemyBase.h"
#include "Animation/AnimInstance.h"

// ── 헬퍼: Context에서 AIController 가져오기 ──────────────────
// StateTreeAIComponent는 bAttachToPawn=true → Owner = Pawn(적 캐릭터)
static AAIController* GetAIControllerFromContext(const FStateTreeExecutionContext& Context)
{
	if (const UObject* Owner = Context.GetOwner())
	{
		if (const APawn* Pawn = Cast<APawn>(Owner))
		{
			return Cast<AAIController>(Pawn->GetController());
		}
		// Owner가 AIController 자체인 경우
		if (AAIController* AICon = const_cast<AAIController*>(Cast<AAIController>(Owner)))
		{
			return AICon;
		}
	}
	return nullptr;
}

// ── 헬퍼: 플레이어 캐릭터 가져오기 ──────────────────────────
static ACharacter* GetPlayerCharacter(const FStateTreeExecutionContext& Context)
{
	if (const UObject* Owner = Context.GetOwner())
	{
		if (UWorld* World = Owner->GetWorld())
		{
			return Cast<ACharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
		}
	}
	return nullptr;
}

// ──────────────────────────────────────────────────────────────
// FJYNChasePlayerTask
// ──────────────────────────────────────────────────────────────

EStateTreeRunStatus FJYNChasePlayerTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	AAIController* AICon = GetAIControllerFromContext(Context);
	ACharacter* Player = GetPlayerCharacter(Context);

	if (!AICon || !Player) return EStateTreeRunStatus::Failed;

	AICon->MoveToActor(Player, Data.AcceptanceRadius, true, true, true);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FJYNChasePlayerTask::Tick(FStateTreeExecutionContext& Context, float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	AAIController* AICon = GetAIControllerFromContext(Context);
	ACharacter* Player = GetPlayerCharacter(Context);

	if (!AICon || !Player) return EStateTreeRunStatus::Failed;

	APawn* OwnerPawn = AICon->GetPawn();
	if (!OwnerPawn) return EStateTreeRunStatus::Failed;

	// 공격 거리 진입 → 즉시 정지 후 Attack State로 전환
	const float DistSq = FVector::DistSquared(OwnerPawn->GetActorLocation(), Player->GetActorLocation());
	if (DistSq <= FMath::Square(Data.AttackRange))
	{
		AICon->StopMovement();
		// 관성 제거: CharacterMovement 속도를 즉시 0으로
		if (ACharacter* EnemyChar = Cast<ACharacter>(OwnerPawn))
		{
			EnemyChar->GetCharacterMovement()->StopMovementImmediately();
		}
		return EStateTreeRunStatus::Succeeded;
	}

	// 경로가 끊기면 재요청
	if (AICon->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		AICon->MoveToActor(Player, Data.AcceptanceRadius, true, true, true);
	}

	return EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FJYNChasePlayerTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView,
	const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	return FText::FromString(TEXT("JYN: Chase Player → Attack when in range"));
}
#endif

// ──────────────────────────────────────────────────────────────
// FJYNMeleeAttackTask
// ──────────────────────────────────────────────────────────────

EStateTreeRunStatus FJYNMeleeAttackTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ElapsedTime = 0.0f;
	Data.bHasAttacked = false;

	// 공격 애니메이션 몽타주 재생
	if (AAIController* AICon = GetAIControllerFromContext(Context))
	{
		if (AJYNEnemyBase* Enemy = Cast<AJYNEnemyBase>(AICon->GetPawn()))
		{
			if (Enemy->AttackMontage)
			{
				if (UAnimInstance* AnimInst = Enemy->GetMesh()->GetAnimInstance())
				{
					AnimInst->Montage_Play(Enemy->AttackMontage, 1.0f);
				}
			}
		}
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FJYNMeleeAttackTask::Tick(FStateTreeExecutionContext& Context, float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	AAIController* AICon = GetAIControllerFromContext(Context);
	ACharacter* Player = GetPlayerCharacter(Context);

	if (!AICon || !Player) return EStateTreeRunStatus::Failed;

	APawn* OwnerPawn = AICon->GetPawn();
	if (!OwnerPawn) return EStateTreeRunStatus::Failed;

	// 플레이어가 너무 멀어졌으면 Chase로 복귀
	const float DistSq = FVector::DistSquared(OwnerPawn->GetActorLocation(), Player->GetActorLocation());
	if (DistSq > FMath::Square(Data.MaxAttackRange))
	{
		return EStateTreeRunStatus::Succeeded;
	}

	Data.ElapsedTime += DeltaTime;

	// 공격 중 매 프레임 이동 억제 (루트 모션/관성 차단)
	if (ACharacter* EnemyChar = Cast<ACharacter>(OwnerPawn))
	{
		EnemyChar->GetCharacterMovement()->StopMovementImmediately();
	}

	// 진입 즉시 1회 공격
	if (!Data.bHasAttacked)
	{
		Data.bHasAttacked = true;

		if (AJYNPlayerCharacter* JYNPlayer = Cast<AJYNPlayerCharacter>(Player))
		{
			const FVector HitDir = (Player->GetActorLocation() - OwnerPawn->GetActorLocation()).GetSafeNormal();
			JYNPlayer->TakeDamageJYN(Data.Damage, HitDir);
		}
	}

	// 쿨타임 완료 → Chase로 복귀
	if (Data.ElapsedTime >= Data.AttackCooldown)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FJYNMeleeAttackTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView,
	const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	return FText::FromString(TEXT("JYN: Melee Attack → Chase when done"));
}
#endif
