#include "JYNXPSoul.h"
#include "Components/SphereComponent.h"
#include "Character/JYNPlayerCharacter.h"
#include "Components/JYNExperienceComponent.h"
#include "Kismet/GameplayStatics.h"

AJYNXPSoul::AJYNXPSoul()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetSphereRadius(40.0f);
	CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	RootComponent = CollisionSphere;
}

void AJYNXPSoul::BeginPlay()
{
	Super::BeginPlay();

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AJYNXPSoul::OnBeginOverlap);
	// LifeSpan 없음 — 플레이어가 먹을 때까지 영구 유지
}

void AJYNXPSoul::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bCollected) return;

	// 자석 효과: 플레이어가 MagnetRange 안에 있으면 Soul이 플레이어 쪽으로 이동
	AActor* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (!Player) return;

	const float DistSq = FVector::DistSquared(GetActorLocation(), Player->GetActorLocation());
	if (DistSq <= FMath::Square(MagnetRange))
	{
		const FVector Dir = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		SetActorLocation(GetActorLocation() + Dir * MagnetSpeed * DeltaTime);
	}
}

void AJYNXPSoul::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (Cast<AJYNPlayerCharacter>(OtherActor))
	{
		Collect(OtherActor);
	}
}

void AJYNXPSoul::Collect(AActor* Player)
{
	if (bCollected) return;
	bCollected = true;

	if (AJYNPlayerCharacter* JYNPlayer = Cast<AJYNPlayerCharacter>(Player))
	{
		if (JYNPlayer->ExperienceComponent)
		{
			JYNPlayer->ExperienceComponent->AddXP(XPAmount);
		}
	}

	BP_OnCollected();
	Destroy();
}
