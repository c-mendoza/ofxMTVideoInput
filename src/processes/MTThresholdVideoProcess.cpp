//
// Created by Cristobal Mendoza on 3/4/18.
//

#include "MTThresholdVideoProcess.hpp"
#include "ofxCv.h"
#include "MTVideoProcessStream.hpp"

#pragma mark Threshold

////////////////////////////////////
/// THRESHOLD
///////////////////////////////////

MTThresholdVideoProcess::MTThresholdVideoProcess() : MTVideoProcess("Threshold", "MTThresholdVideoProcess")
{
	parameters.add(threshold.set("Threshold", 127, 0, 255));
}

MTProcessData& MTThresholdVideoProcess::process(MTProcessData& processData)
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
				  CV_THRESH_BINARY);
	processData.processStream = processOutput;
	processData.processResult = processOutput;
	return processData;
}

std::unique_ptr<MTVideoProcessUI> MTThresholdVideoProcess::createUI()
{
	videoProcessUI = std::make_unique<MTThresholdVideoProcessUI>(shared_from_this(), OF_IMAGE_GRAYSCALE);
	return std::move(videoProcessUI);
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
	ofxImGui::BeginTree(vp->getName(), settings);
	MTVideoProcessUIWithImage::draw(settings);
	for (auto& param : vp->getParameters())
	{
		ofxImGui::AddParameter(param);
	}
	ofxImGui::EndTree(settings);
}

