# Mobile Integration Guide for UE4 Canvas GUI

This guide covers the mobile port of UE4 Canvas GUI for Android and iOS platforms using touch input and platform-specific hooking frameworks.

## Overview

The mobile port includes:
- Touch-based input handling (supports up to 10 simultaneous touches)
- Platform detection macros (iOS/Android)
- **Native iOS UIKit touch method hooking** (iOS) — no UE4 PlayerController wiring required
- CydiaSubstrate hooking for iOS (PostRender + UIViewController touch methods)
- Dobby hooking framework for Android
- DPI-aware scaling for different screen sizes
- Touch-friendly hit detection (20% larger hit areas)
- Example implementation with gesture-based menu toggle

## iOS Native Touch Input (Jailbreak)

On jailbroken iOS devices the recommended approach is to intercept touch events
directly at the **UIKit** level rather than through UE4's `InputComponent` bindings.
This mirrors the technique used in the [Dear ImGui Apple Metal example](https://github.com/ocornut/imgui/blob/8957b3df03b4cbe502688208af7d2fda52be985f/examples/example_apple_metal/main.mm).

`source/IOSNativeInput.mm` uses CydiaSubstrate's `MSHookMessageEx` to swizzle
four Objective-C methods on `UIViewController`:

| Hooked method | Trigger |
|---|---|
| `touchesBegan:withEvent:` | One or more fingers begin touching the screen |
| `touchesMoved:withEvent:` | Any active finger moves |
| `touchesCancelled:withEvent:` | Touch sequence is interrupted (e.g. phone call) |
| `touchesEnded:withEvent:` | One or more fingers lift off the screen |

Each hook calls `ZeroGUI_UpdateFromUIEvent()` which translates every `UITouch`
in the event into a `ZeroGUI::Input::UpdateTouchState()` call, then chains to
the original `UIViewController` implementation so the game receives its events
normally.

### Why native UIKit hooks?

- **No UE4 modifications required** — works as a jailbreak tweak injected into
  any UE4 game without recompilation.
- **Lower latency** — events arrive before UE4's own input processing pipeline.
- **Multi-touch out of the box** — all ten touch slots are populated directly
  from the `UIEvent` touch set.
- **Cancel handling** — `touchesCancelled:withEvent:` clears all slots to prevent
  stuck touches when calls or notifications interrupt the session.

### Activation

Simply call `MobileHooks::Initialize()` once at startup.  It now automatically
installs both the PostRender C++ hook and the UIViewController touch hooks:

```cpp
// Your tweak's constructor or +load method
extern "C" void InitializeMobileGUI()
{
    MobileHooks::Initialize();  // installs PostRender hook + iOS touch hooks
}
```

After this, `ZeroGUI::Input::UpdateTouchState()` is called automatically for
every native iOS touch event — no `BindTouch` / `PlayerController` code is
needed.

## File Structure

```
source/
├── PlatformDefines.h           # Platform detection macros
├── ZeroInputMobile.h           # Touch input handling
├── PlatformAbstraction.h       # Platform-independent input abstraction
├── MobileHooks.h               # Hooking framework integration
├── IOSNativeInput.h            # iOS native touch hook – C++ interface
├── IOSNativeInput.mm           # iOS native touch hook – Objective-C++ implementation
├── MobileMenuExample.cpp       # Complete example implementation
├── ZeroGUI.h                   # Main GUI framework (desktop)
└── ZeroInput.h                 # Mouse/keyboard input (desktop)
```

## Platform Macros

The following macros are defined for platform detection:

```cpp
PLATFORM_IOS      // 1 on iOS, 0 otherwise
PLATFORM_ANDROID  // 1 on Android, 0 otherwise
PLATFORM_MOBILE   // 1 on any mobile platform, 0 on desktop
```

## Quick Start

### 1. Basic Integration

```cpp
#include "PlatformDefines.h"
#include "MobileHooks.h"

#if PLATFORM_MOBILE
    #include "ZeroInputMobile.h"
    #include "PlatformAbstraction.h"
#else
    #include "ZeroInput.h"
#endif

// Initialize the hooking system
void InitializeGame()
{
    #if PLATFORM_MOBILE
        MobileHooks::Initialize();
    #endif
}
```

### 2. Feeding Touch Input to the System

#### iOS (Jailbreak — recommended)

On jailbroken iOS devices `MobileHooks::Initialize()` installs native UIKit
hooks via `IOSNativeInput::InstallTouchHooks()`.  Touch data flows into
`ZeroGUI::Input` automatically — **no PlayerController wiring is needed**.

#### iOS / Android (UE4 PlayerController)

If you prefer to receive touch events through UE4's own input system (or for
non-jailbroken builds), bind them in your PlayerController class:

```cpp
void AYourPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    
    InputComponent->BindTouch(IE_Pressed, this, &AYourPlayerController::OnTouchPressed);
    InputComponent->BindTouch(IE_Released, this, &AYourPlayerController::OnTouchReleased);
    InputComponent->BindTouch(IE_Repeat, this, &AYourPlayerController::OnTouchMoved);
}

void AYourPlayerController::OnTouchPressed(ETouchIndex::Type FingerIndex, FVector Location)
{
    FVector2D screenPos = FVector2D(Location.X, Location.Y);
    ZeroGUI::Input::UpdateTouchState((int)FingerIndex, screenPos, true);
}

void AYourPlayerController::OnTouchReleased(ETouchIndex::Type FingerIndex, FVector Location)
{
    FVector2D screenPos = FVector2D(Location.X, Location.Y);
    ZeroGUI::Input::UpdateTouchState((int)FingerIndex, screenPos, false);
}

void AYourPlayerController::OnTouchMoved(ETouchIndex::Type FingerIndex, FVector Location)
{
    FVector2D screenPos = FVector2D(Location.X, Location.Y);
    // Keep touch as active, just update position
    if (ZeroGUI::Input::IsTouchActive((int)FingerIndex))
    {
        ZeroGUI::Input::UpdateTouchState((int)FingerIndex, screenPos, true);
    }
}
```

### 3. Creating a Simple Menu

```cpp
void PostRenderHook(UGameViewportClient* viewport, UCanvas* canvas)
{
    ZeroGUI::SetupCanvas(canvas);
    
    // Handle input
    ZeroGUI::Input::Handle();
    
    // Menu toggle with double-tap gesture
    static bool menuOpened = false;
    if (ZeroGUI::Input::IsKeyPressed(ZeroGUI::Input::Gestures::GESTURE_DOUBLE_TAP, false))
    {
        menuOpened = !menuOpened;
    }
    
    // Draw menu
    static FVector2D pos = FVector2D(50, 50);
    if (ZeroGUI::Window("Mobile Menu", &pos, FVector2D{400, 500}, menuOpened))
    {
        static bool feature1 = false;
        static float sensitivity = 50.0f;
        
        ZeroGUI::Text("Touch-Friendly Menu", true, true);
        ZeroGUI::Checkbox("Enable Feature", &feature1);
        ZeroGUI::SliderFloat("Sensitivity", &sensitivity, 0.0f, 100.0f);
        
        if (ZeroGUI::Button("Test Button", FVector2D{150, 45}))
        {
            // Button was tapped
        }
    }
    
    ZeroGUI::Render();
    
    // Draw touch indicator instead of cursor
    if (ZeroGUI::Input::IsTouchActive(0))
    {
        FVector2D touchPos = ZeroGUI::Input::GetPrimaryTouchPosition();
        // Draw touch indicator circle at touchPos
    }
}
```

## Building Instructions

### iOS Build

1. **Add CydiaSubstrate Framework**
   - Download CydiaSubstrate SDK
   - Add `substrate.h` to your include path
   - Link against `libsubstrate.dylib`

2. **Add `IOSNativeInput.mm` to your build**
   - Include `source/IOSNativeInput.mm` and `source/IOSNativeInput.h` in your project
   - The file must be compiled as Objective-C++ (`.mm` extension is sufficient in Xcode)

3. **Project Settings**
   ```bash
   # In your UE4 project's Build.cs
   if (Target.Platform == UnrealTargetPlatform.IOS)
   {
       PublicAdditionalLibraries.Add("substrate");
       PublicIncludePaths.Add("/path/to/substrate/include");
   }
   ```

4. **Code Signing**
   - Ensure your app is properly code-signed
   - Substrate hooks require proper entitlements

### Android Build

1. **Add Dobby Hooking Library**
   - Include Dobby in your project (https://github.com/jmpews/Dobby)
   - Add to your CMakeLists.txt or Android.mk

2. **CMakeLists.txt Example**
   ```cmake
   # Add Dobby
   add_subdirectory(ThirdParty/Dobby)
   
   # Link against your game library
   target_link_libraries(YourGameLib
       dobby
   )
   ```

3. **Android.mk Example**
   ```make
   LOCAL_PATH := $(call my-dir)
   
   # Dobby
   include $(CLEAR_VARS)
   LOCAL_MODULE := dobby
   LOCAL_SRC_FILES := ThirdParty/Dobby/libdobby.a
   include $(PREBUILT_STATIC_LIBRARY)
   
   # Your game
   include $(CLEAR_VARS)
   LOCAL_MODULE := YourGame
   LOCAL_STATIC_LIBRARIES := dobby
   ```

## Advanced Features

### Multi-Touch Support

```cpp
// Check all active touches
for (int i = 0; i < 10; i++)
{
    if (ZeroGUI::Input::IsTouchActive(i))
    {
        FVector2D pos = ZeroGUI::Input::touchPositions[i];
        // Process each touch point
    }
}

// Get active touch count
int touchCount = ZeroGUI::Input::activeTouchCount;
```

### Gesture Recognition

```cpp
using namespace ZeroGUI::Input::Gestures;

// Menu toggle gesture
const int MENU_GESTURE = GESTURE_DOUBLE_TAP;

// Available gestures:
// - GESTURE_TAP
// - GESTURE_DOUBLE_TAP
// - GESTURE_LONG_PRESS
// - GESTURE_SWIPE_LEFT/RIGHT/UP/DOWN
// - GESTURE_PINCH
// - GESTURE_ZOOM
```

### DPI Scaling

```cpp
// Set DPI scale based on device
float dpiScale = GetDeviceDPIScale(); // Your implementation
ZeroGUI::Mobile::SetDPIScale(dpiScale);

// Use scaled sizes
FVector2D buttonSize = FVector2D(100, 30);
FVector2D scaledSize = ZeroGUI::Mobile::GetScaledSize(buttonSize);
```

### Touch-Friendly Hit Detection

The mobile version automatically uses 20% larger hit areas for better touch accuracy:

```cpp
// Automatically applied in Platform::MouseInZone()
bool isHovered = ZeroGUI::Platform::MouseInZone(buttonPos, buttonSize);
```

## Troubleshooting

### Issue: Hooks not working on iOS
- Verify CydiaSubstrate is properly installed
- Check that the PostRender symbol name is correct for your UE4 version
- Ensure proper code signing and entitlements

### Issue: Hooks not working on Android
- Verify Dobby is properly linked
- Check library name (might be different from "libUE4.so" in your build)
- Use `adb logcat` to check for errors

### Issue: Touch input not responding
- On iOS, verify `IOSNativeInput::InstallTouchHooks()` completed (call `MobileHooks::Initialize()`)
- On iOS, `UpdateTouchState()` is now called automatically by the native UIKit hook — no PlayerController `BindTouch` wiring is needed
- Check that `ZeroGUI::Input::Handle()` is called each frame
- Ensure canvas is properly set up with `SetupCanvas()`

### Issue: UI elements too small on high-DPI devices
- Set proper DPI scale with `ZeroGUI::Mobile::SetDPIScale()`
- Verify minimum touch target sizes are being enforced

## Migration from Desktop Version

1. Replace `#include "ZeroInput.h"` with platform-specific includes
2. Replace `GetAsyncKeyState()` calls with `Platform::IsKeyDown()`
3. Replace cursor position code with `Platform::GetInputPosition()`
4. Replace menu toggle key with gesture detection
5. Test on actual devices for touch accuracy

## Platform-Specific Notes

### iOS
- Minimum deployment target: iOS 9.0+
- Substrate hooks work on jailbroken devices or with proper signing
- For App Store builds, consider alternative hooking methods

### Android
- Minimum API level: 21 (Android 5.0+)
- ARM64 support required for modern devices
- Test on various screen sizes and DPI settings

## Example Project

See `MobileMenuExample.cpp` for a complete working example that demonstrates:
- PostRender hook integration
- Touch input handling
- Gesture-based menu toggle
- Platform-specific initialization
- Integration with UE4 touch events

## License

Same as the original UE4 Canvas GUI project.

## Support

For issues specific to the mobile port, please create an issue on GitHub with:
- Platform (iOS/Android)
- UE4 version
- Device information
- Error messages or unexpected behavior
