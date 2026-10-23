// Copyright Lewis Scrivens. All Rights Reserved.

#include "Slate/SImGuiWidget.h"

#include <Engine/Texture.h>
#include <Framework/Application/SlateApplication.h>
#include <RenderingThread.h>

#include "TBImGuiTypes.h"
#include "TBImGuiContext.h"

#if IMGUI_RENDER_MODE == IMGUI_RENDER_MODE_NATIVE
#include "Slate/TBImGuiCustomElement.h"
#endif

void SImGuiWidget::Construct(const FArguments& InArgs)
{
	checkf(InArgs._Context.IsValid(), TEXT("Trying to construct ImGui widget with invalid context!"));
	
	OwningContext = InArgs._Context;
	bIsOwnedByApp = InArgs._IsOwnedByApp;
	
	switch (InArgs._Context->GetInputMode())
	{
		case ETBImGuiInputMode::All:
		{
			bInputEnabled = true;
		}
		break;
		case ETBImGuiInputMode::MainViewportOnly:
		{
			bInputEnabled = bIsOwnedByApp;
		}
		break;
		case ETBImGuiInputMode::ViewportsOnly:
		{
			bInputEnabled = !bIsOwnedByApp;
		}
		break;
		case ETBImGuiInputMode::None:
		{
			bInputEnabled = false;
		}
		break;
		default: break;
	}
	
#if IMGUI_RENDER_MODE == IMGUI_RENDER_MODE_NATIVE
	for (int32 i = 0; i < TBIMGUI_DRAWBUFFERS; ++i)
	{
		ImGuiDrawers[i] = MakeShared<FTBImGuiCustomElement, ESPMode::ThreadSafe>();
	}
#endif
	
	SetContentSize(InArgs._InitialContentSize);
	UpdateVisibility();
	ForceVolatile(true);
	
	if (InArgs._OnTick.IsBound())
	{
		OnTickEvent = InArgs._OnTick;
		SetCanTick(true);
	}
}

SImGuiWidget::~SImGuiWidget()
{
#if IMGUI_RENDER_MODE == IMGUI_RENDER_MODE_NATIVE
	// Can only be destroyed after drawing the last ImGui command.
	for (int32 i = 0; i < TBIMGUI_DRAWBUFFERS; ++i)
	{
		TSharedPtr<FTBImGuiCustomElement> ImGuiDrawer = ImGuiDrawers[i];

		ENQUEUE_RENDER_COMMAND(ResetImGuiDrawer)(
			[ImGuiDrawer](FRHICommandListImmediate& RHICmdList) mutable
			{
				ImGuiDrawer.Reset();
			}
		);
	}
#endif
}

void SImGuiWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SLeafWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	FTBImGuiContextScope ContextScope(OwningContext);
	OnTickEvent.ExecuteIfBound();
}

int32 SImGuiWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TBImGui_SImGuiWidget_OnPaint);
	
	FTBImGuiContextScope ContextScope(OwningContext);
	if (!ContextScope.IsValid())
	{
		return LayerId;
	}
	
#if IMGUI_RENDER_MODE == IMGUI_RENDER_MODE_NATIVE
	{
		if (!CachedDrawData || !CachedDrawData->Valid)
		{
			return LayerId;
		}
		
		UTexture* const MissingTexture = ContextScope->GetMissingTexture();
		TSharedPtr<FTBImGuiCustomElement> ImGuiDrawer = ImGuiDrawers[CurrentDrawBuffer];
		ImGuiDrawer->SetDrawData_GameThread(CachedDrawData, ImGui::GetTime(), 
			MissingTexture, AllottedGeometry, DrawFrameCounter);
		
		OutDrawElements.PushClip(FSlateClippingZone(MyCullingRect));
		{
			FSlateDrawElement::MakeCustom(OutDrawElements, LayerId, ImGuiDrawer);
		}
		OutDrawElements.PopClip();
	}
#elif IMGUI_RENDER_MODE == IMGUI_RENDER_MODE_SLATE
	{
		if (CachedDrawData.DrawLists.IsEmpty())
		{
			return LayerId;
		}
		
		const FSlateRenderTransform Transform(AllottedGeometry.GetAccumulatedRenderTransform().GetTranslation() - FVector2d(CachedDrawData.DisplayPos));
		
		FSlateBrush TextureBrush;
		
		TArray<FSlateVertex> Vertices;
		TArray<SlateIndex> Indices;
		TArray<FSlateVertex> CmdVertices;
		TArray<SlateIndex> CmdIndices;
		
		for (const ImDrawList* const DrawList : CachedDrawData.DrawLists)
		{
			if (!ensureMsgf(DrawList, TEXT("Cannot render stale draw list in slate?! Skipping...")))
			{
				continue;
			}
			
			Vertices.SetNumUninitialized(DrawList->VtxBuffer.Size);
			
			ImDrawVert* SrcVertex = DrawList->VtxBuffer.Data;
			FSlateVertex* DstVertex = Vertices.GetData();
			for (int32 BufferIdx = 0; BufferIdx < Vertices.Num(); ++BufferIdx, ++SrcVertex, ++DstVertex)
			{
				DstVertex->TexCoords[0] = SrcVertex->uv.x;
				DstVertex->TexCoords[1] = SrcVertex->uv.y;
				DstVertex->TexCoords[2] = 1;
				DstVertex->TexCoords[3] = 1;
				DstVertex->Position = TransformPoint(Transform, FVector2f(SrcVertex->pos));
				DstVertex->Color.Bits = SrcVertex->col;
			}
			
			ImGui::CopyArray(DrawList->IdxBuffer, Indices);
			
			for (const ImDrawCmd& DrawCmd : DrawList->CmdBuffer)
			{
				UTexture* Texture = DrawCmd.GetTexID();
				if (TextureBrush.GetResourceObject() != Texture)
				{
					if (!IsValid(Texture) && !GExitPurge)
					{
						Texture = ContextScope->GetMissingTexture();
					}
					
					if (IsValid(Texture))
					{
						TextureBrush.ImageSize.X = Texture->GetSurfaceWidth();
						TextureBrush.ImageSize.Y = Texture->GetSurfaceHeight();
						TextureBrush.ImageType = ESlateBrushImageType::FullColor;
						TextureBrush.DrawAs = ESlateBrushDrawType::Image;
						TextureBrush.SetResourceObject(Texture);
					}
					else
					{
#if IMGUI_MISSING_TEXTURE_MODE == IMGUI_MISSING_TEXTURE_NONE
						continue;
#endif
						
						TextureBrush.ImageSize.X = 0;
						TextureBrush.ImageSize.Y = 0;
						TextureBrush.ImageType = ESlateBrushImageType::NoImage;
						TextureBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
						TextureBrush.SetResourceObject(nullptr);
					}
				}
				
				FSlateRect ClipRect(DrawCmd.ClipRect.x, DrawCmd.ClipRect.y, DrawCmd.ClipRect.z, DrawCmd.ClipRect.w);
				ClipRect = TransformRect(Transform, ClipRect);
				
				OutDrawElements.PushClip(FSlateClippingZone(ClipRect));
				{
					const int32 VertexCount = Vertices.Num() - DrawCmd.VtxOffset;
					CmdVertices.SetNumUninitialized(VertexCount);
					FMemory::Memcpy(CmdVertices.GetData(), Vertices.GetData() + DrawCmd.VtxOffset, VertexCount * sizeof(FSlateVertex));
					
					CmdIndices.SetNumUninitialized(DrawCmd.ElemCount);
					FMemory::Memcpy(CmdIndices.GetData(), Indices.GetData() + DrawCmd.IdxOffset, DrawCmd.ElemCount * sizeof(SlateIndex));
					
					FSlateDrawElement::MakeCustomVerts(OutDrawElements, LayerId, TextureBrush.GetRenderingResource(), 
						CmdVertices, CmdIndices, nullptr, 0, 0);
				}
				OutDrawElements.PopClip();
			}
		}
	}
#endif
	
	return LayerId;
}

FVector2D SImGuiWidget::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	if (LayoutScaleMultiplier <= UE_SMALL_NUMBER)
	{
		return ContentSize;
	}
	
	return ContentSize / static_cast<double>(LayoutScaleMultiplier);
}

FReply SImGuiWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& Event)
{
	return HandleKeyEvent(Event, true);
}

FReply SImGuiWidget::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& Event)
{
	return HandleKeyEvent(Event, false);
}

FReply SImGuiWidget::OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& Event)
{
	FTBImGuiContextScope ContextScope(OwningContext);

	ImGuiIO& IO = ImGui::GetIO();
	IO.AddInputCharacter(Event.GetCharacter());

	return IO.WantTextInput ? FReply::Handled() : FReply::Unhandled();
}

FReply SImGuiWidget::OnAnalogValueChanged(const FGeometry& MyGeometry, const FAnalogInputEvent& Event)
{
	FTBImGuiContextScope ContextScope(OwningContext);
	
	ImGuiIO& IO = ImGui::GetIO();
	const float Value = Event.GetAnalogValue();
	IO.AddKeyAnalogEvent(ImGui::ToImGuiKey(Event.GetKey()), FMath::Abs(Value) > 0.1f, Value);
	
	return FReply::Handled();
}

FReply SImGuiWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& Event)
{
	return HandleMouseButtonEvent(Event, ETBMouseButtonEvent::Pressed);
}

FReply SImGuiWidget::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& Event)
{
	return HandleMouseButtonEvent(Event, ETBMouseButtonEvent::DoubleClick);
}

FReply SImGuiWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& Event)
{
	return HandleMouseButtonEvent(Event, ETBMouseButtonEvent::Released);
}

FReply SImGuiWidget::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& Event)
{
	FTBImGuiContextScope ContextScope(OwningContext);
	
	ImGuiIO& IO = ImGui::GetIO();
	
	IO.AddMouseWheelEvent(UE_REAL_TO_FLOAT(Event.GetGestureDelta().X), Event.GetWheelDelta());
	
	return FReply::Handled();
}

FReply SImGuiWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& Event)
{
	FTBImGuiContextScope ContextScope(OwningContext);
	
	ImGuiIO& IO = ImGui::GetIO();
	
	FVector2f MousePosition = Event.GetScreenSpacePosition();
	if (!(IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
	{
		MousePosition -= ImGui::GetScreenSpaceAbsolutePosition();
	}
	
	IO.AddMousePosEvent(MousePosition.X, MousePosition.Y);
	
	return FReply::Unhandled();
}

FReply SImGuiWidget::OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent)
{
	FTBImGuiContextScope ContextScope(OwningContext);
	
	ImGuiIO& IO = ImGui::GetIO();
	IO.AddFocusEvent(true);
	
	return FReply::Handled();
}

FCursorReply SImGuiWidget::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	FTBImGuiContextScope ContextScope(OwningContext);
	
	ImGuiIO& IO = ImGui::GetIO();
	
	const bool bHasActiveMouse = (IO.ConfigFlags & ImGuiConfigFlags_NoMouse) == 0;
	const bool bHasMouseCursorUpdate = (IO.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) == 0;
	if (bHasActiveMouse && bHasMouseCursorUpdate)
	{
		const EMouseCursor::Type NewCursor = ImGui::ToUnrealMouseCursor(ImGui::GetMouseCursor());
		return FCursorReply::Cursor(NewCursor);
	}
	
	return FCursorReply::Unhandled();
}

void SImGuiWidget::OnFocusLost(const FFocusEvent& InFocusEvent)
{
	FTBImGuiContextScope ContextScope(OwningContext);
	
	ImGuiIO& IO = ImGui::GetIO();
	IO.AddFocusEvent(false);
}

TSharedPtr<FTBImGuiContext> SImGuiWidget::GetOwningContext() const
{
	return OwningContext;
}

bool SImGuiWidget::SupportsKeyboardFocus() const
{
	return bInputEnabled;
}

void SImGuiWidget::SetInputEnabled(bool bEnabled)
{
	if (bInputEnabled == bEnabled)
	{
		return;
	}
	
	bInputEnabled = bEnabled;
	
	UpdateVisibility();
	
	if (!bInputEnabled)
	{
		if (HasKeyboardFocus())
		{
			FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
		}

		if (HasMouseCapture())
		{
			FSlateApplication::Get().ReleaseAllPointerCapture();
		}
	}
}

bool SImGuiWidget::IsInputEnabled() const
{
	return bInputEnabled;
}

void SImGuiWidget::UpdateDrawData(ImDrawData* InDrawData)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TBImGui_SImGuiWidget_UpdateDrawData);
	
#if IMGUI_RENDER_MODE == IMGUI_RENDER_MODE_NATIVE
	// Render thread can lag a max of 1 frame behind the game thread [r.OneFrameThreadLag=1].
	// Rotating FTBImGuiCustomElement instances each tick allows the RT to read last frames draw data, 
	// while the GT writes to a different instance, avoiding the need for expensive copies of draw data 
	// between the GT & RT.
	CurrentDrawBuffer = FMath::Modulo(CurrentDrawBuffer + 1, TBIMGUI_DRAWBUFFERS);
	CachedDrawData = InDrawData;
	DrawFrameCounter++;
#elif IMGUI_RENDER_MODE == IMGUI_RENDER_MODE_SLATE
	CachedDrawData.Update(InDrawData);
#endif
}

void SImGuiWidget::SetContentSize(const FVector2D& InContentSize)
{
	if (!ContentSize.Equals(InContentSize))
	{
		ContentSize = InContentSize;
		Invalidate(EInvalidateWidgetReason::Layout);
	}
}

FVector2D SImGuiWidget::GetContentSize() const
{
	return ContentSize;
}

#if IMGUI_RENDER_MODE == IMGUI_RENDER_MODE_SLATE
void SImGuiWidget::FTBImGuiDrawData::Update(const ImDrawData* InDrawData)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TBImGui_FTBImGuiDrawData_Update);
	
	Empty();

	if (!InDrawData || !InDrawData->Valid)
	{
		return;
	}

	DisplayPos = InDrawData->DisplayPos;
	DisplaySize = InDrawData->DisplaySize;

	DrawLists.Reserve(InDrawData->CmdLists.Size);
	for (ImDrawList* const CmdList : InDrawData->CmdLists)
	{
		if (CmdList)
		{
			// AK CHANGES - ImGui context teardown: the clone is only read here, so it leaves the context's shared data. Registered, a
			// clone still cached when the window closes outlives the context (imgui_draw.cpp DrawLists.Size == 0 ensure).
			ImDrawList* const DrawList = CmdList->CloneOutput();
			DrawList->_SetDrawListSharedData(nullptr);
			DrawLists.Add(DrawList);
		}
	}
}

void SImGuiWidget::FTBImGuiDrawData::Empty()
{
	for (ImDrawList* const DrawList : DrawLists)
	{
		IM_DELETE(DrawList);
	}
	
	DrawLists.Empty();
}
#endif

bool SImGuiWidget::ShouldHandleKeyEvent(const FKeyEvent& InEvent) const
{
#if WITH_EDITORONLY_DATA
	// Discard input events if the game thread is paused.
	if (GIntraFrameDebuggingGameThread)
	{
		return false;
	}
#endif
	
	return true;
}

FReply SImGuiWidget::HandleKeyEvent(const FKeyEvent& InEvent, bool bKeyDown)
{
	if (!ShouldHandleKeyEvent(InEvent))
	{
		return FReply::Unhandled();
	}
	
	FTBImGuiContextScope ContextScope(OwningContext);
	
	ImGuiIO& IO = ImGui::GetIO();
	IO.AddKeyEvent(ImGui::ToImGuiKey(InEvent.GetKey()), bKeyDown);
	IO.AddKeyEvent(ImGuiMod_Ctrl, InEvent.IsControlDown());
	IO.AddKeyEvent(ImGuiMod_Shift, InEvent.IsShiftDown());
	IO.AddKeyEvent(ImGuiMod_Alt, InEvent.IsAltDown());
	IO.AddKeyEvent(ImGuiMod_Super, InEvent.IsCommandDown());
	
	return IO.WantCaptureKeyboard ? FReply::Handled() : FReply::Unhandled();
}

FReply SImGuiWidget::HandleMouseButtonEvent(const FPointerEvent& InEvent, const ETBMouseButtonEvent InMouseButtonEvent)
{
	FTBImGuiContextScope ContextScope(OwningContext);
	if (!ContextScope.IsValid())
	{
		return FReply::Unhandled();
	}
	ImGuiIO& IO = ImGui::GetIO();
	IO.AddKeyEvent(ImGuiMod_Ctrl, InEvent.IsControlDown());
	IO.AddKeyEvent(ImGuiMod_Shift, InEvent.IsShiftDown());
	IO.AddKeyEvent(ImGuiMod_Alt, InEvent.IsAltDown());
	IO.AddKeyEvent(ImGuiMod_Super, InEvent.IsCommandDown());
	const bool bReleased = InMouseButtonEvent == ETBMouseButtonEvent::Released;
	if (!IO.WantCaptureMouse && !bReleased)
	{
		return FReply::Unhandled();
	}

	const ImGuiMouseButton_ MouseButton = ImGui::ToImGuiMouseButton(InEvent.GetEffectingButton());
	if (MouseButton < 0 || MouseButton >= 3)
	{
		return FReply::Unhandled();
	}
	IO.AddMouseButtonEvent(MouseButton, !bReleased);

	switch (InMouseButtonEvent)
	{
	case ETBMouseButtonEvent::Pressed:
	{
		return FReply::Handled().CaptureMouse(AsShared()).PreventThrottling();
	}
	case ETBMouseButtonEvent::Released:
	{
		if (HasMouseCapture())
		{
			return FReply::Handled().ReleaseMouseCapture();
		}
		break;
	}
	case ETBMouseButtonEvent::DoubleClick:
	{
		return FReply::Handled();
	}
	default:
		break;
	}

	return FReply::Unhandled();
}

void SImGuiWidget::UpdateVisibility()
{
	if (bInputEnabled)
	{
		SetVisibility(EVisibility::Visible);
	}
	else
	{
		SetVisibility(EVisibility::HitTestInvisible);
	}
}
