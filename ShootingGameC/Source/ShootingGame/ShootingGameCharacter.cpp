// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShootingGameCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "WeaponInterface.h"
#include "ShootingPlayerState.h"
#include "Weapon.h"
#include "TimerManager.h"
#include "Blueprint/UserWidget.h"
#include "NameTagInterface.h"

//////////////////////////////////////////////////////////////////////////
// AShootingGameCharacter

AShootingGameCharacter::AShootingGameCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)

	ConstructorHelpers::FObjectFinder<UAnimMontage> montage(TEXT("AnimMontage'/Game/RifleAnimsetPro/Animations/InPlace/Rifle_ShootOnce_Montage.Rifle_ShootOnce_Montage'"));

	AnimMontage = montage.Object;

	IsRagdoll = false;
}

void AShootingGameCharacter::BeginPlay()
{
	Super::BeginPlay();

	BindPlayerState();
}

void AShootingGameCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority() == true)
	{
		ControlPitch = GetControlRotation().Pitch;
	}
}

float AShootingGameCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, 
		FString::Printf(TEXT("TakeDamage Damage=%f EventInstigator=%s"), DamageAmount, *EventInstigator->GetName()));

	AShootingPlayerState* ps = Cast<AShootingPlayerState>(GetPlayerState());
	if (ps)
	{
		ps->AddDamage(DamageAmount);
	}

	return 0.0f;
}

void AShootingGameCharacter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShootingGameCharacter, ControlPitch);
}

//////////////////////////////////////////////////////////////////////////
// Input

void AShootingGameCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AShootingGameCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AShootingGameCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AShootingGameCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AShootingGameCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AShootingGameCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AShootingGameCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AShootingGameCharacter::OnResetVR);

	// Shoot
	PlayerInputComponent->BindAction("Trigger", IE_Pressed, this, &AShootingGameCharacter::PressTrigger);
	PlayerInputComponent->BindAction("Trigger", IE_Released, this, &AShootingGameCharacter::ReleaseTrigger);

	// TestKey
	PlayerInputComponent->BindAction("TestKey", IE_Pressed, this, &AShootingGameCharacter::PressTestKey);

	// Reload
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &AShootingGameCharacter::PressReload);
	
	// PickUp
	PlayerInputComponent->BindAction("PickUp", IE_Pressed, this, &AShootingGameCharacter::PressPickUp);

	// DropWeapon
	PlayerInputComponent->BindAction("DropWeapon", IE_Pressed, this, &AShootingGameCharacter::PressDropWeapon);


}

AActor* AShootingGameCharacter::SetEquipWeapon(AActor* Weapon)
{
	if (IsValid(EquipWeapon))
	{
		DisableOwnerWeapon();
	}
	
	ResPickUp(Weapon);

	EnableOwnerWeapon(Weapon);

	return EquipWeapon;
}

void AShootingGameCharacter::OnNotifyShoot()
{
	IWeaponInterface* InterfaceObj = Cast<IWeaponInterface>(EquipWeapon);

	if (InterfaceObj)
	{
		InterfaceObj->Execute_NotifyShoot(EquipWeapon);
	}
}

void AShootingGameCharacter::OnNotifyReload()
{
	IWeaponInterface* InterfaceObj = Cast<IWeaponInterface>(EquipWeapon);

	if (InterfaceObj)
	{
		InterfaceObj->Execute_NotifyReload(EquipWeapon);
	}
}

void AShootingGameCharacter::OnUpdateHp_Implementation(float CurrentHp, float MaxHp)
{
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow,
		FString::Printf(TEXT("OnUpdateHp CurrentHp : %f"), CurrentHp));

	if (CurrentHp <= 0)
	{
		DoRagdoll();
	}
}

void AShootingGameCharacter::DoRagdoll()
{
	IsRagdoll = true;

	GetMesh()->SetSimulatePhysics(true);
}

void AShootingGameCharacter::DoGetup()
{
	IsRagdoll = false;

	GetMesh()->SetSimulatePhysics(false);

	GetMesh()->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);

	FVector loc = { 0.0f, 0.0f, -97.0f };
	FRotator Rot = { 0.0f, 270.0f, 0.0f };
	GetMesh()->SetRelativeLocationAndRotation(loc, Rot);
}

void AShootingGameCharacter::ReqPressTrigger_Implementation(bool isPressed)
{
	ResPressTrigger(isPressed);
}

void AShootingGameCharacter::ResPressTrigger_Implementation(bool isPressed)
{
	IWeaponInterface* InterfaceObj = Cast<IWeaponInterface>(EquipWeapon);

	if (InterfaceObj)
	{
		InterfaceObj->Execute_PressTrigger(EquipWeapon, isPressed);
	}
}

void AShootingGameCharacter::ReqPressC_Implementation()
{
	ResPressC();
}

void AShootingGameCharacter::ResPressC_Implementation()
{
	if (IsRagdoll)
	{
		DoGetup();
	}
	else
	{
		DoRagdoll();
	}
}

void AShootingGameCharacter::ReqPressReload_Implementation()
{
	ResPressReload();
}
void AShootingGameCharacter::ResPressReload_Implementation()
{
	IWeaponInterface* InterfaceObj = Cast<IWeaponInterface>(EquipWeapon);

	if (InterfaceObj)
	{
		InterfaceObj->Execute_PressReload(EquipWeapon);
	}
}

void AShootingGameCharacter::ReqPickUp_Implementation()
{
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("PressPickUp")));

	AActor* nearestActor = GetNearestWeapon();	//		search for NearestWeapon
	if (nearestActor)
	{
		SetEquipWeapon(nearestActor);			//		Equip it
	}

	
}

void AShootingGameCharacter::ResPickUp_Implementation(AActor* weapon)
{
	//		만약 끼고 있으면
	if (IsValid(EquipWeapon))
	{
		IWeaponInterface* InterfaceObj = Cast<IWeaponInterface>(EquipWeapon);

		if (InterfaceObj)
		{
			InterfaceObj->Execute_DetachWeapon(EquipWeapon, this);
		}
	}

	AttachWeapon(weapon);
	
	//		아니면
	EquipWeapon = weapon;	// 무기 정하기 

	IWeaponInterface* InterfaceObj = Cast<IWeaponInterface>(weapon);
	if (InterfaceObj)
	{
		InterfaceObj->Execute_AttachWeapon(weapon, this);
	}
	
}

void AShootingGameCharacter::OnResetVR()
{
	// If ShootingGame is added to a project via 'Add Feature' in the Unreal Editor the dependency on HeadMountedDisplay in ShootingGame.Build.cs is not automatically propagated
	// and a linker error will result.
	// You will need to either:
	//		Add "HeadMountedDisplay" to [YourProject].Build.cs PublicDependencyModuleNames in order to build successfully (appropriate if supporting VR).
	// or:
	//		Comment or delete the call to ResetOrientationAndPosition below (appropriate if not supporting VR)
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AShootingGameCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void AShootingGameCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void AShootingGameCharacter::PressTrigger()
{
	ReqPressTrigger(true);
}

void AShootingGameCharacter::ReleaseTrigger()
{
	ReqPressTrigger(false);
}

void AShootingGameCharacter::PressTestKey()
{
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("PressTestKey")));

	ReqPressC();
}



void AShootingGameCharacter::BindPlayerState()
{
	AShootingPlayerState* ps = Cast<AShootingPlayerState>(GetPlayerState());
	if (IsValid(ps))
	{
		ps->Fuc_Dele_UpdateHp_TwoParams.AddUFunction(this, FName("OnUpdateHp"));
		OnUpdateHp(ps->GetCurHp(), ps->GetMaxHp());
		return;
	}

	FTimerManager& timerManager = GetWorld()->GetTimerManager();
	timerManager.SetTimer(th_BindPlayerState, this, &AShootingGameCharacter::BindPlayerState, 0.1f, false);
}

void AShootingGameCharacter::PressReload()
{
	ReqPressReload();
}

void AShootingGameCharacter::PressPickUp()
{
	ReqPickUp();
}

void AShootingGameCharacter::PressDropWeapon()
{
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("PressDropWeapon")));
	
	ReqDropWeapon();
	
}
void AShootingGameCharacter::ReqDropWeapon_Implementation()
{
	DisableOwnerWeapon();
	ResDropWeapon();
}
void AShootingGameCharacter::ResDropWeapon_Implementation()
{
	DetachWeapon(EquipWeapon);
	
}


void AShootingGameCharacter::OnRep_EquipWeapon()
{
	IWeaponInterface* InterfaceObj = Cast<IWeaponInterface>(EquipWeapon);
	if (InterfaceObj)
	{
		InterfaceObj->Execute_AttachWeapon(EquipWeapon, this);
	}
	
}

AActor* AShootingGameCharacter::GetNearestWeapon()	// 가까운 무기 가져오기 
{
	
	float nearestDist = 999999.0f;
	AActor* nearestActor = nullptr;
	TArray<AActor*> actors;
	GetCapsuleComponent()->GetOverlappingActors(actors, AWeapon::StaticClass());	// this와 겹치는 Weapon 액터에 담음

	for (AActor* weapon : actors)
	{
		IWeaponInterface* InterfaceObj = Cast<IWeaponInterface>(weapon);		// 가진 무기 없으면 
		if (InterfaceObj == nullptr)
			continue;
		float dist = FVector::Distance(GetActorLocation(), weapon->GetActorLocation());	// 자기 자신과 거리 
		//(GetActorLocation() - weapon->GetActorLocation()).Size();
		
		if (dist > nearestDist)	// 거리 비교해서
			continue;

		nearestDist = dist;			// 가장 가까운 거리
		nearestActor = weapon; 	// 가장 가까운 무기
	}
	return nearestActor;
}

void AShootingGameCharacter::EnableOwnerWeapon(AActor* actor)
{
	actor->SetOwner(GetController());//		무기 소유자 설정
	
}

void AShootingGameCharacter::DisableOwnerWeapon()
{
	//		예외 처리_무기가 있을때만 
	if(IsValid(EquipWeapon))
		EquipWeapon->SetOwner(nullptr);	//		무기 소유자 해제
}

void AShootingGameCharacter::AttachWeapon(AActor* weapon)
{
	EquipWeapon = weapon;	// 무기 정하기 

	IWeaponInterface* InterfaceObj = Cast<IWeaponInterface>(weapon);
	if (InterfaceObj)
	{
		InterfaceObj->Execute_AttachWeapon(weapon, this);
	}
	
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	
}

void AShootingGameCharacter::DetachWeapon(AActor* weapon)
{
	IWeaponInterface* InterfaceObj = Cast<IWeaponInterface>(EquipWeapon);
	if (InterfaceObj)
	{
		InterfaceObj->Execute_DetachWeapon(EquipWeapon, this);
	}

	EquipWeapon = nullptr;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
}



void AShootingGameCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AShootingGameCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AShootingGameCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AShootingGameCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}
