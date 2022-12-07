#pragma once
//#include "jni.h"

#define STATUS_UNINITIALIZED 0
#define STATUS_INITIALIZED 1
#define STATUS_RUNING 2
#define STATUS_TERMINATE 3
#define STATUS_ERROR 4

enum {
	DESCRIBE, ANNOUNCE, OPTIONS, SETUP, PLAY, TEARDOWN
};

class JavaInterface
{
public:
    JavaInterface() {}
	~JavaInterface();

	//JavaVM* fJVM;
	//jobject fObject;

protected:
	//JNIEnv * getJNIEnv();

	void fireEvent(int eventType, int resultCode = 0, const char* resultString = 0);
	//void initBuffer(JNIEnv* jniEnv, jclass &cls, jobject &handler, void **buffer, unsigned int &capacity);

private:
	//JNIEnv * fJNIEnv;

	//jstring charTojstring( JNIEnv* env, const char* str);
};

