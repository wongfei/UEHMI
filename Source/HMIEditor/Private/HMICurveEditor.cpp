#include "HMICurveEditor.h"

#include "Subsystems/AssetEditorSubsystem.h"

static UObject* GetOwningAsset(TSharedPtr<IPropertyHandle> Handle)
{
	TArray<UObject*> OuterObjects;
	Handle->GetOuterObjects(OuterObjects);
	return OuterObjects.Num() > 0 ? OuterObjects[0] : nullptr;
}

FReply FHMICurveEditor::OnEditCurveClicked()
{
	if (!CurveHandle.IsValid())
		return FReply::Handled();

	TArray<void*> RawData;
	CurveHandle->AccessRawData(RawData);
	if (RawData.Num() == 0)
		return FReply::Handled();

	FRuntimeFloatCurve* Curve = static_cast<FRuntimeFloatCurve*>(RawData[0]);
	if (!Curve)
		return FReply::Handled();

	const FString GuidStr = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString PackageName = FString::Printf(TEXT("/Temp/HMI_Curve_%s"), *GuidStr);
	UPackage* Package = CreatePackage(*PackageName);

	UCurveFloat* TmpCurve = NewObject<UCurveFloat>(
		Package,
		*FString::Printf(TEXT("HMI_Curve_%s"), *GuidStr),
		RF_Transient | RF_Transactional);

	TmpCurve->FloatCurve = Curve->EditorCurveData;

	if (TmpCurve->FloatCurve.GetNumKeys() == 0)
	{
		TmpCurve->FloatCurve.AddKey(0, 0);
		TmpCurve->FloatCurve.AddKey(1, 1);
	}

	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!AssetEditorSubsystem)
		return FReply::Handled();

	AssetEditorSubsystem->OpenEditorForAsset(TmpCurve);
	
	TWeakObjectPtr<UCurveFloat> WeakTmpCurve = TmpCurve;
	TWeakPtr<IPropertyHandle> WeakHandle = CurveHandle;

	FDelegateHandle DelegateHandle;
	DelegateHandle = AssetEditorSubsystem->OnAssetClosedInEditor().AddLambda(
		[this, WeakTmpCurve, WeakHandle, Curve, AssetEditorSubsystem, DelegateHandle](UObject* ClosedAsset, IAssetEditorInstance* EditorInst)
	{
		if (WeakTmpCurve.IsValid() && ClosedAsset == WeakTmpCurve.Get())
		{
			if (Curve->EditorCurveData != WeakTmpCurve->FloatCurve)
			{
				Curve->EditorCurveData = WeakTmpCurve->FloatCurve;

				if (TSharedPtr<IPropertyHandle> Handle = WeakHandle.Pin())
				{
					Handle->NotifyPostChange(EPropertyChangeType::ValueSet);

					UObject* OwningAsset = GetOwningAsset(Handle);
					if (OwningAsset)
					{
						OwningAsset->Modify();
						OwningAsset->MarkPackageDirty();

						//UpdateCurveButton(WeakTmpCurve->FloatCurve.GetNumKeys() > 0);
						UpdateCurveButton();
					}
				}
			}

			if (UAssetEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
			{
				Subsystem->OnAssetClosedInEditor().Remove(DelegateHandle);
			}
		}
	});

	return FReply::Handled();
}

void FHMICurveEditor::UpdateCurveButton()
{
	if (CurveButton.IsValid())
	{
		bool HaveCurveKeys = false;
		if (CurveHandle.IsValid())
		{
			TArray<void*> RawData;
			CurveHandle->AccessRawData(RawData);
			if (RawData.Num() > 0)
			{
				FRuntimeFloatCurve* Curve = static_cast<FRuntimeFloatCurve*>(RawData[0]);
				if (Curve && Curve->GetRichCurveConst())
				{
					HaveCurveKeys = Curve->GetRichCurveConst()->GetNumKeys() > 0;
				}
			}
		}

		CurveButton->SetColorAndOpacity(HaveCurveKeys ? FLinearColor::Green : FLinearColor::Gray);
		CurveButton->SetContent(
			SNew(STextBlock)
			.Text(HaveCurveKeys ? FText::FromString("Curve*") : FText::FromString("Curve "))
		);
	}
}
