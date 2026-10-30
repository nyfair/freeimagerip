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

namespace {
	// On success the caller owns the returned process-heap buffer.
	HRESULT GetCurrentModulePath(wchar_t **path) {
		if (path == NULL)
			return E_POINTER;
		*path = NULL;
		DWORD capacity = MAX_PATH;
		wchar_t *buffer = static_cast<wchar_t *>(HeapAlloc(GetProcessHeap(), 0,
			static_cast<size_t>(capacity) * sizeof(wchar_t)));
		if (buffer == NULL)
			return E_OUTOFMEMORY;

		HRESULT hr;
		for (;;) {
			const DWORD length = GetModuleFileNameW(g_hInst, buffer, capacity);
			if (length == 0) {
				hr = HRESULT_FROM_WIN32(GetLastError());
				break;
			}
			if (length < capacity) {
				*path = buffer;
				return S_OK;
			}
			if (capacity >= 32768) {
				hr = HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
				break;
			}

			DWORD newCapacity = capacity * 2;
			if (newCapacity > 32768)
				newCapacity = 32768;
			wchar_t *resized = static_cast<wchar_t *>(HeapReAlloc(GetProcessHeap(), 0,
				buffer, static_cast<size_t>(newCapacity) * sizeof(wchar_t)));
			if (resized == NULL) {
				hr = E_OUTOFMEMORY;
				break;
			}
			buffer = resized;
			capacity = newCapacity;
		}
		HeapFree(GetProcessHeap(), 0, buffer);
		return hr;
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
	wchar_t *modulePath = NULL;
	HRESULT hr = GetCurrentModulePath(&modulePath);
	if (FAILED(hr))
		return hr;

	// Register the component.
	hr = RegisterInprocServer(modulePath, FI_GUID,
		L"Sentire.Thumbnailer Class",
		L"Apartment");
	HeapFree(GetProcessHeap(), 0, modulePath);
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
