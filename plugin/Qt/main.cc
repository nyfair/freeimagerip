#include <QImageIOHandler>

#include "FreeImage.h"
#include "fihandler.h"

QT_BEGIN_NAMESPACE

class QfiPlugin : public QImageIOPlugin {
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QImageIOHandlerFactoryInterface" FILE "fi.json")
public:
	QStringList keys() const;
	QImageIOPlugin::Capabilities capabilities(QIODevice *device, const QByteArray &format) const;
	QImageIOHandler *create(QIODevice *device, const QByteArray &format = QByteArray()) const;
};

QImageIOPlugin::Capabilities QfiPlugin::capabilities(
				QIODevice *device, const QByteArray &format) const {
	Capabilities cap;
	FREE_IMAGE_FORMAT fmt = FIHandler::GetFIF(device, format);
	if (FreeImage_FIFSupportsReading(fmt))
		cap |= CanRead;
	if (FreeImage_FIFSupportsWriting(fmt))
		cap |= CanWrite;
	return cap;
}

QImageIOHandler *QfiPlugin::create(
				QIODevice *device, const QByteArray &format) const {
	FIHandler *hand = new FIHandler(device);
	hand->setFormat(format);
	return hand;
}

QStringList QfiPlugin::keys() const {
	return QStringList() << QLatin1String("FreeImage");
}

QT_END_NAMESPACE

#include "main.moc"
