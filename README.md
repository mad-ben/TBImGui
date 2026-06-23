<div align="center">

# ImGui Toolbox

![UE Version](https://img.shields.io/badge/UE-5.8-blue)
![GitHub release (latest by date)](https://img.shields.io/github/v/release/Lewisscrivens/TBImGui)
![GitHub issues](https://img.shields.io/github/issues/Lewisscrivens/TBImGui)
![GitHub discussions](https://img.shields.io/github/discussions/Lewisscrivens/TBImGui)

</div>
<br>

A production ready Dear ImGui integration for Unreal Engine, supporting the Editor, PIE, packaged games, remote debugging via NetImgui, and multi-viewport rendering.

<p>&nbsp;</p>

## 📄 Table of Contents

- [Preview](#-preview)
- [Features](#-features)
- [Supported Platforms](#-supported-platforms)
- [Installation](#️-installation)
- [Example Project](#-example-project)
- [Getting Started](#-getting-started)
- [Core Concepts](#-core-concepts)
   - [Context Scope](#context-scope)
   - [Viewport Mode](#viewport-mode)
   - [Input Mode](#input-mode)
   - [Frame Bindings](#frame-bindings)
   - [Textures](#textures)
   - [Fonts](#fonts)
   - [TBImGuiDemo](#tbimguidemo)
- [ImPlot](#-implot)
- [ImPlot3D](#-implot3d)
- [NetImgui](#-netimgui)
   - [Dedicated Server Support](#dedicated-server-support)
- [Commands](#-commands)
- [Advanced Configuration](#️-advanced-configuration)
   - [ImGui Toolbox Settings](#imgui-toolbox-settings)
   - [ImGui Toolbox Editor Settings](#imgui-toolbox-editor-settings)
   - [Rendering](#rendering)
   - [Missing Textures](#missing-textures)
   - [Contexts](#contexts)
   - [Build](#build)
- [Why](#-why)
- [Credits / Acknowledgements](#-credits--acknowledgements)
- [Additional Resources](#-additional-resources)

<p>&nbsp;</p>

## 🖼️ Preview

<img width="3834" height="2111" alt="Preview-compressed" src="https://github.com/user-attachments/assets/02859377-9cdb-4557-9c6c-40443f44c61c" />

<p>&nbsp;</p>

## ✨ Features

**TBImGui** is an integration of [Dear ImGui](https://github.com/ocornut/imgui)
directly into Unreal Engine rendering via **Slate**, allowing you to draw immediate mode debug and
tooling UI in both the **Editor**, and **Runtime** with support for **PIE**, **packaged builds**, 
and even on **headless dedicated servers** with the use of [NetImgui](https://github.com/sammyfreg/NetImgui).

- **Docking & Multi-Viewports** — Docking branch implementation supporting external application viewports.
- **Slate, Runtime and Editor contexts** — ImGui contexts per game instance, editor session, slate window etc.
- **Unreal Interoperability** — Direct ImGui support for `FColor`/`FLinearColor`/`FVector`/`FKey`/`UTexture*` *etc.*
- **Optional Renderers** — Translate draw commands directly into RHI draw calls (default), or convert and batch geometry as Slate vertices.
- **Frame Bindings** — Choose exactly when ImGui `BeginFrame`/`EndFrame` is run, or manually drive them yourself.
- **Remote Debugging** — View and control runtime contexts from a remote NetImGui server.
- **Editor Multi PIE Support** — Each PIE instance has its own ImGui context and can be debugged separately.
- **Gamepad Support** — ImGui use & navigation is fully functional with a gamepad.
- **QOL Improvements** — Additional widgets, layout/table utilities, scope macros, and conversions under the `ImGui` namespace.

<p>&nbsp;</p>

## 💻 Supported Platforms

TBImGui should work on any platform but remains untested in many for now. The plugin as it stands is tested & working on the following platforms:

- 🪟 Windows (x86_64)
- 🐧 Linux (x86_64)

See the TBImGuiExamples project that uses this plugin working on a Linux build on a Steam Deck [HERE](https://youtu.be/h7z-A6iuT-g).

ImGui gamepad controls as it stands are here:

![image](https://user-images.githubusercontent.com/8225057/36843388-1491dfd8-1d4f-11e8-99ff-21466e680117.png)

<p>&nbsp;</p>

## ⬇️ Installation

1. Download the [latest release](https://github.com/Lewisscrivens/TBImGui/releases/latest) for your engine version.

2. Extract the contents of the zip file either into your engine or projects plugins folder.
- **Engine Plugins:** `UnrealInstallation/Engine/Plugins/`
- **Project Plugins:** `ProjectDirectory/Plugins/`

3. Launch your project and ensure the plugin is enabled in the plugin settings.

<img width="2526" height="551" alt="TBImGuiPluginsWindow" src="https://github.com/user-attachments/assets/1b3509cc-25fe-450f-bfc2-92a698fedb2a" />

<p>&nbsp;</p>

## 📦 Example Project

Before getting started with TBImGui in your own project I would highly recommend checking out the [Example Project](https://github.com/Lewisscrivens/TBImGuiExamples)!

### Runtime Examples

- `UTBImGuiExamplesSubsystem` — Handles per game instance registry of example content using the `ExampleMenuBar`.
- `UTBImGuiExamplesMenuBar` — Contains example logic for a simple runtime ImGui menu bar and window management system.
- **Input** — Example input handling for showing/hiding ImGui content while returning control to the game with full NetImgui support.
- **Docking** — Example docking space in game-viewport.
- **ImPlot Examples** — Runtime graph examples using live engine timing data.
- **ImPlot3D Examples** — 3D plotting example showing player movement history in 3D-space.
- **NetImgui Handling** — Visibility bindings for active connections through NetImgui.

### Editor Examples
- **Slate Window** — ImGui context and content rendered within a custom `SWindow`.
- **Slate Tab** — ImGui context and content rendered within a 'SDockTab'.
- **Editor Main Frame** — ImGui window rendered in the editor main frame.

<p>&nbsp;</p>

## 🚀 Getting Started

### Project Setup

1. Follow the installation section to get the plugin enabled in your project.

2. Add the TBImGui module to your project dependencies in your `.Build.cs` file:
```csharp
PublicDependencyModuleNames.AddRange(new string[] { "TBImGui" });
```

3. By default TBImGui will be in world frame binding mode.
   This means that the runtime context tick is already handled and called post world tick.<br>

So all you need to do is the following to get up and running:

```cpp
UCLASS()
class UExampleImGuiSubsystem : public UGameInstanceSubsystem
{
GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& InCollection) override
	{
		UTBImGuiSubsystem* const ImGuiSubsystem = InCollection.InitializeDependency<UTBImGuiSubsystem>();
		if (IsValid(ImGuiSubsystem))
		{
		    FTBImGuiContextScope ContextScope;
		    if (ContextScope.IsValid())
		    {
		    	ContextScope->OnPreRender().AddUObject(this, &ThisClass::DrawImGuiContent);
		    }
		}
	}

	void DrawImGuiContent()
	{
		FTBImGuiDemo::ShowWindow();
	}
};
```

Alternatively if you want to drive the frame manually yourself ensure runtime frame binding in the plugin settings is set to `None` and then do the following:

```cpp
UCLASS()
class UExampleImGuiSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
GENERATED_BODY()

public:

	virtual void Tick(float DeltaTime) override
	{
		FTBImGuiContextScope ContextScope;
		if (ContextScope.IsValid())
		{
			ContextScope->BeginFrame();
			FTBImGuiDemo::ShowWindow();
			ContextScope->EndFrame();
		}
	}
	
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UExampleImGuiSubsystem, STATGROUP_Tickables);
	}
};
```

### Project Configuration

#### Runtime Settings

These settings can be found in under _Project Settings > Toolbox > ImGui Toolbox_.

| Setting                  | Description                                                                                                                                                                     |
|--------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **CreateContext**        | What runtime game instance contexts should be created automatically? By default a context is created for **Client** , **Standalone**, **ListenServer** and **DedicatedServer**. |
| **bShowDemoWindow**      | Open a demo window at runtime for any active contexts.                                                                                                                          |
| **RuntimeFrameBinding**  | ImGui engine frame binding for rendering ImGui content at runtime.                                                                                                              |
| **HeadlessFrameBinding**  | ImGui engine frame binding for rendering ImGui content for a headless instance (dedicated server).                                                                              |
| **DefaultFont**          | Default font to use when rendering ImGui content.                                                                                                                               |
| **AdditionalFontPaths**  | Additional font paths to scan.                                                                                                                                                  |
| **NetImGuiPort**         | The port used when listening for incoming NetImGui connections on a client.                                                                                                     |

> [!IMPORTANT]
> By default the plugin will create a context for any/all game instances, show an ImGui demo window and render post world tick.

### Editor Settings

These settings can be found in under _Project Settings > Toolbox > ImGui Toolbox Editor_.

| Setting                  | Description                                              |
|--------------------------|----------------------------------------------------------|
| **bCreateEditorContext** | Should an ImGui context be created for the editor?       |
| **bShowDemoWindow**      | Should ImGui render a demo window when the editor opens? |

> [!IMPORTANT]
> By default the editor context is disabled since only more advanced users will make use of it.

<p>&nbsp;</p>

## 🧩 Core Concepts

### Context Scope

`FTBImGuiContextScope` can be used to gain access to the current active ImGui context for the current scope.<br>
If using the default constructor it will find the most appropriate context for the current **runtime** scope.

```cpp
FTBImGuiContextScope ContextScope;
if (ContextScope.IsValid())
{
    ImGui::DoSomething(); // ImGui code here.
}
```

> [!NOTE]
> Using the context scope outside of runtime will require direct access to the context you are trying to scope to.

### Viewport Mode

A contexts viewport mode defines how ImGui viewports/windows function within that context.<br>

```cpp
UENUM()
enum class ETBImGuiViewportMode : uint8
{
	Single,     // Only the main viewport will be considered.
	Multiple,   // Windows can be dragged out of the main viewport.
	PerWindow   // All windows are assigned their own viewport.
};
```

For game contexts this is set to `Multiple` (*only supported on desktop based platforms!*), this means that the game viewport acts as 
the main viewport and if any ImGui windows are dragged out of said viewport, they are instanced and become their own slate hosted ImGui window.

The editor context on the other hand, has `PerWindow` viewport mode. This means that **ALL** viewports are treated as individual slate windows. (There is no main viewport)

### Input Mode

Each `FTBImGuiContext` has an `InputMode` flag, this controls what input state all ImGui widgets owned by that context are in.<br>

```cpp
UENUM()
enum class ETBImGuiInputMode : uint8
{
	All,                // Input is enabled for all viewports.
	MainViewportOnly,   // Input is enabled for the main viewport only.
	ViewportsOnly,      // Input is enabled for all viewports except the main viewport.
	None                // Input is disabled for all viewports.
};
```

By default all contexts are set to `All` and this should be handled by you in your implementation.

### Frame Bindings

Frame bindings allow you to choose when ImGui `BeginFrame`/`EndFrame` is run, or manually drive them yourself.<br>
This is useful if you want to render ImGui content at a different point in the frame than the default or set to `None`
if you want to drive the frame manually yourself by calling `FTBImGuiContext::BeginFrame` & `FTBImGuiContext::EndFrame`.

- **Slate** — Both ImGui BeginFrame and EndFrame are run on FSlateApplication::OnPreTick.
- **World** — Both ImGui BeginFrame and EndFrame are run on FWorldDelegates::OnWorldPostActorTick.
- **Application** — ImGui BeginFrame is run on FApp::BeginFrame and ImGui EndFrame is run on FSlateApplication::OnPreTick. (*Can be useful for using ImGui code anywhere in the application scope*)
- **Headless** — ImGui BeginFrame and EndFrame is run on FApp::EndFrame. (*Important for dedicated servers since they have no valid slate application*)

### Textures

Texture support is built in and you can use `UTexture*` directly in ImGui calls. For example:

```cpp
ImGui::Image(ImTextureRef(GEngine->DefaultTexture), ImVec2(100, 100));
```

<img width="255" height="256" alt="DefaultTexture" src="https://github.com/user-attachments/assets/a5ca3fe8-b883-48c9-ae01-032e7f4d107f" />

Support is included for sending Unreal textures over NetImgui. Using the following functions.

```cpp
FTBImGuiUtils::SendTextureToNetImGui(UTexture2D* InTexture);
FTBImGuiUtils::SendTextureToNetImGui(UTextureRenderTarget2D* InTexture);
```

<img width="1749" height="1008" alt="TexturesOverServer" src="https://github.com/user-attachments/assets/12732f18-9126-4b5e-b4c8-196ae6c45a9d" />

### Fonts

All engine fonts are supported by default and can be used directly in ImGui calls.
Font awesome 7 free is also included in the plugin by default using juliettef's [icon headers](https://github.com/juliettef/IconFontCppHeaders).

<img width="1550" height="274" alt="FontsExample" src="https://github.com/user-attachments/assets/3a8dd623-9a39-4ed7-af1b-722bbd7c519a" />

### TBImGuiDemo

Similar to the ImGui::ShowDemoWindow, ImPlot::ShowDemoWindow etc. I have created FTBImGuiDemo::ShowWindow showcasing these core concepts of the TBImGui plugin.

<img width="700" height="384" alt="DemoWindow" src="https://github.com/user-attachments/assets/4268f697-ee46-4c0b-90bf-a6f1edcd482a" />

<p>&nbsp;</p>

## 📈 ImPlot

The ImPlot library is included and enabled by default in this plugin. 
It is a plotting library for ImGui and can be used to create graphs and charts. 
Learn more about it [here](https://github.com/epezent/implot).

<div align="center">
    <img src="https://raw.githubusercontent.com/wiki/epezent/implot/screenshots3/controls.gif" width="270"> 
    <img src="https://raw.githubusercontent.com/wiki/epezent/implot/screenshots3/dnd.gif" width="270"> 
    <img src="https://raw.githubusercontent.com/wiki/epezent/implot/screenshots3/pie.gif" width="270">
</div>

<div align="center">
    <img src="https://raw.githubusercontent.com/wiki/epezent/implot/screenshots3/query.gif" width="270"> 
    <img src="https://raw.githubusercontent.com/wiki/epezent/implot/screenshots3/bars.gif" width="270">
    <img src="https://raw.githubusercontent.com/wiki/epezent/implot/screenshots3/rt.gif" width="270">
</div>

<p>&nbsp;</p>

## 📈 ImPlot3D

The ImPlot3D library is included and enabled by default in this plugin.
Similarly to ImPlot it is a plotting library for ImGui but with 3-dimensional graphs.
Learn more about it [here](https://github.com/brenocq/implot3d).

<div align="center">
  <img src="https://github.com/user-attachments/assets/6d4dacc8-ddf5-445b-a595-80f83befd626" width="270"/>
  <img src="https://github.com/user-attachments/assets/69c6d660-54c9-42aa-bac4-22bb19749e4b" width="270"/>
  <img src="https://github.com/user-attachments/assets/93f5dfda-cf6f-4e24-ae07-11e03be27f7a" width="270"/>
</div>

<div align="center">
  <img src="https://github.com/user-attachments/assets/b7279508-d63e-4546-8cba-c69c7bf16c47" width="270"/>
  <img src="https://github.com/user-attachments/assets/434187eb-3449-472b-b443-03140b7c9a9f" width="270"/>
  <img src="https://github.com/user-attachments/assets/94cab1c6-0980-45c5-a6a1-8781f536a2ef" width="270"/>
</div>

<p>&nbsp;</p>

## 🌐 NetImgui

NetImgui is supported for remote debugging of runtime contexts. 
You can use commands to start listening for incoming connections from the NetImgui Server application.
Learn more about it [here](https://github.com/sammyfreg/netImgui).

> [!TIP]
> This is extremely useful for debugging platform specific issues and even headless dedicated servers with visual UI.

### Dedicated Server Support

The below image shows ImGui content rendering in the NetImgui server application from a headless dedicated server instance.<br>
This was done using the `-ExecCmds="TB.ImGui.NetImGui.Listen"` command line argument and then connecting from the NetImgui server application.

<img width="3027" height="1263" alt="DedicatedServerUI" src="https://github.com/user-attachments/assets/7cd4fc56-ad48-4760-bb3a-f38f76d8cd66" />

You can see that the content is rendering correctly which is clear since the server view is only updating it's frame data and is running at a lower framerate.

<p>&nbsp;</p>

## 📟 Commands

| Command                        | Description                                                                                                                 |
|--------------------------------|-----------------------------------------------------------------------------------------------------------------------------|
| `TB.ImGui.NetImGui.Listen`     | Start listening for a NetImGui connections for the local instance. Args: [Port] (Optional, default: 8889)                   |
| `TB.ImGui.NetImGui.Connect`    | Connect the local instance to a NetImGui server. Args: [IP] (Optional, default: localhost) [Port] (Optional, default: 8889) |
| `TB.ImGui.NetImGui.Disconnect` | Disconnect any active NetImGui connection on this instance.                                                                 |
| `TB.ImGui.PauseRendering`      | Pause rendering for the ImGui context in the game viewport.                                                                 |
| `TB.ImGui.UnpauseRendering`    | Unpause rendering for the ImGui context in the game viewport.                                                               |

<p>&nbsp;</p>

## ⚙️ Advanced Configuration

### Rendering

Render modes are macro defined in the TBImGuiConfig.h file.
```cpp
#define IMGUI_RENDER_MODE_SLATE	 0
#define IMGUI_RENDER_MODE_NATIVE 1
#define IMGUI_RENDER_MODE IMGUI_RENDER_MODE_NATIVE
```

#### Native RHI

ImGui draw commands are translated directly into RHI draw calls inside `FTBImGuiCustomElement`.<br>
`UnrealPlatform_RenderWindow` calls `SImGuiWidget::UpdateDrawData` which caches the pointer address for the owning viewports draw data.<br>
During `OnPaint`, the draw lists are swapped into a snapshot using [ImDrawDataSnapshot](https://github.com/ocornut/imgui_club/blob/main/imgui_threaded_rendering/imgui_threaded_rendering.h)
to prevent expensive copies like the slate batching method.

Since the render thread is one frame behind the game thread, the draw data is double buffered to allow for safe
game/render-thread access without stalling since we need to persist the snapshot until the render thread accesses it.
At which point it iterate over the ImDrawData and issue RHI draw calls directly using the `TBImGuiShaders` global shaders.

#### Slate Batching

ImGui draw commands are converted into Slate counterparts and drawn via batching via `FSlateDrawElement::MakeCustomVerts`.<br>
`UnrealPlatform_RenderWindow` calls `SImGuiWidget::UpdateDrawData` which caches the draw data for later use.<br>
During `OnPaint`, the draw lists read from the previously cached data and convert the ImGui vertices/indices into Slate
counterparts which are then drawn via `FSlateDrawElement::MakeCustomVerts`.

This is more expensive than the native method but a much simpler implementation.

### Missing Textures

Missing texture method is macro defined in the TBImGuiConfig.h file.
```cpp
#define IMGUI_MISSING_TEXTURE_NONE              0
#define IMGUI_MISSING_TEXTURE_EMPTY             1
#define IMGUI_MISSING_TEXTURE_CUSTOM_COLOR      2
#define IMGUI_MISSING_TEXTURE_ENGINE_DEFAULT    3
#define IMGUI_MISSING_TEXTURE_MODE IMGUI_MISSING_TEXTURE_ENGINE_DEFAULT

#if IMGUI_MISSING_TEXTURE_MODE == IMGUI_MISSING_TEXTURE_CUSTOM_COLOR
#define CustomMissingTextureColor FColor::Black
#endif
```

```cpp
// EXAMPLE MISSING TEXTURE
ImGui::Image(ImTextureRef(nullptr), ImVec2(100, 100));
```

<img width="1377" height="380" alt="MissingTextures" src="https://github.com/user-attachments/assets/1a04ad6b-8a87-4768-aaed-82238fa04b10" />

### Contexts

`FTBImGuiContext::CreateContext` is used to create new ImGui contexts. It takes in context params which 
allow specification of the context name, viewport mode etc.

Different set-ups are used in the plugin depending on context, for example the editor context sets the viewport mode
to `PerWindow` which makes it so that there is no main viewport and all ImGui windows are rendered as individual windows.<br>
This differs from the usual method of using the ImGui mainframe as a sort of main viewport.

| Setting                 | Description                                            |
|-------------------------|--------------------------------------------------------|
| **ContextName**         | The ImGui name for the context. (**Limited to 15 characters!**)                 |
| **ViewportMode**        | What viewport support does this context have.          |
| **bDockingSupport**     | Does this context have docking support?                |
| **bHeadless**           | Is this context headless? Has no slate context?        |
| **bWindowsAlwaysOnTop**  | Should external viewport windows always render on-top. |
| **RawUserData**         | Additional raw data for this context.                  |
| **ObjectUserDataWeak**  | Additional user UObject data for this context.         |

### Build

In the `TBImGui.Build.cs` file you can set the following build settings to compile out additional third-party content:

```csharp
	protected virtual bool WithImPlot => true;
	protected virtual bool WithImPlot3D => true;
	protected virtual bool WithNetImGui => true;
```

<p>&nbsp;</p>

## ❓ Why

I came across a post by [Ocornut](https://github.com/ocornut) discussing converging towards a single
backend [here](https://github.com/ocornut/imgui/issues/9122) where [Sammyfreg](https://github.com/sammyfreg)
created a list going over the pros and cons of each existing implementation available publicly.
After reading this I wasn't satisfied with existing plugins which didn't check all the boxes for me and
had unlisted issues that I was running into regularly when developing my TBDebug plugin.
So I thought I would have a crack at creating my own binding.

<p>&nbsp;</p>

## ⭐ Credits / Acknowledgements

I also want to shoutout other developers and resources that have helped and made this plugin possible.

| Developer                                       | Projects                                                                                                          |
|-------------------------------------------------|-------------------------------------------------------------------------------------------------------------------|
| [ocornut](https://github.com/ocornut)           | [imgui](https://github.com/ocornut/imgui)                                                                         |
| [epezent](https://github.com/epezent)           | [implot](https://github.com/epezent/implot)                                                                       |
| [brenocq](https://github.com/brenocq)           | [implot3d](https://github.com/brenocq/implot3d)                                                                   |
| [VesCodes](https://github.com/VesCodes)         | [ImGui](https://github.com/VesCodes/ImGui)                                                                        |
| [sammyfreg](https://github.com/sammyfreg)       | [NetImgui](https://github.com/sammyfreg/NetImgui) & [UnrealNetImgui](https://github.com/sammyfreg/UnrealNetImgui) |
| [amuTBKT](https://github.com/amuTBKT)           | [ImGuiPlugin](https://github.com/amuTBKT/ImGuiPlugin)                                                             |
| [arnaud-jamin](https://github.com/arnaud-jamin) | [Cog](https://github.com/arnaud-jamin/Cog)                                                                        |

<p>&nbsp;</p>

## 📚 Additional Resources

Dear ImGui — https://www.dearimgui.com/

ImGui Explorer — https://pthom.github.io/imgui_explorer/

More Extensions — https://github.com/ocornut/imgui/wiki/Useful-Extensions
