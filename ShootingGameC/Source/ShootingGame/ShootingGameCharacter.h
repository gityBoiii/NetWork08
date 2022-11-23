// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShootingGameCharacter.generated.h"

UCLASS(config=Game)
class AShootingGameCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
public:
	AShootingGameCharacter();

public:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	UAnimMontage* AnimMontage;

	UFUNCTION(Server, Reliable)
	void ReqPressTrigger(bool isPressed);

	UFUNCTION(NetMulticast, Reliable)
	void ResPressTrigger(bool isPressed);

	UFUNCTION(Server, Reliable)
	void ReqPressC();

	UFUNCTION(NetMulticast, Reliable)
	void ResPressC();

	UFUNCTION(Server, Reliable)
	void ReqPressReload();

	UFUNCTION(NetMulticast, Reliable)
	void ResPressReload();

	UFUNCTION(Server, Reliable)
	void ReqPickUp();
	
	UFUNCTION(NetMulticast, Reliable)
	void ResPickUp(AActor* weapon);

	UFUNCTION(Server, Reliable)
	void ReqDropWeapon();
	
	UFUNCTION(NetMulticast, Reliable)
	void ResDropWeapon();
	

protected:

	/** Resets HMD orientation in VR. */
	void OnResetVR();

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

	void PressTrigger();

	void ReleaseTrigger();

	void PressTestKey();

	void BindPlayerState();

	void PressReload();

	void PressPickUp();

	void PressDropWeapon();

	UFUNCTION()
	void OnRep_EquipWeapon();
	
	AActor* GetNearestWeapon();

	void EnableOwnerWeapon(AActor* actor);
	
	void DisableOwnerWeapon();

	void AttachWeapon(AActor* weapon);

	void DetachWeapon(AActor* weapon);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetControlPitch() const { return ControlPitch; }

	UFUNCTION(BlueprintCallable)
	AActor* SetEquipWeapon(AActor* Weapon);

	UFUNCTION(BlueprintCallable)
	FORCEINLINE AActor* GetEquipWeapon() const { return EquipWeapon; }

	
	UFUNCTION(BlueprintCallable)
	void OnNotifyShoot();

	UFUNCTION(BlueprintCallable)
	void OnNotifyReload();

	UFUNCTION(BlueprintNativeEvent)
	void OnUpdateHp(float CurrentHp, float MaxHp);

	void OnUpdateHp_Implementation(float CurrentHp, float MaxHp);

	UFUNCTION(BlueprintCallable)
	void DoRagdoll();

	UFUNCTION(BlueprintCallable)
	void DoGetup();

private:
	AActor* EquipWeapon;

	UPROPERTY(Replicated)
	float ControlPitch;

	bool IsRagdoll;

	FTimerHandle th_SetOwnerWeapon;

	FTimerHandle th_BindPlayerState;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UUserWidget> NameTagWidgetClass;

	UPROPERTY(BlueprintReadWrite)
	UUserWidget* NameTagWidget;
};

