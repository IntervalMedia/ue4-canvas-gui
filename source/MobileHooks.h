#pragma once
#include "PlatformDefines.h"

// Hooking framework integration for mobile platforms
// iOS: CydiaSubstrate + Native UIGestureRecognizer
// Android: Dobby + Native gesture detection

#if PLATFORM_IOS
    // iOS Hooking with CydiaSubstrate
    #include <substrate.h>
    
    // Forward declarations to avoid UE4 dependencies
    class UGameViewportClient;
    class UCanvas;
    
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
        // Note: Symbol name may vary by UE4 version - adjust as needed
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
    
    // Native iOS Touch Input via UIGestureRecognizer
    // This uses native iOS APIs instead of UE4 functions
    #ifdef __OBJC__
    #import <UIKit/UIKit.h>
    
    namespace iOSNativeTouch
    {
        // Setup native iOS gesture recognizers on the main UIView
        // Note: We don't actually use gesture recognizers for touch tracking,
        // but we need to enable multi-touch on the view
        void SetupNativeGestureRecognizers(UIView* mainView)
        {
            // Enable multi-touch support on the main view
            // This allows us to receive multiple simultaneous touch events
            mainView.multipleTouchEnabled = YES;
            mainView.userInteractionEnabled = YES;
            
            // Touch handling will be done via method swizzling of touch methods
            // rather than using gesture recognizers (more direct control)
        }
        
        // Get the main UIView from the UE4 window
        // This must be called after UE4 initializes its window
        UIView* GetMainView()
        {
            // Get the key window (UE4's main window)
            UIWindow* window = [[UIApplication sharedApplication] keyWindow];
            if (window)
            {
                return window.rootViewController.view;
            }
            return nil;
        }
        
        // Initialize native touch input system
        void Initialize()
        {
            UIView* mainView = GetMainView();
            if (mainView)
            {
                SetupNativeGestureRecognizers(mainView);
            }
        }
    }
    #endif // __OBJC__

#elif PLATFORM_ANDROID
    // Android Hooking with Dobby (modern substrate alternative)
    // Dobby is more actively maintained and works well on ARM64
    #include "dobby.h"
    #include <jni.h>
    #include <android/input.h>
    
    // Forward declarations to avoid UE4 dependencies
    class UGameViewportClient;
    class UCanvas;
    
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
        // On Android, library name might vary (libUE4.so, libUnrealEngine.so, etc.)
        void* postRenderAddr = DobbySymbolResolver("libUE4.so", "_ZN20UGameViewportClient10PostRenderEP7UCanvas");
        
        if (!postRenderAddr)
        {
            // Try alternative library names
            postRenderAddr = DobbySymbolResolver("libUnrealEngine.so", "_ZN20UGameViewportClient10PostRenderEP7UCanvas");
        }
        
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
    
    // Native Android Touch Input
    // Use JNI to interface with Android's native touch system
    namespace AndroidNativeTouch
    {
        static JavaVM* g_JavaVM = nullptr;
        static jobject g_ActivityObject = nullptr;
        
        // Initialize JNI for touch input
        void Initialize(JavaVM* vm, jobject activity)
        {
            g_JavaVM = vm;
            JNIEnv* env;
            if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_OK)
            {
                g_ActivityObject = env->NewGlobalRef(activity);
            }
        }
        
        // Get current touch events from native Android
        // This can be called from the hooked PostRender to poll touch state
        void PollTouchEvents()
        {
            // Touch events will be processed via the InputDevice system
            // or by hooking into Android's MotionEvent dispatch
            // This is a placeholder for the native touch polling mechanism
        }
    }

#else
    // Fallback for non-mobile platforms
    #warning "Mobile hooking framework not available on this platform"
    
    // Forward declarations to avoid dependencies
    class UGameViewportClient;
    class UCanvas;
    
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
            
            #if PLATFORM_IOS && defined(__OBJC__)
                // Initialize native iOS touch input
                iOSNativeTouch::Initialize();
            #elif PLATFORM_ANDROID
                // Android touch initialization happens via JNI
                // Must be called from Java side with: AndroidNativeTouch::Initialize(vm, activity)
            #endif
            
            isInitialized = true;
        }
    }
    
    bool IsInitialized()
    {
        return isInitialized;
    }
}
