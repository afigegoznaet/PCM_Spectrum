#ifndef FT2STREAMREADER_HPP
#define FT2STREAMREADER_HPP

#include <QBuffer>
#include "wavfile.h"
#include <vector>
#include <QFuture>

class QFile;
class FT2StreamConsumer;
class FT2StreamReader : public QObject
{
	Q_OBJECT
public:
	explicit FT2StreamReader(QObject *parent = nullptr);

	void addListener(FT2StreamConsumer* listener);

public slots:
	void readStream();
private:
	std::vector<unsigned char> tmpBuf;
	std::vector<FT2StreamConsumer*> listeners;
	QFuture<void> streamReader;
	QFile* rawSample;
};

class FT2StreamConsumer : public RawFile{
	Q_OBJECT
public:
	explicit FT2StreamConsumer(FT2StreamReader* dataSource, QObject *parent = nullptr);
	bool seek(qint64) override{return true;}
	bool open(QIODevice::OpenMode flags) override;

	qint64 readData(char *data, qint64 len) override;
	qint64 bytesAvailable() const override;

	void setData(std::vector<unsigned char> tmpBuf);
	void resetBuffer(){internalBuffer.seek(0);}
	void writeData(std::vector<unsigned char> tmpBuf);
	qint64 size() const override{
		return 0;
	}
signals:
	void finishedReading();
private:
	QBuffer internalBuffer;
};

#endif // FT2STREAMREADER_HPP
