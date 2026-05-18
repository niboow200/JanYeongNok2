#include "JYNEnemyAIController.h"
#include "Components/StateTreeAIComponent.h"

AJYNEnemyAIController::AJYNEnemyAIController()
{
	StateTreeAI = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));
	check(StateTreeAI);

	// 빙의 즉시 StateTree 시작
	bStartAILogicOnPossess = true;

	// EnvQuery 정확도를 위해 Pawn에 붙임
	bAttachToPawn = true;
}
