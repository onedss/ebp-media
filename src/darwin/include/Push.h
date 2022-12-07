#pragma once

#include <string>

#include "RTSPClient.hh"
#include "MediaSource.hh"
#include "JavaInterface.h"

#define MP3_SDP_FORMAT "v=0\r\no=- %lu%d %u IN IP4 127.0.0.1\r\nc=IN IP4 127.0.0.1\r\nt=0 0\r\nm=audio 0 RTP/AVP 14\r\na=control:trackID=1"
#define TS_SDP_FORMAT "v=0\r\no=- %lu%d %u IN IP4 127.0.0.1\r\nc=IN IP4 127.0.0.1\r\nt=0 0\r\nm=video 0 RTP/AVP 33\r\na=rtpmap:33 MP2T/90000\r\na=control:trackID=1"

class Push: public JavaInterface, public RTSPClient
{
public:
	Push(const char * rtspURL, unsigned int duration, unsigned int loop);
	virtual ~Push();
	virtual void stop(const int errCode = 0, const char *errmsg = "");
	bool start();

	void startPlaying(MediaSource* source = NULL);
	void afterPlay();
	void genericResponseHandler(int responseCode, char* responseString);
	void SetTitles(const char *file);
	void SetTrailer(const char *file);

	static void *ThreadHandler(void* push);

protected:
	unsigned int fDuration;
	unsigned int fLoop;
	unsigned int fPlayCountor;

	virtual MediaSource* getMediaSource() = 0;
	virtual const char* getSDPStr();
	virtual bool isTSData() { return false; }

private:
	void Run();
	virtual Boolean isRTSPClient() const override;
	virtual Boolean handleSETUPResponse(MediaSubsession& subsession, char const* sessionParamsStr, char const* transportParamsStr, Boolean streamUsingTCP) override;

	char *fHeadFile, *fEndFile, *fErrmsg;
	MediaSession* fSession;
	MediaSource* fSource;
	int fMask, fErrcode;
	time_t fPlayTime;
};
