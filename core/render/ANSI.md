
### 🔹 1. **Basic 16 Colors (Foreground / Background)**

```cpp
"\033[30m" // Black text
"\033[31m" // Red text
"\033[32m" // Green text
"\033[33m" // Yellow text
"\033[34m" // Blue text
"\033[35m" // Magenta text
"\033[36m" // Cyan text
"\033[37m" // White text

"\033[90m" // Bright Black (Gray)
"\033[91m" // Bright Red
"\033[92m" // Bright Green
"\033[93m" // Bright Yellow
"\033[94m" // Bright Blue
"\033[95m" // Bright Magenta
"\033[96m" // Bright Cyan
"\033[97m" // Bright White
```

👉 Background = same but with **40–47** and **100–107**.
Example: `"\033[41m"` = Red background.

---

### 🔹 2. **256 Color Mode**

* **Foreground:** `\033[38;5;<n>m`
* **Background:** `\033[48;5;<n>m`
  where `<n>` = 0–255.

Example (C++):

```cpp
std::cout << "\033[38;5;202mOrange text\033[0m\n";
std::cout << "\033[48;5;27mBlue background\033[0m\n";
```

---

### 🔹 3. **24-bit Truecolor (RGB)**

* **Foreground:** `\033[38;2;R;G;Bm`
* **Background:** `\033[48;2;R;G;Bm`

Example:

```cpp
std::cout << "\033[38;2;255;128;0mOrange text\033[0m\n";
std::cout << "\033[48;2;0;128;255mBlue background\033[0m\n";
```

---

### 🔹 4. **Extra Attributes (can combine with colors)**

```cpp
"\033[1m"   // Bold / Bright
"\033[2m"   // Dim
"\033[3m"   // Italic
"\033[4m"   // Underline
"\033[5m"   // Blink
"\033[7m"   // Invert (swap fg/bg)
"\033[8m"   // Hidden
"\033[9m"   // Strikethrough
"\033[0m"   // Reset all
```

---

✅ **Summary of syntaxes:**

* **16-colors:** `\033[30–37m`, `\033[90–97m` (fg) / `40–47`, `100–107` (bg)
* **256-colors:** `\033[38;5;<n>m` / `\033[48;5;<n>m`
* **Truecolor:** `\033[38;2;R;G;Bm` / `\033[48;2;R;G;Bm`
* **Attributes:** `\033[<attr>m` (can combine, e.g. `\033[1;31m` = bold red)

---
