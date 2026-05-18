# UE Project Context

*Last updated: 2026-05-18*

## Engine & Project Overview

**Engine version:** UE 5.7 — Launcher build (`BuildSettingsVersion.V6`, `IncludeOrderVersion.Unreal5_7`)
**Project name:** 잔영록 (殘影錄) / JanYeongNok2
**Description:** 탑다운 뱀서라이크 × 무협 Roguelite — 사천당가 협객 1명, 죽림 권역 1개, MVP 마감 2026-05-21
**Project type:** Game
**Genre / domain:** 탑다운 뱀서라이크 (Vampire Survivors류), 무협 Roguelite
**Target platforms:**
- Windows (Win64) — Primary
**Minimum spec:** GTX 1660 / 16GB RAM / 60fps
**IDE:** JetBrains Rider

---

## Module Structure

**Primary game module:** `JanYeongNok2` (단일 모듈)

| Module | Type | Notes |
|--------|------|-------|
| JanYeongNok2 | Runtime | 게임 로직 전체 |

**PublicDependencies:**
`Core`, `CoreUObject`, `Engine`, `InputCore`, `EnhancedInput`, `AIModule`, `NavigationSystem`, `StateTreeModule`, `GameplayStateTreeModule`, `Niagara`, `UMG`, `Slate`

**PrivateDependencies:** 없음

**PublicIncludePaths (현재):**
- `JanYeongNok2`
- `JanYeongNok2/Variant_Strategy`, `JanYeongNok2/Variant_Strategy/UI`
- `JanYeongNok2/Variant_TwinStick`, `JanYeongNok2/Variant_TwinStick/AI`, `JanYeongNok2/Variant_TwinStick/Gameplay`, `JanYeongNok2/Variant_TwinStick/UI`

> ⚠️ JYN 클래스 폴더(`Character/`, `AI/`, `Components/` 등) 추가 시 PublicIncludePaths도 함께 추가 필요

---

## Plugin Dependencies

**Engine plugins enabled:**
- `StateTree` — 적 AI 행동 트리 (BT/EQS 미사용, StateTree 전용)
- `GameplayStateTree` — StateTree 게임플레이 연동
- `ModelingToolsEditorMode` — 에디터 전용 모델링 도구 (빌드 제외, TargetAllowList: Editor)

**Marketplace / Fab plugins:** 없음

**Custom plugins:**
- `VibeUE` (`Plugins/VibeUE/`) — 인에디터 AI 채팅 및 MCP 서버 (개발 전용)

---

## Coding Conventions

**클래스 접두사:**
- 신규 JYN 클래스: `AJYN`, `UJYN`, `EJYN`, `FJYN` (표준 UE A/U/E/F/I + JYN 삽입)
- 기존 프로토타입 (`ATwinStick*`, `AStrategy*`, `AJanYeongNok2*`)은 레퍼런스용 — 잔영록 로직에 직접 사용 금지
- Blueprint 파생: `BP_JYN*`
- DataAsset: `DA_Mugong_*`, `DA_Yeongyak_*`
- InputAction: `IA_*`, InputMappingContext: `IMC_*`
- Enum: `EJYNEnemyState` 형식
- Struct: `FMugongData` 형식

**Bool 멤버:** `b` 접두사 필수 (`bIsAboveThreshold`, `bIsDashing`)

**Header style:** `#pragma once`

**금지 사항:**
- C++에서 `ConstructorHelpers`로 에셋 하드코딩 로드 금지
- BP에서 게임 로직 구현 금지 (숫자 계산, 상태 판단은 C++ 전담)

**C++ / Blueprint 분담:**
| 담당 | 내용 |
|---|---|
| C++ 전담 | 로직, 수치 계산, 컴포넌트 구조, DataAsset/DataTable 정의, 델리게이트 선언 |
| BP 전담 | 비주얼, 이펙트, 사운드, 애니메이션 연결, 데이터 값 입력, UI 디자인 |

---

## Subsystems in Use

**현재 Gameplay Framework (프로토타입 기반, 교체 예정):**
- GameMode: `BP_TopDownGameMode` (→ `AJYNGameMode`로 교체 예정)
- GameDefaultMap: `/Game/TopDown/Lvl_TopDown`

**목표 JYN Gameplay Framework:**
| Class | Type | Responsibility |
|-------|------|----------------|
| `AJYNPlayerCharacter` | ACharacter | 탑다운 이동, 자동 공격, 경공 |
| `AJYNGameMode` | AGameMode | 웨이브 스폰, 런 시작/종료, 점수 |
| `AJYNPlayerController` | APlayerController | Enhanced Input 처리 |
| `AJYNGameState` | AGameState | 점수 누적, 런 시간 |
| `UJYNNaegongComponent` | UActorComponent | 내공: 실드 흡수, 회복, 임계점 판단 |
| `UJYNExperienceComponent` | UActorComponent | 경험치, 레벨업, 무공 선택 트리거 |
| `AJYNEnemyBase` | ACharacter | 적 캐릭터 베이스 (HP, 피격, 사망, XP드롭) |
| `AJYNEnemyAIController` | AAIController | StateTree 기반 적 AI |
| `AJYNProjectileBase` | AActor | 암기 투사체 베이스 (유도 기능 내장) |
| `AJYNWeaponBase` | AActor | 무기 베이스 (투사체 스폰, OnFired BP훅) |
| `UJYNMugongData` | UPrimaryDataAsset | 무공 데이터 |
| `UJYNYeongyakData` | UPrimaryDataAsset | 영약 데이터 |

> ⚠️ 위 JYN 클래스는 **전부 미구현** 상태 (2026-05-18 기준). `Variant_TwinStick`이 레퍼런스 구현.

**GAS 사용 여부:** ❌ 미사용 — 모든 능력은 커스텀 `UActorComponent`로 구현

---

## Build Configuration

**Build targets:** Game, Editor
**DefaultBuildSettings:** `BuildSettingsVersion.V6`
**Custom macros / build flags:** 없음
**Third-party libraries:** 없음
**Engine modifications:** 없음

**렌더링 설정:**
- Lumen (DynamicGI=1, 저강도만 사용)
- Virtual Shadow Maps (VSM) 활성화
- Ray Tracing 활성화 (프로젝트 레벨)
- Substrate 머티리얼 시스템 활성화
- DX12 / SM6 (Windows)
- Static Lighting 비활성화 (`r.AllowStaticLighting=False`)
- Nanite: 정적 지형·바위 메시에만 적용

**NavMesh:**
- RuntimeGeneration: Dynamic
- AgentRadius: 34, AgentHeight: 144, AgentMaxHeight: 160

---

## Game Systems (잔영록 특화)

**카메라:** 탑다운 고정, 부감각 ~70도, 점프 없음

**내공(內功):**
- 피격 시 내공 먼저 흡수 → 내공 0이면 HP 직격
- 임계점(일정 % 이상): 무공 자동 강화 (UI 미표시)
- 회복: 시간 자동회복(느림) + 평타 적중 소량 회복

**자동 공격:**
- 별도 입력 없이 `AutoAttackInterval`마다 범위 내 최근접 적 자동 발사
- 투사체: 카메라 방향 기준 원뿔(HomingHalfAngle) 내 유도, 범위 밖은 직선 비행

**입력 바인딩 (Enhanced Input):**
| 키 | IA 이름 | 동작 |
|---|---|---|
| WASD | IA_Move | 탑다운 이동 |
| Shift | IA_Dash | 경공 (무적 프레임) |
| ESC | IA_Pause | 일시정지 |

**뱀서식 무공:** 경험치 → 레벨업 → 화면 정지 → 3종 카드 선택 (등급: 백→청→자→금)

**MVP 스코프:**
- ✅ 협객 1명 (사천당가, 암기·독, 자동 원거리)
- ✅ 권역 1개 (죽림, 보스 없음)
- ✅ 내공·무공·영약·점수 시스템
- ✅ HUD + 시작/결과 화면
- ❌ 보스전, 수동 공격, 추가 협객/권역, GAS, 멀티플레이

---

## Team Context

**Team size:** 솔로 개발
**Source control:** Git
**Documentation:** CLAUDE.md (프로젝트 루트), `.agents/ue-project-context.md`
