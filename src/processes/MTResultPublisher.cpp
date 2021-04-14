//
// Created by Cristobal Mendoza on 4/14/2021
//

#include "MTResultPublisher.hpp"
#include "MTVideoInputStream.hpp"


MTResultPublisher::MTResultPublisher() :
		MTVideoProcess("Background Substraction", "MTResultPublisher")
{
	parameters.add(threshold.set("Background Threshold", 16, 1, 46),
				   history.set("History Length", 500, 100, 2000),
				   detectShadows.set("Detect Shadows", true),
				   substractStream.set("Subtract background from Stream", false)
	);

	addEventListener(threshold.newListener([this](float& value)
	{
		updateParams = true;
	}));
	addEventListener(history.newListener([this](int& value)
	{
		updateParams = true;
	}));
	addEventListener(detectShadows.newListener([this](bool& value)
	{
		updateParams = true;
	}));
	setup();
	

}

void MTResultPublisher::setup()
{
	MTVideoProcess::setup();
	bSub = cv::createBackgroundSubtractorMOG2();
	bSub->setShadowValue(0);
	updateParams = true;
}

void MTResultPublisher::process(MTProcessData& processData)
{
	if (updateParams)
	{
		bSub->setHistory(history);
		bSub->setDetectShadows(detectShadows);
		bSub->setVarThreshold(threshold);
		updateParams = false;
	}
	bSub->apply(processData.processStream, processBuffer);
	if (substractStream)
	{
		processBuffer.convertTo(processBuffer, processData.processStream.type());
//		cv::multiply()
//		cv::subtract(processData.processStream, processBuffer, processOutput, processData.processStream.depth());
		cv::bitwise_and(processData.processStream, processData.processStream, processOutput, processBuffer);
	}
	else
	{
		processOutput = processBuffer;
	}
//	cv::threshold(processBuffer, processOutput, 200, 255, cv::THRESH_BINARY);
//	input.at(MTVideoInputStreamKey).copyTo(input.at(MTVideoInputStreamKey), processOutput);
//	if (substractStream)
//	{
		processData.processStream = processOutput;
//	}
	processData.processResult = processOutput;
	processData.processMask = processBuffer;
}

std::shared_ptr<MTVideoProcessUI> MTResultPublisher::createUI()
{
	return std::make_shared<MTResultPublisherUI>(shared_from_this(), OF_IMAGE_GRAYSCALE);
}

#pragma mark Background Substraction UI

/// Background Substraction UI
///////////////////////////////////

MTResultPublisherUI::
MTResultPublisherUI(const std::shared_ptr<MTVideoProcess>& videoProcess, ofImageType imageType) :
		MTVideoProcessUIWithImage(videoProcess, imageType)
{
}

void MTResultPublisherUI::draw(ofxImGui::Settings& settings)
{
	auto vp = videoProcess.lock();
	MTVideoProcessUIWithImage::draw(settings);
//	for (auto& param : vp->getParameters())
//	{
		ofxImGui::AddGroup(vp->getParameters(), settings);
//	}
}
