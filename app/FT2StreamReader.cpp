#include "FT2StreamReader.hpp"
#include <QDebug>
#include <QFile>
#include <QtConcurrent/QtConcurrent>
#include <mutex>

#define BUFFER_SIZE 16384

//#define BUFFER_SIZE 4096

std::mutex rwMutex;

#define READ_FROM_FILE

#ifndef READ_FROM_FILE

#include <ftd2xx.h>
#include <LibFT4222.h>

FT_HANDLE ftHandle = nullptr;
std::vector< FT_DEVICE_LIST_INFO_NODE > g_FTAllDevList;
std::vector< FT_DEVICE_LIST_INFO_NODE > g_FT4222DevList;

int Rxbuffer[10];
int Pattern[10] = { 1,2,3,4,5,101,102,103,104,105 };
int test_counter = 0;

void InitRxbuffer() {
	for (int i = 0;i < 10;i++) {
		Rxbuffer[i] = 0;
	}
}

int CheckData(std::vector< unsigned char > myvector) {
	int seccess = 1;
	static int char_counter = 0;
	static int num = 0;
	static int Rx_counter = 0;
	unsigned char c;

	for (std::vector<unsigned char>::iterator it = myvector.begin(); it != myvector.end(); ++it) {
		c = *it;
		num |= (c << char_counter * 8);
		char_counter++;
		if (char_counter == 4) {
			Rxbuffer[Rx_counter] = num;
			num = 0;
			char_counter = 0;
			Rx_counter++;
		}

		if (Rx_counter == 10) {
			test_counter++;
			if (memcmp(Rxbuffer, Pattern, 10 * sizeof(int)))
				seccess = 0;
			//for (int i = 0; i < 10; i++) {
			//	printf("%d ", Rxbuffer[i]);
			//}
			Rx_counter = 0;
			InitRxbuffer();
		}
	}

	return seccess;
}

void WriteDataToFile(FILE *fp,unsigned char c) {
	static int i = 0;
	static int num = 0;
	//printf("%x ", c);
	num |= (c << i*8);
	i++;
	if (i == 4) {
		//printf("%x\n", num);
		fprintf(fp, "%d\n", num);
		num = 0;
		i = 0;
	}
}

void PrintVector(std::vector< unsigned char > myvector, FILE *fp) {
	for (std::vector<unsigned char>::iterator it = myvector.begin(); it != myvector.end(); ++it) {
		//fprintf(fp, "%d\n", *it);
		WriteDataToFile(fp, *it);
	}
}

inline std::string DeviceFlagToString(DWORD flags)
{
	std::string msg;
	msg += (flags & 0x1) ? "DEVICE_OPEN" : "DEVICE_CLOSED";
	msg += ", ";
	msg += (flags & 0x2) ? "High-speed USB" : "Full-speed USB";
	return msg;
}

void ListFtUsbDevices()
{
	FT_STATUS ftStatus = 0;

	DWORD numOfDevices = 0;
	ftStatus = FT_CreateDeviceInfoList(&numOfDevices);

	for (DWORD iDev = 0; iDev<numOfDevices; ++iDev)
	{
		FT_DEVICE_LIST_INFO_NODE devInfo;
		memset(&devInfo, 0, sizeof(devInfo));

		ftStatus = FT_GetDeviceInfoDetail(iDev, &devInfo.Flags, &devInfo.Type, &devInfo.ID, &devInfo.LocId,
			devInfo.SerialNumber,
			devInfo.Description,
			&devInfo.ftHandle);

		if (FT_OK == ftStatus)
		{
			printf("Dev %lu:\n", iDev);
			printf("  Flags= 0x%x, (%s)\n", devInfo.Flags, DeviceFlagToString(devInfo.Flags).c_str());
			printf("  Type= 0x%x\n", devInfo.Type);
			printf("  ID= 0x%x\n", devInfo.ID);
			printf("  LocId= 0x%x\n", devInfo.LocId);
			printf("  SerialNumber= %s\n", devInfo.SerialNumber);
			printf("  Description= %s\n", devInfo.Description);
			printf("  ftHandle= 0x%x\n", devInfo.ftHandle);

			const std::string desc = devInfo.Description;
			g_FTAllDevList.push_back(devInfo);

			if (desc == "FT4222" || desc == "FT4222 A")
			{
				g_FT4222DevList.push_back(devInfo);
			}
		}
	}
}

#endif
FT2StreamReader::FT2StreamReader(QObject *parent) :
	QObject (parent)
{

	tmpBuf.resize(BUFFER_SIZE);
#ifdef READ_FROM_FILE
	rawSample = new QFile(":/raw_sample.raw", this);
	rawSample->open(QIODevice::ReadOnly);
#else

	FT_STATUS ft4222_status;

	ListFtUsbDevices();

	if (g_FT4222DevList.empty()) {
		printf("No FT4222 device is found!\n");
		return;
	}



	FT_STATUS ftStatus;
	ftStatus = FT_OpenEx((PVOID)g_FT4222DevList[0].LocId, FT_OPEN_BY_LOCATION, &ftHandle);
	if (FT_OK != ftStatus)
	{
		printf("Open a FT4222 device failed!\n");
		return;
	}

	ftStatus = FT4222_SetClock(ftHandle, SYS_CLK_80);
	if (FT_OK != ftStatus)
	{
		printf("FT4222_SetClock failed!\n");
		return;
	}

	ftStatus = FT_SetUSBParameters(ftHandle, 4 * 1024, 0);
	if (FT_OK != ftStatus)
	{
		printf("FT_SetUSBParameters failed!\n");
		return;
	}

	ft4222_status = FT4222_SPISlave_InitEx(ftHandle, SPI_SLAVE_NO_PROTOCOL);
	if (FT4222_OK != ft4222_status)
	{
		printf("Init FT4222 as SPI slave device failed!\n");
		return;
	}

	ft4222_status = FT4222_SPISlave_SetMode(ftHandle, CLK_IDLE_LOW, CLK_TRAILING);
	if (FT4222_OK != ft4222_status)
	{
		printf("Set Node FT4222 as SPI slave device failed!\n");
		return;
	}

	ft4222_status = FT4222_SPI_SetDrivingStrength(ftHandle, DS_4MA, DS_4MA, DS_4MA);
	if (FT4222_OK != ft4222_status)
	{
		printf("FT4222_SPI_SetDrivingStrength failed!\n");
		return;
	}


	InitRxbuffer();
	printf("waiting for data...\n");
#endif

}

void FT2StreamReader::addListener(FT2StreamConsumer* listener){
	listeners.push_back(listener);
	//listener->setData(tmpBuf);
	qDebug()<<"Data address: "<<&tmpBuf[0];
	connect(listeners[0], SIGNAL(finishedReading()), this, SLOT(readStream()),
			Qt::ConnectionType(/*Qt::QueuedConnection | */Qt::UniqueConnection));
}


void FT2StreamReader::readStream(){

	std::lock_guard<std::mutex> locker(rwMutex);
#ifdef READ_FROM_FILE
	sizeTransferred = rawSample->read(reinterpret_cast<char*>(&tmpBuf[0]),BUFFER_SIZE);
	qDebug()<<"Res: "<<sizeTransferred;
#else
	FT_STATUS ft4222_status = FT4222_SPISlave_GetRxStatus(ftHandle, &rxSize);
	if (ft4222_status == FT4222_OK)
	{
		qDebug()<<"RXSize: "<<rxSize;

		ft4222_status = FT4222_SPISlave_Read(ftHandle, &tmpBuf[0], BUFFER_SIZE, &sizeTransferred);

	}
#endif
	if(sizeTransferred > 0)
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
	if(internalBuffer.pos() == internalBuffer.size())
		emit finishedReading();
	////qDebug()<<"pos: "<<internalBuffer.pos();
	std::lock_guard<std::mutex> locker(rwMutex);
	//qDebug()<<"readData";
	//qDebug()<<"len: "<<len;
	qint64 res = internalBuffer.read(data, len);
	auto hz = internalBuffer.data();
	//resetBuffer();
	//qDebug()<<"data: "<<data;
	//qDebug()<<"bufsize: "<<internalBuffer.size();
	//qDebug()<<"Data: "<<internalBuffer.data().data();
	//qDebug()<<res;

	return res;
}

qint64 FT2StreamConsumer::bytesAvailable() const{
	qDebug()<<"bytes available";
	return internalBuffer.bytesAvailable();
}

void FT2StreamConsumer::setData(const std::vector<unsigned char> &tmpBuf){
	qDebug()<<"isOpen: "<<internalBuffer.isOpen();
	qDebug()<<"Data address at set: "<<&tmpBuf[0];
	qDebug()<<"Address: "<<&(internalBuffer.data().data()[0]);
	internalBuffer.setData(reinterpret_cast<const char*>(&tmpBuf[0]),
			static_cast<int>(tmpBuf.size()));
	qDebug()<<"Address: "<<&(internalBuffer.data().data()[0]);
}

void FT2StreamConsumer::writeData(const std::vector<unsigned char>& tmpBuf){
	internalBuffer.seek(0);
	internalBuffer.write(reinterpret_cast<const char*>(&tmpBuf[0]),
			static_cast<int>(tmpBuf.size()));
	internalBuffer.seek(0);
}


