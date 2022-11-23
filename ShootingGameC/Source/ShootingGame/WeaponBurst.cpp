// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponBurst.h"
#include "Weapon.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "Components/AudioComponent.h"
#include "ShootingGameHUD.h"
#include "ShootingGameInstance.h"

void AWeaponBurst::PressTrigger_Implementation(bool isPressed)
{
	//		눌렸을때 
	if (isPressed)
	{
		FTimerManager& timerManager = GetWorld()->GetTimerManager();
		timerManager.SetTimer(th_BurstShooting, this, &AWeaponBurst::BurstShooting, 0.2f, true); //	연발 설정
		BurstShooting();
	}//		땠을때 
	else
	{
		FTimerManager& timerManager = GetWorld()->GetTimerManager();
		timerManager.ClearTimer(th_BurstShooting);
	}
}

void AWeaponBurst::BurstShooting()
{
	bool IsUse = false;
	IsCanUse(IsUse);

	if (IsUse == false)
		return;

	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), WeaponData->FireEffect, Mesh->GetSocketLocation("Muzzle"), Mesh->GetSocketRotation("Muzzle"), FVector(0.3f, 0.3f, 0.3f));

	Audio->Play();

	APlayerController* shooter = GetWorld()->GetFirstPlayerController();
	if (shooter == OwnChar->GetController())
	{
		FVector forward = shooter->PlayerCameraManager->GetActorForwardVector();

		FVector start = (forward * 350) + shooter->PlayerCameraManager->GetCameraLocation();
		FVector end = (forward * 5000) + shooter->PlayerCameraManager->GetCameraLocation();
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("Client - ReqShoot")));
		ReqShoot(start, end);
	}

}
