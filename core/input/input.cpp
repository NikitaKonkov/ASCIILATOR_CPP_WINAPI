#include "input.hpp"
#include <windows.h>
#include <stdio.h>

// Raw Input Mouse Delta Implementation
LRESULT CALLBACK InputManager::HiddenWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INPUT: {
            RAWINPUT raw;
            UINT dwSize = sizeof(RAWINPUT);

            if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw, &dwSize, sizeof(RAWINPUTHEADER)) == (UINT)-1) {
                DWORD error = GetLastError();
                printf("Failed to get raw input data. Error code: %lu\n", error);
                return 0;
            }

            if (raw.header.dwType == RIM_TYPEMOUSE && (raw.data.mouse.lLastX != 0 || raw.data.mouse.lLastY != 0)) {
                LONG deltaX = raw.data.mouse.lLastX;
                LONG deltaY = raw.data.mouse.lLastY;
                printf("Mouse Delta: X = %ld, Y = %ld\n", deltaX, deltaY);
            }
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

void InputManager::InitializeRawInput() {
    // Register window class
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = HiddenWindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"RawInputWindow";

    if (!RegisterClassEx(&wc)) {
        printf("Failed to register window class. Error: %lu\n", GetLastError());
        return;
    }

    // Create hidden window
    hiddenWindow = CreateWindowEx(
        0,
        L"RawInputWindow",
        L"Raw Input Handler",
        0,
        0, 0, 0, 0,
        HWND_MESSAGE,  // Message-only window
        NULL,
        GetModuleHandle(NULL),
        NULL
    );

    if (!hiddenWindow) {
        printf("Failed to create hidden window. Error: %lu\n", GetLastError());
        return;
    }

    // Register raw input
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01; // Generic desktop controls
    rid.usUsage = 0x02;     // Mouse
    rid.dwFlags = RIDEV_INPUTSINK; // Receive input even when not in focus
    rid.hwndTarget = hiddenWindow; // Target our hidden window

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
        DWORD error = GetLastError();
        printf("Failed to register raw input devices. Error code: %lu\n", error);
    } else {
        printf("Raw input registered successfully with hidden window.\n");
    }
}

void InputManager::ProcessRawInput(LPARAM lParam) {
    RAWINPUT raw;
    UINT dwSize = sizeof(RAWINPUT);

    if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw, &dwSize, sizeof(RAWINPUTHEADER)) == (UINT)-1) {
        DWORD error = GetLastError();
        printf("Failed to get raw input data. Error code: %lu\n", error);
        return;
    }

    if (raw.header.dwType == RIM_TYPEMOUSE && (raw.data.mouse.lLastX != 0 || raw.data.mouse.lLastY != 0)) {
        LONG deltaX = raw.data.mouse.lLastX;
        LONG deltaY = raw.data.mouse.lLastY;
        printf("Mouse Delta: X = %ld, Y = %ld\n", deltaX, deltaY);
    }
}

////////////////////// Get the state of multiple keys; returns true if all specified keys are pressed
bool InputManager::GetPressedKeys(int count, ...) {
    va_list args;
    va_start(args, count);

    for (int i = 0; i < count; i++) {
        int key = va_arg(args, int);
        if (!(GetAsyncKeyState(key) & 0x8000)) {
            va_end(args);
            return false;
        }
    }
    va_end(args);
    return true;
}

////////////////////// Check if a key was pressed since the last call
bool InputManager::GetKeyLSB(int key) {
    SHORT keyState = GetAsyncKeyState(key);
    return (keyState & 0x0001) != 0;
}

////////////////////// Check if a key is currently pressed (most significant bit)
bool InputManager::GetKeyMSB(int key) {
    SHORT keyState = GetAsyncKeyState(key);
    return (keyState & 0x8000) != 0;
}

////////////////////// Print all currently pressed keys to the console
void InputManager::PrintPressedKeys() {
    printf("Pressed Keys: ");
    for (int key = 8; key <= 255; key++) {
        if (GetAsyncKeyState(key) & 0x8000) {
            printf("%c ", key);
        }
    }
    printf("\n");
}

////////////////////// Simulate key presses for specified virtual keys
void InputManager::PressVirtualKeys(int count, ...) {
    va_list args;
    va_start(args, count);

    INPUT inputs[256] = {0};
    int inputIndex = 0;

    for (int i = 0; i < count; i++) {
        int key = va_arg(args, int);
        inputs[inputIndex].type = INPUT_KEYBOARD;
        inputs[inputIndex].ki.wVk = (WORD)key;
        inputIndex++;
    }

    va_end(args);
    SendInput(inputIndex, inputs, sizeof(INPUT));
}

////////////////////// Get current mouse position
void InputManager::GetMousePosition(int *x, int *y) {
    POINT cursorPos;
    if (GetCursorPos(&cursorPos)) {
        *x = cursorPos.x;
        *y = cursorPos.y;
    } else {
        *x = -1;
        *y = -1;
    }
}

////////////////////// Print current mouse position to the console
void InputManager::PrintMousePosition() {
    POINT cursorPos;
    if (GetCursorPos(&cursorPos)) {
        printf("Mouse Position: X = %ld, Y = %ld\n", cursorPos.x, cursorPos.y);
    } else {
        printf("Unable to get mouse position.\n");
    }
}

////////////////////// Set mouse position to specified coordinates
void InputManager::SetMousePosition(int x, int y) {
    SetCursorPos(x, y);
}

////////////////////// Get state of mouse buttons (left, right, middle)
bool InputManager::GetMouseButtonState(int button) {
    return (GetAsyncKeyState(button) & 0x8000) != 0;
}

////////////////////// Print current mouse button states
void InputManager::PrintMouseButtons() {
    printf("Mouse Buttons: ");
    if (GetMouseButtonState(VK_LBUTTON)) printf("LEFT ");
    if (GetMouseButtonState(VK_RBUTTON)) printf("RIGHT ");
    if (GetMouseButtonState(VK_MBUTTON)) printf("MIDDLE ");
    printf("\n");
}

// Previous mouse position for movement detection
POINT InputManager::lastMousePos = {0, 0};
HWND InputManager::hiddenWindow = NULL;

////////////////////// Check if mouse moved since last call
bool InputManager::IsMouseMoved() {
    POINT currentPos;
    GetCursorPos(&currentPos);
    bool moved = (currentPos.x != lastMousePos.x || currentPos.y != lastMousePos.y);
    lastMousePos = currentPos;
    return moved;
}

