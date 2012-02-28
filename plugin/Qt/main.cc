#include "FreeImage.h"
#include "fihandler.h"

QT_BEGIN_NAMESPACE

class QfiPlugin : public QImageIOPlugin {
public:
	QStringList keys() const;
	Capabilities capabilities(QIODevice *device, const QByteArray &format) const;
	QImageIOHandler *create(QIODevice *device,
							const QByteArray &format = QByteArray()) const;
};

QStringList QfiPlugin::keys() const {
	return QStringList() << QLatin1String("FreeImage");
}

QImageIOPlugin::Capabilities QfiPlugin::capabilities(
				QIODevice *device, const QByteArray &format) const {
	Capabilities cap;
	FREE_IMAGE_FORMAT fmt = fiHandler::GetFIF(device, format);
	if (FreeImage_FIFSupportsReading(fmt))
		cap |= CanRead;
	if (FreeImage_FIFSupportsWriting(fmt))
		cap |= CanWrite;
	return cap;
}

QImageIOHandler *QfiPlugin::create(
				QIODevice *device, const QByteArray &format) const {
	fiHandler *hand = new fiHandler();
	hand->setDevice(device);
	hand->setFormat(format);
	return hand;
}

Q_EXPORT_STATIC_PLUGIN(QfiPlugin)
Q_EXPORT_PLUGIN2(qfi, QfiPlugin)

QT_END_NAMESPACE
