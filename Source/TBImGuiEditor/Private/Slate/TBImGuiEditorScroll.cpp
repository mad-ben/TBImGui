// Copyright Lewis Scrivens. All Rights Reserved.
#include "Slate/TBImGuiEditorScroll.h"

#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Slate/SImGuiWidget.h"

void FTBImGuiEditorScroll::MouseDown(const FPointerEvent& Event)
{
	Reset();
	bRightDown = Event.GetEffectingButton() == EKeys::RightMouseButton;
	Origin = Event.GetScreenSpacePosition();
}

bool FTBImGuiEditorScroll::MouseMove(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (!bRightDown || !Event.IsMouseButtonDown(EKeys::RightMouseButton))
	{
		return false;
	}
	const FVector2D Position = Event.GetScreenSpacePosition();
	if (!bPanning && FVector2D::DistSquared(Position, Origin) >= FSlateApplication::Get().GetDragTriggerDistanceSquared())
	{
		bPanning = true;
	}
	if (bPanning)
	{
		PendingY -= static_cast<float>(Event.GetCursorDelta().Y) / Geometry.GetAccumulatedLayoutTransform().GetScale();
	}
	return bPanning;
}

bool FTBImGuiEditorScroll::MouseUp(const FPointerEvent& Event)
{
	if (Event.GetEffectingButton() != EKeys::RightMouseButton)
	{
		return false;
	}
	const bool bWasPanning = bPanning;
	bRightDown = false;
	bPanning = false;
	return bWasPanning;
}

void FTBImGuiEditorScroll::Reset()
{
	PendingY = 0.0f;
	bRightDown = false;
	bPanning = false;
}

bool FTBImGuiEditorScroll::IsPanning() const
{
	return bPanning;
}

void FTBImGuiEditorScroll::CaptureLost()
{
	bRightDown = false;
	bPanning = false;
}

void FTBImGuiEditorScroll::Apply(const FGeometry& Geometry, bool bDragScrolling)
{
	if (PendingY != 0.0f)
	{
		ImGui::SetScrollY(ImGui::GetScrollY() + PendingY);
		PendingY = 0.0f;
	}
	if (!bDragScrolling && !FSlateApplication::Get().IsDragDropping())
	{
		return;
	}
	const FVector2D Mouse = Geometry.AbsoluteToLocal(FSlateApplication::Get().GetCursorPos());
	const float Top = ImGui::GetWindowPos().y + ImGui::GetFrameHeight();
	const float Bottom = ImGui::GetWindowPos().y + ImGui::GetWindowHeight();
	if (Mouse.Y >= Top && Mouse.Y <= Bottom && Mouse.X >= 0 && Mouse.X <= Geometry.GetLocalSize().X)
	{
		const float Direction = Mouse.Y < Top + 24.0f ? -1.0f : (Mouse.Y > Bottom - 24.0f ? 1.0f : 0.0f);
		ImGui::SetScrollY(ImGui::GetScrollY() + Direction * 400.0f * ImGui::GetIO().DeltaTime);
	}
}
