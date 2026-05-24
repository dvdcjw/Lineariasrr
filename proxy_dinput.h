#ifndef PROXY_DINPUT_H
#define PROXY_DINPUT_H

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

class ProxyDirectInput8W : public IDirectInput8W {
public:
    ProxyDirectInput8W(IDirectInput8W* realDInput);
    virtual ~ProxyDirectInput8W();

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObj) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;

    // IDirectInput8W methods
    STDMETHOD(CreateDevice)(REFGUID rguid, LPDIRECTINPUTDEVICE8W* lplpDirectInputDevice, LPUNKNOWN pUnkOuter) override;
    STDMETHOD(EnumDevices)(DWORD dwDevType, LPDIENUMDEVICESCALLBACKW lpCallback, LPVOID pvRef, DWORD dwFlags) override;
    STDMETHOD(GetDeviceStatus)(REFGUID rguidInstance) override;
    STDMETHOD(RunControlPanel)(HWND hwndOwner, DWORD dwFlags) override;
    STDMETHOD(Initialize)(HINSTANCE hinst, DWORD dwVersion) override;
    STDMETHOD(FindDevice)(REFGUID rguidClass, LPCWSTR ptszName, LPGUID pguidInstance) override;
    STDMETHOD(EnumDevicesBySemantics)(LPCWSTR ptszUserName, LPDIACTIONFORMATW lpdiActionFormat, LPDIENUMDEVICESBYSEMANTICSCBW lpCallback, LPVOID pvRef, DWORD dwFlags) override;
    STDMETHOD(ConfigureDevices)(LPDICONFIGUREDEVICESCALLBACK lpdiCallback, LPDICONFIGUREDEVICESPARAMSW lpdiParams, DWORD dwFlags, LPVOID pvRefData) override;

private:
    IDirectInput8W* RealDInput;
    ULONG RefCount;
};

#endif // PROXY_DINPUT_H
