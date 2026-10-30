#include <windows.h>
#include <Guiddef.h>
#include <shlobj.h>
#include <new>
#include <string>
#include <vector>

#include "FreeImage.h"
#include "ClassFactory.h"
#include "Reg.h"

const CLSID FI_GUID =
{ 0x65E7AC0C, 0xC54C, 0x40F8,{ 0xAA, 0x5D, 0x82, 0x14, 0xED, 0xF3, 0x87, 0xC5 } };

HINSTANCE g_hInst = NULL;
long g_cDllRef = 0;

namespace {
	HRESULT GetCurrentModulePath(std::wstring *path) {
		if (path == NULL)
			return E_POINTER;
		path->clear();
		try {
			std::vector<wchar_t> buffer(MAX_PATH);
			for (;;) {
				const DWORD length = GetModuleFileNameW(g_hInst, buffer.data(),
					static_cast<DWORD>(buffer.size()));
				if (length == 0)
					return HRESULT_FROM_WIN32(GetLastError());
				if (length < buffer.size()) {
					path->assign(buffer.data(), length);
					return S_OK;
				}
				if (buffer.size() >= 32768)
					return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);

				size_t newCapacity = buffer.size() * 2;
				if (newCapacity > 32768)
					newCapacity = 32768;
				buffer.resize(newCapacity);
			}
		} catch (const std::bad_alloc &) {
			return E_OUTOFMEMORY;
		} catch (...) {
			return E_FAIL;
		}
	}
}

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
	if (ppv == NULL)
		return E_POINTER;
	*ppv = NULL;

	HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;
	if (IsEqualCLSID(FI_GUID, rclsid)) {
		hr = E_OUTOFMEMORY;
		ClassFactory *pClassFactory = new (std::nothrow) ClassFactory();
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
	std::wstring modulePath;
	HRESULT hr = GetCurrentModulePath(&modulePath);
	if (FAILED(hr))
		return hr;

	// Register the component.
	hr = RegisterInprocServer(modulePath.c_str(), FI_GUID,
		L"Sentire.Thumbnailer Class",
		L"Apartment");
	if (SUCCEEDED(hr))
		SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
	return hr;
}

STDAPI DllUnregisterServer(void) {
	HRESULT hr = UnregisterInprocServer(FI_GUID);
	if (SUCCEEDED(hr))
		SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
	return hr;
}
