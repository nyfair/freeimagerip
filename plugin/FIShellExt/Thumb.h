#pragma once

#include <shlobj.h>
#include <Shlwapi.h>
#include <string>

#pragma comment(lib, "shlwapi.lib")

class FileContextMenuExt : public IShellExtInit, public IContextMenu3 {
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

	// IContextMenu2 / IContextMenu3: Explorer forwards owner-draw menu messages.
	IFACEMETHODIMP HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam);
	IFACEMETHODIMP HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);

	FileContextMenuExt(void) noexcept;

protected:
	~FileContextMenuExt(void);

private:
	HRESULT MeasureThumbnail(MEASUREITEMSTRUCT *item) const;
	HRESULT DrawThumbnail(const DRAWITEMSTRUCT *item) const;

	volatile long m_cRef;

	std::wstring m_selectedFile;
	HBITMAP m_thumbnail;
};
