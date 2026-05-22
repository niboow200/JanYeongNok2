#include "JYNPyochangOrbit.h"
#include "Components/SphereComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "AI/JYNEnemyBase.h"
#include "Engine/World.h"

AJYNPyochangOrbit::AJYNPyochangOrbit()
{
	PrimaryActorTick.bCanEverTick = true;

	// 루트 (단순 SceneComponent)
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	// 부모(캐릭터) 회전 무시 — 위치만 따라가고 회전은 자체적으로만 (Tick 회전 알고리즘 사용)
	SceneRoot->SetUsingAbsoluteRotation(true);
}

void AJYNPyochangOrbit::BeginPlay()
{
	Super::BeginPlay();

	// 부모(캐릭터) 회전 무시 — 위치만 따라가고 회전은 자체 Tick 알고리즘으로만 (강제 적용)
	if (USceneComponent* Root = GetRootComponent())
	{
		Root->SetUsingAbsoluteRotation(true);
		Root->SetWorldRotation(FRotator::ZeroRotator);
	}

	RebuildPyochangs();
}

void AJYNPyochangOrbit::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 회전 각도 누적
	CurrentRotation += RotationSpeed * DeltaTime;
	if (CurrentRotation > 360.0f) CurrentRotation -= 360.0f;

	const int32 Count = PyochangSpheres.Num();
	if (Count == 0 || DeltaTime <= 0.0f) return;

	const float AngleStep = 360.0f / Count;

	// 각 표창 위치 갱신 + velocity 강제 update (Niagara trail이 인식하도록)
	for (int32 i = 0; i < Count; i++)
	{
		USphereComponent* Sphere = PyochangSpheres[i];
		if (!Sphere) continue;

		const float AngleDeg = CurrentRotation + AngleStep * i;
		const float AngleRad = FMath::DegreesToRadians(AngleDeg);

		const FVector NewLocalPos(
			FMath::Cos(AngleRad) * OrbitRadius,
			FMath::Sin(AngleRad) * OrbitRadius,
			0.0f
		);

		// 이전 world position 기록
		const FVector OldWorldPos = Sphere->GetComponentLocation();

		Sphere->SetRelativeLocation(NewLocalPos);

		// Velocity 강제 update (Niagara Trail emitter가 사용)
		const FVector NewWorldPos = Sphere->GetComponentLocation();
		const FVector Velocity = (NewWorldPos - OldWorldPos) / DeltaTime;
		Sphere->ComponentVelocity = Velocity;
	}
}

void AJYNPyochangOrbit::RebuildPyochangs()
{
	ClearPyochangs();

	for (int32 i = 0; i < PyochangCount; i++)
	{
		// Sphere collision (visual mesh 없음 - Niagara가 visual)
		const FName SphereName = *FString::Printf(TEXT("Pyochang_%d_Sphere"), i);
		USphereComponent* Sphere = NewObject<USphereComponent>(this, SphereName);
		Sphere->SetupAttachment(GetRootComponent());
		Sphere->SetSphereRadius(PyochangRadius);
		Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
		Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		Sphere->SetGenerateOverlapEvents(true);
		Sphere->OnComponentBeginOverlap.AddDynamic(this, &AJYNPyochangOrbit::OnPyochangBeginOverlap);
		Sphere->RegisterComponent();
		PyochangSpheres.Add(Sphere);

		// Niagara VFX (visual) — sphere에 attach
		if (PyochangVFX)
		{
			UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(
				PyochangVFX,
				Sphere,
				NAME_None,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::KeepRelativeOffset,
				/*bAutoDestroy=*/ false
			);
			if (NiagaraComp)
			{
				PyochangNiagaras.Add(NiagaraComp);
			}
		}
	}

	CurrentRotation = 0.0f;
}

void AJYNPyochangOrbit::ClearPyochangs()
{
	for (UNiagaraComponent* NS : PyochangNiagaras)
	{
		if (NS)
		{
			NS->DestroyInstance();
			NS->DestroyComponent();
		}
	}
	PyochangNiagaras.Empty();

	for (USphereComponent* Sphere : PyochangSpheres)
	{
		if (Sphere) Sphere->DestroyComponent();
	}
	PyochangSpheres.Empty();

	LastHitTimes.Empty();
}

void AJYNPyochangOrbit::SetDamageMultiplier(float Multiplier)
{
	CachedDamageMultiplier = FMath::Max(0.0f, Multiplier);
}

void AJYNPyochangOrbit::OnPyochangBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor) return;

	AJYNEnemyBase* Enemy = Cast<AJYNEnemyBase>(OtherActor);
	if (!Enemy) return;

	const float Now = GetWorld()->GetTimeSeconds();
	if (float* Last = LastHitTimes.Find(Enemy))
	{
		if (Now - *Last < HitCooldownPerTarget) return;
	}
	LastHitTimes.Add(Enemy, Now);

	const FVector HitDir = (Enemy->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	const float TotalDamage = Damage * CachedDamageMultiplier;
	Enemy->TakeDamageJYN(TotalDamage, HitDir);
}
