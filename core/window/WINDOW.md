WindowManager (window.cpp / window.hpp)
=======================================

This document focuses only on the `WindowManager` implementation found in `core/window/window.cpp` and its header `window.hpp`.

It documents the public API, how the code expects to be used (threading / message loop), raw input behavior, and common pitfalls specific to this file.

Overview
--------

`WindowManager` is a small helper that:

- Creates a Win32 top-level window (class name `ASCIILATOR_TextWindowClass`).
- Hosts a standard Win32 `EDIT` control for simple text output.
- Registers for RAWINPUT mouse events to accumulate mouse delta values.
- Provides a tiny API to print/clear text in the edit control and to read/reset the collected mouse delta.

Public API (what you call)
--------------------------

Functions in `window.hpp` (short signatures and behavior):

- void InitializeRawInput();
   - Ensures the window exists and registers for RAWINPUT mouse events (RIDEV_INPUTSINK).

- void GetDeltaPosition(unsigned long *x, unsigned long *y);
   - Reads accumulated mouse delta (signed values stored into unsigned args) and resets internal deltas to zero.
   - NOTE: this clears the internal counters.

- void SetDeltaPosition(unsigned long x, unsigned long y);
   - Overwrites the internal delta counters.

- bool InitiatelizeWindow();
   - Creates the window if needed and returns true when a valid HWND exists.

- void SetWindowTitle(const char* title);
   - UTF-8 -> UTF-16 conversion helper used to update the window title.

- void SetWindowSize(int width, int height);
   - Updates desired client size and resizes the window accordingly.

- bool CreateTextWindow();
   - Ensures the `EDIT` child control is created and visible.

- void PrintToWindow(const char* format, ...);
   - Writes formatted text to the edit control.
   - Current implementation REPLACES the internal text buffer with the formatted string (no append).

- void ClearWindow();
   - Clears internal text buffer and resets the edit control text to empty.

- void MoveCursorInWindow(int x, int y);
   - Minimal caret movement helper: sets focus and moves selection to the end. Not an exact (x,y) mapping.

- void CloseWindow();
   - Destroys the window and releases handles.

- Accessors: HWND GetWindowHandle(), bool ShouldClose(), void SetShouldClose(bool)

Important implementation details
--------------------------------

- Window class registration: `RegisterClassIfNeeded()` registers `kWndClassName` with a `StaticWndProc` that forwards to the instance `WndProc` using GWLP_USERDATA. This allows multiple WindowManager instances if desired.

- Message handling: `WndProc` handles WM_SIZE (keeps edit control sized), WM_INPUT (reads RAWINPUT, accumulates mouse deltas), WM_KEYDOWN (ESC sets ShouldClose + posts quit), WM_CLOSE/WM_DESTROY (set should-close and post quit once).

- RAWINPUT: `RegisterRawInput()` registers a mouse RAWINPUT device with `RIDEV_INPUTSINK` so the window receives raw mouse deltas even when unfocused. `WndProc` reads the RAWINPUT buffer using `GetRawInputData` and accumulates `m_deltaX`/`m_deltaY`.

Threading & message loop
------------------------

- The `WindowManager` is designed to be used from a thread that runs a Win32 message loop. That means creating the window and then running `PeekMessage`/`TranslateMessage`/`DispatchMessage` in a loop.

- Example (window thread):

```
WindowManager window;
window.SetWindowSize(600, 400);
if (!window.InitiatelizeWindow() || !window.CreateTextWindow()) return;
window.SetWindowTitle("ASCIILATOR Text Window");
window.InitializeRawInput();

MSG msg;
while (!shouldExit && !window.ShouldClose()) {
   while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }

   // Example periodic work: read mouse deltas
   unsigned long dx=0, dy=0;
   window.GetDeltaPosition(&dx, &dy);
   if (dx || dy) {
      window.ClearWindow();
      window.PrintToWindow("Mouse delta: %ld, %ld\r\n", (long)dx, (long)dy);
   }

   Sleep(10);
}
```

Raw input notes and caveats
--------------------------

- `WndProc` uses `GetRawInputData` twice: first to get required buffer size, then to retrieve the payload. The implementation allocates a `BYTE*` buffer with `new[]` and deletes it after use.

- RAWINPUT mice deliver relative deltas; the `m_deltaX`/`m_deltaY` fields are LONG and are accumulated across events until `GetDeltaPosition()` resets them.

- If you plan to call `GetDeltaPosition()` from a different thread than the window thread, you must synchronize access (the code is not internally thread-safe).

Text handling behavior
---------------------

- `PrintToWindow()` currently replaces the entire internal `m_textBuffer` with the new formatted string. It then updates the edit control text and scrolls to caret. This avoids unbounded growth but means you don't get an appended log.

- If you want an appending log, implement a small circular buffer of lines (N lines) and update the edit control from that buffer.

Common pitfalls & troubleshooting
--------------------------------

- If `RegisterRawInputDevices` fails, check permissions and make sure the HWND is valid before calling `InitializeRawInput()`.

- ESC handling posts WM_QUIT once by using `m_quitPosted` to avoid multiple `PostQuitMessage` calls. If your architecture posts additional quits elsewhere, you may see the app exit earlier than expected.

- Be careful with `SetWindowTextW` and UTF-8 input: `ToWide()` assumes UTF-8 input for `SetWindowTitle` and `PrintToWindow` formatting.

- `CloseWindow()` calls `DestroyWindow`. The application that owns the window thread must still pump messages until WM_DESTROY/WM_QUIT are processed to allow graceful cleanup.

Suggested improvements
----------------------

- Add a small mutex or critical section if you want `GetDeltaPosition()` to be safe across threads.
- Replace `new[]/delete[]` in `WndProc` with a stack allocation for small sizes or use a single reusable buffer to reduce churn.
- Implement a rolling N-line buffer for `PrintToWindow()` when you need appending behavior.

File references
---------------

- Implementation: `core/window/window.cpp`
- Header: `core/window/window.hpp`

If you want, I can also add a small unit test or example program that runs only the `WindowManager` in isolation so you can iterate faster on UI behavior.

---

After building, run `engine.exe` from the repository root (double-click or from PowerShell):

- The console thread prints status to the console window
- The window thread opens a small text window labeled "ASCIILATOR Text Window"
- Press `ESC` in either the console or the window to trigger shutdown

Threading and shutdown behavior
-------------------------------

- `g_shouldExit` is the shared volatile flag both worker threads check frequently.
- When any thread sets `g_shouldExit = true`, other threads will notice and start shutdown.
- The main thread waits up to 3 seconds for both threads to exit gracefully, then force-terminates if necessary.

Performance / tuning
--------------------

- `ConsoleThread` uses a 60 Hz clock to remain very responsive to input.
- `WindowThread` uses a low-frequency update (5 Hz) to avoid wasting CPU on UI updates. Increase `windowClock` frequency in `main.cpp` if you need smoother updates.
- The main thread only coordinates; avoid putting heavy work there.

Troubleshooting
---------------

- If the window doesn't show, ensure `RegisterClassExW` succeeds and you are not running in an environment that blocks GUI windows (services).
- If ESC doesn't exit immediately, make sure the console window has focus or press ESC in the window itself. The `g_shouldExit` flag is polled frequently.
- If the app hangs on exit, check for long-running blocking calls in any of the threads. The main thread will wait 3 seconds before force-terminating.

Code style & notes
------------------

- The project prefers small, focused C++ source files with clear section separators (see `core/input/input.cpp` for style inspiration).
- All Win32 APIs are used directly (no external GUI frameworks).
- The `WindowManager` uses a simple `EDIT` control for text output to keep UI code minimal.

Contributing
------------

If you want to help:
- Add unit tests for `clock`/`input` helpers
- Replace the `EDIT` control with a custom drawing surface for better performance
- Add proper thread synchronization primitives (condition variables or events) if you need complex inter-thread messaging

License
-------

This repository is provided as-is. Add your preferred license file if you plan to share this publicly.
# Further development of my CMD Game Engine using WINAPI and CPP

- reformating code