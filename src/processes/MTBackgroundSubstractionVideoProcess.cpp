//
// Created by Cristobal Mendoza on 3/6/18.
//

#include "MTBackgroundSubstractionVideoProcess.hpp"
#include "MTVideoProcessStream.hpp"

#pragma mark Background Substraction

/// Background Substraction
///////////////////////////////////

MTBackgroundSubstractionVideoProcess::MTBackgroundSubstractionVideoProcess() :
		MTVideoProcess("Background Substraction")
{
	setup();
}

void MTBackgroundSubstractionVideoProcess::setup()
{
	bSub = cv::createBackgroundSubtractorMOG2();
}

MTProcessData& MTBackgroundSubstractionVideoProcess::process(MTProcessData& input)
{
	bSub->apply(input.at(MTVideoProcessStreamKey), processOutput);
	cv::threshold(processOutput, processOutput, 200, 255, cv::THRESH_BINARY);
	input.at(MTVideoProcessStreamKey).copyTo(input.at(MTVideoProcessStreamKey), processOutput);
	input[MTVideoProcessResultKey] = input.at(MTVideoProcessStreamKey);
	input[MTVideoProcessMaskKey] = processOutput;
	return input;
}

std::unique_ptr<MTVideoProcessUI> MTBackgroundSubstractionVideoProcess::createUI()
{
	auto vp = shared_from_this();
	videoProcessUI = std::make_unique<MTBackgroundSubstractionVideoProcessUI>(vp, OF_IMAGE_GRAYSCALE);
	return std::move(videoProcessUI);
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
	MTVideoProcessUIWithImage::draw(settings);
	MTVideoProcessUI::draw(settings);
}