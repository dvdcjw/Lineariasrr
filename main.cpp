#include <windows.h>
#include <shlobj.h>
#include "logger.h"
#include "proxy_dinput.h"
#include "lut.h"

// Original function pointers
typedef HRESULT(WINAPI* DirectInput8Create_t)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
typedef HRESULT(WINAPI* DllCanUnloadNow_t)();
typedef HRESULT(WINAPI* DllGetClassObject_t)(REFCLSID, REFIID, LPVOID*);
typedef HRESULT(WINAPI* DllRegisterServer_t)();
typedef HRESULT(WINAPI* DllUnregisterServer_t)();
typedef const DIDATAFORMAT*(WINAPI* GetdfDIJoystick_t)(void);

static HMODULE hOriginalDll = nullptr;
static DirectInput8Create_t pOriginalCreate = nullptr;
static DllCanUnloadNow_t pOriginalCanUnloadNow = nullptr;
static DllGetClassObject_t pOriginalGetClassObject = nullptr;
static DllRegisterServer_t pOriginalRegisterServer = nullptr;
static DllUnregisterServer_t pOriginalUnregisterServer = nullptr;
static GetdfDIJoystick_t pOriginalGetdfDIJoystick = nullptr;

void LoadOriginalDll() {
    if (hOriginalDll != nullptr) return;

    wchar_t systemPath[MAX_PATH];
    GetSystemDirectoryW(systemPath, MAX_PATH);
    std::wstring originalPath = std::wstring(systemPath) + L"\\dinput8.dll";

    LOG_FMT("[SYSTEM] Loading real dinput8.dll from: %ls", originalPath.c_str());
    hOriginalDll = LoadLibraryW(originalPath.c_str());
    if (hOriginalDll == nullptr) {
        LOG_MSG("[SYSTEM] Error: Failed to load real dinput8.dll");
        return;
    }

    pOriginalCreate = (DirectInput8Create_t)GetProcAddress(hOriginalDll, "DirectInput8Create");
    pOriginalCanUnloadNow = (DllCanUnloadNow_t)GetProcAddress(hOriginalDll, "DllCanUnloadNow");
    pOriginalGetClassObject = (DllGetClassObject_t)GetProcAddress(hOriginalDll, "DllGetClassObject");
    pOriginalRegisterServer = (DllRegisterServer_t)GetProcAddress(hOriginalDll, "DllRegisterServer");
    pOriginalUnregisterServer = (DllUnregisterServer_t)GetProcAddress(hOriginalDll, "DllUnregisterServer");
    pOriginalGetdfDIJoystick = (GetdfDIJoystick_t)GetProcAddress(hOriginalDll, "GetdfDIJoystick");
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        break;
    case DLL_PROCESS_DETACH:
        if (hOriginalDll != nullptr) {
            FreeLibrary(hOriginalDll);
            hOriginalDll = nullptr;
        }
        break;
    }
    return TRUE;
}

extern "C" HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter) {
    LoadOriginalDll();

    LOG_MSG("[MAIN] DirectInput8Create called.");

    if (pOriginalCreate == nullptr) {
        LOG_MSG("[MAIN] Error: original DirectInput8Create pointer is null.");
        return E_FAIL;
    }

    // Force load LUT
    LUT::GetInstance().Load();

    // Call original
    HRESULT hr = pOriginalCreate(hinst, dwVersion, riidltf, ppvOut, punkOuter);
    if (FAILED(hr)) {
        LOG_FMT("[MAIN] Original DirectInput8Create failed: HRESULT 0x%X", hr);
        return hr;
    }

    // Wrap the returned interface if it's Unicode
    if (riidltf == IID_IDirectInput8W && ppvOut != nullptr && *ppvOut != nullptr) {
        LOG_MSG("[MAIN] Wrapping IDirectInput8W interface.");
        IDirectInput8W* realDInput = (IDirectInput8W*)*ppvOut;
        *ppvOut = new ProxyDirectInput8W(realDInput);
    } else {
        LOG_MSG("[MAIN] ANSI requested or custom interface, passing through without wrap.");
    }

    return hr;
}

extern "C" HRESULT WINAPI DllCanUnloadNow() {
    LoadOriginalDll();
    if (pOriginalCanUnloadNow) return pOriginalCanUnloadNow();
    return S_FALSE;
}

extern "C" HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) {
    LoadOriginalDll();
    if (pOriginalGetClassObject) return pOriginalGetClassObject(rclsid, riid, ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}

extern "C" HRESULT WINAPI DllRegisterServer() {
    LoadOriginalDll();
    if (pOriginalRegisterServer) return pOriginalRegisterServer();
    return E_FAIL;
}

extern "C" HRESULT WINAPI DllUnregisterServer() {
    LoadOriginalDll();
    if (pOriginalUnregisterServer) return pOriginalUnregisterServer();
    return E_FAIL;
}

extern "C" const DIDATAFORMAT* WINAPI GetdfDIJoystick(void) {
    LoadOriginalDll();
    if (pOriginalGetdfDIJoystick) return pOriginalGetdfDIJoystick();
    return nullptr;
}
