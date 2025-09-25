@echo off
setlocal enabledelayedexpansion

REM Set up MSVC environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

echo Starting incremental build with hash checking...

REM Ensure the bin directory exists
if not exist "bin" mkdir "bin"

REM Create hash file if it doesn't exist
if not exist "compile_hashes.txt" (
    echo Creating initial hash file...
    echo. > "compile_hashes.txt"
)

REM Function to check and compile if needed
call :CheckAndCompile "core/main.cpp" "bin/main.obj"
call :CheckAndCompile "core/input/input.cpp" "bin/input.obj"
call :CheckAndCompile "core/console/console.cpp" "bin/console.obj"
call :CheckAndCompile "core/clock/clock.cpp" "bin/clock.obj"
call :CheckAndCompile "core/sound/sound.cpp" "bin/sound.obj"
call :CheckAndCompile "core/render/render.cpp" "bin/render.obj"


echo Linking object files to create executable...

REM Link all object files together
link /OUT:engine.exe bin\main.obj bin\input.obj bin\console.obj bin\clock.obj bin\sound.obj bin\render.obj /SUBSYSTEM:CONSOLE user32.lib kernel32.lib

echo Build complete!
echo Hash information stored in compile_hashes.txt
goto :eof

:CheckAndCompile
set SOURCE_FILE=%~1
set OBJECT_FILE=%~2

REM Calculate current hash using certutil (Windows built-in)
set CURRENT_HASH=
for /f "skip=1 tokens=1 delims= " %%i in ('certutil -hashfile "!SOURCE_FILE!" MD5 2^>nul ^| findstr /v "certutil"') do (
    if "!CURRENT_HASH!"=="" set CURRENT_HASH=%%i
)

REM Remove any trailing spaces from hash
set CURRENT_HASH=!CURRENT_HASH: =!

REM Check if object file exists
if not exist "!OBJECT_FILE!" (
    echo Compiling !SOURCE_FILE! ^(object file missing^)...
    cl /c /EHsc /O2 "!SOURCE_FILE!" /Fo"!OBJECT_FILE!"
    goto :update_hash
)

REM Get stored hash for this file
set STORED_HASH=
for /f "tokens=2 delims= " %%a in ('findstr "!SOURCE_FILE!" "compile_hashes.txt" 2^>nul') do set STORED_HASH=%%a

REM Remove any trailing spaces from stored hash
set STORED_HASH=!STORED_HASH: =!

REM Debug output (remove this later if you want)
REM echo DEBUG: Current hash for !SOURCE_FILE!: !CURRENT_HASH!
REM echo DEBUG: Stored hash for !SOURCE_FILE!: !STORED_HASH!

REM Compare hashes
if "!CURRENT_HASH!"=="!STORED_HASH!" (
    echo !SOURCE_FILE! is up to date
    goto :eof
) else (
    echo Compiling !SOURCE_FILE! ^(source changed^)...
    cl /c /EHsc /O2 "!SOURCE_FILE!" /Fo"!OBJECT_FILE!"
)

:update_hash
REM Remove old entry for this file
findstr /V /C:"!SOURCE_FILE!" "compile_hashes.txt" > "temp_hashes.txt" 2>nul
REM Add new entry
echo !SOURCE_FILE! !CURRENT_HASH! >> "temp_hashes.txt"
REM Replace original file
move "temp_hashes.txt" "compile_hashes.txt" > nul

goto :eof