#include <shlobj.h>
#include <Shlwapi.h>
#pragma once
#pragma comment(lib, "shlwapi.lib")

class FileContextMenuExt : public IShellExtInit, public IContextMenu {
public:
	// IUnknown
	IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv);
	IFACEMETHODIMP_(ULONG) AddRef();
	IFACEMETHODIMP_(ULONG) Release();

	// IShellExtInit
	IFACEMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hKeyProgID);

	// IContextMenu
	IFACEMETHODIMP QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
	IFACEMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pici);
	IFACEMETHODIMP GetCommandString(UINT_PTR idCommand, UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax);
	
	FileContextMenuExt(void);

protected:
	~FileContextMenuExt(void);

private:
	// Reference count of component.
	long m_cRef;

	// The name of the selected file.
	wchar_t m_szSelectedFile[MAX_PATH];
};
