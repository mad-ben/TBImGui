// Copyright Lewis Scrivens. All Rights Reserved.
#include "TBImGuiAssetThumbnails.h"

#include "AssetThumbnail.h"
#include "ContentBrowserItem.h"
#include "ContentBrowserModule.h"
#include "ContentBrowserDataSubsystem.h"
#include "Editor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Materials/MaterialInterface.h"
#include "Misc/App.h"
#include "RenderDeferredCleanup.h"
#include "Slate/WidgetRenderer.h"
#include "UObject/StrongObjectPtr.h"
#include "ImageUtils.h"
#include "ImageCore.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace TBIMGUI_ASSET_THUMBNAILS
{
constexpr uint32 THUMBNAIL_SIZE = 128;
constexpr int32 MAX_CACHED_THUMBNAILS = 128;
constexpr int32 MAX_MATERIAL_LOADS = 2;

static bool IsMaterial(const FAssetData& Asset)
{
	const UClass* AssetClass = Asset.GetClass();
	return AssetClass && AssetClass->IsChildOf(UMaterialInterface::StaticClass());
}
} // namespace TBIMGUI_ASSET_THUMBNAILS

// AK CHANGES - Shared asset placeholders
struct FTBImGuiAssetThumbnails::FPlaceholderCache
{
	struct FBackground
	{
		TStrongObjectPtr<UTexture2D> Texture;
		FSlateBrush Brush;
	};

	FPlaceholderCache()
	{
		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("TBImGui"));
		const FString Path = Plugin ? FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources/SharedThumbBackground.png")) : FString();
		if (Path.IsEmpty() || !FImageUtils::LoadImage(*Path, Background))
		{
			UE_LOG(LogTemp, Warning, TEXT("TBImGui: Cannot load shared thumbnail background '%s'; using native asset icons."), *Path);
			return;
		}
		Background.ChangeFormat(ERawImageFormat::BGRA8, EGammaSpace::sRGB);
	}

	TSharedPtr<SWidget> MakeWidget(const FAssetData& Asset, const FLinearColor& TypeColor)
	{
		if (Background.RawData.IsEmpty())
		{
			return nullptr;
		}
		const FColor Color = TypeColor.ToFColorSRGB();
		TSharedPtr<FBackground>& Cached = Backgrounds.FindOrAdd(Color);
		if (!Cached)
		{
			FImage Tinted;
			Background.CopyTo(Tinted);
			const FLinearColor EncodedColor(Color.R / 255.0f, Color.G / 255.0f, Color.B / 255.0f);
			const float Hue = EncodedColor.LinearRGBToHSV().R;
			for (FColor& Pixel : Tinted.AsBGRA8())
			{
				FLinearColor HSV = FLinearColor(Pixel.R / 255.0f, Pixel.G / 255.0f, Pixel.B / 255.0f, Pixel.A / 255.0f).LinearRGBToHSV();
				HSV.R = FMath::Fmod(HSV.R + Hue, 360.0f);
				Pixel = HSV.HSVToLinearRGB().ToFColor(false);
			}
			Cached = MakeShared<FBackground>();
			Cached->Texture.Reset(FImageUtils::CreateTexture2DFromImage(Tinted));
			if (!Cached->Texture.IsValid())
			{
				Backgrounds.Remove(Color);
				return nullptr;
			}
			Cached->Brush.SetResourceObject(Cached->Texture.Get());
			Cached->Brush.ImageSize = FVector2D(128.0f, 128.0f);
			Cached->Brush.DrawAs = ESlateBrushDrawType::Image;
		}
		const UClass* Class = Asset.GetClass();
		const FText Label = Class ? Class->GetDisplayNameText() : FText::FromString(FName::NameToDisplayString(Asset.AssetClassPath.GetAssetName().ToString(), false));
		// clang-format off
		return SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SImage).Image(&Cached->Brush)
			]
			+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(6.0f)
			[
				SNew(STextBlock)
				.Text(Label.IsEmpty() ? NSLOCTEXT("TBImGui", "AssetPlaceholder", "Asset") : Label)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
				.WrapTextAt(112.0f)
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f))
				.ShadowOffset(FVector2D(0.0f, 1.0f))
				.ShadowColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.5f))
			];
		// clang-format on
	}

	FImage Background;
	TMap<FColor, TSharedPtr<FBackground>> Backgrounds;
};

struct FTBImGuiAssetThumbnails::FEntry
{
	~FEntry()
	{
		if (MaterialLoad && !MaterialLoad->HasLoadCompleted())
		{
			MaterialLoad->CancelHandle();
		}
	}

	FContentBrowserItem Item;
	TSharedPtr<FAssetThumbnail> Thumbnail;
	TSharedPtr<SWidget> Widget;
	TStrongObjectPtr<UTextureRenderTarget2D> Texture;
	TSharedPtr<FStreamableHandle> MaterialLoad;
	bool bRequestedMaterialLoad = false;
	bool bFreshMaterialPreview = false;
	FLinearColor TypeColor = FLinearColor::White;
	uint64 LastSeen = 0;
	uint64 LastDrawn = 0;
	uint64 LastUpdated = 0;
	bool bNeedsDraw = true;
};

FTBImGuiAssetThumbnails::FTBImGuiAssetThumbnails()
{
	Placeholders = MakeUnique<FPlaceholderCache>();
	Pool = MakeShared<FAssetThumbnailPool>(TBIMGUI_ASSET_THUMBNAILS::MAX_CACHED_THUMBNAILS);
	if (FApp::CanEverRender())
	{
		WidgetRenderer = new FWidgetRenderer(true);
	}
	Pool->OnThumbnailRendered().AddRaw(this, &FTBImGuiAssetThumbnails::OnThumbnailRendered);
}

FTBImGuiAssetThumbnails::~FTBImGuiAssetThumbnails()
{
	Pool->OnThumbnailRendered().RemoveAll(this);
	Entries.Reset();
	// The Slate renderer can still have work queued on the render thread.
	if (WidgetRenderer)
	{
		BeginCleanup(WidgetRenderer);
	}
}

void FTBImGuiAssetThumbnails::OnThumbnailRendered(const FAssetData& Asset)
{
	for (const auto& Pair : Entries)
	{
		if (Pair.Value->Thumbnail && Pair.Value->Thumbnail->GetAssetData().GetSoftObjectPath() == Asset.GetSoftObjectPath())
		{
			Pair.Value->bNeedsDraw = true;
			Pair.Value->bFreshMaterialPreview = true;
		}
	}
}

UTexture* FTBImGuiAssetThumbnails::FindOrQueue(const FContentBrowserItem& Item)
{
	if (!WidgetRenderer || !Item.IsValid())
	{
		return nullptr;
	}
	const FName Path = Item.GetVirtualPath();
	auto Entry = Entries.FindRef(Path);
	if (!Entry)
	{
		if (Entries.Num() >= TBIMGUI_ASSET_THUMBNAILS::MAX_CACHED_THUMBNAILS)
		{
			FName Oldest;
			uint64 OldestFrame = Frame;
			for (const auto& Pair : Entries)
			{
				if (Pair.Value->LastSeen < OldestFrame)
				{
					Oldest = Pair.Key;
					OldestFrame = Pair.Value->LastSeen;
				}
			}
			if (Oldest.IsNone())
			{
				return nullptr;
			}
			Entries.Remove(Oldest);
		}
		Entry = MakeShared<FEntry>();
		if (Item.IsFile())
		{
			Entry->Thumbnail = MakeShared<FAssetThumbnail>(FAssetData(), TBIMGUI_ASSET_THUMBNAILS::THUMBNAIL_SIZE, TBIMGUI_ASSET_THUMBNAILS::THUMBNAIL_SIZE, Pool);
		}
		Entries.Add(Path, Entry);
	}
	if (Entry->Item.IsTemporary() != Item.IsTemporary())
	{
		Entry->bNeedsDraw = true;
		Entry->MaterialLoad.Reset();
		Entry->bRequestedMaterialLoad = false;
		Entry->bFreshMaterialPreview = false;
	}
	if (Entry->Thumbnail && (!Entry->Item.IsValid() || Entry->Item.IsTemporary() != Item.IsTemporary() || Entry->LastUpdated + 60 < Frame))
	{
		Item.UpdateThumbnail(*Entry->Thumbnail);
		Entry->TypeColor = FModuleManager::GetModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser")).GetItemThumbnailColor(Item, Entry->Thumbnail);
		Entry->LastUpdated = Frame;
	}
	Entry->Item = Item;
	Entry->LastSeen = Frame;
	if (Entry->Thumbnail)
	{
		const FAssetData& Asset = Entry->Thumbnail->GetAssetData();
		if (TBIMGUI_ASSET_THUMBNAILS::IsMaterial(Asset) && (!IsValid(Asset.FastGetAsset(false)) || !Entry->bFreshMaterialPreview))
		{
			if (IsValid(Asset.FastGetAsset(false)))
			{
				Entry->Thumbnail->GetViewportRenderTargetTexture();
			}
			return Entry->Texture.Get();
		}
		Entry->Thumbnail->GetViewportRenderTargetTexture();
	}
	return Entry->Texture.Get();
}

TSharedPtr<FAssetThumbnail> FTBImGuiAssetThumbnails::FindThumbnail(const FContentBrowserItem& Item)
{
	FindOrQueue(Item);
	const auto Entry = Entries.FindRef(Item.GetVirtualPath());
	return Entry ? Entry->Thumbnail : nullptr;
}

FLinearColor FTBImGuiAssetThumbnails::GetTypeColor(const FContentBrowserItem& Item) const
{
	const TSharedPtr<FEntry> Entry = Entries.FindRef(Item.GetVirtualPath());
	return Entry ? Entry->TypeColor : FLinearColor::White;
}

UTexture* FTBImGuiAssetThumbnails::FindOrQueue(const FAssetData& Asset)
{
	UContentBrowserDataSubsystem* Data = GEditor ? GEditor->GetEditorSubsystem<UContentBrowserDataSubsystem>() : nullptr;
	if (!Asset.IsValid() || !Data)
	{
		return nullptr;
	}
	const FName VirtualPath = Data->ConvertInternalPathToVirtual(FName(*Asset.GetObjectPathString()));
	return FindOrQueue(Data->GetItemAtPath(VirtualPath, EContentBrowserItemTypeFilter::IncludeFiles));
}

void FTBImGuiAssetThumbnails::Tick()
{
	++Frame;
	if (!WidgetRenderer)
	{
		return;
	}
	int32 DrawsRemaining = 4;
	int32 ActiveMaterialLoads = 0;
	for (const auto& Pair : Entries)
	{
		if (Pair.Value->MaterialLoad && !Pair.Value->MaterialLoad->HasLoadCompleted() && !Pair.Value->MaterialLoad->WasCanceled())
		{
			++ActiveMaterialLoads;
		}
	}
	bool bStartedMaterialLoad = false;
	for (auto Iterator = Entries.CreateIterator(); Iterator; ++Iterator)
	{
		const auto& Entry = Iterator.Value();
		if (Entry->LastSeen + 300 < Frame)
		{
			Iterator.RemoveCurrent();
			continue;
		}
		if (Entry->LastSeen + 2 < Frame)
		{
			continue;
		}
		if (Entry->Thumbnail && TBIMGUI_ASSET_THUMBNAILS::IsMaterial(Entry->Thumbnail->GetAssetData()))
		{
			const FAssetData& Asset = Entry->Thumbnail->GetAssetData();
			if (!IsValid(Asset.FastGetAsset(false)))
			{
				Entry->bFreshMaterialPreview = false;
				if (!Entry->bRequestedMaterialLoad && !bStartedMaterialLoad && ActiveMaterialLoads < TBIMGUI_ASSET_THUMBNAILS::MAX_MATERIAL_LOADS)
				{
					Entry->bRequestedMaterialLoad = true;
					Entry->MaterialLoad = UAssetManager::GetStreamableManager().RequestAsyncLoad(Asset.GetSoftObjectPath(), FStreamableDelegate());
					++ActiveMaterialLoads;
					bStartedMaterialLoad = true;
				}
			}
			else
			{
				Entry->Thumbnail->GetViewportRenderTargetTexture();
			}
		}
		// Periodic refresh also picks up generic-icon/async-loading state changes without a rendered-thumbnail event.
		if ((!Entry->bNeedsDraw && Entry->LastDrawn + 60 >= Frame) || DrawsRemaining == 0)
		{
			continue;
		}

		if (!Entry->Texture.IsValid())
		{
			Entry->Texture.Reset(FWidgetRenderer::CreateTargetFor(FVector2D(TBIMGUI_ASSET_THUMBNAILS::THUMBNAIL_SIZE, TBIMGUI_ASSET_THUMBNAILS::THUMBNAIL_SIZE), TF_Bilinear, true));
			if (!Entry->Texture.IsValid())
			{
				continue;
			}
			Entry->Texture->ClearColor = FLinearColor::Transparent;
		}
		Entry->Widget = FModuleManager::GetModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"))
		                    .CreateItemThumbnailWidget(Entry->Item, Entry->Thumbnail);
		// AK CHANGES - Shared asset placeholders
		if (Entry->Thumbnail)
		{
			const FAssetData& Asset = Entry->Thumbnail->GetAssetData();
			const bool bHasPreview = Asset.IsValid() && !Entry->Item.IsTemporary() && Pool->IsRendered(Entry->Thumbnail) && (!TBIMGUI_ASSET_THUMBNAILS::IsMaterial(Asset) || Entry->bFreshMaterialPreview);
			if (!bHasPreview)
			{
				if (TSharedPtr<SWidget> Placeholder = Placeholders->MakeWidget(Asset, Entry->TypeColor))
				{
					Entry->Widget = MoveTemp(Placeholder);
				}
			}
		}
		WidgetRenderer->DrawWidget(Entry->Texture.Get(), Entry->Widget.ToSharedRef(), FVector2D(TBIMGUI_ASSET_THUMBNAILS::THUMBNAIL_SIZE, TBIMGUI_ASSET_THUMBNAILS::THUMBNAIL_SIZE), 0.0f);
		Entry->bNeedsDraw = false;
		Entry->LastDrawn = Frame;
		--DrawsRemaining;
	}
}
