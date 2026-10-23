// Copyright Lewis Scrivens. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "Slate/SImGuiContextWidget.h"

/** A searchable module browser rendered with Dear ImGui. */
class SImGuiModules final : public SImGuiContextWidget
{
public:
	SLATE_BEGIN_ARGS(SImGuiModules) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SImGuiModules() override;
	virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;

protected:
	virtual void DrawImGuiContent() override;

private:
	void OnModulesChanged(FName ModuleName, EModuleChangeReason Reason);
	void RefreshModules();
	void DrawModuleActions(const FModuleStatus& Status);
	void QueueModuleCommand(const TCHAR* Action, const FString& ModuleName);

	TArray<FModuleStatus> Modules;
	FDelegateHandle ModulesChangedHandle;
	ANSICHAR Filter[256] = {};
	bool bLoadedOnly = false;
	bool bModulesDirty = true;
	bool bThemeApplied = false;
	bool bCommandQueued = false;
};
