// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ShootingGameHUD.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTINGGAME_API AShootingGameHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class UUserWidget> HudWidgetClass;

	UPROPERTY(BlueprintReadWrite)
	UUserWidget* HudWidget;

public:
	UFUNCTION(BlueprintNativeEvent)
	void OnUpdateMyHp(float CurrentHp, float MaxHp);

	void OnUpdateMyHp_Implementation(float CurrentHp, float MaxHp);

	UFUNCTION(BlueprintNativeEvent)
	void OnUpdateMyAmmo(int Ammo);

	void OnUpdateMyAmmo_Implementation(int Ammo);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void BindPlayerState();

	FTimerHandle th_BindPlayerState;
};
