//
// Created by Cristobal Mendoza on 3/6/18.
//

#include "MTBackgroundSubstractionVideoProcess.hpp"
#include "MTVideoProcessStream.hpp"

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
	bSub = cv::createBackgroundSubtractorMOG2();
}

MTProcessData& MTBackgroundSubstractionVideoProcess::process(MTProcessData& processData)
{
	if (updateParams)
	{
		bSub->setHistory(history);
		bSub->setDetectShadows(detectShadows);
		bSub->setVarThreshold(threshold);
	}
	bSub->apply(processData.processStream, processBuffer);
	if (substractStream)
	{
		cv::subtract(processBuffer, processData.processStream, processOutput);
	}
	else
	{
		processOutput = processBuffer;
	}
//	cv::threshold(processBuffer, processOutput, 200, 255, cv::THRESH_BINARY);
//	input.at(MTVideoProcessStreamKey).copyTo(input.at(MTVideoProcessStreamKey), processOutput);
	if (substractStream)
	{
		processData.processStream = processOutput;
	}
	processData.processResult = processOutput;
	processData.processMask = processOutput;
	return processData;
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
	auto vp = videoProcess.lock();
//	ImGui::PushID(vp.get());
	ofxImGui::BeginTree(vp->getName(), settings);
	MTVideoProcessUIWithImage::draw(settings);
	for (auto& param : vp->getParameters())
	{
		ofxImGui::AddParameter(param);
	}
	ofxImGui::EndTree(settings);
//	ImGui::PopID();
}
