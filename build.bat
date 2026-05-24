@echo off
echo ===================================================
echo Building DirectInput 8 FFB LUT Wrapper...
echo ===================================================

echo Compiling dinput8.dll...
g++ -O2 -shared -std=c++17 -static -o dinput8.dll main.cpp logger.cpp lut.cpp proxy_device.cpp proxy_dinput.cpp dinput8.def -ldxguid -lkernel32 -luser32 -lole32

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Failed to compile dinput8.dll!
    exit /b %ERRORLEVEL%
)
echo [SUCCESS] Compiled dinput8.dll

echo Compiling ffb_test.exe...
g++ -O2 -std=c++17 -static -o ffb_test.exe ffb_test.cpp -ldinput8 -ldxguid -lkernel32 -luser32

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Failed to compile ffb_test.exe!
    exit /b %ERRORLEVEL%
)
echo [SUCCESS] Compiled ffb_test.exe

echo ===================================================
echo Build completed successfully.
echo ===================================================
