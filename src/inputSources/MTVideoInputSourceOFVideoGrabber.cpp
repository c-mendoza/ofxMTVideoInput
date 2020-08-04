//
// Created by Cristobal Mendoza on 7/17/20.
//

#include <ofxMTVideoInput.h>
#include "MTVideoInputSourceOFVideoGrabber.hpp"
#include "ofxCv/Utilities.h"


MTVideoInputSourceOFGrabber::MTVideoInputSourceOFGrabber(std::string devID) :
		MTVideoInputSource("ofVideoGrabber", "MTVideoInputSourceOFGrabber", "ofVideoGrabber", devID){}

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
//	setup(captureSize->x, captureSize->y, frameRate, 0);
}

void MTVideoInputSourceOFGrabber::close()
{
	grabber.close();
}

void MTVideoInputSourceOFGrabber::update()
{
	grabber.update();
	if (grabber.isFrameNew())
	{
//		MTCaptureEventArgs args;
//		args.inputSource = this->shared_from_this();
//		args.frame = grabber.getPixels();
		auto me = this->shared_from_this();
		frameCapturedEvent.notify(this, me);
	}
}


void MTVideoInputSourceOFGrabber::setup()
{
	setup(captureSize->x, captureSize->y, frameRate, deviceID);
}

void MTVideoInputSourceOFGrabber::setup(int width, int height, int framerate, std::string deviceID)
{
	grabber.close();
	grabber.setDeviceID(ofFromString<int>(deviceID));
	grabber.setDesiredFrameRate(framerate);
	grabber.setup(width, height, false);
	// Get the width and height that the grabber actually resolved:
	captureSize.setWithoutEventNotifications(glm::ivec2(grabber.getWidth(), grabber.getHeight()));
	frameRate.setWithoutEventNotifications(framerate);
	this->deviceID.setWithoutEventNotifications(deviceID);
}
