# ue4-canvas-gui

## A simple Canvas GUI for Unreal Engine 4 supporting both desktop (mouse/keyboard) and mobile (touch) platforms.

### Platform Support
- **Desktop:** Windows with mouse and keyboard input
- **Mobile:** iOS and Android with touch input and gesture controls

Included elements:<br>
Rendering Text (left/center);<br>
Rendering Rects;<br>
Rendering Circles (filled and not);<br>
Button, Slider, Checkbox, Combobox, Hotkeys and ColorPicker.<br>
<br>
Implemented a simple post-render system to draw on top of menu and all.<br>
<br>
## Screenshots with default style:<br>
![EU4 GUI](screenshots/canvas1.jpg "")
 
![EU4 GUI](screenshots/canvas2.jpg "")
 
![EU4 GUI](screenshots/canvas3.jpg "")
 
Ingame render
![EU4 GUI](screenshots/canvas4.jpg "")

---

## 🚀 NEW: Mobile Platform Support (iOS & Android)

This GUI now supports mobile devices with touch input! See the dedicated mobile documentation:

- **[Mobile Integration Guide](MOBILE_INTEGRATION.md)** - Complete guide for iOS and Android
- **[Build Configuration Guide](BUILD_CONFIGURATION.md)** - Platform-specific build setup

### Mobile Features:
- ✅ Touch input support (up to 10 simultaneous touches)
- ✅ Platform detection macros (iOS/Android)
- ✅ Gesture-based menu controls (double-tap, swipe, etc.)
- ✅ DPI-aware scaling for different screen sizes
- ✅ Touch-friendly hit detection (20% larger hit areas)
- ✅ PostRender hooking via CydiaSubstrate (iOS) and Dobby (Android)
- ✅ Complete example implementation included

### Quick Mobile Example:

```cpp
#include "PlatformDefines.h"
#include "MobileHooks.h"
#include "ZeroInputMobile.h"

void InitializeGame()
{
    #if PLATFORM_MOBILE
        MobileHooks::Initialize();  // Install PostRender hook
    #endif
}

// Feed touch events from UE4
void OnTouchPressed(ETouchIndex::Type FingerIndex, FVector Location)
{
    FVector2D screenPos = FVector2D(Location.X, Location.Y);
    ZeroGUI::Input::UpdateTouchState((int)FingerIndex, screenPos, true);
}
```

See `source/MobileMenuExample.cpp` for a complete working example.

---

## Desktop Usage

## Small "How to use" guide:<br>
First you need get **UCanvas** from game.<br>
After it you can draw like this:<br>

```cpp
//I'll show you with an example Post Render Hook
void PostRenderHook(UGameViewportClient* viewport, UCanvas* canvas)
{
	ZeroGUI::SetupCanvas(canvas);
	Menu::Tick();
}

//Menu.h
void Tick()
{
	ZeroGUI::Input::Handle();
	
	static bool menu_opened = false;
	if (GetAsyncKeyState(VK_F2) & 1) menu_opened = !menu_opened; //Our menu key

	if (ZeroGUI::Window("Superior UE4 GUI", &pos, FVector2D{ 500.0f, 400.0f }, menu_opened))
	{
		//Simple Tabs
		static int tab = 0;
		if (ZeroGUI::ButtonTab("Tab 1", FVector2D{ 110, 25 }, tab == 0)) tab = 0;
		if (ZeroGUI::ButtonTab("Tab 2", FVector2D{ 110, 25 }, tab == 1)) tab = 1;
		if (ZeroGUI::ButtonTab("Tab 3", FVector2D{ 110, 25 }, tab == 2)) tab = 2;
		if (ZeroGUI::ButtonTab("Tab 4", FVector2D{ 110, 25 }, tab == 3)) tab = 3;
		ZeroGUI::NextColumn(130.0f);
		//
		
		//Some Elements
		static bool text_check = false;
		static float text_slider = 15.0f;
		static int test_hotkey = 0x2;
		static FLinearColor test_color{ 0.0f, 0.0f, 1.0f, 1.0f };

		ZeroGUI::Checkbox("Test Checkbox", &text_check);
		ZeroGUI::SliderFloat("Test Slider", &text_slider, 0.0f, 180.0f);
		ZeroGUI::Hotkey("Test Hotkey", FVector2D{ 80, 25 }, &test_hotkey);

		ZeroGUI::Text("Left aligned text!");
		ZeroGUI::Text("Outline and Center aligned text!", true, true);

		//Element with padding
		ZeroGUI::PushNextElementY(50.0f);
		ZeroGUI::Combobox("Combobox", FVector2D{ 100, 25 }, &test_number, "None", "First", "Second", "Third", NULL); //NULL at end is required!
		ZeroGUI::SameLine();//inline items
		if (ZeroGUI::Button("It's a Button!", FVector2D{ 100, 25 })) { /*clicked!*/ }

		//Color Picker
		ZeroGUI::ColorPicker("Color Picker", &test_color);
	}
	ZeroGUI::Render();//Custom Render. I use it for drawing Combobox and ColorPicker over the menu
	ZeroGUI::Draw_Cursor(menu_opened);
}
```
