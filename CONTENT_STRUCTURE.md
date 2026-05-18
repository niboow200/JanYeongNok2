# Unreal Engine 5 — Content Folder Structure Guide

이 문서는 Claude Code가 UE5 프로젝트의 Content 폴더 구조를 이해하고, 에셋 생성 및 배치 작업 시 일관된 규칙을 따르도록 작성된 가이드입니다.

이 프로젝트(잔영록 / JanYeongNok)는 무협 장르이며, 본 가이드는 무협 게임 컨텍스트에 맞춰 적용한다.

---

## 핵심 원칙

1. **최상위 폴더는 반드시 프로젝트명으로 시작한다.** Content 루트에 직접 에셋을 올리지 않는다.
2. **폴더 분류는 기능/오브젝트 기반이 우선이다.** "어떤 타입인가"가 아니라 "게임에서 무엇을 담당하는가"로 묶는다.
3. **여러 오브젝트가 공용으로 쓰는 에셋만 Art/ 폴더에 넣는다.** 특정 BP에만 쓰이는 에셋은 해당 기능 폴더에 같이 둔다.
4. **에셋 타입은 접두사로 구분한다.** 폴더 이름으로 타입을 또 나누지 않는다.
5. **외부(Marketplace, Migrate) 에셋 폴더는 건드리지 않는다.** 내 폴더에 `_` 접두사를 붙여 최상단에 고정한다.

---

## 전체 폴더 구조 템플릿

```
Content/
├── _JanYeongNok/           ← 언더스코어로 최상단 고정 (내 작업 폴더)
│   │
│   ├── Core/                ← 게임 전체를 관장하는 핵심 BP (건드리지 않음)
│   │   ├── BP_JYNGameMode.uasset
│   │   ├── BP_JYNGameInstance.uasset
│   │   ├── BP_JYNPlayerController.uasset
│   │   └── BP_JYNGameState.uasset
│   │
│   ├── Characters/          ← 플레이어 캐릭터 전용 (폰, 캐릭터 BP + 전용 에셋)
│   │   ├── BP_JYNPlayerCharacter.uasset
│   │   ├── Animations/      ← 캐릭터 전용 애니메이션 시퀀스
│   │   └── Montages/        ← 캐릭터 전용 애니메이션 몽타주
│   │
│   ├── Enemies/             ← 적 캐릭터 (마교도, 도적, 보스 등)
│   │   ├── BP_JYNEnemyBase.uasset
│   │   └── [EnemyType]/
│   │
│   ├── Skills/              ← 무공 / 초식 / 비전 절기 (무협 장르 핵심)
│   │   └── [SkillName]/
│   │
│   ├── Weapons/             ← 무기 (도, 검, 암기 등)
│   │   └── [WeaponName]/
│   │
│   ├── Items/               ← 영약, 소모품 등
│   │   └── [ItemName]/
│   │
│   ├── Abilities/           ← 캐릭터 능력 BP (Projectile, AreaEffect 등)
│   │   ├── BP_JYNProjectileBase.uasset
│   │   └── BP_JYNAreaEffectBase.uasset
│   │
│   ├── Props/               ← 게임플레이에 영향을 주는 배치 오브젝트 (문, 상자, 함정 등)
│   │   └── [PropName]/
│   │
│   ├── Environment/         ← 순수 배경/데코 오브젝트 (대나무, 바위, 건물 외벽 등)
│   │   └── [EnvCategory]/
│   │
│   ├── Interactables/       ← 플레이어가 상호작용하는 오브젝트 (NPC 대화, 줍기 등)
│   │
│   ├── Input/               ← Enhanced Input
│   │   ├── Actions/         ← IA_*
│   │   └── IMC_*
│   │
│   ├── Data/                ← PrimaryDataAsset, DataTable (무공/영약/협객 데이터)
│   │
│   ├── Maps/                ← 모든 레벨(.umap) 파일
│   │   ├── MainMenu.umap
│   │   ├── BambooForest.umap
│   │   └── Test/            ← 테스트용 레벨
│   │
│   ├── UI/                  ← 모든 UMG 위젯 (WBP_, HUD 포함)
│   │   ├── HUD/
│   │   ├── Menu/
│   │   └── Inventory/
│   │
│   ├── Art/                 ← 여러 오브젝트가 공용으로 사용하는 에셋만 보관
│   │   ├── Meshes/          ← 공용 스태틱/스켈레탈 메시
│   │   ├── Textures/        ← 공용 텍스처
│   │   ├── Audio/           ← 공용 사운드 에셋
│   │   ├── VFX/             ← 공용 나이아가라/파티클
│   │   └── Materials/
│   │       ├── Master/      ← 마스터 머티리얼 (절대 직접 사용 금지, MI만 참조)
│   │       ├── Instances/   ← 머티리얼 인스턴스
│   │       └── Utility/     ← 유틸리티 머티리얼 (포스트프로세스 등)
│   │
│   └── Developers/          ← 개인 테스트 샌드박스 (빌드 제외 권장)
│       └── [YourName]/
│
├── [MarketplaceAssetFolder]/ ← Marketplace 에셋 (폴더명 변경 금지)
└── [MigratedAssetFolder]/    ← Migrate된 에셋 (폴더명 변경 금지)
```

---

## 에셋 네이밍 접두사 규칙

| 에셋 타입 | 접두사 | 예시 |
|-----------|--------|------|
| Blueprint Class | `BP_` | `BP_JYNPlayerCharacter` |
| Widget Blueprint (UMG) | `WBP_` | `WBP_HUD` |
| Static Mesh | `SM_` | `SM_Barrel` |
| Skeletal Mesh | `SK_` | `SK_PlayerMesh` |
| Material | `M_` | `M_Rock_Master` |
| Material Instance | `MI_` | `MI_Rock_01` |
| Texture | `T_` | `T_Rock_D` (D=Diffuse, N=Normal, M=Mask) |
| Animation Blueprint | `ABP_` | `ABP_Player` |
| Animation Sequence | `A_` | `A_Run_Forward` |
| Animation Montage | `AM_` | `AM_Attack_01` |
| Niagara System | `NS_` | `NS_FireEffect` |
| Sound Wave | `SW_` | `SW_FootstepGrass` |
| Sound Cue | `SC_` | `SC_Footstep` |
| Data Table | `DT_` | `DT_WeaponStats` |
| Data Asset | `DA_` | `DA_Mugong_ManCheonHwaWoo` |
| Input Action | `IA_` | `IA_Move` |
| Input Mapping Context | `IMC_` | `IMC_Default` |
| Enum | `E_` | `EJYNEnemyState` |
| Structure | `F_` | `FMugongData` |
| Interface | `I_` | `I_Interactable` |

> GAS는 본 프로젝트에서 사용하지 않으므로 `GA_`, `GE_`, `AT_` 접두사는 제외한다.

---

## 공용(Art/) vs 전용(기능 폴더) 에셋 구분 기준

| 조건 | 위치 |
|------|------|
| 2개 이상의 BP에서 참조하는 메시/텍스처 | `Art/` |
| 특정 BP 하나에서만 사용하는 메시/텍스처 | 해당 기능 폴더 (예: `Weapons/Sword/`) |
| 마스터 머티리얼 | `Art/Materials/Master/` (무조건) |
| 머티리얼 인스턴스 | 공용이면 `Art/Materials/Instances/`, 전용이면 기능 폴더 |

---

## 잔영록(무협) 장르 적용

기본 템플릿에서 본 프로젝트에 맞게 적용된 폴더:

| 폴더 | 용도 |
|------|------|
| `Skills/` | 무공 / 초식 / 비전 절기 BP, DataAsset |
| `Weapons/` | 도, 검, 암기, 비수 등 무기류 |
| `Items/` | 영약, 보패 등 게임플레이 소모/장착 아이템 |
| `Characters/NPCs/` | 마을 NPC, 상점 등 (Post-MVP) |

제거된 폴더: `Projectiles/` (없음 — 비수 등 투사체는 `Abilities/` 또는 `Weapons/`에 통합)

---

## Claude Code 작업 규칙

- 새 에셋 생성 시 반드시 위 네이밍 접두사를 적용한다.
- 에셋 배치 경로는 이 문서의 폴더 구조를 기준으로 결정한다.
- 공용 여부가 불분명한 에셋은 일단 기능 폴더에 두고, 재사용이 확인되면 `Art/`로 이동한다.
- Marketplace 또는 Migrate된 외부 폴더의 경로와 이름은 절대 변경하지 않는다.
- 새 기능 카테고리가 필요할 경우, 이 문서의 장르별 가이드를 참고해 `_JanYeongNok/` 바로 아래에 추가한다.
- 에셋 이동 시 `unreal.AssetDiscoveryService.move_asset(src, dst)`를 사용해 reference fixup을 보장한다.
