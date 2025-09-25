#if !defined(WINDOW_HPP)
#define WINDOW_HPP

#include <windows.h>
#include <string>


// Window Manager Class
class WindowManager {
public:
    // Mouse Methods
    void InitializeRawInput();
    void GetDeltaPosition(unsigned long *x, unsigned long *y);
    void SetDeltaPosition(unsigned long x, unsigned long y);

    // Window Methods
    bool InitiatelizeWindow();

    void SetWindowTitle(const char* title);
    void SetWindowSize(int width, int height);
    
    bool CreateTextWindow();

    void PrintToWindow(const char* format, ...);
    void ClearWindow();

    void MoveCursorInWindow(int x, int y);
    void CloseWindow();

    // High-level window thread management
    bool SetupWindow(int width, int height, const char* title);
    void ProcessWindowMessages();
    void UpdateMouseDelta();
    void PrintHeartbeat();
    // RunWindowThread methods removed - functionality moved to main.cpp WindowThreadProc

    // Accessors
    HWND GetWindowHandle() const { return m_hWnd; }
    bool ShouldClose() const { return m_shouldClose; }
    void SetShouldClose(bool shouldClose) { m_shouldClose = shouldClose; }

private:
    // Window internals
    static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void RegisterClassIfNeeded();
    void EnsureEditControl();
    void RegisterRawInput();

    void CreateOrResizeWindow();

private:
    HWND m_hWnd = nullptr;
    HWND m_hEdit = nullptr;
    std::string m_textBuffer;
    LONG m_deltaX = 0;
    LONG m_deltaY = 0;
    int m_width = 400;
    int m_height = 300;
    bool m_classRegistered = false;
    bool m_rawInputInitialized = false;
    bool m_shouldClose = false;
    bool m_quitPosted = false;
};


#endif // WINDOW_HPP
