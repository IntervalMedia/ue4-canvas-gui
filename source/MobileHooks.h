#pragma once
#include "PlatformDefines.h"

// Hooking framework integration for mobile platforms
// iOS: CydiaSubstrate
// Android: Dobby/Substrate-compatible framework

#if PLATFORM_IOS
    // iOS Hooking with CydiaSubstrate
    #include <substrate.h>
    
    // Function pointer type for PostRender
    typedef void (*PostRenderFunc)(UGameViewportClient*, UCanvas*);
    
    // Original function pointer
    PostRenderFunc originalPostRender = nullptr;
    
    // Hook handler declaration
    void HookedPostRender(UGameViewportClient* viewport, UCanvas* canvas);
    
    // Install PostRender hook using CydiaSubstrate
    void InstallPostRenderHook()
    {
        // Find the PostRender function address
        // Note: This is a simplified example - actual implementation needs proper symbol resolution
        void* postRenderAddr = MSFindSymbol(nullptr, "_ZN20UGameViewportClient10PostRenderEP7UCanvas");
        
        if (postRenderAddr)
        {
            MSHookFunction(postRenderAddr, (void*)&HookedPostRender, (void**)&originalPostRender);
        }
    }
    
    // Call original function
    void CallOriginalPostRender(UGameViewportClient* viewport, UCanvas* canvas)
    {
        if (originalPostRender)
        {
            originalPostRender(viewport, canvas);
        }
    }

#elif PLATFORM_ANDROID
    // Android Hooking with Dobby (modern substrate alternative)
    // Dobby is more actively maintained and works well on ARM64
    #include "dobby.h"
    
    // Function pointer type for PostRender
    typedef void (*PostRenderFunc)(UGameViewportClient*, UCanvas*);
    
    // Original function pointer
    PostRenderFunc originalPostRender = nullptr;
    
    // Hook handler declaration
    void HookedPostRender(UGameViewportClient* viewport, UCanvas* canvas);
    
    // Install PostRender hook using Dobby
    void InstallPostRenderHook()
    {
        // Find the PostRender function address
        // On Android, you can use dlsym or pattern scanning
        void* postRenderAddr = DobbySymbolResolver("libUE4.so", "_ZN20UGameViewportClient10PostRenderEP7UCanvas");
        
        if (postRenderAddr)
        {
            DobbyHook(postRenderAddr, (void*)&HookedPostRender, (void**)&originalPostRender);
        }
    }
    
    // Call original function
    void CallOriginalPostRender(UGameViewportClient* viewport, UCanvas* canvas)
    {
        if (originalPostRender)
        {
            originalPostRender(viewport, canvas);
        }
    }

#else
    // Fallback for non-mobile platforms
    #warning "Mobile hooking framework not available on this platform"
    
    void InstallPostRenderHook()
    {
        // No-op on desktop
    }
    
    void CallOriginalPostRender(UGameViewportClient* viewport, UCanvas* canvas)
    {
        // No-op on desktop
    }
    
    void HookedPostRender(UGameViewportClient* viewport, UCanvas* canvas)
    {
        // No-op on desktop
    }
#endif

// Common hook initialization
namespace MobileHooks
{
    bool isInitialized = false;
    
    void Initialize()
    {
        if (!isInitialized)
        {
            InstallPostRenderHook();
            isInitialized = true;
        }
    }
    
    bool IsInitialized()
    {
        return isInitialized;
    }
}
