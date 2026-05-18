#include "JYNProjectileBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AI/JYNEnemyBase.h"
#include "Character/JYNPlayerCharacter.h"
#include "Components/JYNNaegongComponent.h"

AJYNProjectileBase::AJYNProjectileBase()
{
	PrimaryActorTick.bCanEverTick = true; // 타겟 소멸 감지용

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetSphereRadius(16.0f);
	// Pawn만 감지 (바닥/World에 닿아 즉시 파괴되는 문제 방지)
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RootComponent = CollisionSphere;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 1200.0f;
	ProjectileMovement->MaxSpeed = 1200.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.0f;

	// 유도 설정 (Launch에서 활성화)
	ProjectileMovement->bIsHomingProjectile = false;
	ProjectileMovement->HomingAccelerationMagnitude = 3000.0f;
}

void AJYNProjectileBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 유도 타겟이 소멸했거나 이미 죽었으면 유도 해제 → 직선 비행 유지
	// IsValid()만으로는 부족 — SetLifeSpan으로 지연 파괴되는 적은 bIsDead=true여도 Actor가 살아있음
	if (ProjectileMovement->bIsHomingProjectile)
	{
		bool bLostTarget = !HomingTarget.IsValid();

		if (!bLostTarget)
		{
			if (AJYNEnemyBase* Enemy = Cast<AJYNEnemyBase>(HomingTarget.Get()))
			{
				bLostTarget = Enemy->IsDead();
			}
		}

		if (bLostTarget)
		{
			ProjectileMovement->bIsHomingProjectile = false;
			ProjectileMovement->HomingTargetComponent = nullptr;
			// 현재 속도 방향 그대로 직선 비행
		}
	}
}

void AJYNProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	// BP에서 추가된 메시 컴포넌트의 충돌을 NoCollision으로 강제 설정
	// KunaiMesh 등 비주얼 메시가 플레이어를 막는 것을 방지
	TArray<UStaticMeshComponent*> MeshComps;
	GetComponents<UStaticMeshComponent>(MeshComps);
	for (UStaticMeshComponent* Mesh : MeshComps)
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	CollisionSphere->OnComponentHit.AddDynamic(this, &AJYNProjectileBase::OnSphereHit);
	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AJYNProjectileBase::OnSphereBeginOverlap);

	SetLifeSpan(LifeSpanSeconds);
}

void AJYNProjectileBase::Launch(const FVector& Direction, AActor* InHomingTarget, AActor* InInstigator)
{
	Instigator_JYN = InInstigator;

	if (InHomingTarget)
	{
		HomingTarget = InHomingTarget;
		ProjectileMovement->bIsHomingProjectile = true;
		ProjectileMovement->HomingTargetComponent = InHomingTarget->GetRootComponent();
	}

	ProjectileMovement->Velocity = Direction.GetSafeNormal() * ProjectileMovement->InitialSpeed;
}

void AJYNProjectileBase::OnSphereHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	HandleHitActor(OtherActor, Hit.ImpactNormal * -1.0f);
}

void AJYNProjectileBase::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	HandleHitActor(OtherActor, ProjectileMovement->Velocity.GetSafeNormal());
}

void AJYNProjectileBase::HandleHitActor(AActor* OtherActor, const FVector& HitDirection)
{
	if (!OtherActor || OtherActor == Instigator_JYN.Get()) return;

	// Pawn 채널만 감지하므로 적 캐릭터만 여기 도달
	if (AJYNEnemyBase* Enemy = Cast<AJYNEnemyBase>(OtherActor))
	{
		Enemy->TakeDamageJYN(Damage, HitDirection);

		// 시전자 내공 소량 회복
		if (AJYNPlayerCharacter* Player = Cast<AJYNPlayerCharacter>(Instigator_JYN.Get()))
		{
			Player->RegainNaegongOnHit(NaegongRegainOnHit);
		}

		BP_OnHitEnemy(OtherActor);
		Destroy();
	}
	// WorldStatic/바닥은 무시 (Pawn 채널만 감지하므로 이 else는 사실상 불필요)
}
