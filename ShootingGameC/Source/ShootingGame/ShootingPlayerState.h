// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ShootingPlayerState.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FDele_Multi_UpdateHp_TwoParams, float, float);
/**
 * 
 */
UCLASS()
class SHOOTINGGAME_API AShootingPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	AShootingPlayerState();

protected:
	UPROPERTY(ReplicatedUsing = OnRep_CurHp)
	float CurHp;

	UPROPERTY(ReplicatedUsing = OnRep_MaxHp)
	float MaxHp;

public:
	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetCurHp() const { return CurHp; }

	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetMaxHp() const { return MaxHp; }

public:
	UFUNCTION()
	void OnRep_CurHp();

	UFUNCTION()
	void OnRep_MaxHp();

	UFUNCTION(BlueprintCallable)
	void AddDamage(float Damage);

	FDele_Multi_UpdateHp_TwoParams Fuc_Dele_UpdateHp_TwoParams;
};
