#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "JYNEnemyAIController.generated.h"

class UStateTreeAIComponent;

/**
 * 잔영록 적 AI 컨트롤러
 * StateTree 기반으로 플레이어 추적/공격 행동 처리
 * 실제 행동 로직은 ST_JYNEnemy (StateTree 에셋)에서 정의
 */
UCLASS()
class AJYNEnemyAIController : public AAIController
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStateTreeAIComponent> StateTreeAI;

public:
	AJYNEnemyAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
};
