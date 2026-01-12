// Fill out your copyright notice in the Description page of Project Settings.

#include "Actor/Turret/GeoTurretBase.h"

#include "Components/CapsuleComponent.h"

// ---------------------------------------------------------------------------------------------------------------------
AGeoTurretBase::AGeoTurretBase()
{
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	SetRootComponent(CapsuleComponent);
	CapsuleComponent->SetCollisionProfileName(TEXT("GeoCapsule"));
}
