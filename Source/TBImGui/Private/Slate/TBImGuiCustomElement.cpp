// Copyright Lewis Scrivens. All Rights Reserved.

#include "Slate/TBImGuiCustomElement.h"

#include <Containers/Array.h>
#include <Engine/Texture.h>
#include <RenderGraphBuilder.h>

#include <TBImGuiShaders.h>

namespace TBImGuiRendering
{
	class FImGuiVertexDeclaration : public FRenderResource
	{
	public:
		
		FVertexDeclarationRHIRef VertexDeclarationRHI;
		
		virtual void InitRHI(FRHICommandListBase& RHICmdList) override
		{
			FVertexDeclarationElementList Elements;
			const uint16 Stride = sizeof(ImDrawVert);
			Elements.Add(FVertexElement(0, STRUCT_OFFSET(ImDrawVert, pos), VET_Float2, 0, Stride));
			Elements.Add(FVertexElement(0, STRUCT_OFFSET(ImDrawVert, uv),  VET_Float2, 1, Stride));
			Elements.Add(FVertexElement(0, STRUCT_OFFSET(ImDrawVert, col), VET_Color,  2, Stride));
			VertexDeclarationRHI = PipelineStateCache::GetOrCreateVertexDeclaration(Elements);
		}
		
		virtual void ReleaseRHI() override
		{
			VertexDeclarationRHI.SafeRelease();
		}
	};

	static TGlobalResource<FImGuiVertexDeclaration> GImGuiVertexDeclaration;

	/// @brief Helper conversion function for slate 2D transform (2x2 linear part + 2D translation) -> 4x4 Matrix.
	static FMatrix44f SlateRenderTransformToMatrix44(const FSlateRenderTransform& Transform)
	{
		float A, B, C, D;
		Transform.GetMatrix().GetMatrix(A, B, C, D);
		const FVector2D Trans = Transform.GetTranslation();
		
		FMatrix44f M = FMatrix44f::Identity;
		M.M[0][0] = A; M.M[0][1] = B;
		M.M[1][0] = C; M.M[1][1] = D;
		M.M[3][0] = static_cast<float>(Trans.X);
		M.M[3][1] = static_cast<float>(Trans.Y);
		return M;
	}
}

void FTBImGuiCustomElement::SetDrawData_GameThread(ImDrawData* InDrawData, double InImGuiTime, UTexture* InMissingTexture, const FGeometry& InGeometry, uint64 InDrawFrame)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TBImGui_FTBImGuiCustomElement_SetDrawData_GameThread);
	
	CachedGeometry = InGeometry;

	// Flickering fix during resizing for individual viewports.
	// Detect requests to draw the same frame multiple times. (Like when resizing)
	// Don't bother recapturing data since we have already swapped it...
	if (LastDrawFrame == InDrawFrame)
	{
		return;
	}
	
	DrawDataSnapshot.SnapUsingSwap(InDrawData, InImGuiTime);
	
	MissingTexture = FTBImGuiTextureData();
	if (IsValid(InMissingTexture) && !GExitPurge)
	{
		if (FTextureResource* const MissingTextureResource = InMissingTexture->GetResource())
		{
			// Update last render time so texture streaming doesn't drop higher detail mips.
			MissingTextureResource->LastRenderTime = FApp::GetCurrentTime();
			MissingTexture.TextureRHI = MissingTextureResource->TextureRHI;
			MissingTexture.bSRGB = MissingTextureResource->bSRGB;
		}
	}

	SnapshotTextureResources_GameThread();
	
	LastDrawFrame = InDrawFrame;
}

void FTBImGuiCustomElement::SnapshotTextureResources_GameThread()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TBImGui_FTBImGuiCustomElement_SnapshotTextureResources_GameThread);
	
	const ImDrawData& DrawData = DrawDataSnapshot.DrawData;
	TextureResources.Reset();
	TextureResources.SetNum(DrawData.CmdLists.Size);
	
	if (!GExitPurge)
	{
		for (int32 ListIdx = 0; ListIdx < DrawData.CmdLists.Size; ++ListIdx)
		{
			const ImDrawList* const DrawList = DrawData.CmdLists[ListIdx];
			if (!DrawList)
			{
				continue;
			}
			
			TArray<FTBImGuiTextureData>& TextureDataList = TextureResources[ListIdx];
			TextureDataList.Reserve(DrawList->CmdBuffer.Size);
		
			for (const ImDrawCmd& DrawCmd : DrawList->CmdBuffer)
			{
				FTBImGuiTextureData& TextureData = TextureDataList.AddDefaulted_GetRef();
				UTexture* const Texture = DrawCmd.GetTexID();
				if (IsValid(Texture))
				{
					if (FTextureResource* const TextureResource = Texture->GetResource())
					{
						// Update last render time so texture streaming doesn't drop higher detail mips.
						TextureResource->LastRenderTime = FApp::GetCurrentTime();
						TextureData.TextureRHI = TextureResource->TextureRHI;
						TextureData.bSRGB = TextureResource->bSRGB;
					}
				}
			}
		}
	}
}

void FTBImGuiCustomElement::Draw_RenderThread(FRDGBuilder& GraphBuilder, const FDrawPassInputs& Inputs)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(TBImGui_FTBImGuiCustomElement_Draw_RenderThread);
	
	const ImDrawData* const DrawData = &DrawDataSnapshot.DrawData;
	if (!DrawData->Valid || DrawData->CmdLists.Size == 0 || Inputs.OutputTexture == nullptr)
	{
		return;
	}
	
	FTBImGuiRenderParameters* RenderParameters = GraphBuilder.AllocParameters<FTBImGuiRenderParameters>();
	RenderParameters->RenderTargets[0] = FRenderTargetBinding(Inputs.OutputTexture, ERenderTargetLoadAction::ELoad);
	
	FIntPoint ViewExtent = Inputs.OutputTexture->Desc.Extent;
	const FIntRect ViewRect(0, 0, ViewExtent.X, ViewExtent.Y);
	
	FIntRect ClippingRect = Inputs.SceneViewRect;
	if (ClippingRect.IsEmpty())
	{
		ClippingRect.Max = ViewExtent;
	}
	
	const FSlateRenderTransform Transform(CachedGeometry.GetAccumulatedRenderTransform().GetTranslation() - FVector2d(DrawData->DisplayPos));
	const FMatrix44f TransformMatrix = TBImGuiRendering::SlateRenderTransformToMatrix44(Transform);
	const FMatrix44f ProjectionMatrix = TransformMatrix * Inputs.ElementsMatrix;
	
	// We're using DrawData memory directly here to avoid expensive copies.
	// It is safe since SImGuiWidget rotates the custom elements per frame which keeps each
	// instances data valid alive until the RT pass is submitted. (max 1 frame behind game thread)
	
	RDG_EVENT_SCOPE(GraphBuilder, "TBImGui");
	
	GraphBuilder.AddPass(
		RDG_EVENT_NAME("TBImGui_RenderPass"),
		RenderParameters,
		ERDGPassFlags::Raster,
		[DrawData, Transform, ViewRect, ClippingRect, ProjectionMatrix, TextureResources = TextureResources, 
			MissingTexture = MissingTexture](FRHICommandList& RHICmdList)
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(TBImGui_RenderPass);
			
			FGlobalShaderMap* const ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
			TShaderMapRef<FTBImGuiVS> VertexShader(ShaderMap);
			TShaderMapRef<FTBImGuiPS> PixelShader(ShaderMap);
			
			RHICmdList.SetViewport(ViewRect.Min.X, ViewRect.Min.Y, 0.0f,
				ViewRect.Max.X, ViewRect.Max.Y, 1.0f);
			
			// Set render state.
			{
				FGraphicsPipelineStateInitializer GraphicsPSOInit;
				RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
				GraphicsPSOInit.BlendState = TStaticBlendState<CW_RGBA, BO_Add, BF_SourceAlpha, BF_InverseSourceAlpha, BO_Add, BF_One, BF_InverseSourceAlpha>::GetRHI();
				GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
				GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
				GraphicsPSOInit.PrimitiveType = PT_TriangleList;
				GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = TBImGuiRendering::GImGuiVertexDeclaration.VertexDeclarationRHI;
				GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
				GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
				SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);
			}
			
			// Set-up view projection for the vertex shader.
			{
				FRHIBatchedShaderParameters& BatchedShaderParams = RHICmdList.GetScratchShaderParameters();
				VertexShader->SetProjectionMatrix(BatchedShaderParams, ProjectionMatrix);
				RHICmdList.SetBatchedShaderParameters(VertexShader.GetVertexShader(), BatchedShaderParams);
			}
			
			FRHISamplerState* const TextureSampler = TStaticSamplerState<SF_Bilinear, AM_Wrap, AM_Wrap, AM_Wrap>::GetRHI();
			
			for (int32 ListIdx = 0; ListIdx < DrawData->CmdLists.Size; ++ListIdx)
			{
				const ImDrawList* const DrawList = DrawData->CmdLists[ListIdx];
				if (!DrawList || DrawList->VtxBuffer.Size <= 0 || DrawList->IdxBuffer.Size <= 0)
				{
					continue;
				}
				
				// Upload vertex data into the TBImGuiVB vertex buffer.
				const uint32 VBSize = static_cast<uint32>(DrawList->VtxBuffer.Size) * sizeof(ImDrawVert);
				const FRHIBufferCreateDesc VBDesc =
					FRHIBufferCreateDesc::CreateVertex(TEXT("TBImGui_VB"), VBSize)
					.AddUsage(EBufferUsageFlags::Volatile)
					.SetInitialState(ERHIAccess::VertexOrIndexBuffer)
					.SetInitActionInitializer();
				
				// Pass the vtx data directly since we are transforming the view projection 
				// matrix instead of individual points like the slate method.
				FRHIBufferInitializer VBInit = RHICmdList.CreateBufferInitializer(VBDesc);
				VBInit.WriteData(DrawList->VtxBuffer.Data, VBSize); 
				FBufferRHIRef VertexBuffer = VBInit.Finalize();
				RHICmdList.SetStreamSource(0, VertexBuffer, 0);
				
				// Pass index data into the TBImGuiIB index buffer.
				const uint32 IBSize = static_cast<uint32>(DrawList->IdxBuffer.Size) * sizeof(ImDrawIdx);
				const FRHIBufferCreateDesc IBDesc =
					FRHIBufferCreateDesc::CreateIndex(TEXT("TBImGui_IB"), IBSize, sizeof(ImDrawIdx))
					.AddUsage(EBufferUsageFlags::Volatile)
					.SetInitialState(ERHIAccess::VertexOrIndexBuffer)
					.SetInitActionInitializer();
				
				FRHIBufferInitializer IBInit = RHICmdList.CreateBufferInitializer(IBDesc);
				IBInit.WriteData(DrawList->IdxBuffer.Data, IBSize);
				FBufferRHIRef IndexBuffer = IBInit.Finalize();
				
				const TArray<FTBImGuiTextureData> EmptyTextureDataList;
				const TArray<FTBImGuiTextureData>& TextureDataList = TextureResources.IsValidIndex(ListIdx)
					? TextureResources[ListIdx]
					: EmptyTextureDataList;
				
				for (int32 CmdIdx = 0; CmdIdx < DrawList->CmdBuffer.Size; ++CmdIdx)
				{
					const ImDrawCmd& DrawCmd = DrawList->CmdBuffer[CmdIdx];
					if (DrawCmd.UserCallback != nullptr)
					{
						DrawCmd.UserCallback(DrawList, &DrawCmd);
						continue;
					}
					
					if (DrawCmd.ElemCount == 0)
					{
						continue;
					}
					
					// Set-up texture data for the current draw command.
					{
						FRHITexture* Texture = nullptr;
						bool bSRGB = false;
						
						if (TextureDataList.IsValidIndex(CmdIdx) && TextureDataList[CmdIdx].TextureRHI.IsValid())
						{
							const FTBImGuiTextureData& TextureData = TextureDataList[CmdIdx];
							Texture = TextureData.TextureRHI.GetReference();
							bSRGB = TextureData.bSRGB;
						}
						// Fall back to a missing texture when the resolved RHI is missing.
						else
						{
#if IMGUI_MISSING_TEXTURE_MODE == IMGUI_MISSING_TEXTURE_NONE
							continue;
#elif IMGUI_MISSING_TEXTURE_MODE == IMGUI_MISSING_TEXTURE_EMPTY
							Texture = GWhiteTexture->TextureRHI;
#else
							if (MissingTexture.TextureRHI.IsValid())
							{
								Texture = MissingTexture.TextureRHI;
								bSRGB = MissingTexture.bSRGB;
							}
							else
							{
								Texture = GWhiteTexture->TextureRHI;
							}
#endif
						}
						
						// Bind texture resources for the current draw command.
						FRHIBatchedShaderParameters& BatchedPS = RHICmdList.GetScratchShaderParameters();
						PixelShader->SetTexture(BatchedPS, Texture, TextureSampler, bSRGB);
						RHICmdList.SetBatchedShaderParameters(PixelShader.GetPixelShader(), BatchedPS);
					}
					
					// Apply clipping from ImGui before drawing the UI.
					{
						FSlateRect ClipRect(DrawCmd.ClipRect.x, DrawCmd.ClipRect.y, DrawCmd.ClipRect.z, DrawCmd.ClipRect.w);
						ClipRect = TransformRect(Transform, ClipRect);
						
						const int32 ScissorMinX = FMath::Clamp(FMath::FloorToInt(ClipRect.Left), ClippingRect.Min.X, ClippingRect.Max.X);
						const int32 ScissorMinY = FMath::Clamp(FMath::FloorToInt(ClipRect.Top), ClippingRect.Min.Y, ClippingRect.Max.Y);
						const int32 ScissorMaxX = FMath::Clamp(FMath::CeilToInt(ClipRect.Right), ClippingRect.Min.X, ClippingRect.Max.X);
						const int32 ScissorMaxY = FMath::Clamp(FMath::CeilToInt(ClipRect.Bottom), ClippingRect.Min.Y, ClippingRect.Max.Y);
						
						// Skip fully-clipped draw commands
						if (ScissorMaxX <= ScissorMinX || ScissorMaxY <= ScissorMinY)
						{
							continue;
						}
						
						RHICmdList.SetScissorRect(true,
	  						static_cast<uint32>(ScissorMinX), static_cast<uint32>(ScissorMinY),
	  						static_cast<uint32>(ScissorMaxX), static_cast<uint32>(ScissorMaxY));
					}
					
					const uint32 PrimitiveCount = DrawCmd.ElemCount / 3;
					RHICmdList.DrawIndexedPrimitive(IndexBuffer, DrawCmd.VtxOffset, 0, 
						static_cast<uint32>(DrawList->VtxBuffer.Size) - DrawCmd.VtxOffset,
						DrawCmd.IdxOffset, PrimitiveCount, 1);
				}
			}
			
			RHICmdList.SetScissorRect(false, 0, 0, 0, 0);
		});
}