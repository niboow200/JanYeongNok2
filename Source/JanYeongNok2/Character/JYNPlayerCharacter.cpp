#include "JYNPlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Components/JYNNaegongComponent.h"
#include "Components/JYNExperienceComponent.h"
#include "Abilities/JYNProjectileBase.h"
#include "Abilities/JYNPyochangOrbit.h"
#include "AI/JYNEnemyBase.h"
#include "UI/JYNLevelUpScreen.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/GameModeBase.h"
#include "Gameplay/JYNGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"

AJYNPlayerCharacter::AJYNPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// ── 카메라 ──────────────────────────────────────────
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->SetRelativeRotation(FRotator(-70.0f, 0.0f, 0.0f));
	SpringArm->TargetArmLength = 1500.0f;
	SpringArm->bDoCollisionTest = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritRoll = false;
	SpringArm->bInheritYaw = false;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 5.0f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
	Camera->SetFieldOfView(75.0f);

	// ── 컴포넌트 ──────────────────────────────────────────
	NaegongComponent = CreateDefaultSubobject<UJYNNaegongComponent>(TEXT("NaegongComponent"));
	ExperienceComponent = CreateDefaultSubobject<UJYNExperienceComponent>(TEXT("ExperienceComponent"));

	// ── 이동 ──────────────────────────────────────────
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 640.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 500.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f;

	// 캐릭터가 컨트롤러 회전을 따르지 않음 (이동 방향을 바라봄)
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Profile을 Custom으로 변경해야 채널 응답 변경이 적용됨
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Custom"));
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AJYNPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	CurrentHP = MaxHP;

	// BP/CDO 설정 이후에 강제로 Custom + Pawn Overlap (BP가 'Pawn' 프리셋을 덮어쓰는 문제 해결)
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Custom"));
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// 이동 기본값 저장 (경공 종료 후 복구용)
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	DefaultMaxWalkSpeed       = MoveComp->MaxWalkSpeed;
	DefaultMaxAcceleration    = MoveComp->MaxAcceleration;
	DefaultBrakingDeceleration= MoveComp->BrakingDecelerationWalking;
	DefaultGroundFriction     = MoveComp->GroundFriction;

	// DashTrailComponent는 대쉬 시작 시 Spawn으로 처리 (BeginPlay에서 불필요)

	// Enhanced Input 매핑 컨텍스트 등록
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}

	// 자동 공격 타이머 시작
	GetWorld()->GetTimerManager().SetTimer(
		AutoAttackTimerHandle,
		this,
		&AJYNPlayerCharacter::PerformAutoAttack,
		AutoAttackInterval,
		true
	);
}

void AJYNPlayerCharacter::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	GetWorld()->GetTimerManager().ClearTimer(AutoAttackTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(QinggongDurationTimer);
	GetWorld()->GetTimerManager().ClearTimer(QinggongCooldownTimer);
	GetWorld()->GetTimerManager().ClearTimer(InvincibilityTimerHandle);
}

void AJYNPlayerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
}

void AJYNPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsRecovering)
	{
		UpdateRecovery(DeltaTime);
	}

	// 대쉬 중 AddMovementInput 유지 → ABP의 ShouldMove(가속도 조건) 충족 → 걷기 애니 재생
	if (bIsDashing && !DashDirection.IsNearlyZero())
	{
		AddMovementInput(DashDirection, 1.0f, true);
	}

	// HP 자동 회복 (사망 상태가 아니고, MaxHP 미만일 때)
	if (!bIsDead && CurrentHP < MaxHP && HPRegenPerSecond > 0.0f)
	{
		CurrentHP = FMath::Min(MaxHP, CurrentHP + HPRegenPerSecond * DeltaTime);
		BP_OnDamaged(CurrentHP, MaxHP);
	}
}

void AJYNPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AJYNPlayerCharacter::OnMove);
			EIC->BindAction(MoveAction, ETriggerEvent::Completed, this, &AJYNPlayerCharacter::OnMove);
		}
		if (DashAction)
		{
			EIC->BindAction(DashAction, ETriggerEvent::Triggered, this, &AJYNPlayerCharacter::OnDash);
		}
		if (PauseAction)
		{
			EIC->BindAction(PauseAction, ETriggerEvent::Triggered, this, &AJYNPlayerCharacter::OnPause);
		}
	}
}

// ── 입력 핸들러 ──────────────────────────────────────────

void AJYNPlayerCharacter::OnMove(const FInputActionValue& Value)
{
	// 넉백 중 + 사망 시 입력 차단
	if (bIsKnockedBack || bIsDead) return;

	const FVector2D Input = Value.Get<FVector2D>();

	if (FMath::IsNearlyZero(Input.SizeSquared())) return;

	// 카메라 고정이므로 월드 축 기준으로 이동
	AddMovementInput(FVector::ForwardVector, Input.Y);
	AddMovementInput(FVector::RightVector, Input.X);
}

void AJYNPlayerCharacter::OnDash(const FInputActionValue& Value)
{
	if (!CanDash() || bIsDead) return;

	// 내공 소모 (부족해도 대쉬는 가능 — 단순 cost only)
	if (NaegongComponent && DashNaegongCost > 0.0f)
	{
		NaegongComponent->UseNaegong(DashNaegongCost);
	}

	// 이동 방향 결정 (GetLastMovementInputVector: 실제 입력 벡터, Z=0)
	FVector DashDir = GetLastMovementInputVector();
	if (DashDir.IsNearlyZero())
	{
		DashDir = GetActorForwardVector();
	}
	DashDir.Z = 0.0f;
	DashDir = DashDir.GetSafeNormal2D();

	bIsDashing    = true;
	DashDirection = DashDir;

	// 경공 지속 시간 + 0.1s 후까지 무적
	SetInvincible(QinggongDuration + 0.1f);

	ApplyDashMovementBoost(DashDir);

	// 대쉬 이펙트: 이동 방향의 직각(수직) 방향으로 버스트
	if (DashTrailSystem)
	{
		FRotator PerpRotation = DashDir.Rotation();
		//PerpRotation.Yaw += 90.0f;
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), DashTrailSystem,
			GetActorLocation(),
			PerpRotation,
			FVector(1.5f, 1.5f, 1.5f),
			true, true);
	}

	BP_OnDashStarted();

	GetWorld()->GetTimerManager().SetTimer(
		QinggongDurationTimer, this,
		&AJYNPlayerCharacter::EndQinggong,
		QinggongDuration, false);
}

void AJYNPlayerCharacter::ApplyDashMovementBoost(const FVector& Dir)
{
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->MaxWalkSpeed               = QinggongMaxWalkSpeed;
	MoveComp->MaxAcceleration            = QinggongMaxAcceleration;
	MoveComp->BrakingDecelerationWalking = QinggongBrakingDeceleration;
	MoveComp->GroundFriction             = QinggongGroundFriction;
	// 즉각 속도 주입 (LaunchCharacter 없이 달리는 느낌)
	MoveComp->Velocity = Dir * QinggongMaxWalkSpeed;
}

void AJYNPlayerCharacter::EndQinggong()
{
	bIsDashing            = false;
	// bIsInvincible는 SetInvincible 타이머가 0.1s 뒤에 자동으로 꺼줌
	bIsQinggongOnCooldown = true;
	DashDirection         = FVector::ZeroVector;

	// 부드러운 속도 복구 시작
	bIsRecovering   = true;
	RecoveryElapsed = 0.0f;

	BP_OnDashStarted(); // BP 훅 (종료 알림 — OnQinggongEnded 추가 원하면 별도 선언)

	GetWorld()->GetTimerManager().SetTimer(
		QinggongCooldownTimer, this,
		&AJYNPlayerCharacter::EndQinggongCooldown,
		DashCooldown, false);
}

void AJYNPlayerCharacter::EndQinggongCooldown()
{
	bIsQinggongOnCooldown = false;
}

void AJYNPlayerCharacter::UpdateRecovery(float DeltaTime)
{
	RecoveryElapsed += DeltaTime;
	const float Alpha = FMath::Clamp(RecoveryElapsed / QinggongRecoveryDuration, 0.0f, 1.0f);

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->MaxWalkSpeed               = FMath::Lerp(QinggongMaxWalkSpeed,       DefaultMaxWalkSpeed,        Alpha);
	MoveComp->MaxAcceleration            = FMath::Lerp(QinggongMaxAcceleration,    DefaultMaxAcceleration,     Alpha);
	MoveComp->BrakingDecelerationWalking = FMath::Lerp(QinggongBrakingDeceleration,DefaultBrakingDeceleration, Alpha);
	MoveComp->GroundFriction             = FMath::Lerp(QinggongGroundFriction,     DefaultGroundFriction,      Alpha);

	if (Alpha >= 1.0f)
	{
		bIsRecovering = false;
	}
}

void AJYNPlayerCharacter::OnPause(const FInputActionValue& Value)
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		const bool bIsPaused = PC->IsPaused();
		PC->SetPause(!bIsPaused);
	}
}

// ── 자동 공격 ──────────────────────────────────────────

void AJYNPlayerCharacter::PerformAutoAttack()
{
	if (bIsDead || !ProjectileClass) return;

	const int32 TotalShots = 1 + ExtraProjectileCount;
	TArray<AActor*> Targets = FindEnemiesInRange(TotalShots);

	FVector SpawnLocation = GetActorLocation();
	SpawnLocation.Z += 40.0f;

	// 메인 발사 방향 (첫 번째 타겟 또는 전방)
	FVector MainDir;
	if (Targets.IsValidIndex(0))
	{
		MainDir = (Targets[0]->GetActorLocation() - SpawnLocation);
		MainDir.Z = 0.0f;
		MainDir = MainDir.GetSafeNormal2D();
	}
	else
	{
		MainDir = GetActorForwardVector();
		MainDir.Z = 0.0f;
		if (MainDir.IsNearlyZero()) MainDir = FVector::ForwardVector;
		MainDir.Normalize();
	}

	for (int32 i = 0; i < TotalShots; i++)
	{
		AActor* Target = Targets.IsValidIndex(i) ? Targets[i] : nullptr;

		FVector FireDirection;
		AActor* HomingActorTarget = nullptr;

		if (Target)
		{
			FVector ToTarget = Target->GetActorLocation() - SpawnLocation;
			ToTarget.Z = 0.0f;
			FireDirection = ToTarget.GetSafeNormal();
			HomingActorTarget = Target;
		}
		else
		{
			// 타겟이 없는 추가 발사체: 메인 방향 기준 fan spread (20도 간격)
			const float SpreadAngle = (i - TotalShots / 2) * 20.0f;
			FireDirection = MainDir.RotateAngleAxis(SpreadAngle, FVector::UpVector);
		}

		const FTransform SpawnTransform(FireDirection.Rotation(), SpawnLocation);
		FActorSpawnParameters Params;
		Params.Instigator = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AJYNProjectileBase* Projectile = GetWorld()->SpawnActor<AJYNProjectileBase>(
			ProjectileClass, SpawnTransform, Params);

		if (Projectile)
		{
			Projectile->Damage *= ProjectileDamageMultiplier;
			Projectile->Launch(FireDirection, HomingActorTarget, this);
		}
	}
}

AActor* AJYNPlayerCharacter::FindNearestEnemyInRange() const
{
	AActor* NearestEnemy = nullptr;
	float NearestDistSq = AutoAttackRange * AutoAttackRange;

	for (TActorIterator<AJYNEnemyBase> It(GetWorld()); It; ++It)
	{
		AJYNEnemyBase* Enemy = *It;
		if (!Enemy || Enemy->IsDead()) continue;

		const float DistSq = FVector::DistSquared(GetActorLocation(), Enemy->GetActorLocation());
		if (DistSq <= NearestDistSq)
		{
			NearestDistSq = DistSq;
			NearestEnemy = Enemy;
		}
	}

	return NearestEnemy;
}

TArray<AActor*> AJYNPlayerCharacter::FindEnemiesInRange(int32 MaxCount) const
{
	const float RangeSq = AutoAttackRange * AutoAttackRange;
	TArray<TPair<float, AActor*>> EnemyDistances;

	for (TActorIterator<AJYNEnemyBase> It(GetWorld()); It; ++It)
	{
		AJYNEnemyBase* Enemy = *It;
		if (!Enemy || Enemy->IsDead()) continue;

		const float DistSq = FVector::DistSquared(GetActorLocation(), Enemy->GetActorLocation());
		if (DistSq <= RangeSq)
		{
			EnemyDistances.Add({ DistSq, Enemy });
		}
	}

	// 거리 오름차순 정렬
	EnemyDistances.Sort([](const TPair<float, AActor*>& A, const TPair<float, AActor*>& B)
	{
		return A.Key < B.Key;
	});

	TArray<AActor*> Result;
	for (int32 i = 0; i < FMath::Min(MaxCount, EnemyDistances.Num()); i++)
	{
		Result.Add(EnemyDistances[i].Value);
	}
	return Result;
}

void AJYNPlayerCharacter::UpdateAutoAttackTimer()
{
	GetWorld()->GetTimerManager().ClearTimer(AutoAttackTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(
		AutoAttackTimerHandle,
		this,
		&AJYNPlayerCharacter::PerformAutoAttack,
		AutoAttackInterval,
		true
	);
}

void AJYNPlayerCharacter::ApplyLevelUpBonus(int32 NewLevel)
{
	// MaxHP +5, 즉시 회복
	const float HPBonus = 5.0f;
	MaxHP += HPBonus;
	CurrentHP = FMath::Min(CurrentHP + HPBonus, MaxHP);

	// MaxNaegong +3
	if (NaegongComponent)
	{
		NaegongComponent->MaxNaegong += 3.0f;
	}

	// HUD 갱신
	BP_OnDamaged(CurrentHP, MaxHP);
}

void AJYNPlayerCharacter::ApplyMugongCard(EJYNMugongCardType CardType)
{
	switch (CardType)
	{
	// ── HP 카테고리 ────────────────────────────────────────
	case EJYNMugongCardType::IronBody:
		// 철신공: 최대 HP +20, 즉시 회복
		MaxHP += 20.0f;
		CurrentHP = FMath::Min(CurrentHP + 20.0f, MaxHP);
		BP_OnDamaged(CurrentHP, MaxHP);
		break;

	case EJYNMugongCardType::HPRegen:
		// 회복신공: HP 초당 회복 +1
		HPRegenPerSecond += 1.0f;
		break;

	case EJYNMugongCardType::GreatPill:
		// 대환단: 현재 HP +30 즉시 회복
		CurrentHP = FMath::Min(MaxHP, CurrentHP + 30.0f);
		BP_OnDamaged(CurrentHP, MaxHP);
		break;

	// ── 내공 카테고리 ──────────────────────────────────────
	case EJYNMugongCardType::NaegongMax:
		// 내공 증진: 최대 내공 +20, 즉시 채움
		if (NaegongComponent)
		{
			NaegongComponent->MaxNaegong += 20.0f;
			NaegongComponent->CurrentNaegong = FMath::Min(
				NaegongComponent->CurrentNaegong + 20.0f, NaegongComponent->MaxNaegong);
			NaegongComponent->OnNaegongChanged.Broadcast(
				NaegongComponent->CurrentNaegong, NaegongComponent->MaxNaegong);
		}
		break;

	case EJYNMugongCardType::NaegongRegen:
		// 내공 재생: 초당 내공 회복 +2
		if (NaegongComponent)
		{
			NaegongComponent->RegenPerSecond += 2.0f;
		}
		break;

	case EJYNMugongCardType::NaegongAbsorb:
		// 내공 흡수 강화: 흡수 효율 +25% (최대 3배)
		if (NaegongComponent)
		{
			NaegongComponent->AbsorbMultiplier = FMath::Min(3.0f, NaegongComponent->AbsorbMultiplier + 0.25f);
		}
		break;

	// ── 암기 카테고리 (4단계 순환, maxed phase는 skip) ───
	case EJYNMugongCardType::Amgi:
	{
		const int32 Phase = GetNextAmgiPhase();
		switch (Phase)
		{
		case 0: // 암기 피해 +15%
			ProjectileDamageMultiplier *= 1.15f;
			break;
		case 1: // 공격 간격 -20% (최소 0.1s)
			AutoAttackInterval = FMath::Max(0.1f, AutoAttackInterval * 0.8f);
			UpdateAutoAttackTimer();
			break;
		case 2: // 유도 반각 +20% (최대 89도)
			HomingHalfAngle = FMath::Min(89.0f, HomingHalfAngle * 1.2f);
			break;
		case 3: // 투사 수 +1
			ExtraProjectileCount++;
			break;
		}
		// 다음 호출 시 Phase 다음부터 시작 (사이클 진행)
		if (Phase >= 0) AmgiStackCount = Phase + 1;
		break;
	}

	// ── 표창 카테고리 ─────────────────────────────────────
	case EJYNMugongCardType::Pyochang:
		GrantPyochang();
		break;
	case EJYNMugongCardType::PyochangSpeed:
		GrantPyochangSpeed();
		break;
	case EJYNMugongCardType::PyochangDamage:
		GrantPyochangDamage();
		break;

	// ── 기타: XP 흡수 범위 ─────────────────────────────
	case EJYNMugongCardType::XPMagnet:
		// 최대 10회 (50cm × 10 = 500cm)
		XPMagnetBonus = FMath::Min(500.0f, XPMagnetBonus + 50.0f);
		break;

	// ── 경공 카테고리 (기존 유지) ─────────────────────────
	case EJYNMugongCardType::Agility:
		// 신행술: 기본 이동 속도 10% 증가
		DefaultMaxWalkSpeed *= 1.1f;
		if (!bIsDashing && !bIsRecovering)
		{
			GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed;
		}
		break;

	case EJYNMugongCardType::NoForm:
		// 무형지기: 경공 쿨타임 -0.3s (최소 0.1s)
		DashCooldown = FMath::Max(0.1f, DashCooldown - 0.3f);
		break;

	// ── Deprecated 카드 (혹시라도 호출되면 동일 효과로 fallback) ──
	case EJYNMugongCardType::AmkiSpeedUp:
		AutoAttackInterval = FMath::Max(0.1f, AutoAttackInterval * 0.8f);
		UpdateAutoAttackTimer();
		break;
	case EJYNMugongCardType::PoisonFang:
		ProjectileDamageMultiplier *= 1.15f;
		break;
	case EJYNMugongCardType::ChasingBullet:
		HomingHalfAngle = FMath::Min(89.0f, HomingHalfAngle * 1.2f);
		break;
	case EJYNMugongCardType::StormRain:
		ExtraProjectileCount++;
		break;

	default:
		break;
	}
}

// ── 데미지 / 사망 ──────────────────────────────────────

void AJYNPlayerCharacter::EndKnockback()
{
	bIsKnockedBack = false;
}

float AJYNPlayerCharacter::GetDashCooldownProgress() const
{
	// 쿨다운 중이 아니면 1.0 (준비완료)
	if (!bIsQinggongOnCooldown) return 1.0f;

	if (!GetWorld() || DashCooldown <= 0.0f) return 1.0f;

	const float Remaining = GetWorld()->GetTimerManager().GetTimerRemaining(QinggongCooldownTimer);
	return FMath::Clamp(1.0f - (Remaining / DashCooldown), 0.0f, 1.0f);
}

void AJYNPlayerCharacter::GrantPyochang()
{
	if (!PyochangOrbitClass) return;

	// 1번째: 활성화 (3개로 시작)
	if (!PyochangOrbitInstance)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = this;
		PyochangOrbitInstance = GetWorld()->SpawnActor<AJYNPyochangOrbit>(
			PyochangOrbitClass, GetActorLocation(), FRotator::ZeroRotator, Params);

		if (PyochangOrbitInstance)
		{
			PyochangOrbitInstance->AttachToActor(this,
				FAttachmentTransformRules::KeepRelativeTransform);
			PyochangOrbitInstance->SetActorRelativeLocation(FVector::ZeroVector);

			if (USceneComponent* Root = PyochangOrbitInstance->GetRootComponent())
			{
				Root->SetUsingAbsoluteRotation(true);
				Root->SetWorldRotation(FRotator::ZeroRotator);
			}

			PyochangOrbitInstance->PyochangCount = 3;
			PyochangOrbitInstance->RebuildPyochangs();
			PyochangOrbitInstance->SetDamageMultiplier(ProjectileDamageMultiplier);
		}

		PyochangStackCount = 1;  // 활성화됨
		return;
	}

	// 2번째 이후: 사이클 (개수 → 가속 → 데미지)
	const int32 Phase = GetNextPyochangPhase();
	switch (Phase)
	{
	case 0:  // 개수 +1
		PyochangOrbitInstance->PyochangCount = FMath::Min(12, PyochangOrbitInstance->PyochangCount + 1);
		PyochangOrbitInstance->RebuildPyochangs();
		break;
	case 1:  // 회전 속도 +25% (최대치 cap)
		PyochangOrbitInstance->RotationSpeed = FMath::Min(MaxPyochangRotation, PyochangOrbitInstance->RotationSpeed * 1.25f);
		break;
	case 2:  // 데미지 +20%
		PyochangOrbitInstance->Damage *= 1.20f;
		break;
	}
	PyochangStackCount = (Phase + 1) + 1;  // Phase 다음으로 진행 (다음 호출 시 시작점)
}

void AJYNPlayerCharacter::GrantPyochangSpeed()
{
	// Deprecated — Pyochang 카드에 통합됨. 혹시 호출되면 단독 효과만.
	if (!PyochangOrbitInstance) return;
	PyochangOrbitInstance->RotationSpeed = FMath::Min(MaxPyochangRotation, PyochangOrbitInstance->RotationSpeed * 1.25f);
}

void AJYNPlayerCharacter::GrantPyochangDamage()
{
	// Deprecated — Pyochang 카드에 통합됨. 혹시 호출되면 단독 효과만.
	if (!PyochangOrbitInstance) return;
	PyochangOrbitInstance->Damage *= 1.20f;
}

FString AJYNPlayerCharacter::GetNextAmgiDescription() const
{
	const int32 Phase = GetNextAmgiPhase();
	switch (Phase)
	{
	case 0: return TEXT("암기 피해 15% 증가");
	case 1: return TEXT("자동 공격 간격 20% 감소");
	case 2: return TEXT("유도 반각 20% 확대");
	case 3: return TEXT("투사 수 +1");
	default: return TEXT("암기 능력 강화");
	}
}

FString AJYNPlayerCharacter::GetNextPyochangDescription() const
{
	if (!PyochangOrbitInstance)
	{
		return TEXT("회전 표창 활성화 (3개)");
	}

	const int32 Phase = GetNextPyochangPhase();
	switch (Phase)
	{
	case 0: return TEXT("표창 +1개");
	case 1: return TEXT("회전 속도 +25%");
	case 2: return TEXT("데미지 +20%");
	default: return TEXT("표창 강화");
	}
}

// ── Amgi Phase 헬퍼 ────────────────────────────────────

int32 AJYNPlayerCharacter::GetNextAmgiPhase() const
{
	const int32 Start = AmgiStackCount % 4;
	for (int32 i = 0; i < 4; i++)
	{
		const int32 P = (Start + i) % 4;
		if (IsAmgiPhaseAvailable(P)) return P;
	}
	return -1;  // 모두 maxed (실제로는 피해/투사가 항상 available이라 발생 안 함)
}

bool AJYNPlayerCharacter::IsAmgiPhaseAvailable(int32 Phase) const
{
	switch (Phase)
	{
	case 0: return true;  // 피해 — 항상
	case 1: return AutoAttackInterval > 0.1f + KINDA_SMALL_NUMBER;  // 간격이 0.1s 초과 시만
	case 2: return HomingHalfAngle < 89.0f - KINDA_SMALL_NUMBER;     // 반각이 89도 미만일 때만
	case 3: return true;  // 투사 — 항상
	}
	return false;
}

// ── Pyochang Phase 헬퍼 ────────────────────────────────

int32 AJYNPlayerCharacter::GetNextPyochangPhase() const
{
	if (!PyochangOrbitInstance) return -1;  // 활성화 전엔 phase 없음

	// 활성화 후 (PyochangStackCount=1)부터 사이클: 0=개수, 1=가속, 2=데미지
	// PyochangStackCount-1을 3으로 나눈 나머지가 다음 phase
	const int32 Start = ((PyochangStackCount - 1) % 3 + 3) % 3;
	for (int32 i = 0; i < 3; i++)
	{
		const int32 P = (Start + i) % 3;
		if (IsPyochangPhaseAvailable(P)) return P;
	}
	return 2;  // fallback (데미지는 항상 available)
}

bool AJYNPlayerCharacter::IsPyochangPhaseAvailable(int32 Phase) const
{
	if (!PyochangOrbitInstance) return false;
	switch (Phase)
	{
	case 0: return PyochangOrbitInstance->PyochangCount < 12;            // 개수
	case 1: return PyochangOrbitInstance->RotationSpeed < MaxPyochangRotation - KINDA_SMALL_NUMBER;  // 가속
	case 2: return true;                                                  // 데미지 — 항상
	}
	return false;
}

void AJYNPlayerCharacter::TakeDamageJYN(float Damage, const FVector& HitDirection)
{
	if (bIsDead || bIsInvincible || Damage <= 0.0f) return;

	// 내공 먼저 흡수
	float RemainingDamage = Damage;
	if (NaegongComponent)
	{
		RemainingDamage = NaegongComponent->AbsorbDamage(Damage);
	}

	// 내공 초과분 → HP 직격
	if (RemainingDamage > 0.0f)
	{
		CurrentHP = FMath::Max(0.0f, CurrentHP - RemainingDamage);
	}

	// 넉백 적용 (맞은 방향으로) + 일정 시간 WASD 입력 차단
	if (HitKnockbackForce > 0.0f)
	{
		FVector LaunchDir = HitDirection;
		LaunchDir.Z = 0.0f;
		LaunchCharacter(LaunchDir.GetSafeNormal() * HitKnockbackForce, true, true);

		bIsKnockedBack = true;
		GetWorld()->GetTimerManager().SetTimer(
			KnockbackTimerHandle, this, &AJYNPlayerCharacter::EndKnockback,
			KnockbackInputBlockDuration, false);
	}

	BP_OnDamaged(CurrentHP, MaxHP);

	if (CurrentHP <= 0.0f)
	{
		bIsDead = true;
		GetWorld()->GetTimerManager().ClearTimer(InvincibilityTimerHandle);
		GetCharacterMovement()->DisableMovement();
		GetWorld()->GetTimerManager().ClearTimer(AutoAttackTimerHandle);
		BP_OnDied();

		// 게임 모드에 런 종료 알림
		if (AJYNGameMode* GameMode = Cast<AJYNGameMode>(GetWorld()->GetAuthGameMode()))
		{
			GameMode->EndRun();
		}
	}
	else
	{
		// 피격 후 짧은 무적 (여러 적 동시 공격 방지)
		SetInvincible(HitInvincibilityDuration);
	}
}

void AJYNPlayerCharacter::RegainNaegongOnHit(float Amount)
{
	if (NaegongComponent)
	{
		NaegongComponent->RegainOnHit(Amount);
	}
}

// ── 무적 ──────────────────────────────────────────────────────

void AJYNPlayerCharacter::SetInvincible(float Duration)
{
	bIsInvincible = true;

	// 이미 더 긴 무적이 걸려 있으면 갱신하지 않음
	const float Remaining = GetWorld()->GetTimerManager().GetTimerRemaining(InvincibilityTimerHandle);
	if (Remaining >= Duration) return;

	GetWorld()->GetTimerManager().SetTimer(
		InvincibilityTimerHandle,
		this,
		&AJYNPlayerCharacter::ClearInvincibility,
		Duration,
		false
	);
}

void AJYNPlayerCharacter::ClearInvincibility()
{
	bIsInvincible = false;
}
