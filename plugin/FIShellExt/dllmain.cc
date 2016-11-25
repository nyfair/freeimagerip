#include <windows.h>
#include <Guiddef.h>
#include <shlobj.h>

#include "FreeImage.h"
#include "ClassFactory.h"
#include "Reg.h"

const CLSID FI_GUID =
{ 0x65E7AC0C, 0xC54C, 0x40F8,{ 0xAA, 0x5D, 0x82, 0x14, 0xED, 0xF3, 0x87, 0xC5 } };

HINSTANCE g_hInst = NULL;
long g_cDllRef = 0;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		g_hInst = hModule;
		DisableThreadLibraryCalls(hModule);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv) {
	HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;
	if (IsEqualCLSID(FI_GUID, rclsid)) {
		hr = E_OUTOFMEMORY;
		ClassFactory *pClassFactory = new ClassFactory();
		if (pClassFactory) {
			hr = pClassFactory->QueryInterface(riid, ppv);
			pClassFactory->Release();
		}
	}
	return hr;
}

STDAPI DllCanUnloadNow(void) {
	return g_cDllRef > 0 ? S_FALSE : S_OK;
}

STDAPI DllRegisterServer(void) {
	HRESULT hr;
	wchar_t szModule[MAX_PATH];
	if (GetModuleFileName(g_hInst, szModule, ARRAYSIZE(szModule)) == 0) {
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}

	// Register the component.
	hr = RegisterInprocServer(szModule, FI_GUID,
		L"Sentire.Thumbnailer Class",
		L"Apartment");
	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
	SystemParametersInfo(SPI_SETICONS, 0, NULL, SPIF_SENDCHANGE);
	return hr;
}

STDAPI DllUnregisterServer(void) {
	HRESULT hr = S_OK;
	wchar_t szModule[MAX_PATH];
	if (GetModuleFileName(g_hInst, szModule, ARRAYSIZE(szModule)) == 0) {
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}

	// Unregister the component.
	hr = UnregisterInprocServer(FI_GUID);
	CoFreeUnusedLibraries();
	return hr;
}
