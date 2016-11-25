#include <windows.h>

#include "FreeImage.h"
#include "Thumb.h"

#define MAXWH 256
extern HINSTANCE g_hInst;
extern long g_cDllRef;

FileContextMenuExt::FileContextMenuExt(void) : m_cRef(1) {
	InterlockedIncrement(&g_cDllRef);
}

FileContextMenuExt::~FileContextMenuExt(void) {
	InterlockedDecrement(&g_cDllRef);
}

IFACEMETHODIMP FileContextMenuExt::QueryInterface(REFIID riid, void **ppv) {
	static const QITAB qit[] = {
		QITABENT(FileContextMenuExt, IContextMenu),
		QITABENT(FileContextMenuExt, IShellExtInit), 
		{ 0 },
	};
	return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) FileContextMenuExt::AddRef() {
	return InterlockedIncrement(&m_cRef);
}

IFACEMETHODIMP_(ULONG) FileContextMenuExt::Release() {
	ULONG cRef = InterlockedDecrement(&m_cRef);
	if (0 == cRef)
		delete this;
	return cRef;
}

IFACEMETHODIMP FileContextMenuExt::Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hKeyProgID) {
	if (NULL == pDataObj)
		return E_INVALIDARG;
	HRESULT hr = E_FAIL;
	FORMATETC fe = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stm;

	// The pDataObj pointer contains the objects being acted upon.
	// We get an HDROP handle for enumerating the selected files and folders.
	if (SUCCEEDED(pDataObj->GetData(&fe, &stm))) {
		HDROP hDrop = static_cast<HDROP>(GlobalLock(stm.hGlobal));
		if (hDrop != NULL) {
			// Determine how many files are involved in this operation.
			UINT nFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
			if (nFiles == 1) {
				// Get the path of the file.
				if (0 != DragQueryFile(hDrop, 0, m_szSelectedFile, ARRAYSIZE(m_szSelectedFile)))
					hr = S_OK;
			}
			GlobalUnlock(stm.hGlobal);
		}
		ReleaseStgMedium(&stm);
	}
	return hr;
}

IFACEMETHODIMP FileContextMenuExt::QueryContextMenu(HMENU hMenu, UINT indexMenu,
									UINT idCmdFirst, UINT idCmdLast, UINT uFlags) {
	FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeU(m_szSelectedFile);
	if (fif != FIF_UNKNOWN) {
		FIBITMAP *dib = FreeImage_LoadU(fif, m_szSelectedFile, 0);
		if (dib != FALSE) {
			UINT bpp = FreeImage_GetBPP(dib), w = FreeImage_GetWidth(dib), h = FreeImage_GetHeight(dib);

			// hdr dither
			if (bpp > 32) {
				FIBITMAP *orig = dib;
				if (bpp % 64 == 0) {
					dib = FreeImage_ConvertTo32Bits(orig);
				} else {
					dib = FreeImage_ConvertTo24Bits(orig);
				}
				FreeImage_Unload(orig);
			}

			// rescale
			if (w > MAXWH || h > MAXWH) {
				FIBITMAP *orig = dib;
				float ratio = 1.0f * w / h;
				if (w > h) {
					dib = FreeImage_Rescale(orig, MAXWH, (int)(MAXWH / ratio), FILTER_LANCZOS3);
				}
				else if (w < h) {
					dib = FreeImage_Rescale(orig, (int)(MAXWH * ratio), MAXWH, FILTER_LANCZOS3);
				}
				else
					dib = FreeImage_Rescale(orig, MAXWH, MAXWH, FILTER_LANCZOS3);
				FreeImage_Unload(orig);
			}

			// generate thumb
			HBITMAP thumb = CreateDIBitmap(GetWindowDC(NULL), FreeImage_GetInfoHeader(dib), CBM_INIT,
							FreeImage_GetBits(dib), FreeImage_GetInfo(dib), DIB_RGB_COLORS);
			FreeImage_Unload(dib);

			// Add a separator.
			MENUITEMINFO sep = { sizeof(sep) };
			sep.fMask = MIIM_TYPE;
			sep.fType = MFT_SEPARATOR;
			if (!InsertMenuItem(hMenu, indexMenu, TRUE, &sep))
				return HRESULT_FROM_WIN32(GetLastError());

			// Add thumbnails
			if (!InsertMenu(hMenu, indexMenu + 1, MF_BYPOSITION | MF_BITMAP, idCmdFirst, (LPCTSTR)thumb))
				return HRESULT_FROM_WIN32(GetLastError());

			// Add another separator.
			if (!InsertMenuItem(hMenu, indexMenu + 2, TRUE, &sep))
				return HRESULT_FROM_WIN32(GetLastError());

			// Return an HRESULT value with the severity set to SEVERITY_SUCCESS. 
			// Set the code value to the offset of the largest command identifier that was assigned, plus one (1).
			return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 1);
		}
	}
	return E_FAIL;
}

IFACEMETHODIMP FileContextMenuExt::InvokeCommand(LPCMINVOKECOMMANDINFO pici) {
	return S_OK;
}

IFACEMETHODIMP FileContextMenuExt::GetCommandString(UINT_PTR idCommand, 
			UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax) {
	return S_OK;
}
