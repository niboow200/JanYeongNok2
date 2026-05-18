#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "JYNStateTreeTasks.generated.h"

// ──────────────────────────────────────────────────────────────
// Task 1: 플레이어 추적 (바인딩 불필요 — Context.GetOwner()로 자동 해결)
// OnStateCompleted(Succeeded) → Attack State로 전환
// ──────────────────────────────────────────────────────────────

USTRUCT()
struct FJYNChasePlayerInstanceData
{
	GENERATED_BODY()

	/** 공격 판정 거리 (캡슐 반경 합계 ~84cm + AcceptanceRadius 보다 반드시 커야 함) */
	UPROPERTY(EditAnywhere, Category="Settings", meta=(ClampMin=50.0f, Units="cm"))
	float AttackRange = 250.0f;

	/** MoveToActor 허용 오차 (작을수록 바짝 붙음) */
	UPROPERTY(EditAnywhere, Category="Settings", meta=(ClampMin=5.0f, Units="cm"))
	float AcceptanceRadius = 20.0f;
};

USTRUCT(meta=(DisplayName="JYN Chase Player", Category="JYN"))
struct FJYNChasePlayerTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FJYNChasePlayerInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView,
		const IStateTreeBindingLookup& BindingLookup,
		EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};

// ──────────────────────────────────────────────────────────────
// Task 2: 근접 공격 (바인딩 불필요)
// OnStateCompleted(Succeeded) → Chase State로 복귀
// ──────────────────────────────────────────────────────────────

USTRUCT()
struct FJYNMeleeAttackInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Settings", meta=(ClampMin=1.0f))
	float Damage = 7.0f;

	UPROPERTY(EditAnywhere, Category="Settings", meta=(ClampMin=0.1f, Units="s"))
	float AttackCooldown = 1.5f;

	/** 공격이 유효한 최대 거리 (이 밖이면 Chase로 복귀) */
	UPROPERTY(EditAnywhere, Category="Settings", meta=(ClampMin=50.0f, Units="cm"))
	float MaxAttackRange = 350.0f;

	float ElapsedTime = 0.0f;
	bool bHasAttacked = false;
};

USTRUCT(meta=(DisplayName="JYN Melee Attack", Category="JYN"))
struct FJYNMeleeAttackTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FJYNMeleeAttackInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView,
		const IStateTreeBindingLookup& BindingLookup,
		EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif
};
