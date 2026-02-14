# Mobile Build Configuration for UE4 Canvas GUI

## Build System Integration

This document describes how to integrate the mobile port into your UE4 project's build system.

## UE4 Build.cs Configuration

Add this to your project's `Source/YourProject/YourProject.Build.cs` file:

```csharp
using UnrealBuildTool;
using System.IO;

public class YourProject : ModuleRules
{
    public YourProject(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] 
        { 
            "Core", 
            "CoreUObject", 
            "Engine", 
            "InputCore",
            "UMG", // For Canvas
            "Slate",
            "SlateCore"
        });

        // Mobile-specific configuration
        if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            // iOS with CydiaSubstrate
            PublicDefinitions.Add("PLATFORM_IOS=1");
            
            // Add CydiaSubstrate framework path
            string SubstratePath = Path.Combine(ModuleDirectory, "ThirdParty", "Substrate");
            PublicIncludePaths.Add(Path.Combine(SubstratePath, "include"));
            
            // Link substrate library (if using static library)
            // PublicAdditionalLibraries.Add(Path.Combine(SubstratePath, "lib", "libsubstrate.a"));
            
            // Or link as framework (if using framework)
            PublicFrameworks.Add("CydiaSubstrate");
        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            // Android with Dobby
            PublicDefinitions.Add("PLATFORM_ANDROID=1");
            
            // Add Dobby library path
            string DobbyPath = Path.Combine(ModuleDirectory, "ThirdParty", "Dobby");
            PublicIncludePaths.Add(Path.Combine(DobbyPath, "include"));
            
            // Add Dobby library for different architectures
            string DobbyLibPath = Path.Combine(DobbyPath, "lib");
            
            // ARM64
            PublicAdditionalLibraries.Add(Path.Combine(DobbyLibPath, "arm64-v8a", "libdobby.a"));
            
            // ARMv7 (if supporting older devices)
            // PublicAdditionalLibraries.Add(Path.Combine(DobbyLibPath, "armeabi-v7a", "libdobby.a"));
        }

        // Include mobile GUI source files
        string MobileGUIPath = Path.Combine(ModuleDirectory, "MobileGUI");
        PublicIncludePaths.Add(MobileGUIPath);
        
        bEnableExceptions = true;
    }
}
```

## CMake Configuration (Android Native)

If building Android native code separately, use this CMakeLists.txt:

```cmake
cmake_minimum_required(VERSION 3.10)
project(MobileGUI)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform detection
if(ANDROID)
    add_definitions(-DPLATFORM_ANDROID=1)
    add_definitions(-DPLATFORM_MOBILE=1)
    
    # Add Dobby
    add_subdirectory(ThirdParty/Dobby)
    include_directories(ThirdParty/Dobby/include)
    
    # Source files
    add_library(mobilegui SHARED
        source/MobileMenuExample.cpp
    )
    
    # Link libraries
    target_link_libraries(mobilegui
        dobby
        log
        android
    )
    
elseif(IOS)
    add_definitions(-DPLATFORM_IOS=1)
    add_definitions(-DPLATFORM_MOBILE=1)
    
    # Add CydiaSubstrate
    include_directories(ThirdParty/Substrate/include)
    
    # Source files
    add_library(mobilegui STATIC
        source/MobileMenuExample.cpp
    )
    
    # Link frameworks
    target_link_libraries(mobilegui
        "-framework CydiaSubstrate"
    )
endif()

# Include directories
target_include_directories(mobilegui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/source
)
```

## Android NDK Build (Android.mk)

Alternative build configuration for Android NDK:

```makefile
LOCAL_PATH := $(call my-dir)

# Dobby hooking library
include $(CLEAR_VARS)
LOCAL_MODULE := dobby
LOCAL_SRC_FILES := ThirdParty/Dobby/lib/$(TARGET_ARCH_ABI)/libdobby.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/ThirdParty/Dobby/include
include $(PREBUILT_STATIC_LIBRARY)

# Mobile GUI library
include $(CLEAR_VARS)
LOCAL_MODULE := mobilegui
LOCAL_SRC_FILES := source/MobileMenuExample.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)/source
LOCAL_STATIC_LIBRARIES := dobby
LOCAL_CPPFLAGS += -DPLATFORM_ANDROID=1 -DPLATFORM_MOBILE=1 -std=c++17
LOCAL_LDLIBS := -llog -landroid
include $(BUILD_SHARED_LIBRARY)
```

## Xcode Project Configuration (iOS)

### 1. Add Substrate Framework

1. Download CydiaSubstrate SDK
2. Drag `CydiaSubstrate.framework` into your Xcode project
3. Ensure it's added to "Link Binary With Libraries" build phase

### 2. Add Preprocessor Macros

In Build Settings, add to "Preprocessor Macros":
```
PLATFORM_IOS=1
PLATFORM_MOBILE=1
```

### 3. Add Header Search Paths

Add to "Header Search Paths":
```
$(PROJECT_DIR)/source
$(PROJECT_DIR)/ThirdParty/Substrate/include
```

### 4. Linker Flags

Add to "Other Linker Flags":
```
-framework CydiaSubstrate
```

## Gradle Configuration (Android with UE4)

Add to your `app/build.gradle`:

```gradle
android {
    defaultConfig {
        // ...
        externalNativeBuild {
            cmake {
                cppFlags "-std=c++17 -DPLATFORM_ANDROID=1 -DPLATFORM_MOBILE=1"
                arguments "-DANDROID_STL=c++_shared"
            }
        }
        ndk {
            abiFilters 'arm64-v8a', 'armeabi-v7a'
        }
    }
    
    externalNativeBuild {
        cmake {
            path "CMakeLists.txt"
            version "3.10.2"
        }
    }
}

dependencies {
    // Add any required dependencies
}
```

## Directory Structure for Third-Party Libraries

```
YourProject/
├── Source/
│   ├── YourProject/
│   │   ├── MobileGUI/
│   │   │   ├── PlatformDefines.h
│   │   │   ├── ZeroInputMobile.h
│   │   │   ├── PlatformAbstraction.h
│   │   │   ├── MobileHooks.h
│   │   │   └── MobileMenuExample.cpp
│   │   └── ThirdParty/
│   │       ├── Substrate/           # iOS
│   │       │   ├── include/
│   │       │   │   └── substrate.h
│   │       │   └── lib/
│   │       │       └── libsubstrate.a
│   │       └── Dobby/               # Android
│   │           ├── include/
│   │           │   └── dobby.h
│   │           └── lib/
│   │               ├── arm64-v8a/
│   │               │   └── libdobby.a
│   │               └── armeabi-v7a/
│   │                   └── libdobby.a
```

## Compiler Flags

### iOS (Clang)
```bash
-DPLATFORM_IOS=1 -DPLATFORM_MOBILE=1 -std=c++17 -ObjC++
```

### Android (Clang/GCC)
```bash
-DPLATFORM_ANDROID=1 -DPLATFORM_MOBILE=1 -std=c++17 -fPIC
```

## Linking Requirements

### iOS
- CydiaSubstrate.framework (or libsubstrate.dylib)
- Foundation.framework
- UIKit.framework

### Android
- libdobby.a (or libdobby.so)
- liblog.so (for Android logging)
- libandroid.so

## Troubleshooting Build Issues

### Issue: "substrate.h not found" (iOS)
**Solution:** Add Substrate include path to Header Search Paths in Xcode

### Issue: "dobby.h not found" (Android)
**Solution:** Verify Dobby is in ThirdParty/Dobby and CMakeLists.txt includes the correct path

### Issue: Undefined symbols for CydiaSubstrate (iOS)
**Solution:** Ensure CydiaSubstrate framework is linked in "Link Binary With Libraries"

### Issue: Undefined reference to Dobby functions (Android)
**Solution:** Check that libdobby.a is included for the correct architecture (arm64-v8a)

### Issue: Build fails with "cannot find -lsubstrate"
**Solution:** Use framework linking instead of library, or provide full path to library

## Testing Build Configuration

### iOS
```bash
# Build from command line
xcodebuild -project YourProject.xcodeproj -scheme YourScheme -sdk iphoneos
```

### Android
```bash
# Build with Gradle
./gradlew assembleDebug

# Build with CMake directly
mkdir build && cd build
cmake .. -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-21
make
```

## Deployment

### iOS
- Sign with development or distribution certificate
- For jailbroken devices: Install CydiaSubstrate from Cydia
- For App Store: Consider alternative hooking methods that don't require Substrate

### Android
- Build APK with gradle
- Install Dobby library with your APK
- Test on various ARM architectures (arm64-v8a, armeabi-v7a)

## Performance Optimization

### Compiler Optimizations
```bash
# Release builds
-O3 -DNDEBUG -ffast-math

# Size optimization
-Os -flto
```

### Link-Time Optimization
Enable LTO in build settings for better code optimization across modules.

## Additional Resources

- [UE4 Build Configuration Documentation](https://docs.unrealengine.com/en-US/ProductionPipelines/BuildTools/UnrealBuildTool/index.html)
- [CydiaSubstrate Documentation](http://www.cydiasubstrate.com/api/)
- [Dobby GitHub Repository](https://github.com/jmpews/Dobby)
- [Android NDK Documentation](https://developer.android.com/ndk/guides)
