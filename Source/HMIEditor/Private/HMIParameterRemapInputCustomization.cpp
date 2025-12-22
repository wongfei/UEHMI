#include "HMIParameterRemapInputCustomization.h"
#include "Remap/HMIParameterRemapAsset.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyTypeCustomization.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"

TSharedRef<IPropertyTypeCustomization> FHMIParameterRemapInputCustomization::MakeInstance()
{
	return MakeShareable(new FHMIParameterRemapInputCustomization);
}

void FHMIParameterRemapInputCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	NameHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FHMIParameterRemapInput, Name));
	WeightHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FHMIParameterRemapInput, Weight));
	CurveHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FHMIParameterRemapInput, Curve));

	HeaderRow
	.NameContent()
	[
		SNew(STextBlock)
		.Text(FText::FromString("IN"))
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	.MinDesiredWidth(800)
	.MaxDesiredWidth(1500)
	[
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(300.0f)
			[
				NameHandle->CreatePropertyValueWidget()
			]
		]

		+SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(5, 0)
		[
			SNew(SBox)
			.WidthOverride(150.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.4f)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Weight:"))
					.Font(IDetailLayoutBuilder::GetDetailFont())
				]
				+ SHorizontalBox::Slot()
				[
					WeightHandle->CreatePropertyValueWidget()
				]
			]
		]

		+SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SAssignNew(CurveButton, SButton)
			.OnClicked(this, &FHMICurveEditor::OnEditCurveClicked)
			.ToolTipText(FText::FromString("Customize curve"))
		]
	];

	UpdateCurveButton();
}

void FHMIParameterRemapInputCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}
