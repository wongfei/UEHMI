#include "HMIPoseRemapAnimGraphNode.h"

#define LOCTEXT_NAMESPACE "HMI"

FText UHMIPoseRemapAnimGraphNode::GetMenuCategory() const
{
	return LOCTEXT("UHMIPoseRemapAnimGraphNode_Category", "HMI");
}

FText UHMIPoseRemapAnimGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("UHMIPoseRemapAnimGraphNode_Title", "HMI Pose Remap");
}

FText UHMIPoseRemapAnimGraphNode::GetTooltipText() const
{
	return LOCTEXT("UHMIPoseRemapAnimGraphNode_Tooltip", "HMI Pose Remap");
}

#undef LOCTEXT_NAMESPACE
