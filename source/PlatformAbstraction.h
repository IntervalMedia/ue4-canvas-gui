#pragma once
#include "PlatformDefines.h"

/*
 * Mobile Platform Abstraction Layer for ZeroGUI
 * Provides platform-independent cursor/touch position and input handling
 */

#if PLATFORM_MOBILE
    // Mobile platforms - use touch input
    #include "ZeroInputMobile.h"
    
    namespace ZeroGUI
    {
        namespace Platform
        {
            // Get current input position (touch or mouse)
            inline FVector2D GetInputPosition()
            {
                return Input::GetPrimaryTouchPosition();
            }
            
            // Check if primary input is active
            inline bool IsInputActive()
            {
                return Input::IsTouchActive(0);
            }
            
            // Check if input was clicked/tapped
            inline bool IsInputClicked(int element_id, bool repeat = false)
            {
                return Input::IsTouchClicked(0, element_id, repeat);
            }
            
            // Check if any input is active
            inline bool IsAnyInputActive()
            {
                return Input::IsAnyTouchDown();
            }
            
            // Virtual key check (for gestures on mobile)
            inline bool IsKeyDown(int key)
            {
                return Input::IsKeyPressed(key, false);
            }
            
            // Touch-friendly hit testing - larger hit areas
            const float TOUCH_HIT_SCALE = 1.2f; // 20% larger hit areas for touch
            
            inline bool MouseInZone(FVector2D pos, FVector2D size)
            {
                FVector2D inputPos = GetInputPosition();
                FVector2D scaledSize = size * TOUCH_HIT_SCALE;
                FVector2D offset = (scaledSize - size) * 0.5f;
                FVector2D adjustedPos = pos - offset;
                
                return inputPos.X > adjustedPos.X && inputPos.Y > adjustedPos.Y &&
                       inputPos.X < adjustedPos.X + scaledSize.X && 
                       inputPos.Y < adjustedPos.Y + scaledSize.Y;
            }
        }
    }

#else
    // Desktop platforms - use original Windows input
    #include <Windows.h>
    
    namespace ZeroGUI
    {
        namespace Platform
        {
            // Get current cursor position
            inline FVector2D GetInputPosition()
            {
                POINT cursorPos;
                GetCursorPos(&cursorPos);
                return FVector2D(static_cast<float>(cursorPos.x), static_cast<float>(cursorPos.y));
            }
            
            // Check if primary button is active
            inline bool IsInputActive()
            {
                return GetAsyncKeyState(VK_LBUTTON) != 0;
            }
            
            // Check if input was clicked (uses Input namespace)
            inline bool IsInputClicked(int element_id, bool repeat = false)
            {
                // This would use the Input::IsMouseClicked from ZeroInput.h
                return false; // Placeholder
            }
            
            // Check if any mouse button is down
            inline bool IsAnyInputActive()
            {
                return GetAsyncKeyState(VK_LBUTTON) || GetAsyncKeyState(VK_RBUTTON) || 
                       GetAsyncKeyState(VK_MBUTTON);
            }
            
            // Key check
            inline bool IsKeyDown(int key)
            {
                return GetAsyncKeyState(key) != 0;
            }
            
            // Normal hit testing for mouse
            const float TOUCH_HIT_SCALE = 1.0f; // No scaling for mouse
            
            inline bool MouseInZone(FVector2D pos, FVector2D size)
            {
                FVector2D inputPos = GetInputPosition();
                return inputPos.X > pos.X && inputPos.Y > pos.Y &&
                       inputPos.X < pos.X + size.X && inputPos.Y < pos.Y + size.Y;
            }
        }
    }
#endif

// Platform-independent UI scaling for different DPIs
namespace ZeroGUI
{
    namespace Mobile
    {
        // DPI scaling factor (should be set based on device)
        static float DPIScale = 1.0f;
        
        // Minimum touch target size (44x44 points is Apple's recommendation)
        const float MIN_TOUCH_TARGET = 44.0f;
        
        // Set DPI scale factor
        inline void SetDPIScale(float scale)
        {
            DPIScale = scale;
        }
        
        // Get scaled size
        inline FVector2D GetScaledSize(FVector2D size)
        {
            FVector2D scaled = size * DPIScale;
            
            #if PLATFORM_MOBILE
            // Ensure minimum touch target size
            if (scaled.X < MIN_TOUCH_TARGET) scaled.X = MIN_TOUCH_TARGET;
            if (scaled.Y < MIN_TOUCH_TARGET) scaled.Y = MIN_TOUCH_TARGET;
            #endif
            
            return scaled;
        }
        
        // Get scaled position
        inline FVector2D GetScaledPosition(FVector2D pos)
        {
            return pos * DPIScale;
        }
    }
}
