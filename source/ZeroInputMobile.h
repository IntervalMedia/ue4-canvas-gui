#pragma once
#include "PlatformDefines.h"

// Mobile-specific touch input handling for UE4 Canvas GUI
// Supports Android and iOS touch input

namespace ZeroGUI
{
	namespace Input
	{
		// Touch state tracking (support up to 10 simultaneous touch points)
		bool touchDown[10];
		// Element ID tracking - allows tracking state for up to 256 different UI elements
		// This is separate from touch count as multiple elements may be interacted with
		// across different touches/frames
		bool touchDownAlready[256];
		FVector2D touchPositions[10];
		int activeTouchCount = 0;

		// Key state tracking (for virtual buttons/gestures)
		bool keysDown[256];
		bool keysDownAlready[256];

		bool IsAnyTouchDown()
		{
			for (int i = 0; i < 10; i++)
			{
				if (touchDown[i]) return true;
			}
			return false;
		}

		// Touch equivalent of mouse click
		bool IsTouchClicked(int touchIndex, int element_id, bool repeat)
		{
			if (touchIndex >= 10) return false;

			if (touchDown[touchIndex])
			{
				if (!touchDownAlready[element_id])
				{
					touchDownAlready[element_id] = true;
					return true;
				}
				if (repeat)
					return true;
			}
			else
			{
				touchDownAlready[element_id] = false;
			}
			return false;
		}

		// Get primary touch position (first active touch)
		FVector2D GetPrimaryTouchPosition()
		{
			for (int i = 0; i < 10; i++)
			{
				if (touchDown[i])
					return touchPositions[i];
			}
			return FVector2D(0, 0);
		}

		// Check if touch is active
		bool IsTouchActive(int touchIndex)
		{
			if (touchIndex >= 10) return false;
			return touchDown[touchIndex];
		}

		// Virtual key press handling (for gesture-based actions)
		bool IsKeyPressed(int key, bool repeat)
		{
			if (keysDown[key])
			{
				if (!keysDownAlready[key])
				{
					keysDownAlready[key] = true;
					return true;
				}
				if (repeat)
					return true;
			}
			else
			{
				keysDownAlready[key] = false;
			}
			return false;
		}

		// Main input handler - to be called from game code with touch data
		// This should be called with touch events from UE4's touch input system
		void Handle()
		{
			// This is a placeholder - actual implementation should receive touch data
			// from UE4's player controller touch events
			// Example integration:
			// APlayerController* PC = GetPlayerController();
			// PC->GetInputTouchState(ETouchIndex::Touch1, x, y, isDown);
		}

		// Update touch state - called from game code
		void UpdateTouchState(int touchIndex, FVector2D position, bool isDown)
		{
			if (touchIndex >= 10) return;

			touchDown[touchIndex] = isDown;
			touchPositions[touchIndex] = position;

			// Update active touch count
			activeTouchCount = 0;
			for (int i = 0; i < 10; i++)
			{
				if (touchDown[i]) activeTouchCount++;
			}
		}

		// Clear all touch states
		void ClearTouchStates()
		{
			for (int i = 0; i < 10; i++)
			{
				touchDown[i] = false;
				touchPositions[i] = FVector2D(0, 0);
			}
			activeTouchCount = 0;
		}

		// Virtual button/gesture key setting (for menu toggling, etc.)
		void SetVirtualKey(int keyCode, bool pressed)
		{
			if (keyCode >= 0 && keyCode < 256)
			{
				keysDown[keyCode] = pressed;
			}
		}

		// Gesture recognition helpers
		namespace Gestures
		{
			const int GESTURE_TAP = 0x01;
			const int GESTURE_DOUBLE_TAP = 0x02;
			const int GESTURE_LONG_PRESS = 0x03;
			const int GESTURE_SWIPE_LEFT = 0x04;
			const int GESTURE_SWIPE_RIGHT = 0x05;
			const int GESTURE_SWIPE_UP = 0x06;
			const int GESTURE_SWIPE_DOWN = 0x07;
			const int GESTURE_PINCH = 0x08;
			const int GESTURE_ZOOM = 0x09;

			// Simple gesture state tracking
			FVector2D gestureStartPos;
			float gestureStartTime = 0.0f;
			bool gestureInProgress = false;

			// Menu toggle gesture (can be customized)
			const int MENU_TOGGLE_GESTURE = GESTURE_DOUBLE_TAP;
		}
	}
}
