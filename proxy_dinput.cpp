#include "proxy_dinput.h"
#include "proxy_device.h"
#include "logger.h"

ProxyDirectInput8W::ProxyDirectInput8W(IDirectInput8W* realDInput)
    : RealDInput(realDInput), RefCount(1) {
    LOG_MSG("[DINPUT] Proxy IDirectInput8W created.");
}

ProxyDirectInput8W::~ProxyDirectInput8W() {
    if (RealDInput) {
        RealDInput->Release();
    }
}

STDMETHODIMP ProxyDirectInput8W::QueryInterface(REFIID riid, LPVOID* ppvObj) {
    if (!ppvObj) return E_POINTER;
    if (riid == IID_IUnknown || riid == IID_IDirectInput8W) {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    return RealDInput->QueryInterface(riid, ppvObj);
}

STDMETHODIMP_(ULONG) ProxyDirectInput8W::AddRef() {
    return InterlockedIncrement(&RefCount);
}

STDMETHODIMP_(ULONG) ProxyDirectInput8W::Release() {
    ULONG ref = InterlockedDecrement(&RefCount);
    if (ref == 0) {
        delete this;
    }
    return ref;
}

STDMETHODIMP ProxyDirectInput8W::CreateDevice(REFGUID rguid, LPDIRECTINPUTDEVICE8W* lplpDirectInputDevice, LPUNKNOWN pUnkOuter) {
    LOG_FMT("[DINPUT] CreateDevice called for GUID: %08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
            rguid.Data1, rguid.Data2, rguid.Data3,
            rguid.Data4[0], rguid.Data4[1], rguid.Data4[2], rguid.Data4[3],
            rguid.Data4[4], rguid.Data4[5], rguid.Data4[6], rguid.Data4[7]);

    IDirectInputDevice8W* realDevice = nullptr;
    HRESULT hr = RealDInput->CreateDevice(rguid, &realDevice, pUnkOuter);

    if (SUCCEEDED(hr) && lplpDirectInputDevice != nullptr && realDevice != nullptr) {
        *lplpDirectInputDevice = new ProxyDirectInputDevice8W(realDevice);
        LOG_MSG("[DINPUT] Device wrapped successfully.");
    } else if (lplpDirectInputDevice != nullptr) {
        *lplpDirectInputDevice = nullptr;
    }

    return hr;
}

STDMETHODIMP ProxyDirectInput8W::EnumDevices(DWORD dwDevType, LPDIENUMDEVICESCALLBACKW lpCallback, LPVOID pvRef, DWORD dwFlags) {
    return RealDInput->EnumDevices(dwDevType, lpCallback, pvRef, dwFlags);
}

STDMETHODIMP ProxyDirectInput8W::GetDeviceStatus(REFGUID rguidInstance) {
    return RealDInput->GetDeviceStatus(rguidInstance);
}

STDMETHODIMP ProxyDirectInput8W::RunControlPanel(HWND hwndOwner, DWORD dwFlags) {
    return RealDInput->RunControlPanel(hwndOwner, dwFlags);
}

STDMETHODIMP ProxyDirectInput8W::Initialize(HINSTANCE hinst, DWORD dwVersion) {
    return RealDInput->Initialize(hinst, dwVersion);
}

STDMETHODIMP ProxyDirectInput8W::FindDevice(REFGUID rguidClass, LPCWSTR ptszName, LPGUID pguidInstance) {
    return RealDInput->FindDevice(rguidClass, ptszName, pguidInstance);
}

STDMETHODIMP ProxyDirectInput8W::EnumDevicesBySemantics(LPCWSTR ptszUserName, LPDIACTIONFORMATW lpdiActionFormat, LPDIENUMDEVICESBYSEMANTICSCBW lpCallback, LPVOID pvRef, DWORD dwFlags) {
    return RealDInput->EnumDevicesBySemantics(ptszUserName, lpdiActionFormat, lpCallback, pvRef, dwFlags);
}

STDMETHODIMP ProxyDirectInput8W::ConfigureDevices(LPDICONFIGUREDEVICESCALLBACK lpdiCallback, LPDICONFIGUREDEVICESPARAMSW lpdiParams, DWORD dwFlags, LPVOID pvRefData) {
    return RealDInput->ConfigureDevices(lpdiCallback, lpdiParams, dwFlags, pvRefData);
}
