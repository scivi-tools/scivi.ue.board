// Fill out your copyright notice in the Description page of Project Settings.


#include "RichButton.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SButton.h"
#include "Components/ButtonSlot.h"

#define LOCTEXT_NAMESPACE "UMG"
/////////////////////////////////////////////////////
// URichButton

static FButtonStyle* DefaultButtonStyle = nullptr;

URichButton::URichButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (DefaultButtonStyle == nullptr)
	{
		// HACK: THIS SHOULD NOT COME FROM CORESTYLE AND SHOULD INSTEAD BE DEFINED BY ENGINE TEXTURES/PROJECT SETTINGS
		DefaultButtonStyle = new FButtonStyle(FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button"));

		// Unlink UMG default colors from the editor settings colors.
		DefaultButtonStyle->UnlinkColors();
	}

	WidgetStyle = *DefaultButtonStyle;

	ColorAndOpacity = FLinearColor::White;
	BackgroundColor = FLinearColor::White;

	ClickMethod = EButtonClickMethod::DownAndUp;
	TouchMethod = EButtonTouchMethod::DownAndUp;

	IsFocusable = true;

#if WITH_EDITORONLY_DATA
	AccessibleBehavior = ESlateAccessibleBehavior::Summary;
	bCanChildrenBeAccessible = false;
#endif
}

void URichButton::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	MyButton.Reset();
}

TSharedRef<SWidget> URichButton::RebuildWidget()
{
	MyButton = SNew(SButton)
		.OnClicked(BIND_UOBJECT_DELEGATE(FOnClicked, SlateHandleClicked))
		.OnPressed(BIND_UOBJECT_DELEGATE(FSimpleDelegate, SlateHandlePressed))
		.OnReleased(BIND_UOBJECT_DELEGATE(FSimpleDelegate, SlateHandleReleased))
		.OnHovered_UObject(this, &ThisClass::SlateHandleHovered)
		.OnUnhovered_UObject(this, &ThisClass::SlateHandleUnhovered)
		.ButtonStyle(&WidgetStyle)
		.ClickMethod(ClickMethod)
		.TouchMethod(TouchMethod)
		.PressMethod(PressMethod)
		.IsFocusable(IsFocusable)
		;

	if (GetChildrenCount() > 0)
	{
		Cast<UButtonSlot>(GetContentSlot())->BuildSlot(MyButton.ToSharedRef());
	}

	return MyButton.ToSharedRef();
}

void URichButton::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	MyButton->SetColorAndOpacity(ColorAndOpacity);
	MyButton->SetBorderBackgroundColor(BackgroundColor);
}

UClass* URichButton::GetSlotClass() const
{
	return UButtonSlot::StaticClass();
}

void URichButton::OnSlotAdded(UPanelSlot* InSlot)
{
	// Add the child to the live slot if it already exists
	if (MyButton.IsValid())
	{
		CastChecked<UButtonSlot>(InSlot)->BuildSlot(MyButton.ToSharedRef());
	}
}

void URichButton::OnSlotRemoved(UPanelSlot* InSlot)
{
	// Remove the widget from the live slot if it exists.
	if (MyButton.IsValid())
	{
		MyButton->SetContent(SNullWidget::NullWidget);
	}
}

void URichButton::SetStyle(const FButtonStyle& InStyle)
{
	WidgetStyle = InStyle;
	if (MyButton.IsValid())
	{
		MyButton->SetButtonStyle(&WidgetStyle);
	}
}

void URichButton::SetColorAndOpacity(FLinearColor InColorAndOpacity)
{
	ColorAndOpacity = InColorAndOpacity;
	if (MyButton.IsValid())
	{
		MyButton->SetColorAndOpacity(InColorAndOpacity);
	}
}

void URichButton::SetBackgroundColor(FLinearColor InBackgroundColor)
{
	BackgroundColor = InBackgroundColor;
	if (MyButton.IsValid())
	{
		MyButton->SetBorderBackgroundColor(InBackgroundColor);
	}
}

bool URichButton::IsPressed() const
{
	if (MyButton.IsValid())
	{
		return MyButton->IsPressed();
	}

	return false;
}

void URichButton::SetClickMethod(EButtonClickMethod::Type InClickMethod)
{
	ClickMethod = InClickMethod;
	if (MyButton.IsValid())
	{
		MyButton->SetClickMethod(ClickMethod);
	}
}

void URichButton::SetTouchMethod(EButtonTouchMethod::Type InTouchMethod)
{
	TouchMethod = InTouchMethod;
	if (MyButton.IsValid())
	{
		MyButton->SetTouchMethod(TouchMethod);
	}
}

void URichButton::SetPressMethod(EButtonPressMethod::Type InPressMethod)
{
	PressMethod = InPressMethod;
	if (MyButton.IsValid())
	{
		MyButton->SetPressMethod(PressMethod);
	}
}

void URichButton::PostLoad()
{
	Super::PostLoad();

	if (GetChildrenCount() > 0)
	{
		//TODO UMG Pre-Release Upgrade, now buttons have slots of their own.  Convert existing slot to new slot.
		if (UPanelSlot* PanelSlot = GetContentSlot())
		{
			UButtonSlot* ButtonSlot = Cast<UButtonSlot>(PanelSlot);
			if (ButtonSlot == NULL)
			{
				ButtonSlot = NewObject<UButtonSlot>(this);
				ButtonSlot->Content = GetContentSlot()->Content;
				ButtonSlot->Content->Slot = ButtonSlot;
				Slots[0] = ButtonSlot;
			}
		}
	}
}

FReply URichButton::SlateHandleClicked()
{
	OnClicked.Broadcast(this, FVector2D(0,0));

	return FReply::Handled();
}

void URichButton::SlateHandlePressed()
{
	OnPressed.Broadcast(this, FVector2D(0, 0));
}

void URichButton::SlateHandleReleased()
{
	OnReleased.Broadcast(this, FVector2D(0, 0));
}

void URichButton::SlateHandleHovered()
{
	OnHovered.Broadcast(this);
}

void URichButton::SlateHandleUnhovered()
{
	OnUnhovered.Broadcast(this);
}

#if WITH_ACCESSIBILITY
TSharedPtr<SWidget> URichButton::GetAccessibleWidget() const
{
	return MyButton;
}
#endif

#if WITH_EDITOR

const FText URichButton::GetPaletteCategory()
{
	return LOCTEXT("Common", "Common");
}

#endif
#undef LOCTEXT_NAMESPACE