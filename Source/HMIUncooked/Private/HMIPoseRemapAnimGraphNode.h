#pragma once

#include "AnimGraphNode_Base.h"
#include "Remap/HMIPoseRemapAnimNode.h"
#include "HMIPoseRemapAnimGraphNode.generated.h"

UCLASS(MinimalAPI, ClassGroup="HMI|Remap")
class UHMIPoseRemapAnimGraphNode : public UAnimGraphNode_Base
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="HMI|Remap")
	FHMIPoseRemapAnimNode Node;

	public:

	// UEdGraphNode interface
	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetMenuCategory() const override;
	// End of UEdGraphNode interface
};
