// Fill out your copyright notice in the Description page of Project Settings.


#include "ShootingGameInstance.h"

FST_Weapon* UShootingGameInstance::GetWeaponRowData(FName name)
{
	return WeaponTable->FindRow<FST_Weapon>(name, TEXT(""));
}
