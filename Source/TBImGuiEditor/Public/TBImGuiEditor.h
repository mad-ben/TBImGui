// Copyright Lewis Scrivens. All Rights Reserved.

#pragma once

#include <Modules/ModuleManager.h>

class FTBImGuiContext;

/// @brief This module manages editors ImGui context.
class TBIMGUIEDITOR_API FTBImGuiEditorModule : public IModuleInterface
{
public:
    
    static FTBImGuiEditorModule& Get();
    
    /// Begin IModuleInterface Override
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    /// End IModuleInterface Override
    
	/// @brief Returns the editors ImGui context.
	static TSharedPtr<FTBImGuiContext> GetEditorContext();
	
	/// @brief Returns true if the editors context has been created, false otherwise.
	static bool HasEditorContext();
	
	/// Ran when the editor context is created.
	static TMulticastDelegate<void(TSharedPtr<FTBImGuiContext>)> OnContextCreated;
	
protected:
	
    /// ImGui Editor context. Allows rendering of ImGui 
    /// content in the editors scope.
    static TSharedPtr<FTBImGuiContext> EditorContext;
};