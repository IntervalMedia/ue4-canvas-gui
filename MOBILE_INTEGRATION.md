# Mobile Integration Guide for UE4 Canvas GUI

This guide covers the mobile port of UE4 Canvas GUI for Android and iOS platforms.

## IMPORTANT: Runtime-Injected Dynamic Library

This framework is designed to be **injected at runtime** into a production UE4 application as a dynamic library. This means:

- ❌ **We CANNOT use UE4 API functions directly** (no compile-time access)
- ✅ **We MUST use function hooking** to intercept UE4 functions at runtime
- ✅ **OR use native iOS/Android APIs** for touch input and other functionality

This is fundamentally different from a compile-time integrated library where you would have access to UE4 classes like `APlayerController`, `InputComponent`, etc.

## Overview

The mobile port includes:
- Touch-based input handling (supports up to 10 simultaneous touches)
- Platform detection macros (iOS/Android)
- **Native iOS UITouch** event handling (via Objective-C method swizzling)
- **Native Android MotionEvent** handling (via JNI)
- **CydiaSubstrate hooking** for iOS PostRender interception
- **Dobby hooking framework** for Android PostRender interception
- DPI-aware scaling for different screen sizes
- Touch-friendly hit detection (20% larger hit areas)
- Example implementation with gesture-based menu toggle

## File Structure

```
source/
├── PlatformDefines.h           # Platform detection macros
├── ZeroInputMobile.h           # Touch input handling
├── PlatformAbstraction.h       # Platform-independent input abstraction
├── MobileHooks.h               # Hooking framework integration
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

Since this is a runtime-injected library, integration is different from traditional libraries:

```cpp
// Build as a dynamic library
#include "PlatformDefines.h"
#include "MobileHooks.h"

#if PLATFORM_MOBILE
    #include "ZeroInputMobile.h"
    #include "PlatformAbstraction.h"
#else
    #include "ZeroInput.h"
#endif

// Entry point called when library is injected
extern "C" void InitializeMobileGUI()
{
    #if PLATFORM_MOBILE
        MobileHooks::Initialize();
    #endif
}
```

### 2. Touch Input - Native Approach (Recommended)

**iOS - Using Native UITouch Events:**

The framework automatically intercepts UITouch events via Objective-C method swizzling. No code needed - it's all handled in MobileMenuExample.cpp.

**Android - Using Native MotionEvent:**

Add this Java code to intercept touch events:

```java
// In your GameActivity.java or custom Activity
public class GameActivity extends NativeActivity {
    
    // Native method declaration
    private native void nativeTouchEvent(int action, int pointerIndex, float x, float y);
    
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        int action = event.getActionMasked();
        int pointerIndex = event.getActionIndex();
        float x = event.getX(pointerIndex);
        float y = event.getY(pointerIndex);
        
        // Forward to native code
        nativeTouchEvent(action, pointerIndex, x, y);
        
        return super.onTouchEvent(event);
    }
    
    static {
        System.loadLibrary("MobileGUI");
    }
}
```

### 3. Alternative: Hook UE4's Touch Input Functions

If you prefer to hook UE4's internal touch processing instead of using native APIs:

```cpp
// iOS: Hook FIOSInputInterface
#if PLATFORM_IOS
    void* touchFunc = MSFindSymbol(nullptr, 
        "_ZN18FIOSInputInterface16HandleTouchEventEiiffffb");
    if (touchFunc) {
        MSHookFunction(touchFunc, (void*)&HookedTouchEvent, 
            (void**)&originalTouchEvent);
    }
#endif

// Android: Hook FAndroidInputInterface
#if PLATFORM_ANDROID
    void* touchFunc = DobbySymbolResolver("libUE4.so",
        "_ZN22FAndroidInputInterface10TouchEventEiiiffb");
    if (touchFunc) {
        DobbyHook(touchFunc, (void*)&HookedTouchEvent,
            (void**)&originalTouchEvent);
    }
#endif

// In your hooked function, extract touch data and call:
// ZeroGUI::Input::UpdateTouchState(touchIndex, position, isDown);
```

**Note:** Symbol names may vary by UE4 version. Use `nm` (iOS) or `readelf` (Android) to find correct symbols.

### 3. Creating a Simple Menu

The menu rendering happens automatically in the hooked PostRender function:

```cpp
void PostRenderHook(UGameViewportClient* viewport, UCanvas* canvas)
{
    // Call original PostRender first
    CallOriginalPostRender(viewport, canvas);
    
    // Setup canvas for drawing (if ZeroGUI.h is available)
    // ZeroGUI::SetupCanvas(canvas);
    
    // Handle input
    ZeroGUI::Input::Handle();
    
    // Menu toggle with double-tap gesture
    static bool menuOpened = false;
    if (ZeroGUI::Input::IsKeyPressed(ZeroGUI::Input::Gestures::GESTURE_DOUBLE_TAP, false))
    {
        menuOpened = !menuOpened;
    }
    
    // Draw menu (example structure)
    /*
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
    */
    
    // Draw touch indicator
    if (ZeroGUI::Input::IsTouchActive(0))
    {
        FVector2D touchPos = ZeroGUI::Input::GetPrimaryTouchPosition();
        // Draw circle or ripple effect at touchPos
    }
}
```

## Building Instructions

### iOS Build (Dynamic Library)

1. **Add CydiaSubstrate Framework**
   - Download CydiaSubstrate SDK
   - Add `substrate.h` to your include path
   - Link against `libsubstrate.dylib`

2. **Build Command**
   ```bash
   # Compile as dynamic library (.dylib)
   clang++ -std=c++11 -stdlib=libc++ \
           -shared -fPIC \
           -framework UIKit -framework Foundation \
           -lsubstrate \
           -I/path/to/substrate/include \
           -o MobileGUI.dylib \
           source/*.cpp source/*.mm
   
   # Code sign the library
   codesign -s "Your Identity" MobileGUI.dylib
   ```

3. **Injection Methods**
   ```bash
   # Method 1: DYLD_INSERT_LIBRARIES (jailbroken devices)
   DYLD_INSERT_LIBRARIES=MobileGUI.dylib ./YourUE4Game.app/YourUE4Game
   
   # Method 2: Modify app bundle (for sideloading)
   # - Add MobileGUI.dylib to Frameworks folder
   # - Modify Info.plist to load the dylib
   # - Re-sign the app
   
   # Method 3: Use injection tools
   # - CydiaSubstrate (jailbreak)
   # - Frida
   # - Insert Dylib (for patching apps)
   ```

4. **Important Notes**
   - App Store builds cannot use dylib injection
   - Requires jailbreak OR custom app signing
   - For production, consider alternative hooking methods

### Android Build (Shared Library)

1. **Add Dobby Hooking Library**
   - Include Dobby in your project (https://github.com/jmpews/Dobby)
   - Build Dobby for Android (armeabi-v7a and arm64-v8a)

2. **CMakeLists.txt Example**
   ```cmake
   cmake_minimum_required(VERSION 3.4.1)
   
   # Add Dobby
   add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/Dobby dobby)
   
   # Your library
   add_library(MobileGUI SHARED
       source/MobileMenuExample.cpp
       source/MobileHooks.h
       source/ZeroInputMobile.h
       source/PlatformAbstraction.h
       source/PlatformDefines.h
   )
   
   # Link against Dobby
   target_link_libraries(MobileGUI
       dobby
       log
       android
   )
   ```

3. **Android.mk Example (alternative to CMake)**
   ```make
   LOCAL_PATH := $(call my-dir)
   
   # Dobby
   include $(CLEAR_VARS)
   LOCAL_MODULE := dobby
   LOCAL_SRC_FILES := ThirdParty/Dobby/$(TARGET_ARCH_ABI)/libdobby.a
   include $(PREBUILT_STATIC_LIBRARY)
   
   # MobileGUI
   include $(CLEAR_VARS)
   LOCAL_MODULE := MobileGUI
   LOCAL_SRC_FILES := source/MobileMenuExample.cpp
   LOCAL_STATIC_LIBRARIES := dobby
   LOCAL_LDLIBS := -llog -landroid
   include $(BUILD_SHARED_LIBRARY)
   ```

4. **Build Command**
   ```bash
   # Using ndk-build
   ${ANDROID_NDK}/ndk-build APP_ABI="armeabi-v7a arm64-v8a"
   
   # Or using CMake
   cmake -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake \
         -DANDROID_ABI=arm64-v8a \
         -DANDROID_PLATFORM=android-21 \
         .
   make
   ```

5. **Injection Methods**
   ```bash
   # Method 1: Modify APK
   # - Unpack APK with apktool
   # - Add libMobileGUI.so to lib/<ABI>/
   # - Modify Java code to System.loadLibrary("MobileGUI")
   # - Repack and sign APK
   
   # Method 2: LD_PRELOAD (rooted devices)
   LD_PRELOAD=/data/local/tmp/libMobileGUI.so am start your.package.name
   
   # Method 3: Xposed/Magisk modules
   # - Create a module that loads your library
   # - Install module on rooted device
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
- Verify CydiaSubstrate is properly installed on the device
- Check that the PostRender symbol name is correct for your UE4 version
  ```bash
  # Find the correct symbol name
  nm -gU /path/to/UE4/binary | grep PostRender
  ```
- Ensure proper code signing (ad-hoc signing may work for testing)
- On non-jailbroken devices, use alternative injection methods
- Check that DYLD_INSERT_LIBRARIES environment variable is set correctly

### Issue: Hooks not working on Android
- Verify Dobby is properly linked (check with `readelf -d libMobileGUI.so`)
- Check library name (might be different from "libUE4.so")
  ```bash
  # Find the correct library
  ls /data/app/your.package/lib/arm64/
  ```
- Find the correct PostRender symbol:
  ```bash
  readelf -Ws /path/to/libUE4.so | grep PostRender
  ```
- Use `adb logcat` to check for errors during hook installation
- Ensure SELinux is permissive or properly configured

### Issue: Touch input not responding
- Verify native touch handlers are properly installed
  - iOS: Check that method swizzling succeeded
  - Android: Verify JNI methods are being called (add log statements)
- Check that `ZeroGUI::Input::Handle()` is called each frame in PostRender hook
- Use debug logging to verify UpdateTouchState() is being called:
  ```cpp
  void UpdateTouchState(int idx, FVector2D pos, bool isDown)
  {
      #ifdef DEBUG
      printf("Touch %d at (%.0f, %.0f) %s\n", idx, pos.X, pos.Y, 
             isDown ? "DOWN" : "UP");
      #endif
      // ... rest of function
  }
  ```

### Issue: Symbol not found errors
- UE4 symbol names vary by version and build configuration
- Use `nm` (iOS) or `readelf` (Android) to find correct symbols
- Try these alternative symbol patterns:
  ```cpp
  // PostRender variations
  "_ZN20UGameViewportClient10PostRenderEP7UCanvas"  // Typical
  "_ZN20UGameViewportClient10PostRenderEPN7UEngine7UCanvasE"  // Some versions
  
  // Touch input variations (for hooking UE4 touch handlers)
  "_ZN18FIOSInputInterface16HandleTouchEventEiiffffb"  // iOS
  "_ZN22FAndroidInputInterface10TouchEventEiiiffb"  // Android
  ```

### Issue: UI elements too small on high-DPI devices
- Set proper DPI scale (this must be determined at runtime)
- For iOS, use UIScreen.mainScreen.scale
- For Android, use DisplayMetrics.density
- Example implementation would require hooking into UE4's viewport creation

### Issue: Library injection fails
- **iOS:** Check code signing, entitlements, and jailbreak status
- **Android:** Verify target app is debuggable OR device is rooted
- Use Frida or similar tools for easier injection during development
- Check file permissions on the injected library

## Migration from Compile-Time Integration

**IMPORTANT:** Previous documentation may have shown UE4 API usage like this:

```cpp
// ❌ THIS DOES NOT WORK FOR RUNTIME-INJECTED LIBRARIES
void AYourPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindTouch(IE_Pressed, ...);  // WRONG
}
```

**This approach requires:**
- Access to UE4 source at compile time
- Ability to subclass APlayerController
- Linking against UE4 headers and libraries

**For runtime injection, use instead:**

✅ **Native iOS Approach:**
```cpp
// Use Objective-C method swizzling (see MobileMenuExample.cpp)
- Override UIView touch methods
- Intercept touches before UE4 processes them
- No UE4 API dependencies
```

✅ **Native Android Approach:**
```java
// Override onTouchEvent in Java
@Override
public boolean onTouchEvent(MotionEvent event) {
    nativeTouchEvent(action, index, x, y);  // JNI call
    return super.onTouchEvent(event);
}
```

✅ **Hooking UE4 Internals:**
```cpp
// Hook UE4's internal touch processing
MSHookFunction(FindSymbol("HandleTouchEvent"), ...);  // iOS
DobbyHook(FindSymbol("TouchEvent"), ...);  // Android
```

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
