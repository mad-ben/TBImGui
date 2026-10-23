// Copyright Lewis Scrivens. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

class FAssetThumbnailPool;
class FAssetThumbnail;
class FWidgetRenderer;
struct FContentBrowserItem;
class UTexture;
struct FAssetData;

/** Render native asset thumbnail widgets and folder brushes into textures understood by TBImGui. */
class FTBImGuiAssetThumbnails
{
public:
	FTBImGuiAssetThumbnails();
	~FTBImGuiAssetThumbnails();
	void Tick();
	UTexture* FindOrQueue(const FContentBrowserItem& Item);
	UTexture* FindOrQueue(const FAssetData& Asset);
	TSharedPtr<FAssetThumbnail> FindThumbnail(const FContentBrowserItem& Item);
	FLinearColor GetTypeColor(const FContentBrowserItem& Item) const;

private:
	struct FEntry;
	struct FPlaceholderCache;
	TUniquePtr<FPlaceholderCache> Placeholders;
	void OnThumbnailRendered(const FAssetData& Asset);
	TSharedPtr<FAssetThumbnailPool> Pool;
	FWidgetRenderer* WidgetRenderer = nullptr;
	TMap<FName, TSharedPtr<FEntry>> Entries;
	uint64 Frame = 0;
};
