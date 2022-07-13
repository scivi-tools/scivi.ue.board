// Fill out your copyright notice in the Description page of Project Settings.


#include "UI_Blank.h"
#include "Components/WidgetComponent.h"

// Sets default values
AUI_Blank::AUI_Blank()
{
	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("Widget"));
	WidgetComponent->SetBlendMode(EWidgetBlendMode::Transparent);
	RootComponent = WidgetComponent;
}

void AUI_Blank::SetWidgetClass(TSubclassOf<UUserWidget> widget_class, const FVector2D& draw_size)
{
	WidgetComponent->SetWidgetClass(widget_class);
	WidgetComponent->SetDrawSize(draw_size);
}

UUserWidget* AUI_Blank::GetWidget()
{
	return WidgetComponent->GetWidget();
}

void AUI_Blank::SetEnabled(bool enabled)
{
	if (IsValid(WidgetComponent->GetWidget())) 
	{
		auto root = WidgetComponent->GetWidget()->GetRootWidget();
		if (IsValid(root))
			root->SetIsEnabled(enabled);
	}
}

void AUI_Blank::SetVisibility(bool visibility)
{
	SetActorHiddenInGame(!visibility);
	SetActorEnableCollision(visibility);
}
