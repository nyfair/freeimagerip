#ifndef FIHANDLER_H
#define FIHANDLER_H

#include <QtGui/qimageiohandler.h>

QT_BEGIN_NAMESPACE

class QImage;
class QByteArray;
class QIODevice;
class QVariant;
class fiHandlerPrivate;

class fiHandler : public QImageIOHandler {
public:
	fiHandler();
	~fiHandler();
	virtual bool canRead() const;
	virtual QByteArray name() const;
	virtual bool read(QImage *image);
	//virtual bool write(const QImage &image);
	virtual QVariant option(ImageOption option) const;
	virtual bool supportsOption(ImageOption option) const;

	static FreeImageIO& fiio();
	static FREE_IMAGE_FORMAT GetFIF(QIODevice *device, const QByteArray &fmt);
private:
	FIBITMAP *dib;
	QSize *imgsize;
	int fmt;
	int qtfmt;
};

QT_END_NAMESPACE

#endif
