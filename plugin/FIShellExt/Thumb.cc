#include <windows.h>
#include <limits.h>
#include <stdint.h>
#include <uxtheme.h>
#include <vssym32.h>

#include "FreeImage.h"
#include "Thumb.h"

#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "uxtheme.lib")

namespace {
	const unsigned kMaxThumbnailSize = 480;
	const LONG kPreviewPadding = 4;

	bool GetThumbnailSize(HBITMAP thumbnail, SIZE *size) {
		BITMAP bitmap = {};
		if (thumbnail == NULL || GetObjectW(thumbnail, sizeof(bitmap), &bitmap) != sizeof(bitmap)
			|| bitmap.bmWidth <= 0 || bitmap.bmHeight <= 0)
			return false;
		size->cx = bitmap.bmWidth;
		size->cy = bitmap.bmHeight;
		return true;
	}

	HBITMAP CreateThumbnailBitmap(PCWSTR path) {
		FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeU(path, 0);
		if (fif == FIF_UNKNOWN)
			fif = FreeImage_GetFIFFromFilenameU(path);
		if (fif == FIF_UNKNOWN || !FreeImage_FIFSupportsReading(fif))
			return NULL;

		const int loadFlags = (fif == FIF_JPEG) ? JPEG_EXIFROTATE : 0;
		FIBITMAP *dib = FreeImage_LoadU(fif, path, loadFlags);
		if (dib == NULL)
			return NULL;

		FIBITMAP *converted = FreeImage_ConvertTo32Bits(dib);
		FreeImage_Unload(dib);
		if (converted == NULL)
			return NULL;
		dib = converted;

		unsigned width = FreeImage_GetWidth(dib);
		unsigned height = FreeImage_GetHeight(dib);
		if (width == 0 || height == 0 || width > LONG_MAX || height > LONG_MAX) {
			FreeImage_Unload(dib);
			return NULL;
		}

		if (width > kMaxThumbnailSize || height > kMaxThumbnailSize) {
			unsigned targetWidth;
			unsigned targetHeight;
			if (width >= height) {
				targetWidth = kMaxThumbnailSize;
				targetHeight = static_cast<unsigned>(
					(static_cast<uint64_t>(height) * kMaxThumbnailSize) / width);
			} else {
				targetWidth = static_cast<unsigned>(
					(static_cast<uint64_t>(width) * kMaxThumbnailSize) / height);
				targetHeight = kMaxThumbnailSize;
			}
			if (targetWidth == 0)
				targetWidth = 1;
			if (targetHeight == 0)
				targetHeight = 1;

			FIBITMAP *scaled = FreeImage_Rescale(dib, targetWidth, targetHeight, FILTER_LANCZOS3);
			FreeImage_Unload(dib);
			if (scaled == NULL)
				return NULL;
			dib = scaled;
			width = FreeImage_GetWidth(dib);
			height = FreeImage_GetHeight(dib);
		}

		BITMAPINFO bitmapInfo = {};
		bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bitmapInfo.bmiHeader.biWidth = static_cast<LONG>(width);
		bitmapInfo.bmiHeader.biHeight = static_cast<LONG>(height);
		bitmapInfo.bmiHeader.biPlanes = 1;
		bitmapInfo.bmiHeader.biBitCount = 32;
		bitmapInfo.bmiHeader.biCompression = BI_RGB;

		const unsigned sourcePitch = FreeImage_GetPitch(dib);
		const BYTE *sourceBits = FreeImage_GetBits(dib);
		const size_t targetPitch = static_cast<size_t>(width) * 4;
		if (sourceBits == NULL || sourcePitch < targetPitch) {
			FreeImage_Unload(dib);
			return NULL;
		}

		// AlphaBlend expects a 32-bit DIB with premultiplied BGRA.
		BYTE *targetBits = NULL;
		HBITMAP bitmap = CreateDIBSection(NULL, &bitmapInfo, DIB_RGB_COLORS,
			reinterpret_cast<void **>(&targetBits), NULL, 0);
		if (bitmap != NULL && targetBits == NULL) {
			DeleteObject(bitmap);
			bitmap = NULL;
		}
		if (bitmap != NULL && targetBits != NULL) {
			for (unsigned y = 0; y < height; ++y) {
				const BYTE *source = sourceBits + static_cast<size_t>(y) * sourcePitch;
				BYTE *target = targetBits + static_cast<size_t>(y) * targetPitch;
				for (unsigned x = 0; x < width; ++x) {
					const BYTE alpha = source[FI_RGBA_ALPHA];
					target[0] = static_cast<BYTE>((source[FI_RGBA_BLUE] * alpha + 127) / 255);
					target[1] = static_cast<BYTE>((source[FI_RGBA_GREEN] * alpha + 127) / 255);
					target[2] = static_cast<BYTE>((source[FI_RGBA_RED] * alpha + 127) / 255);
					target[3] = alpha;
					source += 4;
					target += 4;
				}
			}
		}

		FreeImage_Unload(dib);
		return bitmap;
	}

	HRESULT CommandCount(USHORT count) {
		return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, count);
	}
}

extern long g_cDllRef;

FileContextMenuExt::FileContextMenuExt(void) noexcept
	: m_cRef(1), m_selectedFile(NULL), m_thumbnail(NULL) {
	InterlockedIncrement(&g_cDllRef);
}

FileContextMenuExt::~FileContextMenuExt(void) {
	if (m_selectedFile != NULL)
		HeapFree(GetProcessHeap(), 0, m_selectedFile);
	if (m_thumbnail != NULL)
		DeleteObject(m_thumbnail);
	InterlockedDecrement(&g_cDllRef);
}

IFACEMETHODIMP FileContextMenuExt::QueryInterface(REFIID riid, void **ppv) {
	static const QITAB qit[] = {
		QITABENT(FileContextMenuExt, IContextMenu),
		QITABENT(FileContextMenuExt, IContextMenu2),
		QITABENT(FileContextMenuExt, IContextMenu3),
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
	if (m_selectedFile != NULL) {
		HeapFree(GetProcessHeap(), 0, m_selectedFile);
		m_selectedFile = NULL;
	}
	if (m_thumbnail != NULL) {
		DeleteObject(m_thumbnail);
		m_thumbnail = NULL;
	}
	if (NULL == pDataObj)
		return E_INVALIDARG;

	HRESULT hr = E_FAIL;
	FORMATETC fe = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stm = {};

	// The pDataObj pointer contains the objects being acted upon.
	// We get an HDROP handle for enumerating the selected files and folders.
	if (SUCCEEDED(pDataObj->GetData(&fe, &stm))) {
		HDROP hDrop = static_cast<HDROP>(GlobalLock(stm.hGlobal));
		if (hDrop != NULL) {
			UINT nFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
			if (nFiles == 1) {
				const UINT pathLength = DragQueryFileW(hDrop, 0, NULL, 0);
				if (pathLength >= 32768) {
					hr = HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
				} else if (pathLength != 0) {
					wchar_t *path = static_cast<wchar_t *>(HeapAlloc(GetProcessHeap(), 0,
						(static_cast<size_t>(pathLength) + 1) * sizeof(wchar_t)));
					if (path == NULL) {
						hr = E_OUTOFMEMORY;
					} else {
						if (DragQueryFileW(hDrop, 0, path, pathLength + 1) == pathLength) {
							m_selectedFile = path;
							hr = S_OK;
						} else {
							HeapFree(GetProcessHeap(), 0, path);
						}
					}
				}
			}
			GlobalUnlock(stm.hGlobal);
		}
		ReleaseStgMedium(&stm);
	}
	return hr;
}

IFACEMETHODIMP FileContextMenuExt::QueryContextMenu(HMENU hMenu, UINT indexMenu,
									UINT idCmdFirst, UINT idCmdLast, UINT uFlags) {
	if ((uFlags & CMF_DEFAULTONLY) != 0 || m_selectedFile == NULL)
		return CommandCount(0);
	if (idCmdFirst > idCmdLast)
		return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

	if (m_thumbnail == NULL)
		m_thumbnail = CreateThumbnailBitmap(m_selectedFile);
	if (m_thumbnail == NULL)
		return CommandCount(0);
	SIZE thumbnailSize;
	if (!GetThumbnailSize(m_thumbnail, &thumbnailSize))
		return E_FAIL;

	if (!InsertMenuW(hMenu, indexMenu, MF_BYPOSITION | MF_SEPARATOR, 0, NULL))
		return HRESULT_FROM_WIN32(GetLastError());

	MENUITEMINFOW item = {};
	item.cbSize = sizeof(item);
	// hbmpItem reserves a shared icon column across the entire menu. A large
	// preview belongs to its own full-width row instead, so draw only this item.
	item.fMask = MIIM_FTYPE | MIIM_DATA | MIIM_ID | MIIM_STATE;
	item.fType = MFT_OWNERDRAW;
	// This is a non-interactive preview. Our drawing preserves its original colors.
	item.fState = MFS_DISABLED;
	item.wID = idCmdFirst;
	item.dwItemData = reinterpret_cast<ULONG_PTR>(this);
	if (!InsertMenuItemW(hMenu, indexMenu + 1, TRUE, &item)) {
		const DWORD error = GetLastError();
		RemoveMenu(hMenu, indexMenu, MF_BYPOSITION);
		return HRESULT_FROM_WIN32(error);
	}

	if (!InsertMenuW(hMenu, indexMenu + 2, MF_BYPOSITION | MF_SEPARATOR, 0, NULL)) {
		const DWORD error = GetLastError();
		RemoveMenu(hMenu, indexMenu + 1, MF_BYPOSITION);
		RemoveMenu(hMenu, indexMenu, MF_BYPOSITION);
		return HRESULT_FROM_WIN32(error);
	}

	return CommandCount(1);
}

HRESULT FileContextMenuExt::MeasureThumbnail(MEASUREITEMSTRUCT *item) const {
	SIZE size;
	if (!GetThumbnailSize(m_thumbnail, &size))
		return E_FAIL;
	item->itemWidth = static_cast<UINT>(size.cx + kPreviewPadding * 2);
	item->itemHeight = static_cast<UINT>(size.cy + kPreviewPadding * 2);
	return S_OK;
}

HRESULT FileContextMenuExt::DrawThumbnail(const DRAWITEMSTRUCT *item) const {
	SIZE size;
	if (item->hDC == NULL || !GetThumbnailSize(m_thumbnail, &size))
		return E_FAIL;

	const RECT &rect = item->rcItem;
	const LONG availableWidth = rect.right - rect.left - kPreviewPadding * 2;
	const LONG availableHeight = rect.bottom - rect.top - kPreviewPadding * 2;
	if (availableWidth <= 0 || availableHeight <= 0)
		return S_OK;

	LONG width = size.cx;
	LONG height = size.cy;
	if (width > availableWidth) {
		height = MulDiv(height, availableWidth, width);
		width = availableWidth;
	}
	if (height > availableHeight) {
		width = MulDiv(width, availableHeight, height);
		height = availableHeight;
	}
	if (width < 1)
		width = 1;
	if (height < 1)
		height = 1;

	const int savedDC = SaveDC(item->hDC);
	if (savedDC == 0)
		return E_FAIL;
	if (IntersectClipRect(item->hDC, rect.left, rect.top, rect.right, rect.bottom) == ERROR) {
		RestoreDC(item->hDC, savedDC);
		return E_FAIL;
	}

	// Paint a normal menu background even though the preview cannot be selected.
	// Alpha pixels then blend against the actual theme, not a baked-in white fill.
	FillRect(item->hDC, &rect, GetSysColorBrush(COLOR_MENU));
	HTHEME theme = OpenThemeData(NULL, L"MENU");
	if (theme != NULL) {
		DrawThemeBackground(theme, item->hDC, MENU_POPUPBACKGROUND, 0, &rect, NULL);
		CloseThemeData(theme);
	}

	HDC sourceDC = CreateCompatibleDC(item->hDC);
	if (sourceDC == NULL) {
		RestoreDC(item->hDC, savedDC);
		return E_FAIL;
	}
	HGDIOBJ previousBitmap = SelectObject(sourceDC, m_thumbnail);
	if (previousBitmap == NULL || previousBitmap == HGDI_ERROR) {
		DeleteDC(sourceDC);
		RestoreDC(item->hDC, savedDC);
		return E_FAIL;
	}

	const LONG x = rect.left + (rect.right - rect.left - width) / 2;
	const LONG y = rect.top + (rect.bottom - rect.top - height) / 2;
	const BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
	const BOOL drawn = AlphaBlend(item->hDC, x, y, width, height,
		sourceDC, 0, 0, size.cx, size.cy, blend);

	SelectObject(sourceDC, previousBitmap);
	DeleteDC(sourceDC);
	RestoreDC(item->hDC, savedDC);
	return drawn ? S_OK : E_FAIL;
}

IFACEMETHODIMP FileContextMenuExt::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam) {
	return HandleMenuMsg2(uMsg, wParam, lParam, NULL);
}

IFACEMETHODIMP FileContextMenuExt::HandleMenuMsg2(UINT uMsg, WPARAM, LPARAM lParam, LRESULT *plResult) {
	if (plResult != NULL)
		*plResult = 0;

	HRESULT hr;
	switch (uMsg) {
	case WM_MEASUREITEM: {
		if (lParam == 0)
			return E_INVALIDARG;
		MEASUREITEMSTRUCT *item = reinterpret_cast<MEASUREITEMSTRUCT *>(lParam);
		if (item->CtlType != ODT_MENU || item->itemData != reinterpret_cast<ULONG_PTR>(this))
			return E_NOTIMPL;
		hr = MeasureThumbnail(item);
		break;
	}
	case WM_DRAWITEM: {
		if (lParam == 0)
			return E_INVALIDARG;
		const DRAWITEMSTRUCT *item = reinterpret_cast<const DRAWITEMSTRUCT *>(lParam);
		if (item->CtlType != ODT_MENU || item->itemData != reinterpret_cast<ULONG_PTR>(this))
			return E_NOTIMPL;
		hr = DrawThumbnail(item);
		break;
	}
	default:
		return E_NOTIMPL;
	}

	if (SUCCEEDED(hr) && plResult != NULL)
		*plResult = TRUE;
	return hr;
}

IFACEMETHODIMP FileContextMenuExt::InvokeCommand(LPCMINVOKECOMMANDINFO pici) {
	if (pici == NULL)
		return E_POINTER;
	if (HIWORD(pici->lpVerb) != 0 || LOWORD(pici->lpVerb) != 0)
		return E_FAIL;
	return S_OK;
}

IFACEMETHODIMP FileContextMenuExt::GetCommandString(UINT_PTR idCommand,
			UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax) {
	return E_NOTIMPL;
}
