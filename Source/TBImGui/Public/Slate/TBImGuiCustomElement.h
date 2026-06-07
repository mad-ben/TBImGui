// Copyright Lewis Scrivens. All Rights Reserved.

#pragma once

#include <Rendering/RenderingCommon.h>

THIRD_PARTY_INCLUDES_START
#include <imgui.h>
#include <imgui_threaded_rendering.h>
THIRD_PARTY_INCLUDES_END

/// @brief ICustomSlateElement that snapshots ImDrawData on the game thread and renders it
/// natively on the render thread using a custom shader (TBImGui.usf)
class TBIMGUI_API FTBImGuiCustomElement : public ICustomSlateElement
{
	BEGIN_SHADER_PARAMETER_STRUCT(FTBImGuiRenderParameters, )
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()
	
	friend class SImGuiWidget;
	
public:

	/// @brief Use ImDrawDataSnapshot to snapshot the supplied ImGui draw data via a swap.
	/// @warning Transfers draw-list contents out of *InDrawData into the snapshot (move/swap).
	void SetDrawData_GameThread(ImDrawData* InDrawData, double InImGuiTime, UTexture* InMissingTexture, const FGeometry& InGeometry, uint64 InDrawFrame);

	/// Begin ICustomSlateElement Override
	virtual void Draw_RenderThread(FRDGBuilder& GraphBuilder, const FDrawPassInputs& Inputs) override;
	/// End ICustomSlateElement Override

private:

	/// Capture TextureResources resolving ImTextureID into the render thread owned resource.
	/// (Cache GameThread: UTexture2D -> RenderThread: FTextureRHIRef)
	void SnapshotTextureResources_GameThread();
	
	ImDrawDataSnapshot DrawDataSnapshot;
	FGeometry CachedGeometry = FGeometry();
	
	struct FTBImGuiTextureData
	{
		FTextureRHIRef TextureRHI = nullptr;
		bool bSRGB = false;
	};
	
	/// Per [ImDrawList][ImDrawCmd] Cached texture render thread handles for the active 
	/// frame to avoid touching UObjects on the RT.
	TArray<TArray<FTBImGuiTextureData>> TextureResources;
	
	/// Missing texture to use when ImTextureID is invalid or the texture resource can't be found.
	FTBImGuiTextureData MissingTexture;
	
	/// This instances last frame data was updated.
	uint64 LastDrawFrame = 0;
};