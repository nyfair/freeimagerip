#include <windows.h>
#include <strsafe.h>

#include "Reg.h"

HRESULT SetHKCRRegistryKeyAndValue(PCWSTR pszSubKey, PCWSTR pszValueName, PCWSTR pszData) {
	HKEY hKey = NULL;
	HRESULT hr = HRESULT_FROM_WIN32(RegCreateKeyEx(HKEY_CLASSES_ROOT, pszSubKey, 0,
		NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL));

	if (SUCCEEDED(hr)) {
		if (pszData != NULL) {
			// Set the specified value of the key.
			DWORD cbData = (lstrlenW(pszData) + 1) * sizeof(*pszData);
			hr = HRESULT_FROM_WIN32(RegSetValueEx(hKey, pszValueName, 0,
				REG_SZ, reinterpret_cast<const BYTE *>(pszData), cbData));
		}
		RegCloseKey(hKey);
	}
	return hr;
}

HRESULT RegisterInprocServer(PCWSTR pszModule, const CLSID& clsid, PCWSTR pszFriendlyName, PCWSTR pszThreadModel) {
	wchar_t szCLSID[MAX_PATH], szSubkey[MAX_PATH];
	if (StringFromGUID2(clsid, szCLSID, ARRAYSIZE(szCLSID)) == 0)
		return E_FAIL;

	HRESULT hr = StringCchPrintfW(szSubkey, ARRAYSIZE(szSubkey), L"CLSID\\%s", szCLSID);
	if (FAILED(hr))
		return hr;
	hr = SetHKCRRegistryKeyAndValue(szSubkey, NULL, pszFriendlyName);
	// Create the HKCR\CLSID\{<CLSID>}\InprocServer32 key.
	if (SUCCEEDED(hr)) {
		hr = StringCchPrintfW(szSubkey, ARRAYSIZE(szSubkey), L"CLSID\\%s\\InprocServer32", szCLSID);
	}
	if (SUCCEEDED(hr)) {
		// Set the default value of the InprocServer32 key to the path of the COM module.
		hr = SetHKCRRegistryKeyAndValue(szSubkey, NULL, pszModule);
	}
	if (SUCCEEDED(hr))
		hr = SetHKCRRegistryKeyAndValue(szSubkey, L"ThreadingModel", pszThreadModel);
	if (SUCCEEDED(hr))
		hr = StringCchPrintfW(szSubkey, ARRAYSIZE(szSubkey),
			L"*\\shellex\\ContextMenuHandlers\\%s", szCLSID);
	if (SUCCEEDED(hr))
		hr = SetHKCRRegistryKeyAndValue(szSubkey, NULL, szCLSID);

	if (FAILED(hr))
		UnregisterInprocServer(clsid);
	return hr;
}

HRESULT UnregisterInprocServer(const CLSID& clsid) {
	wchar_t szCLSID[MAX_PATH], szSubkey[MAX_PATH];
	if (StringFromGUID2(clsid, szCLSID, ARRAYSIZE(szCLSID)) == 0)
		return E_FAIL;

	HRESULT hr = StringCchPrintfW(szSubkey, ARRAYSIZE(szSubkey),
		L"*\\shellex\\ContextMenuHandlers\\%s", szCLSID);
	if (FAILED(hr))
		return hr;
	LSTATUS status = RegDeleteTreeW(HKEY_CLASSES_ROOT, szSubkey);
	if (status != ERROR_SUCCESS && status != ERROR_FILE_NOT_FOUND)
		hr = HRESULT_FROM_WIN32(status);

	HRESULT formatHr = StringCchPrintfW(szSubkey, ARRAYSIZE(szSubkey), L"CLSID\\%s", szCLSID);
	if (FAILED(formatHr))
		return formatHr;
	status = RegDeleteTreeW(HKEY_CLASSES_ROOT, szSubkey);
	if (SUCCEEDED(hr) && status != ERROR_SUCCESS && status != ERROR_FILE_NOT_FOUND)
		hr = HRESULT_FROM_WIN32(status);
	return hr;
}
