# CLAUDE.md

@Plugins/VibeUE/Content/samples/AGENTS.md.sample
@CONTENT_STRUCTURE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

---

## 프로젝트 개요

**잔영록 (殘影錄)** — 탑다운 뱀서라이크 × 무협 Roguelite (Unreal Engine 5.7)  
마감: **2026-05-21** (Win64 빌드 + 플레이 영상)  
레퍼런스: Vampire Survivors, 20 Minutes Till Dawn  
모듈: `JanYeongNok2` / uproject: `JanYeongNok2.uproject`

---

## 빌드 & 개발

UE5 표준 빌드 도구(UnrealBuildTool)를 사용한다. IDE는 JetBrains Rider.

```powershell
# 에디터 빌드 (Development Editor)
& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" `
    JanYeongNok2Editor Win64 Development `
    "C:\Users\niboo\Documents\Unreal Projects\JanYeongNok2\JanYeongNok2.uproject"

# 패키징 (Win64 Shipping)
& "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\RunUAT.bat" `
    BuildCookRun `
    -project="C:\Users\niboo\Documents\Unreal Projects\JanYeongNok2\JanYeongNok2.uproject" `
    -noP4 -platform=Win64 -clientconfig=Shipping `
    -cook -build -stage -pak -archive -archivedirectory="Packaged"
```

---

## C++ / Blueprint 분담 원칙

| 담당 | 내용 |
|---|---|
| **C++ 전담** | 로직, 수치 계산, 컴포넌트 구조, DataAsset/DataTable 정의, 델리게이트 선언 |
| **BP 전담** | 비주얼, 이펙트, 사운드, 애니메이션 연결, 데이터 값 입력, UI 디자인 |
| **금지** | C++에서 `ConstructorHelpers`로 에셋 하드코딩 로드 |
| **금지** | BP에서 게임 로직 구현 (숫자 계산, 상태 판단) |

---

## 네이밍 컨벤션

- **신규 JYN 클래스**: `AJYNPlayerCharacter`, `UJYNNaegongComponent`, `AJYNEnemyBase`
- **Bool 멤버**: `bIsAboveThreshold`, `bIsDashing` (b 접두사 필수)
- **DataAsset**: `DA_Mugong_ManCheonHwaWoo`, `DA_Yeongyak_DaeHwanDan`
- **InputAction**: `IA_Move`, `IA_Dash`
- **BP 파생**: `BP_JYNPlayerCharacter`, `BP_WolfEnemy`
- **Enum**: `EJYNEnemyState`
- **Struct**: `FMugongData`

> 기존 프로토타입 클래스(`ATwinStick*`, `AStrategy*`, `AJanYeongNok2*`)는 레퍼런스용이며, 잔영록 게임 로직은 `JYN` 접두사로 새로 작성한다.

---

## 현재 코드베이스 구조

현재 `Source/JanYeongNok2/`에는 두 개의 프로토타입 Variant가 존재한다. 잔영록 JYN 클래스는 아직 미구현 상태이며, 이 Variant들을 레퍼런스로 활용한다.

```
Source/JanYeongNok2/
├── JanYeongNok2Character.h/cpp     — 추상 탑다운 캐릭터 베이스 (SpringArm + Camera)
├── JanYeongNok2GameMode.h/cpp      — 추상 GameMode 베이스
├── JanYeongNok2PlayerController    — PlayerController 베이스
│
├── Variant_TwinStick/              — 트윈스틱 슈터 프로토타입 (잔영록 레퍼런스)
│   ├── TwinStickCharacter          — 이동/조준/대시/발사/AoE (Enhanced Input)
│   ├── TwinStickGameMode           — 점수·콤보·NPC 캡 관리, UI 생성
│   ├── TwinStickPlayerController
│   ├── AI/
│   │   ├── TwinStickNPC            — 적 캐릭터 (피격→지연 파괴, 픽업 드롭)
│   │   ├── TwinStickAIController   — NPC AIController
│   │   ├── TwinStickSpawner        — 그룹 스폰, NavMesh 기반 위치 선택
│   │   ├── TwinStickNPCDestruction — 파괴 이펙트 프록시
│   │   └── TwinStickStateTreeUtility — StateTree Task (GetPlayer)
│   ├── Gameplay/
│   │   ├── TwinStickProjectile     — 바운싱 투사체 (SphereCollision + ProjectileMovement)
│   │   ├── TwinStickPickup         — 픽업 아이템
│   │   └── TwinStickAoEAttack      — 범위 공격 액터
│   └── UI/
│       └── TwinStickUI             — UMG HUD
│
└── Variant_Strategy/               — 전략 게임 프로토타입 (최소 구현)
    ├── StrategyGameMode / StrategyPawn / StrategyPlayerController
    ├── StrategyUnit
    └── UI/ StrategyHUD / StrategyUI
```

---

## 목표 JYN 클래스 구조 (미구현 — 구현 예정)

```
AJYNPlayerCharacter       (ACharacter)        — 탑다운 이동, 자동 공격, 경공
UJYNNaegongComponent      (UActorComponent)   — 내공: 실드 흡수, 회복, 임계점 판단
UJYNExperienceComponent   (UActorComponent)   — 경험치, 레벨업, 무공 선택 트리거
AJYNGameMode              (AGameMode)         — 웨이브 스폰, 런 시작/종료, 점수
AJYNPlayerController      (APlayerController)
AJYNGameState             (AGameState)        — 점수 누적, 런 시간
AJYNProjectileBase        (AActor)            — 암기 투사체 베이스 (유도 기능 내장)
AJYNEnemyBase             (ACharacter)        — 적 캐릭터 베이스 (HP, 피격, 사망, XP드롭)
AJYNEnemyAIController     (AAIController)     — 적 AI (StateTree 기반)
AJYNWeaponBase            (AActor)            — 무기 베이스 (투사체 스폰, OnFired BP훅)
UJYNMugongData            (UPrimaryDataAsset) — 무공 데이터
UJYNYeongyakData          (UPrimaryDataAsset) — 영약 데이터
```

파일 배치 규칙:
```
Source/JanYeongNok2/
├── Character/     — AJYNPlayerCharacter
├── AI/            — AJYNEnemyBase, AJYNEnemyAIController
├── Components/    — UJYNNaegongComponent, UJYNExperienceComponent
├── Gameplay/      — GameMode, GameState, PlayerController
├── Abilities/     — AJYNProjectileBase
├── Weapons/       — AJYNWeaponBase
├── Data/          — DataAsset 구조체 정의
└── UI/            — UserWidget 베이스
```

---

## 게임 시스템 규칙

### 카메라 & 시점
- **탑다운 고정 카메라**: 부감각 ~70도, 플레이어 위 고정
- 캐릭터는 이동 방향을 바라봄 (컨트롤러 회전 무관)
- 점프 없음

### 내공(內功)
- 피격 시 내공이 먼저 흡수 → 내공 0이면 HP 직격
- **임계점**: 내공 일정 % 이상 → 무공 자동 강화, **UI에 표시하지 않음**
- 회복: 시간 자동 회복(느림) + 평타 적중 소량 회복

### 자동 공격
- 별도 입력 없이 `AutoAttackInterval`마다 범위 내 최근접 적 자동 발사
- 투사체는 카메라 방향 기준 원뿔(HomingHalfAngle) 내 적에게 유도
- 범위 밖 적이면 직선 비행

### 뱀서식 무공(武功)
- 적 처치 → 경험치 → 게이지 충족 → **화면 일시정지** → 3종 카드 중 1택
- 등급: 백(白) → 청(靑) → 자(紫) → 금(金)
- 같은 무공 N번 선택 = N 스택 = 효과 N배 (누적형)
- MVP 풀: 5~10종 (백·청 위주)

### 영약(靈藥)
- 런당 1~3개 등장, 픽업 즉시 적용, 스택 없음, 런 내 영구 효과
- MVP 풀: 2~3종

### 적 웨이브
- GameMode가 시간 기반으로 웨이브 스폰
- 적 사망 시 경험치 Soul 드롭 → 플레이어가 수집

### 점수(點數)
- 적 처치 시 누적
- 사망 → 정수(精髓) 환산 로직만 구현 (해금 UI는 Post-MVP)

---

## MVP 스코프

### ✅ MVP에 포함
- 협객 1명: 사천당가 (암기·독, 자동 원거리)
- 권역 1개: 죽림(竹林) — 보스 없음
- 내공·뱀서식 무공·영약·점수 시스템
- HUD (HP·내공·경험치·점수·무공 인벤토리) + 시작/결과 화면

### ❌ MVP 아님 — 요청해도 구현하지 말 것
- 보스전, 수동 공격 버튼, 추가 협객/권역
- 메타 진행 해금 UI, GAS, 멀티플레이, 콘솔 빌드, 풀 보이스

---

## 입력 바인딩 (Enhanced Input)

| 키 | IA 이름 | 동작 |
|---|---|---|
| WASD | IA_Move | 탑다운 이동 |
| Shift | IA_Dash | 경공 (무적 프레임) |
| ESC | IA_Pause | 일시정지 |

> 공격은 자동 (입력 불필요). 마우스 룩/점프/스킬 키 없음.

---

## 기술 제약

- **GAS 미사용**: 모든 능력은 커스텀 `UActorComponent`로 구현
- **적 AI**: `StateTreeModule` + `GameplayStateTreeModule` 사용 (BT/EQS 미사용). `TwinStickStateTreeUtility`가 레퍼런스 구현
- **절차적 생성 미사용**: 수동 맵 제작
- **타깃 사양**: GTX 1660 / 16GB RAM / 60fps
- **Lumen**: 저강도만 사용 / **Nanite**: 정적 지형·바위 메시에만 적용

---

## 플러그인

- `VibeUE` (`Plugins/VibeUE/`) — 인에디터 AI 채팅 및 MCP 서버. `AGENTS.md.sample` 참조
- `StateTree` / `GameplayStateTree` — 적 AI에 사용 (uproject에 활성화됨)
- `ModelingToolsEditorMode` — 에디터 전용 모델링 도구 (빌드 제외)
