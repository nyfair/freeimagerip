#include "FreeImage.h"
#include "fihandler.h"
#include "qimage.h"
#include "qvariant.h"

QT_BEGIN_NAMESPACE

static unsigned DLL_CALLCONV
ReadProc(void *buffer, unsigned size, unsigned count, fi_handle handle) {
	return static_cast<QIODevice*>(handle)->read((char*)buffer, size *count);
}

static unsigned DLL_CALLCONV
WriteProc(void *buffer, unsigned size, unsigned count, fi_handle handle) {
	QIODevice *quid = static_cast<QIODevice*>(handle);
	return quid->write((char*)buffer, size*count);
}

static int DLL_CALLCONV
SeekProc(fi_handle handle, long offset, int origin) {
	QIODevice *quid = static_cast<QIODevice*>(handle);
	switch (origin) {
	default:
	case SEEK_SET:
		return int(!quid->seek(offset));
	case SEEK_CUR:
		return int(!quid->seek(quid->pos()+offset));
	case SEEK_END:
		if (!quid->isSequential()) {
			quint64 len = quid->bytesAvailable();
			return int(!quid->seek(len+offset));
		}
		break;
	}
	return (-1);
}

static long DLL_CALLCONV
TellProc(fi_handle handle) {
	return static_cast<QIODevice*>(handle)->pos();
}

FIHandler::FIHandler(QIODevice *device) {}

FIHandler::~FIHandler() {
	FreeImage_Unload(dib);
}

bool FIHandler::canRead() const {
	FREE_IMAGE_FORMAT fmt = GetFIF(device(), format());
	return FreeImage_FIFSupportsReading(fmt);
}

bool FIHandler::read(QImage *image) {
	fmt = GetFIF(device(), format());
	dib = FreeImage_LoadFromHandle((FREE_IMAGE_FORMAT)fmt, &fiio(), (fi_handle)device());
	if(!dib) {
		image = new QImage(0, 0, QImage::Format_Invalid);
		return false;
	}

	int width = FreeImage_GetWidth(dib);
	int height = FreeImage_GetHeight(dib);
	imgsize = new QSize(width, height);
	
	switch (FreeImage_GetBPP(dib)) {
	case 1:	{
		qtfmt = height,QImage::Format_Mono;
		QImage result(width,height,QImage::Format_Mono);
		FreeImage_ConvertToRawBits(
			result.scanLine(0), dib, result.bytesPerLine(), 1, 0, 0, 0, true);
		*image = result;
		break;
		}
	case 8:	{
		qtfmt = height,QImage::Format_Indexed8;
		QImage result(width,height,QImage::Format_Indexed8);
		FreeImage_ConvertToRawBits(
			result.scanLine(0), dib, result.bytesPerLine(), 8, 0, 0, 0, true);
		*image = result;
		break;
		}
	case 24: {
		qtfmt = height,QImage::Format_RGB32;
		QImage result(width,height,QImage::Format_RGB32);
		FreeImage_ConvertToRawBits(
			result.scanLine(0), dib, result.bytesPerLine(), 32,
			FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, true);
		*image = result;
		break;
		}
	case 32: {
		qtfmt = height,QImage::Format_ARGB32;
		QImage result(width,height,QImage::Format_ARGB32);
		FreeImage_ConvertToRawBits(
			result.scanLine(0), dib, result.bytesPerLine(), 32,
			FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, true);
		*image = result;
		break;
		}
	}
	return true;
}

QVariant FIHandler::option(ImageOption option) const {
	switch(option) {
	case ImageFormat:
		return qtfmt;
		break;
	case Size:
		return &imgsize;
		break;
	default:
		break;
	}
	return QVariant();
}

bool FIHandler::supportsOption(ImageOption option) const {
	switch(option) {
	case Size:
	case ImageFormat:
		return true;
	default:
		break;
	}
	return false;
}

FreeImageIO& FIHandler::fiio() {
	static FreeImageIO io = {ReadProc, WriteProc, SeekProc, TellProc};
	return io;
}

FREE_IMAGE_FORMAT FIHandler::GetFIF(
		QIODevice *device, const QByteArray &format) {
	FREE_IMAGE_FORMAT fmt = 
		FreeImage_GetFileTypeFromHandle(&fiio(), (fi_handle)device);
	if (fmt == FIF_UNKNOWN)
		fmt = FreeImage_GetFIFFromFilename(format);
	return fmt;
}

QT_END_NAMESPACE
