//
// Created by Cristobal Mendoza on 3/6/18.
//

#include "MTBackgroundSubstractionVideoProcess.hpp"
#include "MTVideoInputStream.hpp"

#pragma mark Background Substraction

/// Background Substraction
///////////////////////////////////

MTBackgroundSubstractionVideoProcess::MTBackgroundSubstractionVideoProcess() :
		MTVideoProcess("Background Substraction", "MTBackgroundSubstractionVideoProcess")
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

void MTBackgroundSubstractionVideoProcess::setup()
{
	MTVideoProcess::setup();
	bSub = cv::createBackgroundSubtractorMOG2();
	bSub->setShadowValue(0);
	updateParams = true;
}

void MTBackgroundSubstractionVideoProcess::process(MTProcessData& processData)
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

std::shared_ptr<MTVideoProcessUI> MTBackgroundSubstractionVideoProcess::createUI()
{
	return std::make_shared<MTBackgroundSubstractionVideoProcessUI>(shared_from_this(), OF_IMAGE_GRAYSCALE);
}

#pragma mark Background Substraction UI

/// Background Substraction UI
///////////////////////////////////

MTBackgroundSubstractionVideoProcessUI::
MTBackgroundSubstractionVideoProcessUI(const std::shared_ptr<MTVideoProcess>& videoProcess, ofImageType imageType) :
		MTVideoProcessUIWithImage(videoProcess, imageType)
{
}

void MTBackgroundSubstractionVideoProcessUI::draw(ofxImGui::Settings& settings)
{
	auto vp = videoProcess.lock();
	MTVideoProcessUIWithImage::draw(settings);
//	for (auto& param : vp->getParameters())
//	{
		ofxImGui::AddGroup(vp->getParameters(), settings);
//	}
}
