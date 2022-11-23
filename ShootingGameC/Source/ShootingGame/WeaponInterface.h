// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WeaponInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(Blueprintable)
class UWeaponInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class SHOOTINGGAME_API IWeaponInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void PressTrigger(bool isPressed);

	virtual void PressTrigger_Implementation(bool isPressed) {};

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void NotifyShoot();

	virtual void NotifyShoot_Implementation() {};

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void NotifyReload();

	virtual void NotifyReload_Implementation() {};


	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void PressReload();

	virtual void PressReload_Implementation() {};

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void IsCanUse(bool& IsCanUse);

	virtual void IsCanUse_Implementation(bool& IsCanUse) {};

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void AttachWeapon(ACharacter* targetChar);

	virtual void AttachWeapon_Implementation(ACharacter* targetChar) {};

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void DetachWeapon(ACharacter* targetChar);

	virtual void DetachWeapon_Implementation(ACharacter* targetChar) {};
};
