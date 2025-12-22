#pragma once

#include "IPropertyTypeCustomization.h"
#include "HMICurveEditor.h"

class FHMIParameterRemapOutputCustomization : public FHMICurveEditor, public IPropertyTypeCustomization
{
	public:

	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

	private:

	TSharedPtr<IPropertyHandle> NameHandle;
	TSharedPtr<IPropertyHandle> WeightHandle;
	TSharedPtr<IPropertyHandle> InterpSpeedHandle;
};
