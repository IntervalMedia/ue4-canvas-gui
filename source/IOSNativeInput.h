#pragma once
#include "PlatformDefines.h"

#if PLATFORM_IOS

// C++ interface for the iOS native touch input hook.
// The implementation lives in IOSNativeInput.mm (Objective-C++).
namespace IOSNativeInput
{
    // Install Objective-C method hooks on UIViewController's touch methods
    // (touchesBegan:withEvent:, touchesMoved:withEvent:,
    //  touchesCancelled:withEvent:, touchesEnded:withEvent:).
    // Safe to call multiple times – the second and subsequent calls are no-ops.
    void InstallTouchHooks();

    // Returns true after InstallTouchHooks() has completed successfully.
    bool IsInstalled();
}

#endif // PLATFORM_IOS
