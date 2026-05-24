#ifndef PROXY_DEVICE_H
#define PROXY_DEVICE_H

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

class ProxyDirectInputEffect : public IDirectInputEffect {
public:
    ProxyDirectInputEffect(IDirectInputEffect* realEffect, REFGUID guid);
    virtual ~ProxyDirectInputEffect();

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObj) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;

    // IDirectInputEffect methods
    STDMETHOD(Initialize)(HINSTANCE hinst, DWORD dwVersion, REFGUID rguid) override;
    STDMETHOD(GetEffectGuid)(LPGUID pguid) override;
    STDMETHOD(GetParameters)(LPDIEFFECT peff, DWORD dwFlags) override;
    STDMETHOD(SetParameters)(LPCDIEFFECT peff, DWORD dwFlags) override;
    STDMETHOD(Start)(DWORD dwIterations, DWORD dwFlags) override;
    STDMETHOD(Stop)() override;
    STDMETHOD(GetEffectStatus)(LPDWORD pdwStatus) override;
    STDMETHOD(Download)() override;
    STDMETHOD(Unload)() override;
    STDMETHOD(Escape)(LPDIEFFESCAPE lpIEffectEscape) override;

private:
    IDirectInputEffect* RealEffect;
    GUID EffectGuid;
    ULONG RefCount;
};

class ProxyDirectInputDevice8W : public IDirectInputDevice8W {
public:
    ProxyDirectInputDevice8W(IDirectInputDevice8W* realDevice);
    virtual ~ProxyDirectInputDevice8W();

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObj) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;

    // IDirectInputDevice8W methods
    STDMETHOD(GetCapabilities)(LPDIDEVCAPS lpDIDevCaps) override;
    STDMETHOD(EnumObjects)(LPDIENUMDEVICEOBJECTSCALLBACKW lpCallback, LPVOID pvRef, DWORD dwFlags) override;
    STDMETHOD(GetProperty)(REFGUID rguidProp, LPDIPROPHEADER pdiph) override;
    STDMETHOD(SetProperty)(REFGUID rguidProp, LPCDIPROPHEADER pdiph) override;
    STDMETHOD(Acquire)() override;
    STDMETHOD(Unacquire)() override;
    STDMETHOD(GetDeviceState)(DWORD cbData, LPVOID lpvData) override;
    STDMETHOD(GetDeviceData)(DWORD cbObjectData, LPDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD dwFlags) override;
    STDMETHOD(SetDataFormat)(LPCDIDATAFORMAT lpdf) override;
    STDMETHOD(SetEventNotification)(HANDLE hEvent) override;
    STDMETHOD(SetCooperativeLevel)(HWND hwnd, DWORD dwFlags) override;
    STDMETHOD(GetObjectInfo)(LPDIDEVICEOBJECTINSTANCEW pdidoi, DWORD dwObj, DWORD dwHow) override;
    STDMETHOD(GetDeviceInfo)(LPDIDEVICEINSTANCEW pdidi) override;
    STDMETHOD(RunControlPanel)(HWND hwndOwner, DWORD dwFlags) override;
    STDMETHOD(Initialize)(HINSTANCE hinst, DWORD dwVersion, REFGUID rguid) override;
    STDMETHOD(CreateEffect)(REFGUID rguid, LPCDIEFFECT peff, LPDIRECTINPUTEFFECT* ppdeffect, LPUNKNOWN punkOuter) override;
    STDMETHOD(EnumEffects)(LPDIENUMEFFECTSCALLBACKW lpCallback, LPVOID pvRef, DWORD dwEffType) override;
    STDMETHOD(GetEffectInfo)(LPDIEFFECTINFOW pdei, REFGUID rguid) override;
    STDMETHOD(GetForceFeedbackState)(LPDWORD pdwOut) override;
    STDMETHOD(SendForceFeedbackCommand)(DWORD dwFlags) override;
    STDMETHOD(EnumCreatedEffectObjects)(LPDIENUMCREATEDEFFECTOBJECTSCALLBACK lpCallback, LPVOID pvRef, DWORD dwFlags) override;
    STDMETHOD(Escape)(LPDIEFFESCAPE lpIEffectEscape) override;
    STDMETHOD(WriteEffectToFile)(LPCWSTR lpszFileName, DWORD dwEntries, LPDIFILEEFFECT rgDiFileEft, DWORD dwFlags) override;
    STDMETHOD(BuildActionMap)(LPDIACTIONFORMATW lpdf, LPCWSTR lpszUserName, DWORD dwFlags) override;
    STDMETHOD(SetActionMap)(LPDIACTIONFORMATW lpdf, LPCWSTR lpszUserName, DWORD dwFlags) override;
    STDMETHOD(GetImageInfo)(LPDIDEVICEIMAGEINFOHEADERW lpdiDeviceImageInfoHeader) override;
    
    // Missing pure virtual methods from IDirectInputDevice8W
    STDMETHOD(Poll)() override;
    STDMETHOD(SendDeviceData)(DWORD cbObjectData, LPCDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD dwFlags) override;
    STDMETHOD(EnumEffectsInFile)(LPCWSTR lpszFileName, LPDIENUMEFFECTSINFILECALLBACK pec, LPVOID pvRef, DWORD dwFlags) override;

private:
    IDirectInputDevice8W* RealDevice;
    ULONG RefCount;
};

#endif // PROXY_DEVICE_H
