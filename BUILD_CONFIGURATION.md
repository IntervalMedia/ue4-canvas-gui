# Mobile Build Configuration for UE4 Canvas GUI

## IMPORTANT: Runtime-Injected Dynamic Library

This framework is designed to be **injected at runtime** into production UE4 applications. This means:

- ❌ **DO NOT integrate into UE4 Build.cs** (that's for compile-time integration)
- ✅ **Build as a standalone dynamic library** (.dylib for iOS, .so for Android)
- ✅ **Inject into running UE4 game** (various methods available)
- ✅ **NO UE4 headers or libraries required** for building

## Build System Integration

This document describes how to build the mobile GUI as a dynamic library for runtime injection.

## ~~UE4 Build.cs Configuration~~ (NOT USED - IGNORE THIS SECTION)

**UPDATE:** The following Build.cs configuration is **NOT NEEDED** for runtime injection.

It's only included for reference if you have UE4 source code and want compile-time integration.

For runtime injection (recommended), skip to the "Standalone Dynamic Library Build" section below.

<details>
<summary>Click to expand: Old Build.cs approach (compile-time integration)</summary>

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
            PublicDefinitions.Add("PLATFORM_IOS=1");
            string SubstratePath = Path.Combine(ModuleDirectory, "ThirdParty", "Substrate");
            PublicIncludePaths.Add(Path.Combine(SubstratePath, "include"));
            PublicFrameworks.Add("CydiaSubstrate");
        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            PublicDefinitions.Add("PLATFORM_ANDROID=1");
            string DobbyPath = Path.Combine(ModuleDirectory, "ThirdParty", "Dobby");
            PublicIncludePaths.Add(Path.Combine(DobbyPath, "include"));
            string DobbyLibPath = Path.Combine(DobbyPath, "lib");
            PublicAdditionalLibraries.Add(Path.Combine(DobbyLibPath, "arm64-v8a", "libdobby.a"));
        }

        string MobileGUIPath = Path.Combine(ModuleDirectory, "MobileGUI");
        PublicIncludePaths.Add(MobileGUIPath);
        
        bEnableExceptions = true;
    }
}
```

**Note:** This approach requires UE4 source code and recompilation. Not recommended for runtime injection.

</details>

## Standalone Dynamic Library Build (Recommended)

Build the mobile GUI as a standalone dynamic library without UE4 dependencies.

### iOS Dynamic Library Build (.dylib)

Build as a dynamic library for iOS injection:

```bash
#!/bin/bash
# build_ios.sh - Build MobileGUI as dynamic library for iOS

# Configuration
SDK="iphoneos"  # or "iphonesimulator" for simulator
ARCH="arm64"    # or "x86_64" for simulator
MIN_VERSION="12.0"

# Paths
SUBSTRATE_PATH="/path/to/substrate"  # CydiaSubstrate SDK location
SOURCE_DIR="./source"
OUTPUT="MobileGUI.dylib"

# C++ compiler flags (using C++17 for consistency)
CXXFLAGS="-std=c++17 -stdlib=libc++ -fPIC -shared"
CXXFLAGS="$CXXFLAGS -arch $ARCH"
CXXFLAGS="$CXXFLAGS -miphoneos-version-min=$MIN_VERSION"
CXXFLAGS="$CXXFLAGS -isysroot $(xcrun --sdk $SDK --show-sdk-path)"
CXXFLAGS="$CXXFLAGS -DPLATFORM_IOS=1 -DPLATFORM_MOBILE=1"
CXXFLAGS="$CXXFLAGS -I$SUBSTRATE_PATH/include"

# Frameworks and libraries
FRAMEWORKS="-framework Foundation -framework UIKit"
LIBS="-L$SUBSTRATE_PATH/lib -lsubstrate"

# Objective-C++ flags
OBJCPP_FLAGS="-x objective-c++ $CXXFLAGS"

# Compile
echo "Building iOS dynamic library..."
clang++ $OBJCPP_FLAGS \
    $SOURCE_DIR/MobileMenuExample.cpp \
    $FRAMEWORKS $LIBS \
    -o $OUTPUT

# Sign the library (required for iOS)
echo "Code signing..."
codesign -s "-" $OUTPUT  # Ad-hoc signing for development

echo "Build complete: $OUTPUT"
echo ""
echo "Injection methods:"
echo "1. DYLD_INSERT_LIBRARIES=$OUTPUT ./YourGame.app/YourGame"
echo "2. Add to app bundle and modify Info.plist"
echo "3. Use injection tools like Frida"
```

**Makefile Alternative:**
```make
# Makefile for iOS
TARGET = MobileGUI.dylib
SDK = iphoneos
ARCH = arm64
MIN_VERSION = 12.0

CXX = clang++
CXXFLAGS = -std=c++17 -stdlib=libc++ -fPIC -shared -arch $(ARCH)
CXXFLAGS += -miphoneos-version-min=$(MIN_VERSION)
CXXFLAGS += -isysroot $(shell xcrun --sdk $(SDK) --show-sdk-path)
CXXFLAGS += -DPLATFORM_IOS=1 -DPLATFORM_MOBILE=1
CXXFLAGS += -I/path/to/substrate/include

LDFLAGS = -framework Foundation -framework UIKit
LDFLAGS += -L/path/to/substrate/lib -lsubstrate

SOURCES = source/MobileMenuExample.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(LDFLAGS) -o $@
	codesign -s "-" $@

%.o: %.cpp
	$(CXX) -x objective-c++ $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
```

### Android Shared Library Build (.so)

Build as a shared library for Android injection:

**Method 1: CMake (Recommended)**

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
    
    # Add Dobby hooking framework
    # Option 1: As subdirectory (if you have source)
    add_subdirectory(ThirdParty/Dobby)
    
    # Option 2: As prebuilt library
    # add_library(dobby STATIC IMPORTED)
    # set_target_properties(dobby PROPERTIES
    #     IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/Dobby/lib/${ANDROID_ABI}/libdobby.a
    # )
    
    include_directories(ThirdParty/Dobby/include)
    
    # Source files - NO UE4 headers needed!
    add_library(mobilegui SHARED
        source/MobileMenuExample.cpp
        # Add other source files here
    )
    
    # Link against Dobby and Android libraries
    target_link_libraries(mobilegui
        dobby
        log      # Android logging
        android  # Android native APIs
    )
    
    # Output location
    set_target_properties(mobilegui PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/libs/${ANDROID_ABI}
    )
endif()
```

**Build script:**
```bash
#!/bin/bash
# build_android.sh - Build MobileGUI for Android

NDK_PATH="/path/to/android-ndk"
ABI="arm64-v8a"  # or "armeabi-v7a" for 32-bit
API_LEVEL="21"
BUILD_DIR="build_android"

mkdir -p $BUILD_DIR
cd $BUILD_DIR

cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=$NDK_PATH/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=$ABI \
    -DANDROID_PLATFORM=android-$API_LEVEL \
    -DCMAKE_BUILD_TYPE=Release

make -j8

echo "Build complete:"
echo "Output: libs/$ABI/libmobilegui.so"
```

**Method 2: ndk-build (Android.mk)**

```make
# Android.mk
LOCAL_PATH := $(call my-dir)

# Dobby hooking library
include $(CLEAR_VARS)
LOCAL_MODULE := dobby
LOCAL_SRC_FILES := ThirdParty/Dobby/lib/$(TARGET_ARCH_ABI)/libdobby.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/ThirdParty/Dobby/include
include $(PREBUILT_STATIC_LIBRARY)

# MobileGUI library
include $(CLEAR_VARS)
LOCAL_MODULE := mobilegui
LOCAL_SRC_FILES := source/MobileMenuExample.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)/source
LOCAL_CPPFLAGS := -std=c++17 -DPLATFORM_ANDROID=1 -DPLATFORM_MOBILE=1
LOCAL_STATIC_LIBRARIES := dobby
LOCAL_LDLIBS := -llog -landroid
include $(BUILD_SHARED_LIBRARY)
```

```make
# Application.mk
APP_ABI := arm64-v8a armeabi-v7a
APP_PLATFORM := android-21
APP_STL := c++_shared
APP_CPPFLAGS := -std=c++17
```

**Build:**
```bash
${ANDROID_NDK}/ndk-build
# Output: libs/arm64-v8a/libmobilegui.so
```
    
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
