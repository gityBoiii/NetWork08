// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "Components/AudioComponent.h"
#include "ShootingGameHUD.h"
#include "ShootingGameInstance.h"

// Sets default values
AWeapon::AWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));

	RootComponent = Mesh;

	Audio = CreateDefaultSubobject<UAudioComponent>(TEXT("Audio"));
	Audio->SetupAttachment(RootComponent);

	bReplicates = true;
	SetReplicateMovement(true);

	Ammo = 30;
}

void AWeapon::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, OwnChar);
	DOREPLIFETIME(AWeapon, RowName);
	DOREPLIFETIME(AWeapon, Ammo);
}

// Called when the game starts or when spawned
void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	
	WeaponData = Cast<UShootingGameInstance>(GetGameInstance())->GetWeaponRowData(RowName);
	if (WeaponData)
	{
		Mesh->SetStaticMesh(WeaponData->StaticMesh);
		Audio->SetSound(WeaponData->SoundBase);
		Mesh->SetCollisionProfileName("Weapon");	//		Collision Profile Name
		Mesh->SetSimulatePhysics(true);				//		Physics On
	}
}

// Called every frame
void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeapon::PressTrigger_Implementation(bool isPressed)
{
	if(isPressed)
		OwnChar->PlayAnimMontage(WeaponData->ShootMontage);
}

void AWeapon::NotifyShoot_Implementation()
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

void AWeapon::NotifyReload_Implementation()
{
	DoReload();
	

	
}

void AWeapon::PressReload_Implementation()
{
	OwnChar->PlayAnimMontage(WeaponData->ReloadMontage);
}

void AWeapon::IsCanUse_Implementation(bool& IsCanUse)
{
	if (Ammo <= 0)
	{
		IsCanUse = false;
		return;
	}

	IsCanUse = true;
}

void AWeapon::AttachWeapon_Implementation(ACharacter* targetChar)
{
	//		장착
	OwnChar = targetChar;
	Mesh->SetSimulatePhysics(false);	//	physics off(to attach)
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);	// Collision off
	AttachToComponent(targetChar->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		FName("Weapon"));	//		Attach it to socket 
	UpdateAmmoToHud(Ammo);
}

void AWeapon::DetachWeapon_Implementation(ACharacter* targetChar)
{
	//		탄약 0
	//Ammo = 0;	// 모두 영향을 미침
	UpdateAmmoToHud(0);

	//		장착 해제 
	OwnChar = nullptr;
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // Collision on
	Mesh->SetSimulatePhysics(true);	//	physics on(to detach)
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
}

void AWeapon::OnRep_Ammo()
{
	UpdateAmmoToHud(Ammo);
}

void AWeapon::UpdateAmmoToHud(int newAmmo)
{
	//UI 출력 연결
	APlayerController* firstPlayer = GetWorld()->GetFirstPlayerController();

	if (IsValid(OwnChar))
	{
		if (OwnChar->GetController() == firstPlayer)
		{
			AShootingGameHUD* Hud = Cast<AShootingGameHUD>(firstPlayer->GetHUD());
			if (IsValid(Hud))
			{
				Hud->OnUpdateMyAmmo(newAmmo);
			}
		}
	}

}

void AWeapon::DoReload()
{
	Ammo = WeaponData->MaxAmmo;
	OnRep_Ammo();	//	Ammo 바뀐 것 HUD로 반영 
}

bool AWeapon::UseAmmo()
{
	if (Ammo <= 0)
	{
		return false;
	}
	
	Ammo = Ammo - 1;
	OnRep_Ammo();
	return true;

	// whether Ammo left or not
	
}

void AWeapon::ReqShoot_Implementation(const FVector vStart, const FVector vEnd)
{
	if (UseAmmo() == false)
		return;
	

	FHitResult result;
	bool isHit = GetWorld()->LineTraceSingleByObjectType(result, vStart, vEnd, ECollisionChannel::ECC_Pawn);

	DrawDebugLine(GetWorld(), vStart, vEnd, FColor::Yellow, false, 5.0f);

	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("Server - ReqShoot")));

	if (isHit)
	{
		ACharacter* HitChar = Cast<ACharacter>(result.GetActor());
		if (HitChar)
		{
			UGameplayStatics::ApplyDamage(HitChar, 10, OwnChar->GetController(), this, UDamageType::StaticClass());
		}
	}
}

