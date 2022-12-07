#include <time.h>
#include <string.h>
#include <pthread.h>

#include "Push.h"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "MP3FileSource.hh"

#include "MPEG1or2AudioRTPSink.hh"
#include "MP3ADURTPSink.hh"
#include "H264VideoRTPSink.hh"
#include "H265VideoRTPSink.hh"
#include "SimpleRTPSink.hh"

static void GenericResponseHandler(RTSPClient* rtspClient, int responseCode, char* responseString) {
    ((Push*)rtspClient)->genericResponseHandler(responseCode, responseString);
}

static void AfterPlaying(void * client) {
	((Push*)client)->afterPlay();
}

Push::Push(const char * rtspURL, unsigned int duration, unsigned int loop)
	:RTSPClient(*BasicUsageEnvironment::createNew(*BasicTaskScheduler::createNew()), rtspURL, 0, NULL, 0, -1),
	fDuration(duration),
	fLoop(loop),
	fPlayCountor(0),
	fHeadFile(NULL),
	fEndFile(NULL),
	fErrmsg(NULL),
	fSession(NULL),
	fSource(NULL),
	fMask(DESCRIBE),
	fErrcode(0),
	fPlayTime(0) {
}

Push::~Push()
{
	if(fErrmsg) delete[] fErrmsg;
	// delete &envir().taskScheduler();
}

void Push::SetTitles(const char *file) {
    fHeadFile = strDup(file);
}
void Push::SetTrailer(const char *file) {
    fEndFile = strDup(file);
}

bool Push::start() {
    pthread_t tid;
	return pthread_create(&tid, NULL, Push::ThreadHandler, (void*)this) == 0;
}

void *Push::ThreadHandler(void* push) {
	((Push*) push) ->Run();
	return 0;
}

void Push::stop(const int errcode, const char * errmsg) {
	fErrcode = errcode;
    fErrmsg = strDup(errmsg);
    if(fSession != NULL) {
        fMask = TEARDOWN;
		sendTeardownCommand(*fSession, GenericResponseHandler);
	}
}

int a = 0;
void Push::Run() {
    fireEvent(STATUS_UNINITIALIZED, 0, url());
	fMask = ANNOUNCE;
    sendAnnounceCommand(getSDPStr(), GenericResponseHandler, NULL);
    envir().taskScheduler().doEventLoop();

    fireEvent(STATUS_TERMINATE, fErrcode, fErrmsg);
	if(fSession != NULL) {
		Medium::close(fSession);
		fSession = NULL;
	}

    Medium::close(this);
    // delete this;
}

void Push::genericResponseHandler(int responseCode, char* responseString) {
    if(responseCode != 0 || fMask == TEARDOWN) {
        fErrcode = responseCode;
        fErrmsg = strDup(responseString);
        envir().taskScheduler().exitEventLoop();
        return;
    }

    switch(fMask){
    case ANNOUNCE:
        fSession = MediaSession::createNew(envir(), getSDPStr());
        if (fSession && fSession->hasSubsessions()) {
            fMask = SETUP;
            MediaSubsessionIterator iter(*fSession);
            sendSetupCommand(*iter.next(), GenericResponseHandler, True, True);
        } else {
            stop(-1, "SDP description error");
        }
        break;
    case SETUP:
        fMask = PLAY;
        sendPlayCommand(*fSession, GenericResponseHandler);
        break;
    case PLAY:
        fMask = TEARDOWN;
        fPlayTime = time(0);

        fireEvent(STATUS_INITIALIZED, 0, url());
        startPlaying();
        break;
    // case TEARDOWN:
    default:
        //fMask = TEARDOWN;
        break;
    }
}

void Push::startPlaying(MediaSource* source) {
	fSource = source;
    if(fHeadFile) {
        fLoop ++;
        fSource = MP3FileSource::createNew(envir(), fHeadFile);
        delete fHeadFile; fHeadFile = NULL;
    }

    if(fSource == NULL){
    	fSource = getMediaSource();
    }

	if(fSource) {
		fPlayCountor++;
		MediaSubsessionIterator iter(*fSession);
		iter.next()->sink->startPlaying(*fSource, AfterPlaying, this);
	} else {
		stop(-2, "Media Source is NULL");
	}
}

void Push::afterPlay() {
	if(fSource != NULL) {
		Medium::close(fSource);
		fSource = NULL;
	}
    double duration = difftime(time(0), fPlayTime);
    if((fLoop > 0 && fPlayCountor < fLoop) || (fDuration > 0 && duration < fDuration)) {
        fireEvent(STATUS_RUNING, duration);
        startPlaying();
    } else if(fEndFile) {
        fireEvent(STATUS_RUNING, duration);
        MediaSource* source = MP3FileSource::createNew(envir(), fEndFile);
        if(source) startPlaying(source);

        delete fEndFile; fEndFile = NULL;
    } else {
        char tmp[32];
        sprintf(tmp, "Played:%.0fs",duration);
        stop(0, tmp);
    }
}

Boolean Push::isRTSPClient() const
{
	return False;
}

Boolean Push::handleSETUPResponse(MediaSubsession& subsession,
		const char* sessionParamsStr, const char* transportParamsStr,
		Boolean streamUsingTCP) {
	  char* sessionId = new char[responseBufferSize]; // ensures we have enough space
	  Boolean success = False;
	  do {
	    // Check for a session id:
	    if (sessionParamsStr == NULL || sscanf(sessionParamsStr, "%[^;]", sessionId) != 1) {
	      envir().setResultMsg("Missing or bad \"Session:\" header");
	      break;
	    }
	    subsession.setSessionId(sessionId);
	    delete[] fLastSessionId; fLastSessionId = strDup(sessionId);

	    // Also look for an optional "; timeout = " parameter following this:
	    char const* afterSessionId = sessionParamsStr + strlen(sessionId);
	    int timeoutVal;
	    if (sscanf(afterSessionId, "; timeout = %d", &timeoutVal) == 1) {
	      fSessionTimeoutParameter = timeoutVal;
	    }

	    // Parse the "Transport:" header parameters:
	    char* serverAddressStr;
	    portNumBits serverPortNum;
	    unsigned char rtpChannelId, rtcpChannelId;
	    if (!parseTransportParams(transportParamsStr, serverAddressStr, serverPortNum, rtpChannelId, rtcpChannelId)) {
	      envir().setResultMsg("Missing or bad \"Transport:\" header");
	      break;
	    }
	    delete[] subsession.connectionEndpointName();
	    subsession.connectionEndpointName() = serverAddressStr;
	    subsession.serverPortNum = serverPortNum;
	    subsession.rtpChannelId = rtpChannelId;
	    subsession.rtcpChannelId = rtcpChannelId;

	    if (streamUsingTCP) {

	    	char const* fCodecName = subsession.codecName();
	    	if (strcmp(fCodecName, "MPA") == 0) { // MPEG-1 or 2 audio
	    		subsession.sink = MPEG1or2AudioRTPSink::createNew(envir(), NULL);
	    	} else if (strcmp(fCodecName, "MPA-ROBUST") == 0) { // robust MP3 audio
	    		subsession.sink = MP3ADURTPSink::createNew(envir(), NULL, subsession.rtpPayloadFormat());
	    	} else if (strcmp(fCodecName, "H264") == 0) {
	    		subsession.sink = H264VideoRTPSink::createNew(envir(), NULL, subsession.rtpPayloadFormat());
	    	} else if (strcmp(fCodecName, "H265") == 0) {
	    		subsession.sink = H265VideoRTPSink::createNew(envir(), NULL, subsession.rtpPayloadFormat());
	        } else if (strcmp(fCodecName, "MP2T") == 0) { // MPEG-2 Transport Stream
	        	subsession.sink = SimpleRTPSink::createNew(envir(), NULL, subsession.rtpPayloadFormat()
	        			, subsession.rtpTimestampFrequency(), "video/MP2T", 0, False);
	    	} else {
	    		envir().setResultMsg("RTP payload format unknown or not supported");
	    		return False;
	    	}

			RTPSink *rtpSink = (RTPSink*)subsession.sink;
			rtpSink->setStreamSocket(fInputSocketNum, subsession.rtpChannelId);
			rtpSink->enableRTCPReports() = True;
			increaseReceiveBufferTo(envir(), fInputSocketNum, 50 * 1024);

			if (subsession.rtcpInstance() != NULL) subsession.rtcpInstance()->setStreamSocket(fInputSocketNum, subsession.rtcpChannelId);
			RTPInterface::setServerRequestAlternativeByteHandler(envir(), fInputSocketNum, handleAlternativeRequestByte, this);

	    } else {
	      // Normal case.
	      // Set the RTP and RTCP sockets' destination address and port from the information in the SETUP response (if present):
	      netAddressBits destAddress = subsession.connectionEndpointAddress();
	      if (destAddress == 0) destAddress = fServerAddress;
	      subsession.setDestinations(destAddress);
	    }

	    success = True;
	  } while (0);

	  delete[] sessionId;
	  return success;

	return True;
}

static int SessionIdSequence = 1;
const char* Push::getSDPStr() {
	time_t sdpSessionId;
	time(&sdpSessionId);
	unsigned const sdpVersion = our_random32();

    char* sdp;
    if(isTSData()) {
        size_t sdpLen = strlen(TS_SDP_FORMAT)+ 40;
        sdp = new char[sdpLen];
        sprintf(sdp, TS_SDP_FORMAT, sdpSessionId, SessionIdSequence, sdpVersion);
    } else {
        size_t sdpLen = strlen(MP3_SDP_FORMAT)+ 40;
        sdp = new char[sdpLen];
        sprintf(sdp, MP3_SDP_FORMAT, sdpSessionId, SessionIdSequence, sdpVersion);
    }

	SessionIdSequence++;
	return sdp;
}
