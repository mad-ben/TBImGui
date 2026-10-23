// Copyright Lewis Scrivens. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Input/Events.h"
#include "Layout/Geometry.h"

class FTBImGuiEditorScroll
{
public:
	void MouseDown(const FPointerEvent& Event);
	bool MouseMove(const FGeometry& Geometry, const FPointerEvent& Event);
	bool MouseUp(const FPointerEvent& Event);
	void Reset();
	void CaptureLost();
	void Apply(const FGeometry& Geometry, bool bDragScrolling = false);
	bool IsPanning() const;

private:
	FVector2D Origin = FVector2D::ZeroVector;
	float PendingY = 0.0f;
	bool bRightDown = false;
	bool bPanning = false;
};
