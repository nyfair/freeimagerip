#ifndef FIHANDLER_H
#define FIHANDLER_H

#include <qimageiohandler.h>

QT_BEGIN_NAMESPACE

class FIHandler : public QImageIOHandler {
public:
	FIHandler(QIODevice *device);
	virtual ~FIHandler();

    bool canRead() const;
    bool read(QImage *image);

    QVariant option(ImageOption option) const;
    bool supportsOption(ImageOption option) const;

    static bool canRead(QIODevice *device);

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
