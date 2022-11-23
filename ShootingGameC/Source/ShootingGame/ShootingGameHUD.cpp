// Fill out your copyright notice in the Description page of Project Settings.


#include "ShootingGameHUD.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "ShootingPlayerState.h"

void AShootingGameHUD::OnUpdateMyHp_Implementation(float CurrentHp, float MaxHp)
{
}

void AShootingGameHUD::OnUpdateMyAmmo_Implementation(int Ammo)
{
}

void AShootingGameHUD::BeginPlay()
{
	Super::BeginPlay();

	check(HudWidgetClass);

	HudWidget = CreateWidget<UUserWidget>(GetWorld(), HudWidgetClass);
	HudWidget->AddToViewport();

	BindPlayerState();
}

void AShootingGameHUD::BindPlayerState()
{
	//APlayerController* pc = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	APlayerController* pc = GetWorld()->GetFirstPlayerController();

	if (IsValid(pc) && pc->PlayerState != nullptr)
	{
		AShootingPlayerState* ps = Cast<AShootingPlayerState>(pc->PlayerState);
		if (IsValid(ps))
		{
			ps->Fuc_Dele_UpdateHp_TwoParams.AddUFunction(this, FName("OnUpdateMyHp"));
			OnUpdateMyHp(ps->GetCurHp(), ps->GetMaxHp());
			return;
		}
	}

	FTimerManager& timerManager = GetWorld()->GetTimerManager();
	timerManager.SetTimer(th_BindPlayerState, this, &AShootingGameHUD::BindPlayerState, 0.1f, false);
}
