#if defined(_MSC_VER)
#define	_export
#endif

#include <windows.h>
#include "FreeImage.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PictureInfo {
	long left,top;			/* 画像を展開する位置 */
	long width;					/* 画像の幅(pixel) */
	long height;				/* 画像の高さ(pixel) */
	WORD x_density;			/* 画素の水平方向密度 */
	WORD y_density;			/* 画素の垂直方向密度 */
	short colorDepth;		/* 画素当たりのbit数 */
	HLOCAL hInfo;				/* 画像内のテキスト情報[呼び出し側が解放] */
} PictureInfo;

/* Common Function */
int _export PASCAL GetPluginInfo(int infono, LPSTR buf, int buflen);
int _export PASCAL IsSupported(LPSTR filename, DWORD dw);


/* '00IN'の関数 */
int _export PASCAL GetPictureInfo(LPSTR buf, long len, unsigned int flag, PictureInfo *lpInfo);
int _export PASCAL GetPicture(LPSTR buf, long len, unsigned int flag, HANDLE *pHBInfo, HANDLE *pHBm,
								FARPROC lpPrgressCallback, long lData);
int _export PASCAL GetPreview(LPSTR buf, long len,unsigned int flag, HANDLE *pHBInfo, HANDLE *pHBm,
								FARPROC lpPrgressCallback, long lData);

#ifdef __cplusplus
}
#endif
