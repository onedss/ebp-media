
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>

#include "FilePush.h"
#include "MP3FileSource.hh"
#include "ByteStreamFileSource.hh"
#include "MPEG2TransportStreamFramer.hh"
#include "GroupsockHelper.hh"


#define TRANSPORT_PACKET_SIZE 188
#define TRANSPORT_PACKETS_PER_NETWORK_PACKET 7

#define MP3_FILE 1
#define TS_FILE 2

FilePush::FilePush(const char * rtspUrl, const char * filepath, unsigned int duration, unsigned int loop):
	Push(rtspUrl, duration, loop),
	fFilePath(filepath),
	fIsTSData(false)
{
	const char *pFileExt = strrchr(filepath, '.');
	if (pFileExt != NULL && strncmp(pFileExt, ".ts", 3) == 0) {
		fIsTSData = true;
	}
}

MediaSource * FilePush::getMediaSource()
{
	if(fIsTSData) {
		unsigned const inputDataChunkSize   = TRANSPORT_PACKETS_PER_NETWORK_PACKET * TRANSPORT_PACKET_SIZE;
		FramedSource* fileSource = ByteStreamFileSource::createNew(envir(), fFilePath.c_str(), inputDataChunkSize);  //创建一个流文件源
		if (fileSource!=NULL){
			return MPEG2TransportStreamFramer::createNew(envir(), fileSource);
		}

		return NULL;
	} else {
		return MP3FileSource::createNew(envir(), fFilePath.c_str());
	}
}
