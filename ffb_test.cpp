#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>
#include <iostream>
#include <conio.h> // For _kbhit()
#include <mutex>

typedef HRESULT(WINAPI* DirectInput8Create_t)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);

struct FFBDeviceFinder {
    GUID guidInstance;
    bool found;
};

// Global pointers for cleanup in the interruption handler
LPDIRECTINPUT8W g_pDInput = nullptr;
LPDIRECTINPUTDEVICE8W g_pDevice = nullptr;
LPDIRECTINPUTEFFECT g_pEffect = nullptr;
HWND g_hwnd = nullptr;
HMODULE g_hDll = nullptr;
HINSTANCE g_hInstance = nullptr;

std::mutex g_cleanupMutex;
bool g_isCleanedUp = false;

void CleanupDirectInput() {
    std::lock_guard<std::mutex> lock(g_cleanupMutex);
    if (g_isCleanedUp) return;
    g_isCleanedUp = true;

    std::cout << "\nSafely releasing Force Feedback wheel and stopping effects..." << std::endl;
    
    if (g_pEffect) {
        g_pEffect->Stop();
        g_pEffect->Release();
        g_pEffect = nullptr;
    }
    if (g_pDevice) {
        g_pDevice->Unacquire();
        g_pDevice->Release();
        g_pDevice = nullptr;
    }
    if (g_pDInput) {
        g_pDInput->Release();
        g_pDInput = nullptr;
    }
    if (g_hwnd) {
        DestroyWindow(g_hwnd);
        UnregisterClassA("DummyDInputWindow", g_hInstance);
        g_hwnd = nullptr;
    }
    if (g_hDll) {
        FreeLibrary(g_hDll);
        g_hDll = nullptr;
    }
    std::cout << "Cleanup completed." << std::endl;
}

// Handler for console events like Ctrl+C or closing the window
BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType) {
    switch (ctrlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        std::cout << "\n[ALERT] Program interrupted (signal: " << ctrlType << ")!" << std::endl;
        CleanupDirectInput();
        // Return FALSE to allow Windows to terminate the process after cleanup
        return FALSE; 
    }
    return FALSE;
}

BOOL CALLBACK EnumFFBDevicesCallback(const DIDEVICEINSTANCEW* lpddi, LPVOID pvRef) {
    FFBDeviceFinder* finder = static_cast<FFBDeviceFinder*>(pvRef);
    finder->guidInstance = lpddi->guidInstance;
    finder->found = true;
    std::wcout << L"Found FFB Device: " << lpddi->tszInstanceName << L" (" << lpddi->tszProductName << L")" << std::endl;
    return DIENUM_STOP; // Stop at the first FFB device
}

int main() {
    std::cout << "=========================================================" << std::endl;
    std::cout << "DirectInput 8 Force Feedback Centering Force Tester" << std::endl;
    std::cout << "=========================================================" << std::endl;

    // Register the console control handler
    if (!SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE)) {
        std::cerr << "[WARNING] Failed to register console control handler!" << std::endl;
    }

    g_hInstance = GetModuleHandle(nullptr);

    std::cout << "Loading local dinput8.dll (Wrapper)..." << std::endl;
    g_hDll = LoadLibraryA("dinput8.dll");
    if (!g_hDll) {
        std::cerr << "[ERROR] Failed to load local dinput8.dll! Error: " << GetLastError() << std::endl;
        return 1;
    }

    DirectInput8Create_t pCreate = (DirectInput8Create_t)GetProcAddress(g_hDll, "DirectInput8Create");
    if (!pCreate) {
        std::cerr << "[ERROR] Failed to get DirectInput8Create address!" << std::endl;
        CleanupDirectInput();
        return 1;
    }

    // Create a dummy hidden window owned by our process.
    WNDCLASSEXA wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc = DefWindowProcA;
    wc.hInstance = g_hInstance;
    wc.lpszClassName = "DummyDInputWindow";
    RegisterClassExA(&wc);

    g_hwnd = CreateWindowExA(
        0,
        "DummyDInputWindow",
        "Dummy DInput Window",
        WS_POPUP,
        0, 0, 0, 0,
        nullptr, nullptr, wc.hInstance, nullptr
    );

    if (!g_hwnd) {
        std::cerr << "[ERROR] Could not create dummy window handle!" << std::endl;
        CleanupDirectInput();
        return 1;
    }

    HRESULT hr = pCreate(g_hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8W, (LPVOID*)&g_pDInput, nullptr);
    if (FAILED(hr)) {
        std::cerr << "[ERROR] DirectInput8Create failed: 0x" << std::hex << hr << std::endl;
        CleanupDirectInput();
        return 1;
    }

    // Find the first attached force feedback device (wheel/joystick)
    FFBDeviceFinder finder = { {}, false };
    std::cout << "Scanning for Force Feedback devices..." << std::endl;
    g_pDInput->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumFFBDevicesCallback, &finder, DIEDFL_FORCEFEEDBACK | DIEDFL_ATTACHEDONLY);

    if (!finder.found) {
        std::cerr << "[WARNING] No Force Feedback device found on this system!" << std::endl;
        std::cerr << "Make sure your steering wheel is plugged in and powered." << std::endl;
        CleanupDirectInput();
        return 1;
    }

    // Create device
    hr = g_pDInput->CreateDevice(finder.guidInstance, &g_pDevice, nullptr);
    if (FAILED(hr)) {
        std::cerr << "[ERROR] CreateDevice failed: 0x" << std::hex << hr << std::endl;
        CleanupDirectInput();
        return 1;
    }

    // Set Data Format
    hr = g_pDevice->SetDataFormat(&c_dfDIJoystick2);
    if (FAILED(hr)) {
        std::cerr << "[ERROR] SetDataFormat failed: 0x" << std::hex << hr << std::endl;
        CleanupDirectInput();
        return 1;
    }

    // Set Cooperative Level. Exclusive access is required for FFB.
    hr = g_pDevice->SetCooperativeLevel(g_hwnd, DISCL_EXCLUSIVE | DISCL_BACKGROUND);
    if (FAILED(hr)) {
        std::cerr << "[ERROR] SetCooperativeLevel failed (0x" << std::hex << hr << L")." << std::endl;
        std::cerr << "Ensure that no other software is holding exclusive lock on the wheel." << std::endl;
        CleanupDirectInput();
        return 1;
    }

    // Turn off Auto-Centering so we can apply our own centering force
    DIPROPDWORD diprop;
    diprop.diph.dwSize = sizeof(DIPROPDWORD);
    diprop.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    diprop.diph.dwObj = 0;
    diprop.diph.dwHow = DIPH_DEVICE;
    diprop.dwData = DIPROPAUTOCENTER_OFF;
    g_pDevice->SetProperty(DIPROP_AUTOCENTER, &diprop.diph);

    // Set X-axis range to [-32768, 32767] so that the position is signed and centered at 0
    DIPROPRANGE diprg;
    diprg.diph.dwSize = sizeof(DIPROPRANGE);
    diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    diprg.diph.dwHow = DIPH_BYOFFSET;
    diprg.diph.dwObj = DIJOFS_X;
    diprg.lMin = -32768;
    diprg.lMax = 32767;
    hr = g_pDevice->SetProperty(DIPROP_RANGE, &diprg.diph);
    if (FAILED(hr)) {
        std::cerr << "[WARNING] SetProperty(DIPROP_RANGE) failed: 0x" << std::hex << hr << std::endl;
    } else {
        std::cout << "Successfully set axis range to [-32768, 32767]." << std::endl;
    }

    // Acquire device
    hr = g_pDevice->Acquire();
    if (FAILED(hr)) {
        std::cerr << "[ERROR] Acquire device failed: 0x" << std::hex << hr << std::endl;
        CleanupDirectInput();
        return 1;
    }

    // Set up Constant Force effect on X axis
    DWORD rgdwAxes[1] = { DIJOFS_X };
    LONG rglDirections[1] = { 0 };

    DICONSTANTFORCE cf;
    cf.lMagnitude = 0; // Starts at zero force

    DIEFFECT eff;
    ZeroMemory(&eff, sizeof(eff));
    eff.dwSize = sizeof(DIEFFECT);
    eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration = INFINITE;
    eff.dwSamplePeriod = 0;
    eff.dwGain = 10000;
    eff.dwTriggerButton = DIEB_NOTRIGGER;
    eff.dwTriggerRepeatInterval = 0;
    eff.cAxes = 1;
    eff.rgdwAxes = rgdwAxes;
    eff.rglDirection = rglDirections;
    eff.lpEnvelope = nullptr;
    eff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
    eff.lpvTypeSpecificParams = &cf;

    std::cout << "Creating FFB constant force effect..." << std::endl;
    hr = g_pDevice->CreateEffect(GUID_ConstantForce, &eff, &g_pEffect, nullptr);
    if (FAILED(hr)) {
        std::cerr << "[ERROR] CreateEffect failed: 0x" << std::hex << hr << std::endl;
        CleanupDirectInput();
        return 1;
    }

    // Start FFB effect
    hr = g_pEffect->Start(1, 0);
    if (FAILED(hr)) {
        std::cerr << "[ERROR] Failed to start FFB effect: 0x" << std::hex << hr << std::endl;
        CleanupDirectInput();
        return 1;
    }

    std::cout << "\n>>> Centering Force Active! <<<" << std::endl;
    std::cout << "Gently turn the wheel/joystick to feel it pull back to the center." << std::endl;
    std::cout << "Press any key in the console to exit." << std::endl;

    DIJOYSTATE2 state;
    const double MAX_CENTERING_FORCE = 3500.0; // Gentle centering force (max 35% of 10000)

    while (!_kbhit()) {
        hr = g_pDevice->Poll();
        if (FAILED(hr)) {
            g_pDevice->Acquire();
            Sleep(10);
            continue;
        }

        hr = g_pDevice->GetDeviceState(sizeof(DIJOYSTATE2), &state);
        if (FAILED(hr)) {
            Sleep(10);
            continue;
        }

        double position = static_cast<double>(state.lX);
        double deflectionRatio = position / 32768.0;

        // Clamp deflection ratio between -1.0 and 1.0
        if (deflectionRatio < -1.0) deflectionRatio = -1.0;
        if (deflectionRatio > 1.0) deflectionRatio = 1.0;

        LONG targetForce = static_cast<LONG>(deflectionRatio * MAX_CENTERING_FORCE);

        // Update the FFB effect parameters with new magnitude
        cf.lMagnitude = targetForce;

        DIEFFECT effUpdate;
        ZeroMemory(&effUpdate, sizeof(effUpdate));
        effUpdate.dwSize = sizeof(DIEFFECT);
        effUpdate.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
        effUpdate.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
        effUpdate.lpvTypeSpecificParams = &cf;

        hr = g_pEffect->SetParameters(&effUpdate, DIEP_TYPESPECIFICPARAMS | DIEP_NORESTART);
        if (FAILED(hr)) {
            printf(" [ERROR SetParameters: 0x%lX] ", hr);
        }

        // Display current stats
        printf("\rWheel Position: %6.0f | Applied Centering Force: %5ld  ", position, targetForce);
        fflush(stdout);

        Sleep(15); // Loop at ~66Hz
    }

    std::cout << "\n\nExiting normally, stopping FFB effects..." << std::endl;
    
    // Normal cleanup
    SetConsoleCtrlHandler(ConsoleCtrlHandler, FALSE); // Remove control handler
    CleanupDirectInput();

    std::cout << "Done." << std::endl;
    return 0;
}