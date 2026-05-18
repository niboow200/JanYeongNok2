# 잔영록 (殘影錄) — Project Context

> Claude Code용 프로젝트 컨텍스트 문서.  
> `ue-project-context` 스킬의 기반 역할.

---

## 프로젝트 기본 정보

- **타이틀**: 잔영록 (殘影錄)
- **장르**: 탑다운 뱀서라이크 × 무협 Roguelite
- **엔진**: Unreal Engine 5.7
- **언어**: C++ 50% / Blueprint 50%
- **IDE**: JetBrains Rider
- **모듈명**: JanYeongNok
- **클래스 접두사**: JYN
- **마감**: 2026-05-21 (Win64 빌드 + 플레이 영상)
- **레퍼런스**: Vampire Survivors, 20 Minutes Till Dawn

---

## 게임 개요

탑다운 시점의 무협 뱀서라이크. 사천당가 협객이 죽림 구역에서 쏟아지는 적들을 자동 암기로 처치하며 살아남는 게임.

- **시점**: 고정 탑다운 (~70도 부감각)
- **공격**: 자동 (입력 불필요, 범위 내 최근접 적에 유도 투사체)
- **성장**: 레벨업마다 무공(武功) 카드 3장 중 1택 → 뱀서식 누적 강화
- **차별점**: 내공(內功) 실드 시스템, 임계점 초과 시 무공 자동 강화

---

## MVP 스코프 (이것만 만든다)

### ✅ MVP에 포함
- 협객 1명: **사천당가** (암기·독, 자동 원거리)
- 권역 1개: **죽림(竹林)** — 보스 없음
- 내공 시스템: 피격 실드 + 임계점 강화
- 뱀서식 무공 시스템: 5~10종 + 레벨업 카드 선택
- 영약 시스템: 2~3종 픽업
- 점수 시스템
- HUD (HP·내공·경험치·점수·무공 인벤토리) + 시작/결과 화면
- 적 웨이브 스폰 (시간 기반)

### ❌ MVP 아님 — 요청해도 구현하지 말 것
- 보스전
- 수동 공격 버튼 (공격은 자동)
- 추가 협객/권역
- 메타 진행 해금 UI (정수, 영구 해금)
- GAS, 멀티플레이, 콘솔 빌드, 풀 보이스

---

## C++ / Blueprint 분담 원칙

| 담당 | 내용 |
|---|---|
| **C++ 전담** | 로직, 수치 계산, 컴포넌트 구조, DataAsset/DataTable 정의, 델리게이트 선언 |
| **BP 전담** | 비주얼, 이펙트, 사운드 연결, 애니메이션 연결, 데이터 값 입력, UI 디자인 |
| **절대 금지** | C++에서 하드코딩 경로로 에셋 로드 |
| **절대 금지** | BP에서 게임 로직 구현 (숫자 계산, 상태 판단) |

---

## 핵심 C++ 클래스 구조

```
AJYNPlayerCharacter       (ACharacter)        — 탑다운 이동, 자동 공격, 경공(대시)
UJYNNaegongComponent      (UActorComponent)   — 내공: 실드 흡수, 자동 회복, 임계점 판단
UJYNExperienceComponent   (UActorComponent)   — 경험치, 레벨업, 무공 선택 트리거
AJYNGameMode              (AGameMode)         — 웨이브 스폰, 런 시작/종료, 점수
AJYNPlayerController      (APlayerController)
AJYNGameState             (AGameState)        — 점수 누적, 런 시간
AJYNProjectileBase        (AActor)            — 암기 투사체 (유도 기능 내장)
AJYNEnemyBase             (ACharacter)        — 적 베이스 (HP, 피격, 사망, XP 드롭)
AJYNEnemyAIController     (AAIController)     — 적 AI: Idle→Chase→Attack
AJYNWeaponBase            (AActor)            — 무기 베이스 (투사체 스폰)
```

---

## 게임 시스템 규칙

### 카메라
- 고정 탑다운 (~70도 부감각), 플레이어 추적
- 캐릭터는 이동 방향을 바라봄
- 경공 시 약간 줌아웃

### 내공(內功) 시스템
- 피격 데미지: 내공 먼저 흡수 → 내공 0이면 HP 직격
- 임계점(70% 이상): 무공 자동 강화 버전 발동. **UI 미표시**
- 회복: 시간 자동 회복 + 평타 적중 시 소량 회복

### 자동 공격
- `AutoAttackInterval`마다 자동 발사 (기본 0.5초)
- 범위(`AutoAttackRange`) 내 최근접 적 자동 조준
- 투사체: 카메라 방향 기준 원뿔(45도) 내 적에게 유도. 없으면 직선

### 경공(輕功)
- Shift: 이동 방향으로 순간 가속 대시 + 무적 프레임
- 쿨타임 있음

### 뱀서식 무공(武功) 시스템
- 적 처치 → 경험치 Soul 드롭 → 플레이어 수집
- 게이지 충족 → 화면 일시정지 → 무공 카드 3장 중 1택
- 등급: 백→청→자→금 / 누적형 스택

### 적 웨이브
- GameMode가 시간 기반 웨이브 관리
- 적 사망 시 Soul(경험치 오브) 드롭

---

## 입력 바인딩

| 키 | IA 이름 | 동작 |
|---|---|---|
| WASD | IA_Move | 탑다운 이동 |
| Shift | IA_Dash | 경공 (대시, 무적) |
| ESC | IA_Pause | 일시정지 |

공격은 자동 (별도 입력 없음)

---

## HUD 구성

```
[좌상단]    HP 바 / 내공 바
[좌상단 하] 경험치 바 / 현재 레벨
[우상단]    현재 점수 / 생존 시간
[하단]      무공 인벤토리(스택 카운터) + 영약 슬롯
```

크로스헤어 없음 (탑다운 자동 공격)

---

## 네이밍 컨벤션

- 모든 클래스에 `JYN` 접두사
- Bool: `bIsAboveThreshold`, `bIsDashing` (b 접두사 필수)
- DataAsset: `DA_Mugong_ManCheonHwaWoo`
- InputAction: `IA_Move`, `IA_Dash`
- Blueprint 파생: `BP_JYNPlayerCharacter`, `BP_WolfEnemy`

---

## 기술 제약

- **GAS 미사용**: 커스텀 UActorComponent로 구현
- **절차적 생성 미사용**: 수동 맵 제작
- **AI**: 타이머 기반 단순 상태머신 (BT+EQS 미사용)
- **타깃 사양**: GTX 1660 / 16GB RAM / 60fps
- **Lumen**: 저강도 / **Nanite**: 정적 메시만

---

## 파일 구조

```
JanYeongNok/
├── Source/JanYeongNok/
│   ├── Character/          — AJYNPlayerCharacter
│   ├── AI/                 — AJYNEnemyBase, AJYNEnemyAIController
│   ├── Components/         — UJYNNaegongComponent, UJYNExperienceComponent
│   ├── Gameplay/           — GameMode, GameState, PlayerController
│   ├── Abilities/          — AJYNProjectileBase
│   ├── Weapons/            — AJYNWeaponBase
│   ├── Data/               — DataAsset 구조체 정의
│   └── UI/                 — UserWidget 베이스
├── Content/_JanYeongNok/
│   ├── Characters/         — BP_JYNPlayerCharacter, ABP_JYNPlayer
│   ├── Enemies/            — BP_WolfEnemy, ABP_Wolf
│   ├── Weapons/            — BP_Kunai
│   ├── Abilities/          — BP_KunaiProjectile
│   ├── UI/                 — WBP_JYNHud, WBP_MugongSelectCard
│   ├── Maps/               — L_JukrimForest
│   └── Data/               — DA_Mugong_*, DA_Yeongyak_*
├── JANYEONGNOK-CONTEXT.md  ← 이 파일
└── CLAUDE.md
```
