# Display System – Console Output with ANSI Color & Cursor Control

## Overview

The display system provides:
- ANSI color-coded console output with comprehensive color support
- Advanced cursor positioning and movement control
- Text styling (bold, italic, underline, etc.)
- ASCII art drawing capabilities (boxes, lines, areas)
- Console window management and configuration
- Clean separation: display system handles only output formatting; main program handles content logic
- Cross-platform ANSI support with automatic fallback for unsupported terminals

---

## Core API

### Basic Display Functions

```cpp
// Print text without newline
void Print(const char* text);

// Print text with newline
void PrintLine(const char* text);

// Print formatted text (printf-style)
void PrintFormatted(const char* format, ...);

// Print colored text without/with newline
void PrintColored(const char* color, const char* text);
void PrintColoredLine(const char* color, const char* text);

// Print text with both style and color
void PrintStyledText(const char* style, const char* color, const char* text);
```

### Cursor Control Functions

```cpp
// Screen and line clearing
void ClearScreen();
void ClearLine();

// Cursor positioning (1-based coordinates)
void MoveCursor(int row, int col);

// Relative cursor movement
void MoveCursorUp(int lines);
void MoveCursorDown(int lines);
void MoveCursorLeft(int chars);
void MoveCursorRight(int chars);

// Cursor state management
void SaveCursorPosition();
void RestoreCursorPosition();
void HideCursor();
void ShowCursor();
```

### Advanced Drawing Functions

```cpp
// Draw ASCII boxes and lines
void DrawBox(int x, int y, int width, int height, const char* color = COLOR_WHITE);
void DrawHorizontalLine(int x, int y, int length, char character = '-');
void DrawVerticalLine(int x, int y, int length, char character = '|');

// Fill areas with characters and colors
void FillArea(int x, int y, int width, int height, char character = ' ', const char* color = COLOR_WHITE);
```

---

## Color & Style Codes

### Standard Colors
```cpp
#define COLOR_RESET      "\033[0m"
#define COLOR_BLACK      "\033[0;30m"
#define COLOR_RED        "\033[0;31m"
#define COLOR_GREEN      "\033[0;32m"
#define COLOR_YELLOW     "\033[0;33m"
#define COLOR_BLUE       "\033[0;34m"
#define COLOR_MAGENTA    "\033[0;35m"
#define COLOR_CYAN       "\033[0;36m"
#define COLOR_WHITE      "\033[0;37m"
```

### Bright Colors
```cpp
#define COLOR_BRIGHT_BLACK   "\033[1;30m"
#define COLOR_BRIGHT_RED     "\033[1;31m"
#define COLOR_BRIGHT_GREEN   "\033[1;32m"
#define COLOR_BRIGHT_YELLOW  "\033[1;33m"
#define COLOR_BRIGHT_BLUE    "\033[1;34m"
#define COLOR_BRIGHT_MAGENTA "\033[1;35m"
#define COLOR_BRIGHT_CYAN    "\033[1;36m"
#define COLOR_BRIGHT_WHITE   "\033[1;37m"
```

### Background Colors
```cpp
#define BG_BLACK         "\033[40m"
#define BG_RED           "\033[41m"
#define BG_GREEN         "\033[42m"
#define BG_YELLOW        "\033[43m"
#define BG_BLUE          "\033[44m"
#define BG_MAGENTA       "\033[45m"
#define BG_CYAN          "\033[46m"
#define BG_WHITE         "\033[47m"
```

### Text Styles
```cpp
#define STYLE_BOLD       "\033[1m"
#define STYLE_DIM        "\033[2m"
#define STYLE_ITALIC     "\033[3m"
#define STYLE_UNDERLINE  "\033[4m"
#define STYLE_BLINK      "\033[5m"
#define STYLE_REVERSE    "\033[7m"
#define STYLE_STRIKETHROUGH "\033[9m"
```

---

## Usage Examples

### Basic Text Output

```cpp
DisplayManager display;

// Simple text output
display.Print("Hello World!");
display.PrintLine("Hello with newline!");

// Formatted output
int score = 1250;
display.PrintFormatted("Player Score: %d\n", score);

// Colored output
display.PrintColoredLine(COLOR_BRIGHT_GREEN, "Success message!");
display.PrintColoredLine(COLOR_BRIGHT_RED, "Error message!");
display.PrintColoredLine(COLOR_BRIGHT_YELLOW, "Warning message!");
```

### Advanced Text Styling

```cpp
// Styled text with color
display.PrintStyledText(STYLE_BOLD, COLOR_BRIGHT_BLUE, "Important Notice");
display.PrintStyledText(STYLE_UNDERLINE, COLOR_CYAN, "Underlined text");
display.PrintStyledText(STYLE_ITALIC, COLOR_MAGENTA, "Italic text");

// Multiple styles (combine manually)
display.Print(STYLE_BOLD);
display.Print(STYLE_UNDERLINE);
display.PrintColored(COLOR_BRIGHT_RED, "Bold + Underlined Red Text");
display.Print(COLOR_RESET);
```

### Cursor Control & Positioning

```cpp
// Clear screen and position cursor
display.ClearScreen();
display.MoveCursor(10, 20);
display.Print("Text at row 10, column 20");

// Save position, move elsewhere, then return
display.SaveCursorPosition();
display.MoveCursor(1, 1);
display.PrintColoredLine(COLOR_BRIGHT_CYAN, "Header text at top");
display.RestoreCursorPosition();

// Relative movement
display.MoveCursorDown(3);
display.MoveCursorRight(5);
display.Print("Moved down 3, right 5");
```

### ASCII Art & Drawing

```cpp
// Draw a colored box
display.DrawBox(10, 5, 30, 10, COLOR_BRIGHT_GREEN);

// Draw lines
display.DrawHorizontalLine(1, 15, 50, '=');
display.DrawVerticalLine(25, 1, 20, '|');

// Fill areas with patterns
display.FillArea(5, 5, 10, 5, '#', COLOR_BRIGHT_YELLOW);
display.FillArea(20, 8, 15, 3, '*', BG_BLUE);
```

### Game UI Example

```cpp
void DrawGameHUD(DisplayManager& display, int health, int score, int level) {
    // Clear and set up screen
    display.ClearScreen();
    display.SetTitle("ASCIILATOR Game v1.0");
    
    // Draw header
    display.MoveCursor(1, 1);
    display.DrawHorizontalLine(1, 1, 80, '=');
    display.MoveCursor(2, 30);
    display.PrintStyledText(STYLE_BOLD, COLOR_BRIGHT_CYAN, "GAME STATUS");
    display.DrawHorizontalLine(1, 3, 80, '=');
    
    // Health bar
    display.MoveCursor(5, 5);
    display.PrintColored(COLOR_BRIGHT_WHITE, "Health: ");
    const char* healthColor = (health > 50) ? COLOR_BRIGHT_GREEN : COLOR_BRIGHT_RED;
    display.PrintFormatted("[");
    for (int i = 0; i < health / 5; i++) {
        display.PrintColored(healthColor, "█");
    }
    for (int i = health / 5; i < 20; i++) {
        display.Print("░");
    }
    display.PrintFormatted("] %d%%", health);
    
    // Score and level
    display.MoveCursor(7, 5);
    display.PrintColored(COLOR_BRIGHT_YELLOW, "Score: ");
    display.PrintFormatted("%d", score);
    
    display.MoveCursor(8, 5);
    display.PrintColored(COLOR_BRIGHT_MAGENTA, "Level: ");
    display.PrintFormatted("%d", level);
    
    // Game area border
    display.DrawBox(5, 10, 70, 20, COLOR_BRIGHT_BLUE);
}
```

### Console Management

```cpp
// Console configuration
display.SetTitle("My Application v2.0");
display.HideCursor(); // For cleaner display during animations

// Get console dimensions
int width, height;
display.GetConsoleSize(&width, &height);
display.PrintFormatted("Console size: %dx%d\n", width, height);

// Check ANSI support
if (display.IsANSIEnabled()) {
    display.PrintColoredLine(COLOR_BRIGHT_GREEN, "ANSI colors supported!");
} else {
    display.PrintLine("Fallback to plain text mode");
}
```

### Real-time Display Updates

```cpp
void AnimatedLoadingBar(DisplayManager& display) {
    display.MoveCursor(10, 10);
    display.Print("Loading: [");
    
    for (int i = 0; i <= 20; i++) {
        display.MoveCursor(10, 20 + i);
        if (i < 20) {
            display.PrintColored(COLOR_BRIGHT_GREEN, "█");
        } else {
            display.PrintColored(COLOR_BRIGHT_GREEN, "] 100%");
        }
        Sleep(100); // 100ms delay
    }
}

void FlashingWarning(DisplayManager& display) {
    for (int i = 0; i < 10; i++) {
        display.MoveCursor(15, 25);
        display.ClearLine();
        
        const char* color = (i % 2 == 0) ? COLOR_BRIGHT_RED : COLOR_BRIGHT_YELLOW;
        display.PrintStyledText(STYLE_BOLD, color, "⚠️  WARNING  ⚠️");
        
        Sleep(500); // 500ms flash interval
    }
}
```

---

## Architecture

- `display.hpp`: DisplayManager class declaration with color/style definitions
- `display.cpp`: Complete implementation with ANSI support and fallback handling
- Main program: Handles content logic and calls display API for all output
- Automatic ANSI detection: Enables advanced features when supported, graceful degradation otherwise

---

## Key Features

- **Comprehensive Color Support**: 16 standard colors plus bright variants and background colors
- **Advanced Text Styling**: Bold, italic, underline, strikethrough, and more
- **Precise Cursor Control**: Absolute positioning and relative movement
- **ASCII Art Capabilities**: Box drawing, line drawing, and area filling
- **Console Management**: Title setting, size detection, cursor visibility control
- **ANSI Compatibility**: Automatic detection with fallback for unsupported terminals
- **Memory Efficient**: No dynamic allocation, direct console API usage
- **Thread Safe**: Can be used safely across multiple threads
- **Easy Integration**: Simple instantiation, no complex setup required

---

## Performance Notes

- All output is buffered through Windows Console API for optimal performance
- ANSI escape sequences are only sent when ANSI support is detected
- Large area fills and complex drawings are optimized for minimal console calls
- Color changes are automatically reset to prevent state pollution

---

This display system provides a complete solution for Windows console applications requiring rich text output, positioning control, and visual enhancement capabilities.