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
// Implementation


#include "BasicUsageEnvironment.hh"
#include "HandlerSet.hh"
#include <stdio.h>
#include <event2/event.h>

////////// A subclass of DelayQueueEntry,
//////////     used to implement BasicTaskScheduler0::scheduleDelayedTask()

class DelayedTaskHandler {
public:
	DelayedTaskHandler(TaskFunc* proc, void* clientData):
		fProc(proc), fClientData(clientData) {}

	void execute() {
		(*fProc)(fClientData);
	}

private:
	TaskFunc* fProc;
	void* fClientData;
};

void executeDelayedTask(evutil_socket_t fd, short what, void* arg) {
	((DelayedTaskHandler*)arg)->execute();
}

void executeBackgroundTask(evutil_socket_t fd, short what, void* arg) {
	((HandlerDescriptor*)arg)->execute();
}

////////// BasicTaskScheduler0 //////////


BasicTaskScheduler* BasicTaskScheduler::createNew() {
	return new BasicTaskScheduler();
}

BasicTaskScheduler::BasicTaskScheduler():
		fEventbase(event_base_new()),
		fLastHandledSocketNum(-1){
	fHandlers = new HandlerSet;
}

BasicTaskScheduler::~BasicTaskScheduler() {
	event_base_free((event_base*)fEventbase);
	delete fHandlers;
}

TaskToken BasicTaskScheduler::scheduleDelayedTask(int64_t microseconds, TaskFunc* proc, void* clientData, bool persistent) {
	if (microseconds < 0) microseconds = 0;

	struct timeval tv;
	struct event* delayed_event = NULL;
	delayed_event = event_new((event_base*)fEventbase, -1, (persistent ? EV_PERSIST : 0), executeDelayedTask, new DelayedTaskHandler(proc, clientData));
	timerclear(&tv);
	tv.tv_sec = (long)(microseconds / 1000000);
	tv.tv_usec = (long)(microseconds % 1000000);
	event_add(delayed_event, &tv);

	return (void*)delayed_event;
}

void BasicTaskScheduler::unscheduleDelayedTask(TaskToken& prevTask) {
	if (prevTask) {
		struct event* delayed_event = (event*)prevTask;
		event_free(delayed_event);
		prevTask = NULL;
	}
}
void BasicTaskScheduler::doEventLoop() {
	event_base_dispatch((event_base*)fEventbase);
}
void BasicTaskScheduler::exitEventLoop() {
	event_base_loopexit((event_base*)fEventbase, NULL);
}

void BasicTaskScheduler::setBackgroundHandling(int socketNum, int mask, BackgroundHandlerProc* handlerProc, void* clientData) {
	if (socketNum < 0) return;

	if (mask == 0) {
		fHandlers->clearHandler(socketNum);
	} else {
		fHandlers->assignHandler(socketNum, mask, handlerProc, clientData, fEventbase);
	}
}

void BasicTaskScheduler::moveSocketHandling(int oldSocketNum, int newSocketNum) {
  if (oldSocketNum < 0 || newSocketNum < 0) return;
  fHandlers->moveHandler(oldSocketNum, newSocketNum, fEventbase);
}

////////// HandlerSet (etc.) implementation //////////

HandlerDescriptor::HandlerDescriptor(HandlerDescriptor* nextHandler):
	socketNum(0),mask(0), handlerProc(NULL),clientData(NULL),_event(NULL),
	fNextHandler(NULL), fPrevHandler(NULL){
	// Link this descriptor into a doubly-linked list:
	if (nextHandler == this) { // initialization
		fNextHandler = fPrevHandler = this;
	}
	else {
		fNextHandler = nextHandler;
		fPrevHandler = nextHandler->fPrevHandler;
		nextHandler->fPrevHandler = this;
		fPrevHandler->fNextHandler = this;
	}
}

HandlerDescriptor::~HandlerDescriptor() {
	/*if(_event != NULL) {
		event_free((event *)_event);
	}*/
	// Unlink this descriptor from a doubly-linked list:
	fNextHandler->fPrevHandler = fPrevHandler;
	fPrevHandler->fNextHandler = fNextHandler;
}
void HandlerDescriptor::execute() {
	(*handlerProc)(clientData, mask);
}

HandlerSet::HandlerSet()
	: fHandlers(&fHandlers) {
	fHandlers.socketNum = -1; // shouldn't ever get looked at, but in case...
}

HandlerSet::~HandlerSet() {
	// Delete each handler descriptor:
	while (fHandlers.fNextHandler != &fHandlers) {
		delete fHandlers.fNextHandler; // changes fHandlers->fNextHandler
	}
}

void HandlerSet::assignHandler(int socketNum, int mask,
	TaskScheduler::BackgroundHandlerProc* handlerProc, void* clientData, void* eventBase) {
	// First, see if there's already a handler for this socket:
	HandlerDescriptor* handler = lookupHandler(socketNum);
	if (handler == NULL) { // No existing handler, so create a new descr:
		handler = new HandlerDescriptor(fHandlers.fNextHandler);
		handler->socketNum = socketNum;
	} else if (handler->_event) {
		event_free((event*)handler->_event);
	}

	handler->mask = mask;
	handler->handlerProc = handlerProc;
	handler->clientData = clientData;

	handler->_event = event_new((event_base*)eventBase, socketNum, mask | EV_PERSIST, executeBackgroundTask, handler);
	event_add((event*)handler->_event, NULL);
}

void HandlerSet::clearHandler(int socketNum) {
	HandlerDescriptor* handler = lookupHandler(socketNum);
	if(handler != NULL) {
        event_free((event*)handler->_event);
        delete handler;
	}
}

void HandlerSet::moveHandler(int oldSocketNum, int newSocketNum, void *eventBase) {
	HandlerDescriptor* handler = lookupHandler(oldSocketNum);
	if (handler != NULL) {
		handler->socketNum = newSocketNum;
		if(handler->_event) event_free((event*)handler->_event);
		handler->_event = event_new((event_base*)eventBase, newSocketNum, handler->mask | EV_PERSIST, executeBackgroundTask, handler);
	}
}

HandlerDescriptor* HandlerSet::lookupHandler(int socketNum) {
	HandlerDescriptor* handler;
	HandlerIterator iter(*this);
	while ((handler = iter.next()) != NULL) {
		if (handler->socketNum == socketNum) break;
	}
	return handler;
}

HandlerIterator::HandlerIterator(HandlerSet& handlerSet)
	: fOurSet(handlerSet) {
	reset();
}

HandlerIterator::~HandlerIterator() {
}

void HandlerIterator::reset() {
	fNextPtr = fOurSet.fHandlers.fNextHandler;
}

HandlerDescriptor* HandlerIterator::next() {
	HandlerDescriptor* result = fNextPtr;
	if (result == &fOurSet.fHandlers) { // no more
		result = NULL;
	}
	else {
		fNextPtr = fNextPtr->fNextHandler;
	}

	return result;
}
