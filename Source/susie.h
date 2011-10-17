#ifndef SPI_FI_H
#define SPI_FI_H

#ifdef __cplusplus
extern "C" {
#endif 

typedef struct PictureInfo {
	long left, top;			/* 画像を展開する位置 */
	long width;					/* 画像の幅(pixel) */
	long height;				/* 画像の高さ(pixel) */
	WORD x_density;			/* 画素の水平方向密度 */
	WORD y_density;			/* 画素の垂直方向密度 */
	short colorDepth;		/* 画素当たりのbit数 */
	HLOCAL hInfo;				/* 画像内のテキスト情報[呼び出し側が解放] */
} PictureInfo;

/*-------------------------------------------------------------------------*/
/* エラーコード */
/*-------------------------------------------------------------------------*/
#define SPI_NO_FUNCTION				-1	/* その機能はインプリメントされていない */
#define SPI_ALL_RIGHT					0	/* 正常終了 */
#define SPI_ABORT							1	/* コールバック関数が非0を返したので展開を中止した */
#define SPI_NOT_SUPPORT				2	/* 未知のフォーマット */
#define SPI_OUT_OF_ORDER			3	/* データが壊れている */
#define SPI_NO_MEMORY					4	/* メモリーが確保出来ない */
#define SPI_MEMORY_ERROR			5	/* メモリーエラー */
#define SPI_FILE_READ_ERROR		6	/* ファイルリードエラー */
#define	SPI_WINDOW_ERROR			7	/* 窓が開けない (非公開のエラーコード) */
#define SPI_OTHER_ERROR				8	/* 内部エラー */
#define	SPI_FILE_WRITE_ERROR	9	/* 書き込みエラー (非公開のエラーコード) */
#define	SPI_END_OF_FILE				10/* ファイル終端 (非公開のエラーコード) */

static const char *pluginfo[] = {
	"00IN",
	"FreeImage Susie Plugin by nyfair <nyfair2012@gmail.com>",
	"*.jpg;*.png;*.bmp;*.gif;*.jpeg;*.tga;*.tiff;*.webp;*.wdp;\
	*.psd;*.ico;*.hdr;*.jxr;*.tif;*.hdp",
	"View image with freeimage.dll",
};

const unsigned int infosize = sizeof(BITMAPINFOHEADER);
typedef int (CALLBACK *SPI_PROGRESS)(int, int, long);

/* Common Function */
int DLL_API WINAPI GetPluginInfo(int infono, LPSTR buf, int buflen);
int DLL_API WINAPI IsSupported(LPSTR filename, DWORD dw);

/* '00IN'の関数 */
int DLL_API WINAPI GetPictureInfo(LPSTR buf, long len, 
																	unsigned int flag, PictureInfo *lpInfo);
int DLL_API WINAPI GetPicture(LPSTR buf, long len, unsigned int flag,
															HANDLE *pHBInfo, HANDLE *pHBm,
															SPI_PROGRESS lpPrgressCallback, long lData);
int DLL_API WINAPI GetPreview(LPSTR buf, long len, unsigned int flag,
															HANDLE *pHBInfo, HANDLE *pHBm,
															SPI_PROGRESS lpPrgressCallback, long lData);

#ifdef __cplusplus
}
#endif 

#endif	/* SPI_FI_H */
