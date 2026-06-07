// Copyright Lewis Scrivens. All Rights Reserved.

#pragma once

#include <Misc/AssertionMacros.h>
#include <Math/IntPoint.h>
#include <Math/IntVector.h>
#include <Math/Vector2D.h>
#include <Math/Vector4.h>
#include <Math/Color.h>

class UTexture;

#define IM_ASSERT(Expr) ensure(Expr)

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#define IMGUI_DISABLE_OBSOLETE_KEYIO
#define IMGUI_DISABLE_WIN32_FUNCTIONS
#define IMGUI_DISABLE_DEFAULT_ALLOCATORS
#define IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS
#define IMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS
#define IMGUI_DEFINE_MATH_OPERATORS

#ifdef IMGUI_DISABLE_DEFAULT_FILE_FUNCTIONS
typedef IFileHandle* ImFileHandle;
ImFileHandle ImFileOpen(const char* FileName, const char* Mode);
bool ImFileClose(ImFileHandle File);
uint64 ImFileGetSize(ImFileHandle File);
uint64 ImFileRead(void* Data, uint64 Size, uint64 Count, ImFileHandle File);
uint64 ImFileWrite(const void* Data, uint64 Size, uint64 Count, ImFileHandle File);
#endif

#define IM_VEC2_CLASS_EXTRA \
	operator FVector2f() const { return FVector2f(x, y); } \
	constexpr ImVec2(const FVector2f& V) : x(V.X), y(V.Y) {} \
	operator FVector2d() const { return FVector2d(x, y); } \
	constexpr ImVec2(const FVector2d& V) : x(V.X), y(V.Y) {} \
	operator FIntPoint() const { return FIntPoint(x, y); } \
	constexpr ImVec2(const FIntPoint& V) : x(V.X), y(V.Y) {} \
	operator FIntVector2() const { return FIntVector2(x, y); } \
	constexpr ImVec2(const FIntVector2& V) : x(V.X), y(V.Y) {}

#define IM_VEC4_CLASS_EXTRA \
	operator FVector4() const { return FVector4(x, y, z, w); } \
	constexpr ImVec4(const FVector4& V) : x(V.X), y(V.Y), z(V.Z), w(V.W) {} \
	operator FIntVector4() const { return FIntVector4(x, y, z, w); } \
	constexpr ImVec4(const FIntVector4& V) : x(V.X), y(V.Y), z(V.Z), w(V.W) {} \
	operator FLinearColor() const { return FLinearColor(x, y, z, w); } \
	constexpr ImVec4(const FLinearColor& C) : x(C.R), y(C.G), z(C.B), w(C.A) {}

namespace ImGui
{
	/// Pack ImGui 32-bit colors so they're bit compatible with FColor.
	inline constexpr uint32 ImCol32RShift = offsetof(FColor, R) * 8;
	inline constexpr uint32 ImCol32GShift = offsetof(FColor, G) * 8;
	inline constexpr uint32 ImCol32BShift = offsetof(FColor, B) * 8;
	inline constexpr uint32 ImCol32AShift = offsetof(FColor, A) * 8;
	inline constexpr uint32 ImCol32AMask = (0xFF << ImCol32AShift);
}

#define IM_COL32_R_SHIFT ImGui::ImCol32RShift
#define IM_COL32_G_SHIFT ImGui::ImCol32GShift
#define IM_COL32_B_SHIFT ImGui::ImCol32BShift
#define IM_COL32_A_SHIFT ImGui::ImCol32AShift
#define IM_COL32_A_MASK ImGui::ImCol32AMask

#define ImTextureID UTexture*
#define ImTextureID_Invalid nullptr

/// 0 = Use Slate to render ImGui content by batching custom vertices.
/// 1 = Translate ImGui draw commands directly into RHI draw calls (inside a Slate custom element).
/// @note For implementations see SImGuiWidget::OnPaint.
#define IMGUI_RENDER_MODE_SLATE	 0
#define IMGUI_RENDER_MODE_NATIVE 1
#define IMGUI_RENDER_MODE IMGUI_RENDER_MODE_NATIVE

/// 0 - Don't render anything for missing textures. Skip the draw command entirely.
/// 1 - Use an empty texture resource for missing textures. Let the renderer decide.
/// 2 - Use a custom solid color texture for missing textures.
/// 3 - Use Unreal Engines default texture for missing textures.
#define IMGUI_MISSING_TEXTURE_NONE			 0
#define IMGUI_MISSING_TEXTURE_EMPTY			 1
#define IMGUI_MISSING_TEXTURE_CUSTOM_COLOR	 2
#define IMGUI_MISSING_TEXTURE_ENGINE_DEFAULT 3
#define IMGUI_MISSING_TEXTURE_MODE IMGUI_MISSING_TEXTURE_ENGINE_DEFAULT

#if IMGUI_MISSING_TEXTURE_MODE == IMGUI_MISSING_TEXTURE_CUSTOM_COLOR
#define CustomMissingTextureColor FColor::Black
#endif

DECLARE_LOG_CATEGORY_CLASS(LogTBImGui, Log, VeryVerbose);

#define IMGUI_INCLUDE_IMGUI_USER_H
#define IMGUI_USER_H_FILENAME "TBImGuiExtra.h"