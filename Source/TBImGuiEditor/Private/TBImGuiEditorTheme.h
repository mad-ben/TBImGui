// Copyright Lewis Scrivens. All Rights Reserved.

#pragma once

#include "Templates/SharedPointer.h"

class FTBImGuiContext;

namespace TBImGuiEditorTheme
{
void ApplyConfiguredToContext(const TSharedPtr<FTBImGuiContext>& Context);
void ApplyConfiguredToCurrentContext();
void ApplyConfiguredToAllContexts();
} // namespace TBImGuiEditorTheme
