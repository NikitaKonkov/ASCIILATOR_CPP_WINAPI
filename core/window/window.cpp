// Implementation of WindowManager - small text window with raw input mouse delta collection

#include "window.hpp"
#include <windowsx.h>
#include <cstdio>
#include <cstdarg>

#ifndef IDC_STATIC
#define IDC_STATIC -1
#endif

static const wchar_t* kWndClassName = L"ASCIILATOR_TextWindowClass";

////////////////////// Helper: convert narrow to wide
static std::wstring ToWide(const char* s) {
	if (!s) return L"";
	int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
	if (len <= 0) return L"";
	std::wstring w(len, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s, -1, &w[0], len);
	w.resize(len - 1); // drop the null terminator from the logical size
	return w;
}

////////////////////// Window class registration
void WindowManager::RegisterClassIfNeeded() {
	if (m_classRegistered) return;
	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = &WindowManager::StaticWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = sizeof(LONG_PTR);
	wc.hInstance = GetModuleHandleW(nullptr);
	wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = kWndClassName;
	wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
	if (RegisterClassExW(&wc)) {
		m_classRegistered = true;
	} else {
		// If already registered, continue
		m_classRegistered = (GetLastError() == ERROR_CLASS_ALREADY_EXISTS);
	}
}

////////////////////// Static window procedure (forward to instance)
LRESULT CALLBACK WindowManager::StaticWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	WindowManager* self = reinterpret_cast<WindowManager*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	if (msg == WM_NCCREATE) {
		CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
		self = reinterpret_cast<WindowManager*>(cs->lpCreateParams);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)self);
	}
	if (self) {
		return self->WndProc(hWnd, msg, wParam, lParam);
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

////////////////////// Raw input registration
void WindowManager::RegisterRawInput() {
	if (m_rawInputInitialized) return;
	RAWINPUTDEVICE rid{};
	rid.usUsagePage = 0x01; // Generic desktop controls
	rid.usUsage = 0x02;     // Mouse
	rid.dwFlags = RIDEV_INPUTSINK; // receive input even when not focused
	rid.hwndTarget = m_hWnd;
	if (RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
		m_rawInputInitialized = true;
	}
}

////////////////////// Edit control helper
void WindowManager::EnsureEditControl() {
	if (m_hEdit) return;
	m_hEdit = CreateWindowExW(
		WS_EX_CLIENTEDGE,
		L"EDIT",
		L"",
		WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VSCROLL | WS_HSCROLL,
		0, 0, m_width, m_height,
		m_hWnd,
		(HMENU)1,
		GetModuleHandleW(nullptr),
		nullptr);
}

////////////////////// Create or resize window
void WindowManager::CreateOrResizeWindow() {
	if (!m_hWnd) {
		RegisterClassIfNeeded();
		DWORD style = WS_OVERLAPPEDWINDOW;
		RECT rc{ 0, 0, m_width, m_height };
		AdjustWindowRect(&rc, style, FALSE);
		m_hWnd = CreateWindowExW(
			0,
			kWndClassName,
			L"ASCIILATOR",
			style,
			CW_USEDEFAULT, CW_USEDEFAULT,
			rc.right - rc.left,
			rc.bottom - rc.top,
			nullptr, nullptr, GetModuleHandleW(nullptr), this);
		if (m_hWnd) {
			ShowWindow(m_hWnd, SW_SHOW);
			UpdateWindow(m_hWnd);
			EnsureEditControl();
			RegisterRawInput();
		}
	} else {
		RECT rc{ 0, 0, m_width, m_height };
		DWORD style = (DWORD)GetWindowLongPtr(m_hWnd, GWL_STYLE);
		AdjustWindowRect(&rc, style, FALSE);
		SetWindowPos(m_hWnd, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER);
		if (m_hEdit) {
			MoveWindow(m_hEdit, 0, 0, m_width, m_height, TRUE);
		}
	}
}

////////////////////// Instance window procedure
LRESULT WindowManager::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_SIZE:
		if (m_hEdit) {
			int w = LOWORD(lParam);
			int h = HIWORD(lParam);
			MoveWindow(m_hEdit, 0, 0, w, h, TRUE);
		}
		break;

	case WM_KEYDOWN:
		// Handle ESC key more responsively
		if (wParam == VK_ESCAPE) {
			m_shouldClose = true;
			if (!m_quitPosted) {
				PostQuitMessage(0);
				m_quitPosted = true;
			}
			return 0;
		}
		break;

	case WM_INPUT:
		{
			UINT dwSize = 0;
			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));
			if (dwSize) {
				BYTE* lpb = new BYTE[dwSize];
				if (lpb) {
					if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) == dwSize) {
						RAWINPUT* raw = (RAWINPUT*)lpb;
						if (raw->header.dwType == RIM_TYPEMOUSE) {
							m_deltaX += raw->data.mouse.lLastX;
							m_deltaY += raw->data.mouse.lLastY;
						}
					}
					delete[] lpb;
				}
			}
		}
		break;

	case WM_CLOSE:
		// Handle window close button
		m_shouldClose = true;
		if (!m_quitPosted) {
			PostQuitMessage(0);
			m_quitPosted = true;
		}
		return 0;

	case WM_DESTROY:
		m_hEdit = nullptr;
		m_hWnd = nullptr;
		m_shouldClose = true;
		if (!m_quitPosted) {
			PostQuitMessage(0);
			m_quitPosted = true;
		}
		break;

	default:
		break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

////////////////////// Public API
void WindowManager::InitializeRawInput() {
	if (!m_hWnd) {
		// ensure window exists to receive inputsink
		CreateOrResizeWindow();
	}
	RegisterRawInput();
}

void WindowManager::GetDeltaPosition(unsigned long* x, unsigned long* y) {
	if (x) *x = (unsigned long)m_deltaX;
	if (y) *y = (unsigned long)m_deltaY;
	m_deltaX = 0;
	m_deltaY = 0;
}

void WindowManager::SetDeltaPosition(unsigned long x, unsigned long y) {
	m_deltaX = (LONG)x;
	m_deltaY = (LONG)y;
}

bool WindowManager::InitiatelizeWindow() {
	CreateOrResizeWindow();
	return m_hWnd != nullptr;
}

void WindowManager::SetWindowTitle(const char* title) {
	if (!m_hWnd) return;
	std::wstring w = ToWide(title);
	SetWindowTextW(m_hWnd, w.c_str());
}

void WindowManager::SetWindowSize(int width, int height) {
	m_width = width;
	m_height = height;
	CreateOrResizeWindow();
}

bool WindowManager::CreateTextWindow() {
	if (!m_hWnd) CreateOrResizeWindow();
	EnsureEditControl();
	return m_hWnd != nullptr && m_hEdit != nullptr;
}

void WindowManager::PrintToWindow(const char* format, ...) {
	char buf[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);

	// Instead of appending to buffer, replace it to prevent excessive accumulation
	m_textBuffer = buf;

	if (m_hEdit) {
		std::wstring w = ToWide(m_textBuffer.c_str());
		SetWindowTextW(m_hEdit, w.c_str());
		// Scroll to bottom
		SendMessageW(m_hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
		SendMessageW(m_hEdit, EM_SCROLLCARET, 0, 0);
	}
}

void WindowManager::ClearWindow() {
	m_textBuffer.clear();
	if (m_hEdit) {
		SetWindowTextW(m_hEdit, L"");
	}
}

void WindowManager::MoveCursorInWindow(int x, int y) {
	if (m_hEdit) {
		// Move caret roughly to the character position: convert (x,y) pixels to char index is non-trivial.
		// Simpler: set selection to end then send Home/Up counts is heavy; we keep it minimal by setting focus.
		SetFocus(m_hEdit);
		// Optionally set caret position to end
		SendMessageW(m_hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
	}
}

void WindowManager::CloseWindow() {
	if (m_hWnd) {
		DestroyWindow(m_hWnd);
		m_hWnd = nullptr;
		m_hEdit = nullptr;
	}
}

