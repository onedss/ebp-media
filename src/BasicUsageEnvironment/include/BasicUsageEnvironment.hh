/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2019 Live Networks, Inc.  All rights reserved.
// Basic Usage Environment: for a simple, non-scripted, console application
// C++ header

#ifndef _BASIC_USAGE_ENVIRONMENT_HH
#define _BASIC_USAGE_ENVIRONMENT_HH

#ifndef _BASICUSAGEENVIRONMENT_VERSION_HH
#include "BasicUsageEnvironment_version.hh"
#endif

#ifndef _USAGE_ENVIRONMENT_HH
#include "UsageEnvironment.hh"
#endif

#define RESULT_MSG_BUFFER_MAX 1000

// An abstract base class, useful for subclassing
// (e.g., to redefine the implementation of "operator<<")
class BasicUsageEnvironment: public UsageEnvironment {
public:
	static BasicUsageEnvironment* createNew(TaskScheduler& taskScheduler);

	// redefined virtual functions:
	virtual int getErrno() const;

	virtual UsageEnvironment& operator<<(char const* str);
	virtual UsageEnvironment& operator<<(int i);
	virtual UsageEnvironment& operator<<(unsigned u);
	virtual UsageEnvironment& operator<<(double d);
	virtual UsageEnvironment& operator<<(void* p);

  // redefined virtual functions:
  virtual MsgString getResultMsg() const;

  virtual void setResultMsg(MsgString msg);
  virtual void setResultMsg(MsgString msg1,
		    MsgString msg2);
  virtual void setResultMsg(MsgString msg1,
		    MsgString msg2,
		    MsgString msg3);
  virtual void setResultErrMsg(MsgString msg, int err = 0);

  virtual void appendToResultMsg(MsgString msg);

  virtual void reportBackgroundError();

protected:
  BasicUsageEnvironment(TaskScheduler& taskScheduler);
  virtual ~BasicUsageEnvironment();

private:
  void reset();

  char fResultMsgBuffer[RESULT_MSG_BUFFER_MAX];
  unsigned fCurBufferSize;
  unsigned fBufferMaxSize;
};

class HandlerSet; // forward

// #define MAX_NUM_EVENT_TRIGGERS 32

// An abstract base class, useful for subclassing
// (e.g., to redefine the implementation of socket event handling)
class BasicTaskScheduler: public TaskScheduler {
public:
	static BasicTaskScheduler* createNew();
	virtual ~BasicTaskScheduler();

  // Redefined virtual functions:
  virtual TaskToken scheduleDelayedTask(int64_t microseconds, TaskFunc* proc, void* clientData, bool persistent);
  virtual void unscheduleDelayedTask(TaskToken& prevTask);

  virtual void setBackgroundHandling(int socketNum, int mask, BackgroundHandlerProc* handlerProc, void* clientData);
  virtual void moveSocketHandling(int oldSocketNum, int newSocketNum);

  virtual EventTriggerId createEventTrigger(TaskFunc* eventHandlerProc) { return 0; };
  virtual void deleteEventTrigger(EventTriggerId eventTriggerId) {};
  virtual void triggerEvent(EventTriggerId eventTriggerId, void* clientData = NULL) {};

  virtual void doEventLoop();
  virtual void exitEventLoop();

protected:
  BasicTaskScheduler();

protected:
	void *fEventbase;
  // To implement background reads:
  HandlerSet *fHandlers;
  int fLastHandledSocketNum;
};

#endif
