#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JYNLevelUpScreen.generated.h"

class UButton;
class UTextBlock;

/** 무공 카드 종류 — 카드 선택 시 어떤 효과를 적용할지 식별
 *  ⚠️ Live Coding 호환을 위해 기존 항목 순서/값은 절대 변경하지 말 것!
 *     새 항목은 반드시 끝에 추가. */
UENUM(BlueprintType)
enum class EJYNMugongCardType : uint8
{
	None            UMETA(DisplayName="없음"),
	// ── 기존 항목 (순서 변경 금지) ──
	AmkiSpeedUp     UMETA(DisplayName="암기 속도 강화 (deprecated)"),
	NaegongAbsorb   UMETA(DisplayName="내공 흡수 강화"),
	IronBody        UMETA(DisplayName="철신공"),
	Agility         UMETA(DisplayName="신행술"),
	PoisonFang      UMETA(DisplayName="독침 강화 (deprecated)"),
	NaegongRegen    UMETA(DisplayName="내공 재생"),
	NoForm          UMETA(DisplayName="무형지기"),
	ChasingBullet   UMETA(DisplayName="추혼탄 (deprecated)"),
	GreatPill       UMETA(DisplayName="대환단"),
	StormRain       UMETA(DisplayName="광풍세우 (deprecated)"),

	// ── 신규 항목 (끝에만 추가) ──
	HPRegen         UMETA(DisplayName="회복신공"),       // HP 초당 회복 +1
	NaegongMax      UMETA(DisplayName="내공 증진"),      // 최대 내공 +20
	Amgi            UMETA(DisplayName="암기술"),         // 4단계 순환
	Pyochang        UMETA(DisplayName="표창"),           // 회전 표창 활성화 / +1개
	PyochangSpeed   UMETA(DisplayName="표창 가속"),      // 회전 속도 +25%
	PyochangDamage  UMETA(DisplayName="표창 강화"),      // 데미지 +20%
	XPMagnet        UMETA(DisplayName="혼령통찰"),       // XP Soul 흡수 범위 +100cm
};

/** 카드 한 장의 데이터 */
USTRUCT(BlueprintType)
struct FJYNMugongCardInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category="Card")
	FText CardName;

	UPROPERTY(BlueprintReadWrite, Category="Card")
	FText Description;

	/** 등급: 백(0) 청(1) 자(2) 금(3) */
	UPROPERTY(BlueprintReadWrite, Category="Card")
	int32 Grade = 0;

	/** 카드 효과 종류 */
	UPROPERTY(BlueprintReadWrite, Category="Card")
	EJYNMugongCardType CardType = EJYNMugongCardType::None;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMugongCardPicked, int32, CardIndex);

/**
 * 레벨업 시 표시되는 3-카드 선택 화면
 *
 * WBP에서 아래 위젯 이름과 정확히 일치해야 BindWidget이 연결됨:
 *   Card1Button, Card2Button, Card3Button
 *   Card1Name,   Card2Name,   Card3Name
 *   Card1Desc,   Card2Desc,   Card3Desc
 */
UCLASS(abstract)
class UJYNLevelUpScreen : public UUserWidget
{
	GENERATED_BODY()

	// ── BindWidget ────────────────────────────────────────
protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton>    Card1Button;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton>    Card2Button;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton>    Card3Button;

	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Card1Name;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Card2Name;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Card3Name;

	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Card1Desc;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Card2Desc;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Card3Desc;

	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> LevelTitle;

	// ── 공개 API ─────────────────────────────────────────
public:
	/** GameMode가 레벨업 시 호출 — 카드 3장 데이터 + 레벨 세팅 */
	UFUNCTION(BlueprintCallable, Category="LevelUp")
	void SetupCards(int32 NewLevel, const TArray<FJYNMugongCardInfo>& Cards);

	/** 카드 선택 이벤트 (GameMode가 구독) */
	UPROPERTY(BlueprintAssignable, Category="LevelUp")
	FOnMugongCardPicked OnCardPicked;

protected:
	virtual void NativeConstruct() override;

private:
	TArray<FJYNMugongCardInfo> CurrentCards;

	UFUNCTION() void OnCard1Clicked();
	UFUNCTION() void OnCard2Clicked();
	UFUNCTION() void OnCard3Clicked();

	void PickCard(int32 Index);
	void SetCardText(UTextBlock* NameBlock, UTextBlock* DescBlock, const FJYNMugongCardInfo& Info);
};
