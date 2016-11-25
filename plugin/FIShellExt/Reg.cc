#include <windows.h>

#include "Reg.h"

HRESULT SetHKCRRegistryKeyAndValue(PCWSTR pszSubKey, PCWSTR pszValueName, PCWSTR pszData) {
	HKEY hKey = NULL;
	HRESULT hr = HRESULT_FROM_WIN32(RegCreateKeyEx(HKEY_CLASSES_ROOT, pszSubKey, 0,
		NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL));

	if (SUCCEEDED(hr)) {
		if (pszData != NULL) {
			// Set the specified value of the key.
			DWORD cbData = lstrlen(pszData) * sizeof(*pszData);
			hr = HRESULT_FROM_WIN32(RegSetValueEx(hKey, pszValueName, 0, 
				REG_SZ, reinterpret_cast<const BYTE *>(pszData), cbData));
		}
		RegCloseKey(hKey);
	}
	return hr;
}

HRESULT RegisterInprocServer(PCWSTR pszModule, const CLSID& clsid, PCWSTR pszFriendlyName, PCWSTR pszThreadModel) {
	wchar_t szCLSID[MAX_PATH], szSubkey[MAX_PATH];
	StringFromGUID2(clsid, szCLSID, ARRAYSIZE(szCLSID));

	wsprintf(szSubkey, L"CLSID\\%s", szCLSID);
	HRESULT hr = SetHKCRRegistryKeyAndValue(szSubkey, NULL, pszFriendlyName);
	// Create the HKCR\CLSID\{<CLSID>}\InprocServer32 key.
	if (SUCCEEDED(hr)) {
		wsprintf(szSubkey, L"CLSID\\%s\\InprocServer32", szCLSID);
		// Set the default value of the InprocServer32 key to the path of the COM module.
		hr = SetHKCRRegistryKeyAndValue(szSubkey, NULL, pszModule);
		if (SUCCEEDED(hr))
			// Set the threading model of the component.
			hr = SetHKCRRegistryKeyAndValue(szSubkey, 
				L"ThreadingModel", pszThreadModel);
			if (SUCCEEDED(hr))
				// Regist ContextMenuHandlers
				wsprintf(szSubkey, L"*\\shellex\\ContextMenuHandlers\\%s", szCLSID);
				hr = SetHKCRRegistryKeyAndValue(szSubkey, NULL, pszFriendlyName);
	}
	return hr;
}

HRESULT UnregisterInprocServer(const CLSID& clsid) {
	wchar_t szCLSID[MAX_PATH], szSubkey[MAX_PATH];
	StringFromGUID2(clsid, szCLSID, ARRAYSIZE(szCLSID));

	// Delete the HKCR\CLSID\{<CLSID>} key.
	wsprintf(szSubkey, L"CLSID\\%s", szCLSID);
	HRESULT_FROM_WIN32(RegDeleteTree(HKEY_CLASSES_ROOT, szSubkey));
	wsprintf(szSubkey, L"*\\shellex\\ContextMenuHandlers\\%s", szCLSID);
	HRESULT_FROM_WIN32(RegDeleteTree(HKEY_CLASSES_ROOT, szSubkey));
	return S_OK;
}
