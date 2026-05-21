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

	AActor* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (!Player) return;

	// Magnet range 안에 한 번 들어오면 latched → 멀어져도 계속 따라옴
	if (!bMagnetLatched)
	{
		// PlayerCharacter의 XPMagnetBonus 적용
		float EffectiveRange = MagnetRange;
		if (AJYNPlayerCharacter* JYNPlayer = Cast<AJYNPlayerCharacter>(Player))
		{
			EffectiveRange += JYNPlayer->XPMagnetBonus;
		}

		const float DistSq = FVector::DistSquared(GetActorLocation(), Player->GetActorLocation());
		if (DistSq <= FMath::Square(EffectiveRange))
		{
			bMagnetLatched = true;
		}
	}

	// Latched 상태면 항상 플레이어 쪽으로 이동
	if (bMagnetLatched)
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
