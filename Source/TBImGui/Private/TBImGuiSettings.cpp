// Copyright Lewis Scrivens. All Rights Reserved.

#include "TBImGuiSettings.h"

#include "TBImGuiUtils.h"
#include "TBImGuiFrameBindings.h"

namespace TBImGuiContextHelpers
{
	static const TMap<ENetMode, ETBNetImGuiContext> NetModeMap =
	{
		{ NM_Standalone, ETBNetImGuiContext::Standalone },
		{ NM_DedicatedServer, ETBNetImGuiContext::DedicatedServer },
		{ NM_ListenServer, ETBNetImGuiContext::ListenServer },
		{ NM_Client, ETBNetImGuiContext::Client }
	};
}

UTBImGuiSettings::UTBImGuiSettings()
	: RuntimeFrameBinding(FTBImGuiFrameBinding_World::StaticStruct())
	, HeadlessFrameBinding(FTBImGuiFrameBinding_Headless::StaticStruct())
{
	DefaultFont = GetDefaultFontName();
}

FName UTBImGuiSettings::GetCategoryName() const
{
	return TEXT("Toolbox");
}

#if WITH_EDITOR
bool UTBImGuiSettings::CanEditChange(const FProperty* InProperty) const
{
#if !WITH_NETIMGUI
	const FName PropertyName = InProperty ? InProperty->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UTBImGuiSettings, NetImGuiPort))
	{
		return false;
	}
#endif
	
	return Super::CanEditChange(InProperty);
}

void UTBImGuiSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	const FName Name = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (Name == GET_MEMBER_NAME_CHECKED(UTBImGuiSettings, AdditionalFontPaths))
	{
		// Validate default font when font paths are removed.
		// If we removed our selected default fault reset DefaultFont config.
		const TArray<FString> FontOptions = GetFontOptions();
		if (!FontOptions.Contains(DefaultFont))
		{
			DefaultFont = GetDefaultFontName();
		}
	}
}
#endif

const UTBImGuiSettings* UTBImGuiSettings::Get()
{
	return GetDefault<UTBImGuiSettings>();
}

bool UTBImGuiSettings::ShouldCreateContext(const ENetMode InNetMode) const
{
	if (const ETBNetImGuiContext* const NetImGuiContext = TBImGuiContextHelpers::NetModeMap.Find(InNetMode))
	{
		return ShouldCreateContext(*NetImGuiContext);
	}
	
	return false;
}

bool UTBImGuiSettings::ShouldCreateContext(const ETBNetImGuiContext InContext) const
{
	return EnumHasAnyFlags(static_cast<ETBNetImGuiContext>(CreateContext), InContext);
}

TArray<FString> UTBImGuiSettings::GetFontOptions() const
{
	TArray<FString> FontOptions;
	const TArray<FString> EngineFontPaths = FTBImGuiUtils::GetEngineFontPaths();
	for (const FString& FontPath : EngineFontPaths)
	{
		FontOptions.Add(FPaths::GetCleanFilename(FontPath));
	}
	
	const TArray<FString> ProjectFontPaths = GetAdditionalFontPaths();
	for (const FString& FontPath : ProjectFontPaths)
	{
		FontOptions.Add(FPaths::GetCleanFilename(FontPath));
	}
	
	return FontOptions;
}

TArray<FString> UTBImGuiSettings::GetAdditionalFontPaths() const
{
	TArray<FString> FontPaths;
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	for (const FDirectoryPath& FontDirectory : AdditionalFontPaths)
	{
		FString AbsoluteDirectory;
		if (FontDirectory.Path.StartsWith(TEXT("/")))
		{
			if (!FPackageName::TryConvertLongPackageNameToFilename(FontDirectory.Path, AbsoluteDirectory))
			{
				continue;
			}
		}
		else if (FPaths::IsRelative(AbsoluteDirectory))
		{
			AbsoluteDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), AbsoluteDirectory);
		}
		else
		{
			AbsoluteDirectory = FontDirectory.Path;
		}
		
		FPaths::NormalizeDirectoryName(AbsoluteDirectory);
		
		if (PlatformFile.DirectoryExists(*AbsoluteDirectory))
		{
			TArray<FString> FoundPaths;
			PlatformFile.FindFiles(FoundPaths, *AbsoluteDirectory, TEXT(".ttf"));
			for (const FString& FontPath : FoundPaths)
			{
				if (FPaths::FileExists(FontPath))
				{
					FontPaths.Add(FontPath);
				}
			}
		}
	}
	
	return FontPaths;
}

FString UTBImGuiSettings::GetDefaultFontName() const
{
	const FString DefaultFontPath = FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf");
	if (ensure(FPaths::FileExists(DefaultFontPath)))
	{
		return FPaths::GetCleanFilename(DefaultFontPath);
	}
	
	return "";
}