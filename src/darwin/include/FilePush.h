#pragma once
#include "Push.h"

class FilePush : public Push
{
public:
	FilePush(
		const char * rtspUrl,
		const char * filepath,
		unsigned int duration = 0,
		unsigned int loop = 1);

protected:
	virtual MediaSource * getMediaSource() override;
	virtual bool isTSData() override { return fIsTSData; }

private:
	std::string fFilePath;
	bool fIsTSData;
};

