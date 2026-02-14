#pragma once

// Platform detection macros for mobile devices
#if defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        #define PLATFORM_IOS 1
        #define PLATFORM_MOBILE 1
    #endif
#elif defined(__ANDROID__) || defined(ANDROID)
    #define PLATFORM_ANDROID 1
    #define PLATFORM_MOBILE 1
#endif

// Fallback for non-mobile platforms
#ifndef PLATFORM_MOBILE
    #define PLATFORM_MOBILE 0
#endif

#ifndef PLATFORM_IOS
    #define PLATFORM_IOS 0
#endif

#ifndef PLATFORM_ANDROID
    #define PLATFORM_ANDROID 0
#endif
