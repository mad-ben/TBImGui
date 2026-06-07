// Copyright Lewis Scrivens. All Rights Reserved.

#include "TBImGuiConfig.h"

#include <HAL/PlatformFileManager.h>

// ReSharper disable CppUnusedIncludeDirective
THIRD_PARTY_INCLUDES_START
#include <imgui.cpp>
#include <imgui_demo.cpp>
#include <imgui_draw.cpp>
#include <imgui_tables.cpp>
#include <imgui_widgets.cpp>
#if WITH_IMPLOT
#include <implot.cpp>
#include <implot_demo.cpp>
#include <implot_items.cpp>
#endif
#if WITH_IMPLOT3D
#include <implot3d.cpp>
#include <implot3d_demo.cpp>
#include <implot3d_items.cpp>
#include <implot3d_meshes.cpp>
#endif
THIRD_PARTY_INCLUDES_END
// ReSharper restore CppUnusedIncludeDirective

#ifdef IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS
ImFileHandle ImFileOpen(const char* FileName, const char* Mode)
{
	if (!FileName || !Mode || !*Mode)
	{
		return nullptr;
	}
	
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	
	const bool bWrite = FCStringAnsi::Strchr(Mode, 'w') != nullptr;
	const bool bAppend = FCStringAnsi::Strchr(Mode, 'a') != nullptr;
	const bool bAllowRead = FCStringAnsi::Strchr(Mode, '+') != nullptr;
	
	if (bWrite || bAppend)
	{
		return PlatformFile.OpenWrite(UTF8_TO_TCHAR(FileName), bAppend, bAllowRead);
	}
	
	if (Mode[0] == 'r')
	{
		if (!bAllowRead)
		{
			return PlatformFile.OpenRead(UTF8_TO_TCHAR(FileName), true);
		}

		const FString Path = UTF8_TO_TCHAR(FileName);
		if (!PlatformFile.FileExists(*Path))
		{
			return nullptr;
		}

		IFileHandle* const Handle = PlatformFile.OpenWrite(*Path, true, true);
		if (Handle)
		{
			Handle->Seek(0);
		}
		
		return Handle;
	}
	
	return nullptr;
}

bool ImFileClose(ImFileHandle File)
{
	if (!File)
	{
		return false;
	}

	delete File;
	return true;
}

uint64 ImFileGetSize(ImFileHandle File)
{
	if (!File)
	{
		return TNumericLimits<uint64>::Max();
	}
	
	const uint64 FileSize = File->Size();
	return FileSize;
}

uint64 ImFileRead(void* Data, uint64 Size, uint64 Count, ImFileHandle File)
{
	if (!File || !Data || Size == 0 || Count == 0)
	{
		return 0;
	}
	
	const uint64 BytesToRead = Size * Count;
	if (!File->Read(static_cast<uint8*>(Data), BytesToRead))
	{
		return 0;
	}
	
	return Count;
}

uint64 ImFileWrite(const void* Data, uint64 Size, uint64 Count, ImFileHandle File)
{
	if (!File || !Data || Size == 0 || Count == 0)
	{
		return 0;
	}
	
	const uint64 BytesToWrite = Size * Count;
	if (!File->Write(static_cast<const uint8*>(Data), BytesToWrite))
	{
		return 0;
	}
	
	return Count;
}
#endif