// Copyright Lewis Scrivens. All Rights Reserved.

#pragma once

#include <Math/Vector2D.h>
#include <UObject/ObjectMacros.h>

#include "TBImGuiTypes.generated.h"

class UGameInstance;
class SWindow;

class SImGuiWidget;
class FTBImGuiContext;

struct ImGuiViewport;
struct ImGuiContext;

#if WITH_IMPLOT
struct ImPlotContext;
#endif

#if WITH_IMPLOT3D
struct ImPlot3DContext;
#endif

UENUM()
enum class ETBImGuiInputMode : uint8
{
	All,				// Input is enabled for all viewports.
	MainViewportOnly,	// Input is enabled for the main viewport only.
	ViewportsOnly,		// Input is enabled for all viewports except the main viewport.
	None				// Input is disabled for all viewports.
};

UENUM()
enum class ETBImGuiViewportMode : uint8
{
	Single,		// Only the main viewport will be considered.
	Multiple,	// Windows can be dragged out of the main viewport.
	PerWindow	// All windows are assigned their own viewport.
};

/// @brief Viewport data binding for tracking Unreal Engine windows/widgets 
/// created for their ImGui counterpart viewports.
struct TBIMGUI_API FTBImGuiViewportData
{
	/// @brief Returns the mutable existing viewport data.
	static FTBImGuiViewportData* Find(const ImGuiViewport* InViewport);
	
	/// @brief Returns the mutable existing viewport data or creates one
	static FTBImGuiViewportData* FindOrCreate(ImGuiViewport* InViewport);
	
	TWeakPtr<SWindow> Window = nullptr;
	TWeakPtr<SImGuiWidget> ImGuiWidget = nullptr;
};

/// @brief Scope utility class to set the current ImGui and 
/// ImPlot context for the given FTBImGuiContext.
struct TBIMGUI_API FTBImGuiContextScope
{
	FTBImGuiContextScope(); // Runtime only!
	FTBImGuiContextScope(const UGameInstance* InGameInstance); // Runtime only!
	FTBImGuiContextScope(const TSharedPtr<FTBImGuiContext>& InContext);
	~FTBImGuiContextScope();
	
	/// Returns true if the managed ImGui context is valid.
	bool IsValid() const;
	
	/// Access to the Unreal ImGui context.
	FTBImGuiContext* operator->() const;
	
	/// @brief Returns the the Unreal ImGui context.
	TSharedRef<FTBImGuiContext> GetContext() const;

private:
	
	TSharedPtr<FTBImGuiContext> Context = nullptr;
	
	ImGuiContext* PrevContext = nullptr;
	
#if WITH_IMPLOT
	ImPlotContext* PrevPlotContext = nullptr;
#endif
	
#if WITH_IMPLOT3D
	ImPlot3DContext* PrevPlot3DContext = nullptr;
#endif
};

/// @brief Utility ring buffer for use with ImPlot and ImGui.
template <typename ElementType>
struct TTBImGuiRingBuffer
{
	TTBImGuiRingBuffer(int32 MaxItems)
		: WriteOffset(0)
	{
		Storage.Empty(MaxItems);
	}

	int32 Num() const
	{
		return Storage.Num();
	}

	int32 ArrayMax() const
	{
		return Storage.GetSlack() + Storage.Num();
	}
	
	int32 GetWriteOffset() const
	{
		return WriteOffset;
	}
	
	int32 GetOldestIndex() const
	{
		return Num() < ArrayMax() ? 0 : WriteOffset;
	}
	
	bool IsValidIndex(int32 Index) const
	{
		return Storage.IsValidIndex(Index);
	}
	
	ElementType* GetData()
	{
		return Storage.GetData();
	}
	
	const ElementType& GetElement(int32 Index) const
	{
		return Storage[Index];
	}
	
	ElementType& GetMutableElement(int32 Index)
	{
		return Storage[Index];
	}
	
	void AddElement(const ElementType& Element)
	{
		ElementType& NewElement = WriteNewElementInitialized();
		NewElement = Element;
	}
	
	ElementType& WriteNewElementUninitialized()
	{
		const int32 OldWriteIndex = WriteOffset;
		WriteOffset = (WriteOffset + 1) % ArrayMax();

		if (Storage.GetSlack())
		{
			Storage.AddUninitialized(1);
		}
		
		return Storage[OldWriteIndex];
	}

	ElementType& WriteNewElementInitialized()
	{
		const int32 OldWriteIndex = WriteOffset;
		WriteOffset = (WriteOffset + 1) % ArrayMax();

		if (Storage.GetSlack())
		{
			Storage.Add(ElementType());
		}
		else
		{
			Storage[OldWriteIndex] = ElementType();
		}
		
		return Storage[OldWriteIndex];
	}

private:
	
	TArray<ElementType> Storage;
	int32 WriteOffset;
};