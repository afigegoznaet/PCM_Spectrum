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

	quint16 rxSize;
	quint16 sizeTransferred;
	//char /*trans_buf*/[10] = { 1,2,3,4,5,6,7,8,9,10 };
	int seccess;
};

class FT2StreamConsumer : public RawFile{
	Q_OBJECT
public:
	explicit FT2StreamConsumer(FT2StreamReader* dataSource, QObject *parent = nullptr);
	~FT2StreamConsumer(){setOpenMode(QIODevice::NotOpen);}
	bool seek(qint64) override{return true;}
	bool open(QIODevice::OpenMode flags) override;

	qint64 readData(char *data, qint64 len) override;
	qint64 bytesAvailable() const override;

	void setData(const std::vector<unsigned char> &tmpBuf);
	void resetBuffer(){internalBuffer.seek(0);}
	void writeData(const std::vector<unsigned char> &tmpBuf);
	qint64 size() const override{
		return LLONG_MAX;
	}
	void close() override{}
signals:
	void finishedReading();
private:
	QBuffer internalBuffer;
};

#endif // FT2STREAMREADER_HPP
