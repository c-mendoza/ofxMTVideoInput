//
// Created by Cristobal Mendoza on 7/17/20.
//

#include <ofxMTVideoInput.h>
#include "MTVideoInputSourceOFVideoGrabber.hpp"
#include "ofxCv/Utilities.h"


MTVideoInputSourceOFGrabber::MTVideoInputSourceOFGrabber() :
		MTVideoInputSource("InputSource", "MTVideoInputSourceOFGrabber"){}

bool MTVideoInputSourceOFGrabber::isFrameNew()
{
	return grabber.isFrameNew();
}

ofPixels& MTVideoInputSourceOFGrabber::getPixels()
{
	return grabber.getPixels();
}

cv::Mat MTVideoInputSourceOFGrabber::getCVPixels()
{
	return ofxCv::toCv(grabber.getPixels());
}

void MTVideoInputSourceOFGrabber::start()
{
	setup(captureSize->x, captureSize->y, frameRate, 0);
}

void MTVideoInputSourceOFGrabber::close()
{
	grabber.close();
}

void MTVideoInputSourceOFGrabber::update()
{
	grabber.update();
}


void MTVideoInputSourceOFGrabber::setup()
{
	setup(captureSize->x, captureSize->y, frameRate, deviceID);
}

void MTVideoInputSourceOFGrabber::setup(int width, int height, int framerate, std::string deviceID)
{
	grabber.setDeviceID(ofFromString<int>(deviceID));
	captureSize = glm::vec2 (width, height);
	this->frameRate = framerate;
	this->deviceID = deviceID;
	grabber.setup(width, height, false);
}
