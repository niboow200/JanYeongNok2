#include "JYNEnemyAIController.h"
#include "Components/StateTreeAIComponent.h"

AJYNEnemyAIController::AJYNEnemyAIController()
{
	StateTreeAI = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));
	check(StateTreeAI);

	// StateTree 자동 시작 비활성화 — 추격/공격은 JYNEnemyBase의 Tick에서 직접 처리
	bStartAILogicOnPossess = false;

	bAttachToPawn = true;
}
