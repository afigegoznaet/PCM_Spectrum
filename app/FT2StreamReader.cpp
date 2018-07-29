#include "FT2StreamReader.hpp"
#include <ftd2xx.h>
#include <LibFT4222.h>
#include <QDebug>
#include <QFile>
#include <QtConcurrent/QtConcurrent>
#include <mutex>

#define BUFFER_SIZE 16384

std::mutex rwMutex;

FT2StreamReader::FT2StreamReader(QObject *parent) :
	QObject (parent)
{
	tmpBuf.resize(BUFFER_SIZE);
	rawSample = new QFile(":/raw_sample.raw", this);
	rawSample->open(QIODevice::ReadOnly);
}

void FT2StreamReader::addListener(FT2StreamConsumer* listener){
	listeners.push_back(listener);
	listener->setData(tmpBuf);
	connect(listener, SIGNAL(finishedReading()), this, SLOT(readStream()),
			Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
}


void FT2StreamReader::readStream(){

	std::lock_guard<std::mutex> locker(rwMutex);
	int res = rawSample->read(reinterpret_cast<char*>(&tmpBuf[0]),BUFFER_SIZE);
	if(res > 0)
	for(auto listener : listeners)
		listener->resetBuffer();
	qDebug()<<rawSample->pos();
	qDebug()<<"Read chunk";

}

FT2StreamConsumer::FT2StreamConsumer(FT2StreamReader* dataSource, QObject *parent)
	: RawFile (parent)
{
	dataSource->addListener(this);

}

bool FT2StreamConsumer::open(QIODevice::OpenMode flags){
	qDebug()<<"Open";
	qDebug()<<flags;
	setOpenMode(QIODevice::ReadWrite);
	qDebug()<<isOpen();
	return internalBuffer.open(flags);
}



qint64 FT2StreamConsumer::readData(char *data, qint64 len){
	std::lock_guard<std::mutex> locker(rwMutex);
	qDebug()<<"readData";
	qDebug()<<"len: "<<len;
	int res = internalBuffer.read(data, len);
	qDebug()<<"data: "<<data;
	qDebug()<<res;
	emit finishedReading();
	return res;
}

qint64 FT2StreamConsumer::bytesAvailable() const{
	qDebug()<<"bytes available";
	return internalBuffer.bytesAvailable();
}

void FT2StreamConsumer::setData(std::vector<unsigned char> tmpBuf){
	internalBuffer.setData(reinterpret_cast<const char*>(&tmpBuf[0]),
			static_cast<int>(tmpBuf.size()));
}
