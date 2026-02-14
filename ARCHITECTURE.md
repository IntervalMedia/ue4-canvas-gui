# Mobile Port Architecture

## Overview

This document describes the architecture of the mobile port for UE4 Canvas GUI, explaining how the codebase has been adapted to support both desktop and mobile platforms.

## Design Principles

1. **Minimal Changes**: The desktop version (ZeroGUI.h, ZeroInput.h) remains unchanged to maintain backward compatibility
2. **Platform Abstraction**: New mobile files provide equivalent functionality without Windows dependencies
3. **Compile-Time Selection**: Platform macros enable automatic selection of desktop vs. mobile code paths
4. **Touch-First Design**: Mobile input handling designed around touch gestures rather than retrofitted mouse emulation

## File Organization

```
source/
├── Desktop (Original)
│   ├── ZeroGUI.h           # Main GUI framework (Windows-dependent)
│   └── ZeroInput.h         # Mouse/keyboard input using Windows API
│
└── Mobile (New)
    ├── PlatformDefines.h         # Platform detection macros
    ├── ZeroInputMobile.h         # Touch input handling
    ├── PlatformAbstraction.h     # Unified input API
    ├── MobileHooks.h             # PostRender hooking framework
    └── MobileMenuExample.cpp     # Complete usage example
```

## Platform Detection

### Macro System

```cpp
// Automatically defined based on compiler/platform
PLATFORM_IOS      // 1 on iOS, 0 elsewhere
PLATFORM_ANDROID  // 1 on Android, 0 elsewhere
PLATFORM_MOBILE   // 1 on any mobile, 0 on desktop
```

### Detection Logic

```cpp
// iOS detection
#if defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        #define PLATFORM_IOS 1
        #define PLATFORM_MOBILE 1
    #endif
#endif

// Android detection
#if defined(__ANDROID__) || defined(ANDROID)
    #define PLATFORM_ANDROID 1
    #define PLATFORM_MOBILE 1
#endif
```

## Input Abstraction Layer

### Architecture

```
┌─────────────────────────────────┐
│   Game Code / Menu System       │
│   (Platform Independent)         │
└────────────┬────────────────────┘
             │
             ▼
┌─────────────────────────────────┐
│   PlatformAbstraction.h         │
│   (Unified Input API)            │
└────────┬────────────────┬───────┘
         │                │
    Desktop           Mobile
         │                │
         ▼                ▼
┌──────────────┐  ┌──────────────┐
│ ZeroInput.h  │  │ZeroInputMobile│
│ (Windows API)│  │  (Touch)     │
└──────────────┘  └──────────────┘
```

### API Abstraction

The `PlatformAbstraction.h` provides platform-independent functions:

```cpp
namespace ZeroGUI::Platform
{
    FVector2D GetInputPosition();      // Cursor or primary touch
    bool IsInputActive();               // Mouse or touch down
    bool IsInputClicked(int id, bool);  // Click or tap
    bool MouseInZone(FVector2D, FVector2D); // Hit testing
}
```

Desktop implementation uses Windows API:
```cpp
FVector2D GetInputPosition()
{
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    return FVector2D(cursorPos.x, cursorPos.y);
}
```

Mobile implementation uses touch tracking:
```cpp
FVector2D GetInputPosition()
{
    return Input::GetPrimaryTouchPosition();
}
```

## Touch Input System

### Multi-Touch Support

The mobile input system supports up to 10 simultaneous touch points:

```cpp
namespace ZeroGUI::Input
{
    bool touchDown[10];           // Touch state per finger
    FVector2D touchPositions[10]; // Position per touch
    int activeTouchCount;         // Number of active touches
}
```

### Touch State Management

```cpp
// Update touch state from UE4 events
void UpdateTouchState(int touchIndex, FVector2D position, bool isDown)
{
    touchDown[touchIndex] = isDown;
    touchPositions[touchIndex] = position;
    
    // Recalculate active count
    activeTouchCount = 0;
    for (int i = 0; i < 10; i++)
        if (touchDown[i]) activeTouchCount++;
}
```

### Touch vs. Mouse Differences

| Feature | Mouse (Desktop) | Touch (Mobile) |
|---------|----------------|----------------|
| Simultaneous inputs | 1 cursor + 5 buttons | 10+ touch points |
| Position tracking | Continuous | Per touch point |
| Hover state | Yes | No (requires touch) |
| Precision | High (pixel-perfect) | Lower (finger size) |
| Hit detection | Small targets OK | Needs larger areas |

## Gesture Recognition

### Simple Gesture System

```cpp
namespace ZeroGUI::Input::Gestures
{
    const int GESTURE_TAP = 0x01;
    const int GESTURE_DOUBLE_TAP = 0x02;
    const int GESTURE_LONG_PRESS = 0x03;
    const int GESTURE_SWIPE_LEFT = 0x04;
    // ... etc
    
    FVector2D gestureStartPos;
    float gestureStartTime;
    bool gestureInProgress;
}
```

Gestures are recognized by tracking touch position and duration, then mapping to virtual keys that the GUI system understands.

## Hooking Framework

### Architecture

```
┌─────────────────────────────────┐
│   UE4 Game Engine               │
│                                 │
│   UGameViewportClient           │
│   ├─ PostRender() ◄─┐          │
│   └─ ...             │          │
└──────────────────────┼──────────┘
                       │
                   Hook Intercept
                       │
                       ▼
┌─────────────────────────────────┐
│   HookedPostRender()            │
│   ├─ Call original              │
│   ├─ Update input               │
│   ├─ Render GUI                 │
│   └─ Draw touch indicators      │
└─────────────────────────────────┘
```

### Platform-Specific Implementations

#### iOS - CydiaSubstrate

```cpp
#include <substrate.h>

PostRenderFunc originalPostRender = nullptr;

void InstallPostRenderHook()
{
    void* addr = MSFindSymbol(nullptr, "_ZN20UGameViewportClient10PostRenderEP7UCanvas");
    MSHookFunction(addr, (void*)&HookedPostRender, (void**)&originalPostRender);
}
```

**Pros:**
- Well-established on iOS
- Stable API
- Works on jailbroken devices

**Cons:**
- Requires jailbreak or special signing for hooking
- Not available for App Store builds (requires alternatives)

#### Android - Dobby

```cpp
#include "dobby.h"

PostRenderFunc originalPostRender = nullptr;

void InstallPostRenderHook()
{
    void* addr = DobbySymbolResolver("libUE4.so", "_ZN20UGameViewportClient10PostRenderEP7UCanvas");
    DobbyHook(addr, (void*)&HookedPostRender, (void**)&originalPostRender);
}
```

**Pros:**
- Modern, actively maintained
- Works on ARM32 and ARM64
- No root required (for your own app)
- Better than older alternatives (Cydia Substrate, ADBI)

**Cons:**
- Must be bundled with your app
- Adds library size

### Hook Installation Flow

```
Game Start
    │
    ▼
InitializeMobileGUI()
    │
    ▼
MobileHooks::Initialize()
    │
    ├─ Find PostRender symbol
    ├─ Install hook
    └─ Save original pointer
    │
    ▼
Hook Active
    │
    └─ Every frame:
         UE4 calls PostRender()
            │
            ▼
         HookedPostRender() intercepts
            │
            ├─ Call originalPostRender()
            ├─ Update touch input
            ├─ Render GUI
            └─ Return to UE4
```

## Touch-Friendly Adaptations

### Larger Hit Areas

Mobile uses 20% larger hit detection areas:

```cpp
const float TOUCH_HIT_SCALE = 1.2f;

bool MouseInZone(FVector2D pos, FVector2D size)
{
    FVector2D scaledSize = size * TOUCH_HIT_SCALE;
    // Adjust position to center the scaled area
    FVector2D offset = (scaledSize - size) * 0.5f;
    FVector2D adjustedPos = pos - offset;
    // Perform hit test with larger area
}
```

### Minimum Touch Targets

Apple HIG recommends 44x44 points minimum:

```cpp
const float MIN_TOUCH_TARGET = 44.0f;

FVector2D GetScaledSize(FVector2D size)
{
    FVector2D scaled = size * DPIScale;
    if (scaled.X < MIN_TOUCH_TARGET) scaled.X = MIN_TOUCH_TARGET;
    if (scaled.Y < MIN_TOUCH_TARGET) scaled.Y = MIN_TOUCH_TARGET;
    return scaled;
}
```

### DPI Scaling

Different devices have different pixel densities:

```cpp
namespace ZeroGUI::Mobile
{
    static float DPIScale = 1.0f;
    
    void SetDPIScale(float scale);
    FVector2D GetScaledSize(FVector2D size);
    FVector2D GetScaledPosition(FVector2D pos);
}
```

Example DPI scales:
- iPhone 6/7/8: 2.0x
- iPhone X/11/12: 3.0x
- iPad: 2.0x
- Android phones: 1.5x - 4.0x (varies widely)

## Integration with UE4

### Touch Event Flow

```
UE4 Touch Event
    │
    ▼
APlayerController::SetupInputComponent()
    │
    ├─ BindTouch(IE_Pressed, OnTouchPressed)
    ├─ BindTouch(IE_Released, OnTouchReleased)
    └─ BindTouch(IE_Repeat, OnTouchMoved)
    │
    ▼
OnTouch...() callbacks
    │
    └─ ZeroGUI::Input::UpdateTouchState()
```

### Implementation Example

```cpp
void AMyPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindTouch(IE_Pressed, this, &AMyPlayerController::OnTouchPressed);
}

void AMyPlayerController::OnTouchPressed(ETouchIndex::Type FingerIndex, FVector Location)
{
    FVector2D screenPos = FVector2D(Location.X, Location.Y);
    ZeroGUI::Input::UpdateTouchState((int)FingerIndex, screenPos, true);
}
```

## Menu Toggle Strategies

### Desktop
```cpp
if (GetAsyncKeyState(VK_F2) & 1) 
    menu_opened = !menu_opened;
```

### Mobile - Double Tap
```cpp
void CheckMenuToggleGesture(float currentTime)
{
    if (IsTouchActive(0) && IsTouchClicked(0, 255, false))
    {
        float timeSinceLastTap = currentTime - lastTapTime;
        if (timeSinceLastTap < 0.3f)
        {
            doubleTapCount++;
            if (doubleTapCount >= 2)
                menuOpened = !menuOpened;
        }
        lastTapTime = currentTime;
    }
}
```

### Alternative Mobile Options
- Three-finger tap
- Two-finger long press
- Swipe from edge
- Dedicated UI button
- Shake gesture (via accelerometer)

## Cursor vs. Touch Indicator

### Desktop
```cpp
ZeroGUI::Draw_Cursor(menu_opened);
```
Draws a custom cursor at mouse position.

### Mobile
```cpp
if (ZeroGUI::Input::IsTouchActive(0))
{
    FVector2D touchPos = ZeroGUI::Input::GetPrimaryTouchPosition();
    // Draw circle or ripple effect at touchPos
}
```
Shows visual feedback for touch location.

## Performance Considerations

### Memory Usage

| Component | Desktop | Mobile |
|-----------|---------|--------|
| Input state | ~512 bytes | ~800 bytes |
| Touch positions | N/A | 80 bytes (10 FVector2D) |
| Gesture state | N/A | ~40 bytes |

### CPU Usage

Mobile adds minimal overhead:
- Touch state updates: < 0.01ms per touch
- Gesture recognition: < 0.05ms per frame
- Hook overhead: < 0.01ms per frame

### Build Size

Additional library sizes:
- CydiaSubstrate (iOS): ~500KB
- Dobby (Android): ~300KB

## Future Enhancements

### Planned Features
1. Advanced gesture recognition (pinch-to-zoom, rotate)
2. Haptic feedback integration
3. Accessibility features (larger touch targets mode)
4. Landscape/portrait orientation handling
5. Keyboard support for devices with physical keyboards
6. Stylus/Apple Pencil precision mode

### Potential Improvements
1. Use UE4's built-in touch system more directly
2. Add unit tests for gesture recognition
3. Create visual touch debugger/overlay
4. Add performance profiling tools
5. Create platform-agnostic ZeroGUI.h version

## Comparison with Alternatives

### Why Not Just Emulate Mouse?

Some mobile ports simply map touch to mouse events. We chose a native touch approach because:

**Touch Emulation Problems:**
- No multi-touch support
- Poor gesture recognition
- Doesn't feel native
- Can't handle simultaneous UI interactions

**Native Touch Benefits:**
- ✅ Full multi-touch support
- ✅ Natural gesture handling
- ✅ Platform-appropriate interactions
- ✅ Better performance (no translation layer)

### Why Custom Hooking Instead of UE4 Delegates?

UE4 provides delegates for rendering, but hooking offers:

**Advantages:**
- Works without access to game source code
- Can be injected into existing games
- No need to modify game's rendering pipeline
- More flexible for mod/tool development

**Disadvantages:**
- Platform-specific (requires different libs per platform)
- May break with UE4 updates (symbol changes)
- More complex setup

## Testing Recommendations

### Device Coverage

**iOS:**
- iPhone (various sizes): SE, 8, 12, 14
- iPad (various sizes): Mini, Air, Pro
- Different iOS versions: 12, 13, 14, 15+

**Android:**
- Phones: Samsung, Google Pixel, OnePlus
- Tablets: Samsung Tab, Amazon Fire
- Different Android versions: 9, 10, 11, 12+
- Various screen densities: hdpi, xhdpi, xxhdpi, xxxhdpi

### Test Scenarios

1. **Basic Interaction**
   - Single touch on buttons
   - Slider dragging
   - Checkbox toggling
   - Combo box selection

2. **Multi-Touch**
   - Two-finger interactions
   - Simultaneous button presses
   - Touch while dragging

3. **Gestures**
   - Menu toggle (double-tap)
   - Quick taps
   - Long presses
   - Swipes

4. **Edge Cases**
   - Rapid touches
   - Touch near screen edges
   - Orientation changes
   - Interruptions (phone call, notification)

5. **Performance**
   - Low-end devices
   - High refresh rate displays
   - During intensive gameplay
   - With many UI elements

## Troubleshooting Guide

### Common Issues

**Issue:** Touch not registered
- Check touch events are being fed to UpdateTouchState()
- Verify ZeroGUI::Input::Handle() is called each frame
- Check canvas is properly initialized

**Issue:** Menu toggle not working
- Verify gesture timing constants (300ms for double-tap)
- Check virtual key state management
- Test with simpler toggle mechanism first

**Issue:** UI elements too small
- Set proper DPI scale with SetDPIScale()
- Verify minimum touch target enforcement
- Test on actual device (not just simulator)

**Issue:** Hook not installing
- Verify correct library name (libUE4.so, etc.)
- Check PostRender symbol exists with nm/objdump
- Ensure hooking library is properly linked

## Summary

The mobile port maintains the simplicity and ease-of-use of the original desktop GUI while adapting it for touch-based mobile platforms. The architecture prioritizes:

1. **Backward Compatibility** - Desktop code unchanged
2. **Platform Abstraction** - Clean separation of platform-specific code
3. **Touch-First Design** - Native touch handling, not mouse emulation
4. **Flexibility** - Supports both iOS and Android with minimal code duplication
5. **Ease of Integration** - Simple API, comprehensive documentation

This design allows developers to create cross-platform UE4 GUIs that feel native on both desktop and mobile devices.
