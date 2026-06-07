// Copyright Lewis Scrivens. All Rights Reserved.

#include "TBImGuiEditor.h"

#include <Slate/SImGuiWidget.h>

#include "TBImGuiEditorSettings.h"
#include "TBImGuiFrameBindings.h"

IMPLEMENT_MODULE(FTBImGuiEditorModule, TBImGuiEditor)

DEFINE_LOG_CATEGORY_STATIC(LogTBImGuiEditor, Log, All);

static const FString ImGuiEditorContextName = TEXT("Editor");
TMulticastDelegate<void(TSharedPtr<FTBImGuiContext>)> FTBImGuiEditorModule::OnContextCreated;
TSharedPtr<FTBImGuiContext> FTBImGuiEditorModule::EditorContext = nullptr;

FTBImGuiEditorModule& FTBImGuiEditorModule::Get()
{
    return FModuleManager::LoadModuleChecked<FTBImGuiEditorModule>("TBImGuiEditor");
}

void FTBImGuiEditorModule::StartupModule()
{
	const UTBImGuiEditorSettings* const ImGuiEditorSettings = UTBImGuiEditorSettings::Get();
	if (!IsValid(ImGuiEditorSettings) || !ImGuiEditorSettings->bCreateEditorContext)
	{
		UE_LOG(LogTBImGuiEditor, Log, TEXT("ImGui editor context is disabled via settings, Skipping mainframe context creation..."));
		return;
	}
	
	FTBImGuiContextParams ContextParams;
	ContextParams.ContextName = ImGuiEditorContextName;
	ContextParams.ViewportMode = ETBImGuiViewportMode::PerWindow;
	EditorContext = FTBImGuiContext::CreateContext(ContextParams);
	check(EditorContext.IsValid());
	
	// Bind to the application scope for rendering ImGui in the editor.
	EditorContext->InitializeFrameBinding(FTBImGuiFrameBinding_Slate::StaticStruct());
	
	// Optionally show the demo window by default.
	if (ImGuiEditorSettings->bShowDemoWindow)
	{
		EditorContext->BindPreRender(FSimpleMulticastDelegate::FDelegate::CreateLambda([]
		{
			ImGui::ShowDemoWindow();
		}));
	}
	
	OnContextCreated.Broadcast(EditorContext.ToSharedRef());
}

void FTBImGuiEditorModule::ShutdownModule()
{
	if (EditorContext.IsValid())
	{
		EditorContext->DestroyContext();
		EditorContext.Reset();
	}
	
	OnContextCreated.Clear();
}

TSharedPtr<FTBImGuiContext> FTBImGuiEditorModule::GetEditorContext()
{
	return EditorContext;
}

bool FTBImGuiEditorModule::HasEditorContext()
{
	return EditorContext.IsValid();
}