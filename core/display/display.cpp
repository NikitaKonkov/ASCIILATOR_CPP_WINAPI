#include "display.hpp"

////////////////////// Constructor - Initialize console handle and enable ANSI
DisplayManager::DisplayManager() {
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    ansiEnabled = false;
    EnableANSI();
}

////////////////////// Destructor - Clean up resources
DisplayManager::~DisplayManager() {
    // Reset console to default state
    Print(COLOR_RESET);
    ShowCursor();
}

////////////////////// Print text without newline
void DisplayManager::Print(const char* text) {
    if (hConsole != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteConsoleA(hConsole, text, lstrlenA(text), &written, NULL);
    }
}

////////////////////// Print text with newline
void DisplayManager::PrintLine(const char* text) {
    Print(text);
    Print("\n");
}

////////////////////// Print formatted text (like printf)
void DisplayManager::PrintFormatted(const char* format, ...) {
    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    Print(buffer);
}

////////////////////// Print colored text without newline
void DisplayManager::PrintColored(const char* color, const char* text) {
    if (ansiEnabled) {
        Print(color);
        Print(text);
        Print(COLOR_RESET);
    } else {
        Print(text);
    }
}

////////////////////// Print colored text with newline
void DisplayManager::PrintColoredLine(const char* color, const char* text) {
    PrintColored(color, text);
    Print("\n");
}

////////////////////// Print styled and colored text
void DisplayManager::PrintStyledText(const char* style, const char* color, const char* text) {
    if (ansiEnabled) {
        Print(style);
        Print(color);
        Print(text);
        Print(COLOR_RESET);
    } else {
        Print(text);
    }
}

////////////////////// Clear entire screen and move cursor to top-left
void DisplayManager::ClearScreen() {
    if (ansiEnabled) {
        Print("\033[2J\033[1;1H");
    } else {
        system("cls");
    }
}

////////////////////// Clear current line
void DisplayManager::ClearLine() {
    if (ansiEnabled) {
        Print("\033[2K");
    }
}

////////////////////// Move cursor to specific position (1-based)
void DisplayManager::MoveCursor(int row, int col) {
    if (ansiEnabled) {
        PrintFormatted("\033[%d;%dH", row, col);
    }
}

////////////////////// Move cursor up by specified lines
void DisplayManager::MoveCursorUp(int lines) {
    if (ansiEnabled) {
        PrintFormatted("\033[%dA", lines);
    }
}

////////////////////// Move cursor down by specified lines
void DisplayManager::MoveCursorDown(int lines) {
    if (ansiEnabled) {
        PrintFormatted("\033[%dB", lines);
    }
}

////////////////////// Move cursor left by specified characters
void DisplayManager::MoveCursorLeft(int chars) {
    if (ansiEnabled) {
        PrintFormatted("\033[%dD", chars);
    }
}

////////////////////// Move cursor right by specified characters
void DisplayManager::MoveCursorRight(int chars) {
    if (ansiEnabled) {
        PrintFormatted("\033[%dC", chars);
    }
}

////////////////////// Save current cursor position
void DisplayManager::SaveCursorPosition() {
    if (ansiEnabled) {
        Print("\033[s");
    }
}

////////////////////// Restore saved cursor position
void DisplayManager::RestoreCursorPosition() {
    if (ansiEnabled) {
        Print("\033[u");
    }
}

////////////////////// Hide cursor
void DisplayManager::HideCursor() {
    if (ansiEnabled) {
        Print("\033[?25l");
    }
}

////////////////////// Show cursor
void DisplayManager::ShowCursor() {
    if (ansiEnabled) {
        Print("\033[?25h");
    }
}

////////////////////// Enable ANSI escape code processing
void DisplayManager::EnableANSI() {
    if (hConsole != INVALID_HANDLE_VALUE) {
        DWORD consoleMode;
        if (GetConsoleMode(hConsole, &consoleMode)) {
            if (SetConsoleMode(hConsole, consoleMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
                ansiEnabled = true;
            }
        }
    }
}

////////////////////// Disable ANSI escape code processing
void DisplayManager::DisableANSI() {
    if (hConsole != INVALID_HANDLE_VALUE) {
        DWORD consoleMode;
        if (GetConsoleMode(hConsole, &consoleMode)) {
            if (SetConsoleMode(hConsole, consoleMode & ~ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
                ansiEnabled = false;
            }
        }
    }
}

////////////////////// Check if ANSI is enabled
bool DisplayManager::IsANSIEnabled() {
    return ansiEnabled;
}

////////////////////// Get console size in characters
void DisplayManager::GetConsoleSize(int* width, int* height) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        *width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        *height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    } else {
        *width = 80;  // Default fallback
        *height = 25; // Default fallback
    }
}

////////////////////// Set console window title
void DisplayManager::SetTitle(const char* title) {
    SetConsoleTitleA(title);
}

////////////////////// Draw a box at specified position
void DisplayManager::DrawBox(int x, int y, int width, int height, const char* color) {
    // Draw top border
    MoveCursor(y, x);
    PrintColored(color, "+");
    for (int i = 1; i < width - 1; i++) {
        Print("-");
    }
    Print("+");
    
    // Draw side borders
    for (int i = 1; i < height - 1; i++) {
        MoveCursor(y + i, x);
        PrintColored(color, "|");
        MoveCursor(y + i, x + width - 1);
        PrintColored(color, "|");
    }
    
    // Draw bottom border
    if (height > 1) {
        MoveCursor(y + height - 1, x);
        PrintColored(color, "+");
        for (int i = 1; i < width - 1; i++) {
            Print("-");
        }
        Print("+");
    }
}

////////////////////// Draw horizontal line
void DisplayManager::DrawHorizontalLine(int x, int y, int length, char character) {
    MoveCursor(y, x);
    for (int i = 0; i < length; i++) {
        PrintFormatted("%c", character);
    }
}

////////////////////// Draw vertical line
void DisplayManager::DrawVerticalLine(int x, int y, int length, char character) {
    for (int i = 0; i < length; i++) {
        MoveCursor(y + i, x);
        PrintFormatted("%c", character);
    }
}

////////////////////// Fill area with character and color
void DisplayManager::FillArea(int x, int y, int width, int height, char character, const char* color) {
    for (int row = 0; row < height; row++) {
        MoveCursor(y + row, x);
        PrintColored(color, "");
        for (int col = 0; col < width; col++) {
            PrintFormatted("%c", character);
        }
        Print(COLOR_RESET);
    }
}