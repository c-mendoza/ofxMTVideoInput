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

MTThresholdVideoProcess::MTThresholdVideoProcess() : MTVideoProcess("Threshold")
{
	parameters.add(threshold.set("Threshold", 127, 0, 255));
}

MTProcessData& MTThresholdVideoProcess::process(MTProcessData& input)
{
	cv::cvtColor(input.at(MTVideoProcessStreamKey), processBuffer, cv::COLOR_BGR2GRAY);
	cv::threshold(processBuffer, processOutput, threshold.get(), threshold.getMax(),
				  CV_THRESH_BINARY);
	input[MTVideoProcessStreamKey] = processOutput;
	input[MTVideoProcessResultKey] = processOutput;
	return input;
}

std::unique_ptr<MTVideoProcessUI> MTThresholdVideoProcess::createUI()
{
	videoProcessUI = std::make_unique<MTThresholdVideoProcessUI>(shared_from_this());
	return std::move(videoProcessUI);
}

#pragma mark Threshold UI

/// THRESHOLD UI
///////////////////////////////////

MTThresholdVideoProcessUI::MTThresholdVideoProcessUI(const std::shared_ptr<MTVideoProcess>& videoProcess)
		: MTVideoProcessUI(videoProcess)
{
	auto stream = videoProcess->processChain.lock();
	processImage.allocate(stream->processWidth, stream->processHeight, OF_IMAGE_GRAYSCALE);
	addEventListener(videoProcess->processCompleteFastEvent.newListener([this](
			const MTVideoProcessFastEventArgs<MTVideoProcess>& args)
																		{
																			ofPixels pixels;
																			ofxCv::toOf(args.processOutput, pixels);
																			outputChannel.send(std::move(pixels));
																		}));
}

MTThresholdVideoProcessUI::~MTThresholdVideoProcessUI()
{
	outputChannel.close();
}

void MTThresholdVideoProcessUI::draw(ofxImGui::Settings& settings)
{
	ofPixels pixels;
	while (outputChannel.tryReceive(pixels))
	{
//		ofxCv::toOf(processOutput, processImage);
		processImage.setFromPixels(pixels);
	}
	ofxImGui::AddImage(processImage, glm::vec2(320, 240));
//	ImGui::SameLine();
	MTVideoProcessUI::draw(settings);
}

