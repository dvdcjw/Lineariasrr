#include "proxy_device.h"
#include "logger.h"
#include "lut.h"

// Helper function to check if a GUID matches GUID_ConstantForce
static bool IsConstantForce(REFGUID guid) {
    return (guid == GUID_ConstantForce);
}

static bool g_FirstFFBIntercepted = false;
static unsigned long long g_SetParametersCount = 0;

static void LogFirstFFBIntercept() {
    if (!g_FirstFFBIntercepted) {
        g_FirstFFBIntercepted = true;
        LOG_MSG("[FFB] SUCCESS: Intercepted first FFB call!");
    }
}

// ============================================================================
// ProxyDirectInputEffect Implementation
// ============================================================================

ProxyDirectInputEffect::ProxyDirectInputEffect(IDirectInputEffect* realEffect, REFGUID guid)
    : RealEffect(realEffect), EffectGuid(guid), RefCount(1) {
    LOG_FMT("[EFFECT] Proxy effect created for GUID: %08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
            guid.Data1, guid.Data2, guid.Data3,
            guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
            guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}

ProxyDirectInputEffect::~ProxyDirectInputEffect() {
    if (RealEffect) {
        RealEffect->Release();
    }
}

STDMETHODIMP ProxyDirectInputEffect::QueryInterface(REFIID riid, LPVOID* ppvObj) {
    if (!ppvObj) return E_POINTER;
    if (riid == IID_IUnknown || riid == IID_IDirectInputEffect) {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    return RealEffect->QueryInterface(riid, ppvObj);
}

STDMETHODIMP_(ULONG) ProxyDirectInputEffect::AddRef() {
    return InterlockedIncrement(&RefCount);
}

STDMETHODIMP_(ULONG) ProxyDirectInputEffect::Release() {
    ULONG ref = InterlockedDecrement(&RefCount);
    if (ref == 0) {
        delete this;
    }
    return ref;
}

STDMETHODIMP ProxyDirectInputEffect::Initialize(HINSTANCE hinst, DWORD dwVersion, REFGUID rguid) {
    return RealEffect->Initialize(hinst, dwVersion, rguid);
}

STDMETHODIMP ProxyDirectInputEffect::GetEffectGuid(LPGUID pguid) {
    return RealEffect->GetEffectGuid(pguid);
}

STDMETHODIMP ProxyDirectInputEffect::GetParameters(LPDIEFFECT peff, DWORD dwFlags) {
    return RealEffect->GetParameters(peff, dwFlags);
}

STDMETHODIMP ProxyDirectInputEffect::SetParameters(LPCDIEFFECT peff, DWORD dwFlags) {
    DIEFFECT modifiedEff;
    LPCDIEFFECT pEffToUse = peff;
    DICONSTANTFORCE cf;
    DICONSTANTFORCE originalCf;

    if (peff != nullptr) {
        modifiedEff = *peff;
        // Check if we are modifying constant force parameters
        if (IsConstantForce(EffectGuid) && 
            (dwFlags & DIEP_TYPESPECIFICPARAMS) && 
            peff->lpvTypeSpecificParams != nullptr && 
            peff->cbTypeSpecificParams == sizeof(DICONSTANTFORCE)) {
            
            originalCf = *(DICONSTANTFORCE*)peff->lpvTypeSpecificParams;
            cf = originalCf;
            
            // Apply LUT
            cf.lMagnitude = LUT::GetInstance().Apply(cf.lMagnitude);
            modifiedEff.lpvTypeSpecificParams = &cf;
            pEffToUse = &modifiedEff;
            
            LogFirstFFBIntercept();
            g_SetParametersCount++;
            if ((g_SetParametersCount & (g_SetParametersCount - 1)) == 0) {
                LOG_FMT("[FFB] SetParameters intercept (count=%llu): %ld -> %ld", g_SetParametersCount, originalCf.lMagnitude, cf.lMagnitude);
            }
        }
    }

    return RealEffect->SetParameters(pEffToUse, dwFlags);
}

STDMETHODIMP ProxyDirectInputEffect::Start(DWORD dwIterations, DWORD dwFlags) {
    return RealEffect->Start(dwIterations, dwFlags);
}

STDMETHODIMP ProxyDirectInputEffect::Stop() {
    return RealEffect->Stop();
}

STDMETHODIMP ProxyDirectInputEffect::GetEffectStatus(LPDWORD pdwStatus) {
    return RealEffect->GetEffectStatus(pdwStatus);
}

STDMETHODIMP ProxyDirectInputEffect::Download() {
    return RealEffect->Download();
}

STDMETHODIMP ProxyDirectInputEffect::Unload() {
    return RealEffect->Unload();
}

STDMETHODIMP ProxyDirectInputEffect::Escape(LPDIEFFESCAPE lpIEffectEscape) {
    return RealEffect->Escape(lpIEffectEscape);
}


// ============================================================================
// ProxyDirectInputDevice8W Implementation
// ============================================================================

ProxyDirectInputDevice8W::ProxyDirectInputDevice8W(IDirectInputDevice8W* realDevice)
    : RealDevice(realDevice), RefCount(1) {
    LOG_MSG("[DEVICE] Proxy device created. Waiting for FFB calls...");
}

ProxyDirectInputDevice8W::~ProxyDirectInputDevice8W() {
    if (RealDevice) {
        RealDevice->Release();
    }
}

STDMETHODIMP ProxyDirectInputDevice8W::QueryInterface(REFIID riid, LPVOID* ppvObj) {
    if (!ppvObj) return E_POINTER;
    if (riid == IID_IUnknown || riid == IID_IDirectInputDevice8W) {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    return RealDevice->QueryInterface(riid, ppvObj);
}

STDMETHODIMP_(ULONG) ProxyDirectInputDevice8W::AddRef() {
    return InterlockedIncrement(&RefCount);
}

STDMETHODIMP_(ULONG) ProxyDirectInputDevice8W::Release() {
    ULONG ref = InterlockedDecrement(&RefCount);
    if (ref == 0) {
        delete this;
    }
    return ref;
}

STDMETHODIMP ProxyDirectInputDevice8W::GetCapabilities(LPDIDEVCAPS lpDIDevCaps) {
    return RealDevice->GetCapabilities(lpDIDevCaps);
}

STDMETHODIMP ProxyDirectInputDevice8W::EnumObjects(LPDIENUMDEVICEOBJECTSCALLBACKW lpCallback, LPVOID pvRef, DWORD dwFlags) {
    return RealDevice->EnumObjects(lpCallback, pvRef, dwFlags);
}

STDMETHODIMP ProxyDirectInputDevice8W::GetProperty(REFGUID rguidProp, LPDIPROPHEADER pdiph) {
    return RealDevice->GetProperty(rguidProp, pdiph);
}

STDMETHODIMP ProxyDirectInputDevice8W::SetProperty(REFGUID rguidProp, LPCDIPROPHEADER pdiph) {
    return RealDevice->SetProperty(rguidProp, pdiph);
}

STDMETHODIMP ProxyDirectInputDevice8W::Acquire() {
    return RealDevice->Acquire();
}

STDMETHODIMP ProxyDirectInputDevice8W::Unacquire() {
    return RealDevice->Unacquire();
}

STDMETHODIMP ProxyDirectInputDevice8W::GetDeviceState(DWORD cbData, LPVOID lpvData) {
    return RealDevice->GetDeviceState(cbData, lpvData);
}

STDMETHODIMP ProxyDirectInputDevice8W::GetDeviceData(DWORD cbObjectData, LPDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD dwFlags) {
    return RealDevice->GetDeviceData(cbObjectData, rgdod, pdwInOut, dwFlags);
}

STDMETHODIMP ProxyDirectInputDevice8W::SetDataFormat(LPCDIDATAFORMAT lpdf) {
    return RealDevice->SetDataFormat(lpdf);
}

STDMETHODIMP ProxyDirectInputDevice8W::SetEventNotification(HANDLE hEvent) {
    return RealDevice->SetEventNotification(hEvent);
}

STDMETHODIMP ProxyDirectInputDevice8W::SetCooperativeLevel(HWND hwnd, DWORD dwFlags) {
    return RealDevice->SetCooperativeLevel(hwnd, dwFlags);
}

STDMETHODIMP ProxyDirectInputDevice8W::GetObjectInfo(LPDIDEVICEOBJECTINSTANCEW pdidoi, DWORD dwObj, DWORD dwHow) {
    return RealDevice->GetObjectInfo(pdidoi, dwObj, dwHow);
}

STDMETHODIMP ProxyDirectInputDevice8W::GetDeviceInfo(LPDIDEVICEINSTANCEW pdidi) {
    return RealDevice->GetDeviceInfo(pdidi);
}

STDMETHODIMP ProxyDirectInputDevice8W::RunControlPanel(HWND hwndOwner, DWORD dwFlags) {
    return RealDevice->RunControlPanel(hwndOwner, dwFlags);
}

STDMETHODIMP ProxyDirectInputDevice8W::Initialize(HINSTANCE hinst, DWORD dwVersion, REFGUID rguid) {
    return RealDevice->Initialize(hinst, dwVersion, rguid);
}

STDMETHODIMP ProxyDirectInputDevice8W::CreateEffect(REFGUID rguid, LPCDIEFFECT peff, LPDIRECTINPUTEFFECT* ppdeffect, LPUNKNOWN punkOuter) {
    DIEFFECT modifiedEff;
    LPCDIEFFECT pEffToUse = peff;
    DICONSTANTFORCE cf;
    DICONSTANTFORCE originalCf;

    if (peff != nullptr) {
        modifiedEff = *peff;
        if (IsConstantForce(rguid) && peff->lpvTypeSpecificParams != nullptr && peff->cbTypeSpecificParams == sizeof(DICONSTANTFORCE)) {
            originalCf = *(DICONSTANTFORCE*)peff->lpvTypeSpecificParams;
            cf = originalCf;
            
            // Apply LUT
            cf.lMagnitude = LUT::GetInstance().Apply(cf.lMagnitude);
            modifiedEff.lpvTypeSpecificParams = &cf;
            pEffToUse = &modifiedEff;
            
            LogFirstFFBIntercept();
            LOG_FMT("[FFB] CreateEffect intercept constant force: %ld -> %ld", originalCf.lMagnitude, cf.lMagnitude);
        } else {
            LOG_FMT("[FFB] CreateEffect other force GUID: %08lX", rguid.Data1);
        }
    }

    LPDIRECTINPUTEFFECT realEffect = nullptr;
    HRESULT hr = RealDevice->CreateEffect(rguid, pEffToUse, &realEffect, punkOuter);
    
    if (SUCCEEDED(hr) && ppdeffect != nullptr && realEffect != nullptr) {
        *ppdeffect = new ProxyDirectInputEffect(realEffect, rguid);
    } else if (ppdeffect != nullptr) {
        *ppdeffect = nullptr;
    }
    
    return hr;
}

STDMETHODIMP ProxyDirectInputDevice8W::EnumEffects(LPDIENUMEFFECTSCALLBACKW lpCallback, LPVOID pvRef, DWORD dwEffType) {
    return RealDevice->EnumEffects(lpCallback, pvRef, dwEffType);
}

STDMETHODIMP ProxyDirectInputDevice8W::GetEffectInfo(LPDIEFFECTINFOW pdei, REFGUID rguid) {
    return RealDevice->GetEffectInfo(pdei, rguid);
}

STDMETHODIMP ProxyDirectInputDevice8W::GetForceFeedbackState(LPDWORD pdwOut) {
    return RealDevice->GetForceFeedbackState(pdwOut);
}

STDMETHODIMP ProxyDirectInputDevice8W::SendForceFeedbackCommand(DWORD dwFlags) {
    LOG_FMT("[DEVICE] SendForceFeedbackCommand: 0x%X", dwFlags);
    return RealDevice->SendForceFeedbackCommand(dwFlags);
}

STDMETHODIMP ProxyDirectInputDevice8W::EnumCreatedEffectObjects(LPDIENUMCREATEDEFFECTOBJECTSCALLBACK lpCallback, LPVOID pvRef, DWORD dwFlags) {
    return RealDevice->EnumCreatedEffectObjects(lpCallback, pvRef, dwFlags);
}

STDMETHODIMP ProxyDirectInputDevice8W::Escape(LPDIEFFESCAPE lpIEffectEscape) {
    return RealDevice->Escape(lpIEffectEscape);
}

STDMETHODIMP ProxyDirectInputDevice8W::WriteEffectToFile(LPCWSTR lpszFileName, DWORD dwEntries, LPDIFILEEFFECT rgDiFileEft, DWORD dwFlags) {
    return RealDevice->WriteEffectToFile(lpszFileName, dwEntries, rgDiFileEft, dwFlags);
}

STDMETHODIMP ProxyDirectInputDevice8W::BuildActionMap(LPDIACTIONFORMATW lpdf, LPCWSTR lpszUserName, DWORD dwFlags) {
    return RealDevice->BuildActionMap(lpdf, lpszUserName, dwFlags);
}

STDMETHODIMP ProxyDirectInputDevice8W::SetActionMap(LPDIACTIONFORMATW lpdf, LPCWSTR lpszUserName, DWORD dwFlags) {
    return RealDevice->SetActionMap(lpdf, lpszUserName, dwFlags);
}

STDMETHODIMP ProxyDirectInputDevice8W::GetImageInfo(LPDIDEVICEIMAGEINFOHEADERW lpdiDeviceImageInfoHeader) {
    return RealDevice->GetImageInfo(lpdiDeviceImageInfoHeader);
}

STDMETHODIMP ProxyDirectInputDevice8W::Poll() {
    return RealDevice->Poll();
}

STDMETHODIMP ProxyDirectInputDevice8W::SendDeviceData(DWORD cbObjectData, LPCDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD dwFlags) {
    return RealDevice->SendDeviceData(cbObjectData, rgdod, pdwInOut, dwFlags);
}

STDMETHODIMP ProxyDirectInputDevice8W::EnumEffectsInFile(LPCWSTR lpszFileName, LPDIENUMEFFECTSINFILECALLBACK pec, LPVOID pvRef, DWORD dwFlags) {
    return RealDevice->EnumEffectsInFile(lpszFileName, pec, pvRef, dwFlags);
}
