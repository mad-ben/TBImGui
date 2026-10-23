// Copyright Lewis Scrivens. All Rights Reserved.

#include "Slate/SImGuiModules.h"

#include "Engine/Engine.h"
#include "TBImGuiEditor.h"
#include "UObject/UObjectGlobals.h"

void SImGuiModules::Construct(const FArguments& InArgs)
{
	FTBImGuiContextParams ContextParams(TEXT("EditorModules"));
	ContextParams.ViewportMode = ETBImGuiViewportMode::Single;
	ContextParams.bDockingSupport = false;
	SImGuiContextWidget::Construct(SImGuiContextWidget::FArguments().ContextParams(ContextParams));
	ModulesChangedHandle = FModuleManager::Get().OnModulesChanged().AddSP(this, &SImGuiModules::OnModulesChanged);
}

SImGuiModules::~SImGuiModules()
{
	FModuleManager::Get().OnModulesChanged().Remove(ModulesChangedHandle);
}

void SImGuiModules::Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime)
{
	SetContentSize(AllottedGeometry.GetLocalSize());
	// A deferred command may fail without emitting OnModulesChanged; refresh either way.
	if (bCommandQueued)
	{
		bModulesDirty = true;
		bCommandQueued = false;
	}
	SImGuiContextWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

void SImGuiModules::OnModulesChanged(FName ModuleName, EModuleChangeReason Reason)
{
	// Do not invalidate the rows while an ImGui frame is using them.
	bModulesDirty = true;
}

void SImGuiModules::RefreshModules()
{
	Modules.Reset();
	FModuleManager::Get().QueryModules(Modules);
	Modules.Sort([](const FModuleStatus& A, const FModuleStatus& B) { return A.Name < B.Name; });
	bModulesDirty = false;
}

void SImGuiModules::QueueModuleCommand(const TCHAR* Action, const FString& ModuleName)
{
	if (GEngine && !bCommandQueued)
	{
		// Match the Slate browser: execute outside of the UI draw call.
		GEngine->DeferredCommands.Add(FString::Printf(TEXT("Module %s %s"), Action, *ModuleName));
		bCommandQueued = true;
	}
}

void SImGuiModules::DrawModuleActions(const FModuleStatus& Status)
{
	FModuleManager& ModuleManager = FModuleManager::Get();
	const FName ModuleName(*Status.Name);
	const bool bLoaded = ModuleManager.IsModuleLoaded(ModuleName);
	const bool bBusy = !GEngine || bCommandQueued || UE::IsSavingPackage() || IsGarbageCollecting();
	if (!bLoaded)
	{
		ImGui::BeginDisabled(bBusy);
		if (ImGui::SmallButton("Load"))
		{
			QueueModuleCommand(TEXT("Load"), Status.Name);
		}
		ImGui::EndDisabled();
		return;
	}

	// These modules implement this surface and its rendering; never unload them here.
	const bool bOwnModule = ModuleName == TEXT("TBImGuiEditor") || ModuleName == TEXT("TBImGui") || ModuleName == TEXT("TBImGuiShaders");
	IModuleInterface* Module = ModuleManager.GetModule(ModuleName);
	const bool bCanUnload = !bBusy && !bOwnModule && Module
		&& !ModuleManager.DoesLoadedModuleHaveUObjects(ModuleName) && Module->SupportsDynamicReloading();
	ImGui::BeginDisabled(!bCanUnload);
	if (ImGui::SmallButton("Unload"))
	{
		QueueModuleCommand(TEXT("Unload"), Status.Name);
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("Reload"))
	{
		QueueModuleCommand(TEXT("Reload"), Status.Name);
	}
	ImGui::EndDisabled();
	if (!bCanUnload && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip(bOwnModule ? "This module provides the ImGui window."
			: bBusy ? "Module actions are temporarily unavailable."
			: "Unreal only allows unloading modules without UObject classes that support dynamic reloading.");
	}
}

void SImGuiModules::DrawImGuiContent()
{
	if (!bThemeApplied)
	{
		FTBImGuiEditorModule::ApplyThemeToCurrentContext();
		bThemeApplied = true;
	}
	if (bModulesDirty)
	{
		RefreshModules();
	}

	const ImGuiViewport* Viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(Viewport->Pos);
	ImGui::SetNextWindowSize(Viewport->Size);
	ImGui::SetNextWindowViewport(Viewport->ID);
	constexpr ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
	if (ImGui::Begin("##TBImGuiModules", nullptr, WindowFlags))
	{
		ImGui::TextWrapped("Modules are code libraries that provide engine, editor, and plugin features.");
		if (ImGui::Button("Refresh"))
		{
			RefreshModules();
		}
		ImGui::SameLine();
		ImGui::Checkbox("Loaded only", &bLoadedOnly);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputTextWithHint("##ModuleFilter", "Search module names...", Filter, UE_ARRAY_COUNT(Filter));

		const FString FilterText = FString(UTF8_TO_TCHAR(Filter)).TrimStartAndEnd();
		TArray<int32> VisibleModules;
		for (int32 Index = 0; Index < Modules.Num(); ++Index)
		{
			const FModuleStatus& Status = Modules[Index];
			if ((!bLoadedOnly || Status.bIsLoaded) && (FilterText.IsEmpty() || Status.Name.Contains(FilterText, ESearchCase::IgnoreCase)))
			{
				VisibleModules.Add(Index);
			}
		}
		ImGui::Text("%d of %d modules", VisibleModules.Num(), Modules.Num());
		constexpr ImGuiTableFlags TableFlags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg
			| ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;
		if (ImGui::BeginTable("##Modules", 4, TableFlags, ImVec2(0.0f, -1.0f)))
		{
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("Module", ImGuiTableColumnFlags_WidthStretch, 2.0f);
			ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 85.0f);
			ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 135.0f);
			ImGui::TableSetupColumn("File", ImGuiTableColumnFlags_WidthStretch, 3.0f);
			ImGui::TableHeadersRow();
			ImGuiListClipper Clipper;
			Clipper.Begin(VisibleModules.Num());
			while (Clipper.Step())
			{
				for (int32 Row = Clipper.DisplayStart; Row < Clipper.DisplayEnd; ++Row)
				{
					const FModuleStatus& Status = Modules[VisibleModules[Row]];
					ImGui::PushID(TCHAR_TO_UTF8(*Status.Name));
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::TextUnformatted(TCHAR_TO_UTF8(*Status.Name));
					ImGui::TableSetColumnIndex(1);
					ImGui::TextUnformatted(Status.bIsLoaded ? "Loaded" : "Unloaded");
					ImGui::TableSetColumnIndex(2);
					DrawModuleActions(Status);
					ImGui::TableSetColumnIndex(3);
					ImGui::TextUnformatted(TCHAR_TO_UTF8(*Status.FilePath));
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("%s", TCHAR_TO_UTF8(*Status.FilePath));
					}
					ImGui::PopID();
				}
			}
			ImGui::EndTable();
		}
	}
	ImGui::End();
}
