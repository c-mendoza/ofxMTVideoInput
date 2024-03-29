//
// Created by Cristobal Mendoza on 3/4/18.
//

#include "MTThresholdVideoProcess.hpp"
#include "ofxCv.h"
#include "MTVideoInputStream.hpp"
#include "MTAppFrameworkUtils.hpp"

#pragma mark Threshold

////////////////////////////////////
/// THRESHOLD
///////////////////////////////////

MTThresholdVideoProcess::MTThresholdVideoProcess() : MTVideoProcess("Threshold", "MTThresholdVideoProcess")
{
	parameters.add(threshold.set("Threshold", 127, 0, 255));
}

void MTThresholdVideoProcess::process(MTProcessData& processData)
{
	auto& streamBuffer = processData.processStream;
	if (streamBuffer.channels() >= 3)
	{
		cv::cvtColor(streamBuffer, processBuffer, cv::COLOR_BGR2GRAY);
	}
	else
	{
		processBuffer = streamBuffer;
	}
	cv::threshold(processBuffer, processOutput, threshold.get(), threshold.getMax(),
				  cv::THRESH_BINARY);
	processData.processStream = processOutput;
	processData.processResult = processOutput;
}

std::shared_ptr<MTVideoProcessUI> MTThresholdVideoProcess::createUI()
{
	return std::make_shared<MTThresholdVideoProcessUI>(shared_from_this(), OF_IMAGE_GRAYSCALE);
}

#pragma mark Threshold UI

/// THRESHOLD UI
///////////////////////////////////

MTThresholdVideoProcessUI::MTThresholdVideoProcessUI(const std::shared_ptr<MTVideoProcess>& videoProcess,
													 ofImageType imageType) : MTVideoProcessUIWithImage(videoProcess,
																										imageType)
{

}

void MTThresholdVideoProcessUI::draw(ofxImGui::Settings& settings)
{
	auto vp = videoProcess.lock();
	MTVideoProcessUIWithImage::draw(settings);
	for (auto& param : vp->getParameters())
	{
		ofxImGui::AddParameter(param);
	}
}

