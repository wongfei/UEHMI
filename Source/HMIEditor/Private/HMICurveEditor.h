#pragma once

#include "PropertyHandle.h"

class FHMICurveEditor
{
	public:
	
	TSharedPtr<class IPropertyHandle> CurveHandle;
	TSharedPtr<class SButton> CurveButton;

	FReply OnEditCurveClicked();
	void UpdateCurveButton();
};
