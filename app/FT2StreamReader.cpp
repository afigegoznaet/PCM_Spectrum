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
	//listener->setData(tmpBuf);
	connect(listeners[0], SIGNAL(finishedReading()), this, SLOT(readStream()),
			Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
}


void FT2StreamReader::readStream(){

	std::lock_guard<std::mutex> locker(rwMutex);
	qint64 res = rawSample->read(reinterpret_cast<char*>(&tmpBuf[0]),BUFFER_SIZE);
	qDebug()<<"Res: "<<res;
	if(res > 0)
	for(auto listener : listeners)
		//listener->resetBuffer();
		listener->writeData(tmpBuf);

	qDebug()<<"Read chunk";
	qDebug()<<"tmpsize: "<<tmpBuf.size();
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
	qDebug()<<"Internal: "<<&internalBuffer.data();
	return internalBuffer.open(flags);
}



qint64 FT2StreamConsumer::readData(char *data, qint64 len){
	qDebug()<<"pos: "<<internalBuffer.pos();
	std::lock_guard<std::mutex> locker(rwMutex);
	qDebug()<<"readData";
	qDebug()<<"len: "<<len;
	qint64 res = internalBuffer.read(data, len);
	auto hz = internalBuffer.data();
	//resetBuffer();
	//qDebug()<<"data: "<<data;
	qDebug()<<"bufsize: "<<internalBuffer.size();
	qDebug()<<"Data: "<<internalBuffer.data().data();
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

void FT2StreamConsumer::writeData(std::vector<unsigned char> tmpBuf){
	internalBuffer.seek(0);
	internalBuffer.write(reinterpret_cast<const char*>(&tmpBuf[0]),
			static_cast<int>(tmpBuf.size()));
	internalBuffer.seek(0);
}


