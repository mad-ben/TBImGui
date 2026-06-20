// Copyright Lewis Scrivens. All Rights Reserved.

#include "TBImGuiUtils.h"

#include <Interfaces/IPluginManager.h>
#include <Engine/GameViewportClient.h>
#include <Engine/TextureRenderTarget2D.h>
#include <Engine/Texture2D.h>
#include <Engine/StaticMesh.h>
#include <Engine/World.h>
#include <Widgets/SWidget.h>
#include <Widgets/SWindow.h>
#include <CanvasItem.h>
#include <CanvasTypes.h>

THIRD_PARTY_INCLUDES_START
#include <imgui.h>
#include <imgui_internal.h>
#if WITH_NETIMGUI
#include <NetImgui_Api.h>
#endif
THIRD_PARTY_INCLUDES_END

#include "TBImGuiTypes.h"
#include "TBImGuiContext.h"
#include "TBImGuiSubsystem.h"
#include "Slate/SImGuiViewportWidget.h"

static TOptional<FPluginDescriptor> CachedDescriptor;

void CachePluginDescriptor()
{
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("TBImGui"));
	if (Plugin.IsValid())
	{
		CachedDescriptor = Plugin->GetDescriptor();
	}
}

int32 FTBImGuiUtils::GetSessionId()
{
#if WITH_EDITOR
	return UE::GetPlayInEditorID();
#else
	return 0;
#endif
}

FString FTBImGuiUtils::GetConfigDir()
{
	return FPaths::GeneratedConfigDir() / FPlatformProperties::PlatformName();
}

FString FTBImGuiUtils::GetImGuiConfigPath(const FString& InContextName)
{
	return GetConfigDir() / InContextName + TEXT(".ini");
}

FString FTBImGuiUtils::GetImGuiLogPath(const FString& InContextName)
{
	return FPaths::ProjectLogDir() / InContextName + TEXT(".log");
}

int32 FTBImGuiUtils::GetPluginVersion()
{
	if (!CachedDescriptor.IsSet())
	{
		CachePluginDescriptor();
	}
	
	return CachedDescriptor.GetValue().Version;
}

FString FTBImGuiUtils::GetPluginVersionName()
{
	if (!CachedDescriptor.IsSet())
	{
		CachePluginDescriptor();
	}
	
	return CachedDescriptor.GetValue().VersionName;
}

TArray<FString> FTBImGuiUtils::GetEngineFontPaths()
{
	TArray<FString> FontPaths;
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FString FontsDirectory = FPaths::EngineContentDir() / TEXT("Slate/Fonts");
	if (PlatformFile.DirectoryExists(*FontsDirectory))
	{
		TArray<FString> FoundPaths;
		PlatformFile.FindFiles(FoundPaths, *FontsDirectory, TEXT(".ttf"));
		for (const FString& Path : FoundPaths)
		{
			if (FPaths::FileExists(Path))
			{
				FontPaths.Add(Path);
			}
		}
	}
	
	return FontPaths;
}

bool FTBImGuiUtils::CopyStringToFixedUtf8(UTF8CHAR* InDest, const int32 InDestSize, const FString& InString)
{
	if (InDestSize <= 0)
	{
		return false;
	}
	
	const FTCHARToUTF8 Converter(*InString);
	const int32 RequiredBytes = Converter.Length();
	const int32 BytesToCopy = FMath::Min(RequiredBytes, InDestSize - 1);
	FMemory::Memcpy(InDest, Converter.Get(), BytesToCopy);
	InDest[BytesToCopy] = '\0';

	return RequiredBytes + 1 <= InDestSize;
}

UTexture2D* FTBImGuiUtils::CreateColoredTexture(const FColor& InColor, const int32 InSizeX, const int32 InSizeY)
{
	UTexture2D* const Texture = UTexture2D::CreateTransient(InSizeX, InSizeY);
	if (ensureMsgf(IsValid(Texture), TEXT("Failed to create texture!")))
	{
		Texture->SRGB = true;

		FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
		void* const Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
		FColor* Pixels = static_cast<FColor*>(Data);
		const int32 NumPixels = InSizeX * InSizeY;
		for (int32 i = 0; i < NumPixels; ++i)
		{
			Pixels[i] = InColor;
		}
		
		Mip.BulkData.Unlock();
		Texture->UpdateResource();
		return Texture;
	}
	
	return nullptr;
}

UWorld* FTBImGuiUtils::GetWorld()
{
	check(IsValid(GEngine));

#if WITH_EDITOR	
	if (FWorldContext* const PlayInEditorContext = GEngine->GetWorldContextFromPIEInstance(UE::GetPlayInEditorID()))
	{
		return PlayInEditorContext->World();
	}
#endif
	
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		UWorld* const World = Context.World();
		if (!IsValid(World))
		{
			continue;
		}
		
		if (Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::GameRPC)
		{
			return Context.World();
		}
	}
	
	if (UGameViewportClient* const Viewport = GetValid(GEngine->GameViewport))
	{
		if (UWorld* const ViewportWorld = Viewport->GetWorld())
		{
			return ViewportWorld;
		}
	}
	
	return nullptr;
}

UGameInstance* FTBImGuiUtils::GetGameInstance()
{
	const UWorld* const CurrentWorld = GetWorld();
	if (!IsValid(CurrentWorld))
	{
		return nullptr;
	}
	
	return CurrentWorld->GetGameInstance();
}

TSharedPtr<SWidget> FTBImGuiUtils::GetViewportWidgetInternal(const ImGuiViewport* InViewport)
{
	if (FTBImGuiViewportData* const FoundViewportData = FTBImGuiViewportData::Find(InViewport))
	{
		if (FoundViewportData->ImGuiWidget.IsValid())
		{
			return FoundViewportData->ImGuiWidget.Pin();
		}
	}
	
	return nullptr;
}

TSharedPtr<FTBImGuiContext> FTBImGuiUtils::GetRuntimeContext()
{
	UTBImGuiSubsystem* const ImGuiSubsystem = UTBImGuiSubsystem::GetMutable();
	if (!IsValid(ImGuiSubsystem))
	{
		return nullptr;
	}
	
	return ImGuiSubsystem->GetContext();
}

TSharedPtr<FTBImGuiContext> FTBImGuiUtils::GetRuntimeContext(const UGameInstance* InGameInstance)
{
	if (!IsValid(InGameInstance))
	{
		return nullptr;
	}
	
	UTBImGuiSubsystem* const ImGuiSubsystem = InGameInstance->GetSubsystem<UTBImGuiSubsystem>();
	if (!IsValid(ImGuiSubsystem))
	{
		return nullptr;
	}
	
	return ImGuiSubsystem->GetContext();
}

TSharedPtr<FTBImGuiContext> FTBImGuiUtils::CreateWindowContext(const TSharedRef<SWindow>& InWindow, const FTBImGuiContextParams& InContextParams)
{
	const TSharedRef<FTBImGuiContext> Context = FTBImGuiContext::CreateContext(InContextParams);
	
	FTBImGuiContextScope ContextScope(Context);
	check(ContextScope.IsValid());
	
	if (FTBImGuiViewportData* const ViewportData = FTBImGuiViewportData::FindOrCreate(ImGui::GetMainViewport()))
	{
		ViewportData->Window = InWindow;
		const TSharedRef<SImGuiWidget> ImGuiWidget = SAssignNew(ViewportData->ImGuiWidget, SImGuiWidget)
			.IsOwnedByApp(false)
			.Context(Context);

		InWindow->SetContent(ImGuiWidget);
	}

	return Context;
}

FTBImGuiViewportData* FTBImGuiUtils::FindViewportForWindow(const TSharedPtr<SWindow>& InWindow)
{
	if (!InWindow.IsValid())
	{
		return nullptr;
	}

	for (ImGuiViewport* const Viewport : ImGui::GetPlatformIO().Viewports)
	{
		FTBImGuiViewportData* const ViewportData = FTBImGuiViewportData::Find(Viewport);
		if (!ViewportData)
		{
			continue;
		}
		
		if (ViewportData->Window == InWindow)
		{
			return ViewportData;
		}
	}

	return nullptr;
}

FTBImGuiViewportData* FTBImGuiUtils::FindViewportForWidget(const TSharedPtr<SWidget>& InWidget)
{
	if (!InWidget.IsValid())
	{
		return nullptr;
	}

	for (ImGuiViewport* const Viewport : ImGui::GetPlatformIO().Viewports)
	{
		FTBImGuiViewportData* const ViewportData = FTBImGuiViewportData::Find(Viewport);
		if (!ViewportData)
		{
			continue;
		}
		
		if (ViewportData->ImGuiWidget == InWidget)
		{
			return ViewportData;
		}
	}

	return nullptr;
}

void FTBImGuiUtils::SetAllViewportInputEnabled(bool bEnableInput)
{
	ImGui::ForEachViewport([bEnableInput] (ImGuiViewportP& InViewport)
	{
		SetViewportInputEnabled(&InViewport, bEnableInput);
		return true;
	});
}

void FTBImGuiUtils::SetViewportInputEnabled(const ImGuiViewport* InViewport, bool bEnableInput)
{
	if (!InViewport)
	{
		return;
	}
	
	if (const FTBImGuiViewportData* const ViewportData = FTBImGuiViewportData::Find(InViewport))
	{
		TSharedPtr<SImGuiWidget> ImGuiWidget = ViewportData->ImGuiWidget.Pin();
		if (!ImGuiWidget.IsValid())
		{
			UE_LOG(LogTBImGui, Warning, TEXT("ImGui Viewport ID [%i] has no overlay! Cannot update input mode for viewport! Continuing..."), InViewport->ID);
			return;
		}
		
		ImGuiWidget->SetInputEnabled(bEnableInput);
	}
}

#if WITH_IMPLOT3D
bool FTBImGuiUtils::ExtractMesh(const UStaticMesh* InMesh, TArray<FVector3f>& OutVertices, TArray<uint32>& OutIndices, const int32 LODIndex)
{
	OutVertices.Empty();
	OutIndices.Empty();
		 
	const FStaticMeshRenderData* const RenderData = IsValid(InMesh) ? InMesh->GetRenderData() : nullptr;
	if (!RenderData || !RenderData->LODResources.IsValidIndex(LODIndex))
	{
		return false;
	}

	const FStaticMeshLODResources& LOD = RenderData->LODResources[LODIndex];
	const FPositionVertexBuffer& Positions = LOD.VertexBuffers.PositionVertexBuffer;
	if (Positions.GetAllocatedSize() == 0 || LOD.IndexBuffer.GetIndexDataSize() == 0)
	{
		return false;
	}

	OutVertices.SetNumUninitialized(Positions.GetNumVertices());

	for (uint32 Index = 0; Index < Positions.GetNumVertices(); ++Index)
	{
		OutVertices[Index] = Positions.VertexPosition(Index);
	}
		
	LOD.IndexBuffer.GetCopy(OutIndices);
		
	return !OutVertices.IsEmpty() && OutIndices.Num() >= 3;
}
#endif

#if WITH_NETIMGUI
bool FTBImGuiUtils::SendTextureToNetImGui(UTexture2D* InTexture)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FTBImGuiUtils_SendTextureToNetImGui);
	
	if (!IsValid(InTexture))
	{
		return false;
	}
	
	if (!NetImgui::IsConnected())
	{
		UE_LOG(LogTBImGui, Verbose, TEXT("Cannot send texture [%s] to NetImGui, not connected!"), *InTexture->GetName());
		return false;
	}
	
	const int32 SizeX = InTexture->GetSizeX();
	const int32 SizeY = InTexture->GetSizeY();
	
	// Fast path: If CPU bulk data is still resident.
	FTexturePlatformData* const PlatformData = InTexture->GetPlatformData();
	if (PlatformData && PlatformData->Mips.Num() > 0)
	{
		if (InTexture->NeverStream && PlatformData->Mips[0].BulkData.IsBulkDataLoaded())
		{
			FTexture2DMipMap& Mip = PlatformData->Mips[0];
			if (void* const Data = Mip.BulkData.Lock(LOCK_READ_ONLY))
			{
				const uint32 ByteSize = SizeX * SizeY * sizeof(FColor);
				NetImgui::SendDataTexture(InTexture, Data, static_cast<uint16>(SizeX),
					static_cast<uint16>(SizeY), NetImgui::eTexFormat::kTexFmtRGBA8, ByteSize);
			
				Mip.BulkData.Unlock();
				return true;
			}
		}
	}

	// Slower path: Blit source into a transient render target.
	// We could read surface data but the RHI could be block compressed which causes a crash.
	// This could be improved to check for block compression and do the cheaper read from surface.
	
	UE_LOG(LogTBImGui, Verbose, TEXT("Falling back to GPU for reading [%s] texture data to send to NetImGui."), *InTexture->GetName());
	
	InTexture->SetForceMipLevelsToBeResident(1.0f, 0);
	InTexture->WaitForStreaming(true, false);
	
	FTextureResource* const TextureResource = InTexture->GetResource();
	if (!TextureResource)
	{
		UE_LOG(LogTBImGui, Warning, TEXT("Cannot send texture [%s] to NetImGui, no resource found on the game thread!"), *InTexture->GetName());
		return false;
	}
	
	UTextureRenderTarget2D* const RenderTarget = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), NAME_None, RF_Transient);
	check(IsValid(RenderTarget));
	
	RenderTarget->ClearColor = FLinearColor::Transparent;
	RenderTarget->TargetGamma = 1.0f;
	RenderTarget->bAutoGenerateMips = false;
	RenderTarget->InitCustomFormat(SizeX, SizeY, PF_B8G8R8A8, false);
	RenderTarget->UpdateResourceImmediate(true);
	
	FTextureRenderTargetResource* const RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
	if (!RenderTargetResource)
	{
		return false;
	}
	
	FCanvas Canvas(RenderTargetResource, nullptr, FGameTime(), GMaxRHIFeatureLevel);
	Canvas.SetWriteDestinationAlpha(true);
	
	FCanvasTileItem Tile(FVector2D::ZeroVector, TextureResource, FVector2D(SizeX, SizeY), FLinearColor::White);
	Tile.BlendMode = SE_BLEND_Opaque;
	Canvas.DrawItem(Tile);
	Canvas.Flush_GameThread();
	
	TArray<FColor> Pixels;
	FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
	ReadFlags.SetLinearToGamma(false);
	if (!RenderTargetResource->ReadPixels(Pixels, ReadFlags))
	{
		UE_LOG(LogTBImGui, Warning, TEXT("Cannot send texture [%s] to NetImGui, failed to read pixels!"), *InTexture->GetName());
		return false;
	}
	
	if (Pixels.Num() != SizeX * SizeY)
	{
		UE_LOG(LogTBImGui, Warning, TEXT("Cannot send texture [%s] to NetImGui, texture size differs from pixel data!"), *InTexture->GetName());
		return false;
	}
	
	const uint32 ByteSize = static_cast<uint32>(Pixels.Num() * sizeof(FColor));
	NetImgui::SendDataTexture(InTexture, Pixels.GetData(), static_cast<uint16>(SizeX),
		static_cast<uint16>(SizeY), NetImgui::eTexFormat::kTexFmtRGBA8, ByteSize);
	
	return true;
}

bool FTBImGuiUtils::SendTextureToNetImGui(UTextureRenderTarget2D* InTexture)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FTBImGuiUtils_SendTextureToNetImGui_RenderTarget);
	
	if (!IsValid(InTexture))
	{
		return false;
	}
	
	if (!NetImgui::IsConnected())
	{
		UE_LOG(LogTBImGui, Verbose, TEXT("Cannot send texture [%s] to NetImGui, not connected!"), *InTexture->GetName());
		return false;
	}
	
	FTextureRenderTargetResource* const Resource = InTexture->GameThread_GetRenderTargetResource();
	if (!Resource)
	{
		UE_LOG(LogTBImGui, Warning, TEXT("Cannot send texture [%s] to NetImGui, no resource found on the game thread!"), *InTexture->GetName());
		return false;
	}
		
	TArray<FColor> Pixels;
	FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
	ReadFlags.SetLinearToGamma(false);
	if (!Resource->ReadPixels(Pixels, ReadFlags))
	{
		UE_LOG(LogTBImGui, Warning, TEXT("Cannot send texture [%s] to NetImGui, failed to read pixels!"), *InTexture->GetName());
		return false;
	}
		
	const int32 SizeX = InTexture->SizeX;
	const int32 SizeY = InTexture->SizeY;
	if (Pixels.Num() != SizeX * SizeY)
	{
		UE_LOG(LogTBImGui, Warning, TEXT("Cannot send texture [%s] to NetImGui, texture size differs from pixel data!"), *InTexture->GetName());
		return false;
	}
	
	const uint32 ByteSize = static_cast<uint32>(Pixels.Num() * sizeof(FColor));
	NetImgui::SendDataTexture(InTexture, Pixels.GetData(), static_cast<uint16>(SizeX),
		static_cast<uint16>(SizeY), NetImgui::eTexFormat::kTexFmtRGBA8, ByteSize);
	
	return true;
}
#endif