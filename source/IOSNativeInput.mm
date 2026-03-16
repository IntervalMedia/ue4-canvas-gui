// IOSNativeInput.mm
// Native iOS Objective-C touch input handler for UE4 Canvas GUI.
//
// Hooks UIViewController's four native UIKit touch-delivery methods so that
// touch data is fed directly into ZeroGUI::Input without requiring any
// modification to UE4's PlayerController or its InputComponent bindings.
//
// Inspired by the Apple-Metal example in Dear ImGui:
//   https://github.com/ocornut/imgui/blob/8957b3df03b4cbe502688208af7d2fda52be985f/examples/example_apple_metal/main.mm
//   (commit 8957b3df, 2023)
//
// Requirements:
//   - Jailbroken iOS device with CydiaSubstrate installed.
//   - substrate.h in your include path; libsubstrate.dylib linked.

#include "PlatformDefines.h"

#if PLATFORM_IOS

#import <UIKit/UIKit.h>
#import <objc/runtime.h>
#include <substrate.h>
#include "IOSNativeInput.h"
#include "ZeroInputMobile.h"

// ---------------------------------------------------------------------------
// Stable per-finger slot tracking
// ---------------------------------------------------------------------------
// allTouches returns an unordered NSSet, so iterating it directly would
// assign different slot indices to the same physical finger across frames.
// We maintain a fixed-size table of UITouch* → slot index so that each
// finger keeps the same slot for its entire lifetime (began → ended/cancelled).

static const int MAX_TOUCH_SLOTS = 10;
static UITouch* s_touchSlots[MAX_TOUCH_SLOTS];  // nullptr = empty slot

static void TouchSlots_Init()
{
    for (int i = 0; i < MAX_TOUCH_SLOTS; ++i)
        s_touchSlots[i] = nullptr;
}

// Returns the slot already assigned to this touch, or -1 if not found.
static int TouchSlots_Find(UITouch* touch)
{
    for (int i = 0; i < MAX_TOUCH_SLOTS; ++i)
        if (s_touchSlots[i] == touch)
            return i;
    return -1;
}

// Assigns the first empty slot to this touch and returns its index.
// Returns -1 if all slots are occupied.
static int TouchSlots_Assign(UITouch* touch)
{
    for (int i = 0; i < MAX_TOUCH_SLOTS; ++i)
    {
        if (s_touchSlots[i] == nullptr)
        {
            s_touchSlots[i] = touch;
            return i;
        }
    }
    return -1;
}

// Frees the slot held by this touch.
static void TouchSlots_Release(UITouch* touch)
{
    for (int i = 0; i < MAX_TOUCH_SLOTS; ++i)
        if (s_touchSlots[i] == touch)
            s_touchSlots[i] = nullptr;
}

// ---------------------------------------------------------------------------
// Internal helper
// ---------------------------------------------------------------------------

// Translate a UIEvent's touch set into ZeroGUI per-slot touch state.
//
// Coordinate system note: `locationInView:view` returns UIKit points in the
// coordinate space of `view`.  When UE4's game view occupies the full screen
// these coordinates correspond directly to UE4 screen-space pixels (assuming
// no view transforms and no safe-area offset).  If the game viewport is
// inset, callers should pass the innermost game view rather than the root
// view controller's view.
static void ZeroGUI_UpdateFromUIEvent(UIEvent* event, UIView* view)
{
    if (!event || !view)
        return;

    for (UITouch* touch in event.allTouches)
    {
        bool ended = (touch.phase == UITouchPhaseEnded ||
                      touch.phase == UITouchPhaseCancelled);

        int slot = TouchSlots_Find(touch);
        if (slot < 0)
        {
            if (ended)
                continue;  // touch ended before we tracked it – ignore

            slot = TouchSlots_Assign(touch);
            if (slot < 0)
                continue;  // all slots occupied
        }

        CGPoint loc = [touch locationInView:view];
        ZeroGUI::Input::UpdateTouchState(slot,
                                         FVector2D(loc.x, loc.y),
                                         !ended);

        if (ended)
            TouchSlots_Release(touch);
    }
}

// ---------------------------------------------------------------------------
// Hooked UIViewController touch methods
// ---------------------------------------------------------------------------

static IMP orig_touchesBegan     = nullptr;
static IMP orig_touchesMoved     = nullptr;
static IMP orig_touchesCancelled = nullptr;
static IMP orig_touchesEnded     = nullptr;

static void hook_touchesBegan(UIViewController* self, SEL _cmd,
                               NSSet<UITouch*>* touches, UIEvent* event)
{
    ZeroGUI_UpdateFromUIEvent(event, self.view);
    if (orig_touchesBegan)
        ((void (*)(id, SEL, NSSet*, UIEvent*))orig_touchesBegan)(self, _cmd, touches, event);
}

static void hook_touchesMoved(UIViewController* self, SEL _cmd,
                               NSSet<UITouch*>* touches, UIEvent* event)
{
    ZeroGUI_UpdateFromUIEvent(event, self.view);
    if (orig_touchesMoved)
        ((void (*)(id, SEL, NSSet*, UIEvent*))orig_touchesMoved)(self, _cmd, touches, event);
}

static void hook_touchesCancelled(UIViewController* self, SEL _cmd,
                                   NSSet<UITouch*>* touches, UIEvent* event)
{
    // On cancel, process the event then clear all slots to avoid stuck touches.
    ZeroGUI_UpdateFromUIEvent(event, self.view);
    ZeroGUI::Input::ClearTouchStates();
    TouchSlots_Init();

    if (orig_touchesCancelled)
        ((void (*)(id, SEL, NSSet*, UIEvent*))orig_touchesCancelled)(self, _cmd, touches, event);
}

static void hook_touchesEnded(UIViewController* self, SEL _cmd,
                               NSSet<UITouch*>* touches, UIEvent* event)
{
    ZeroGUI_UpdateFromUIEvent(event, self.view);
    if (orig_touchesEnded)
        ((void (*)(id, SEL, NSSet*, UIEvent*))orig_touchesEnded)(self, _cmd, touches, event);
}

// ---------------------------------------------------------------------------
// Hook installation helper
// ---------------------------------------------------------------------------

// Installs all four touch hooks on `targetClass`.
// This function is designed to be called exactly once during startup from the
// main thread; the `s_installed` guard in InstallTouchHooks() enforces that.
// Each MSHookMessageEx call writes the original IMP into the corresponding
// `orig_*` static; calling this function a second time would overwrite those
// pointers, so it must not be called more than once.
static void HookTouchMethods(Class targetClass)
{
    MSHookMessageEx(targetClass,
                    @selector(touchesBegan:withEvent:),
                    (IMP)hook_touchesBegan,
                    &orig_touchesBegan);

    MSHookMessageEx(targetClass,
                    @selector(touchesMoved:withEvent:),
                    (IMP)hook_touchesMoved,
                    &orig_touchesMoved);

    MSHookMessageEx(targetClass,
                    @selector(touchesCancelled:withEvent:),
                    (IMP)hook_touchesCancelled,
                    &orig_touchesCancelled);

    MSHookMessageEx(targetClass,
                    @selector(touchesEnded:withEvent:),
                    (IMP)hook_touchesEnded,
                    &orig_touchesEnded);
}

// ---------------------------------------------------------------------------
// IOSNativeInput namespace implementation
// ---------------------------------------------------------------------------

namespace IOSNativeInput
{
    static bool s_installed = false;

    void InstallTouchHooks()
    {
        if (s_installed)
            return;

        TouchSlots_Init();

        // Attempt to hook the UE4-specific view controller subclass first.
        // UE4 for iOS uses a class named "IOSViewController"; hooking it
        // directly ensures the hooks fire even if the subclass overrides
        // touch methods without calling [super ...].
        // If the class is not found we fall back to the UIViewController base.
        //
        // NOTE: HookTouchMethods must be called only once (it stores the
        // original IMPs in module-level statics).  The s_installed guard
        // above and the expectation that InstallTouchHooks() is called from
        // the main thread at startup together ensure this.
        Class targetClass = NSClassFromString(@"IOSViewController");
        if (!targetClass)
            targetClass = [UIViewController class];

        HookTouchMethods(targetClass);

        s_installed = true;
    }

    bool IsInstalled()
    {
        return s_installed;
    }
}

#endif // PLATFORM_IOS
