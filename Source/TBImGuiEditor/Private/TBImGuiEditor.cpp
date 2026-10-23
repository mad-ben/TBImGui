// Copyright Lewis Scrivens. All Rights Reserved.

#include "TBImGuiEditor.h"

#include <Slate/SImGuiWidget.h>

#include "TBImGuiEditorSettings.h"
#include "TBImGuiEditorTheme.h"
#include "TBImGuiFrameBindings.h"
#include "TBImGuiContentBrowserIntegration.h"
#include "TBImGuiModulesIntegration.h"
#include "TBImGuiOutlinerIntegration.h"
#include "TBImGuiOutputLogIntegration.h"
#include "TBImGuiDetailsIntegration.h"

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
	TBIMGUI_CONTENT_BROWSER_INTEGRATION::Register();
	TBIMGUI_OUTLINER_INTEGRATION::Register();
	TBIMGUI_DETAILS_INTEGRATION::Register();
	TBImGuiModulesIntegration::Register();
	TBImGuiOutputLogIntegration::Register();

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
	ApplyConfiguredTheme();

	EditorContext->InitializeFrameBinding(FTBImGuiFrameBinding_Slate::StaticStruct());

	if (ImGuiEditorSettings->bShowDemoWindow)
	{
		EditorContext->OnPreRender().AddLambda([]
		{
			ImGui::ShowDemoWindow();
		});
	}

	OnContextCreated.Broadcast(EditorContext.ToSharedRef());
}

void FTBImGuiEditorModule::ShutdownModule()
{
	TBIMGUI_CONTENT_BROWSER_INTEGRATION::Unregister();
	TBIMGUI_OUTLINER_INTEGRATION::Unregister();
	TBIMGUI_DETAILS_INTEGRATION::Unregister();
	TBImGuiModulesIntegration::Unregister();
	TBImGuiOutputLogIntegration::Unregister();

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

void FTBImGuiEditorModule::ApplyConfiguredTheme()
{
	TBImGuiEditorTheme::ApplyConfiguredToContext(EditorContext);
	TBImGuiEditorTheme::ApplyConfiguredToAllContexts();
}

void FTBImGuiEditorModule::ApplyThemeToCurrentContext()
{
	TBImGuiEditorTheme::ApplyConfiguredToCurrentContext();
}
