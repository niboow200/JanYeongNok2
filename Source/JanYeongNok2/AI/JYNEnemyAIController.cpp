#include "JYNEnemyAIController.h"
#include "Components/StateTreeAIComponent.h"
#include "Navigation/PathFollowingComponent.h"

AJYNEnemyAIController::AJYNEnemyAIController()
{
	StateTreeAI = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));
	check(StateTreeAI);

	// StateTree 자동 시작 비활성화 — 추격/공격은 JYNEnemyBase의 Tick에서 직접 처리
	bStartAILogicOnPossess = false;

	bAttachToPawn = true;
}

void AJYNEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// PathFollowingComponent 비활성화 — NavMesh 경계에서 이동 제한 제거
	// (스폰은 GameMode가 NavMesh 안에서 처리, 이동은 자유)
	if (UPathFollowingComponent* PathFC = FindComponentByClass<UPathFollowingComponent>())
	{
		PathFC->Deactivate();
	}
}
