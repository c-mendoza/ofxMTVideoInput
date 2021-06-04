//
// Created by Cristobal Mendoza on 10/13/19.
//

#include <MTVideoInputStream.hpp>
#include "MTBackgroundSubstraction2.hpp"

MTBackgroundSubstraction2::MTBackgroundSubstraction2() :
		MTVideoProcess("Background Substraction 2", "MTBackgroundSubstraction2")
{
	parameters.add(
			erodeSize.set("Erosion Kernel Size", 1, 1, 25),
			erodeShapeType.set("Erosion Shape Type", 0, 0, 2),
			dilateSize.set("Dilation Kernel Size", 1, 1, 25),
			dilateShapeType.set("Dilation Shape Type", 0, 0, 2),
			threshold.set("Background Threshold", 16, 1, 46),
			history.set("History Length", 500, 100, 2000),
			detectShadows.set("Detect Shadows", true)
			);

	addEventListener(erodeSize.newListener([this](int args)
									  {
										  needsUpdate = true;
									  }));
	addEventListener(erodeShapeType.newListener([this](int args)
										   {
											   needsUpdate = true;
										   }));
	addEventListener(dilateSize.newListener([this](int args)
									  {
										  needsUpdate = true;
									  }));
	addEventListener(dilateShapeType.newListener([this](int args)
										   {
											   needsUpdate = true;
										   }));

	addEventListener(threshold.newListener([this](float& value)
										   {
											   needsUpdate = true;
										   }));
	addEventListener(history.newListener([this](int& value)
										 {
											 needsUpdate = true;
										 }));
	addEventListener(detectShadows.newListener([this](bool& value)
											   {
												   needsUpdate = true;
											   }));
}

void MTBackgroundSubstraction2::setup()
{
	MTVideoProcess::setup();
	bSub = cv::createBackgroundSubtractorMOG2();
	bSub->setShadowValue(0);
	updateInternals();
}

void MTBackgroundSubstraction2::process(MTProcessData& processData)
{
	if (needsUpdate) updateInternals();

	if (processData.processStream.type() != CV_8UC1)
	{
		cv::cvtColor(processData.processStream, processBuffer, cv::COLOR_RGB2GRAY);
	}
	else
	{
		processBuffer = processData.processStream;
	}

	bSub->apply(processBuffer, bSubOutput);

	cv::erode(bSubOutput, erodeOutput, erodeKernel);
	cv::dilate(erodeOutput, dilateOutput, dilateKernel);

	cv::bitwise_and(processData.processStream, dilateOutput, processOutput);

	processData.processResult = processOutput;
	processData.processStream = processOutput;
	processData.processMask = dilateOutput;
}

std::shared_ptr<MTVideoProcessUI> MTBackgroundSubstraction2::createUI()
{
	return std::make_shared<MTBackgroundSubstraction2UI>(shared_from_this(), OF_IMAGE_GRAYSCALE);
}

void MTBackgroundSubstraction2::updateInternals()
{
	erodeKernel = cv::getStructuringElement(erodeShapeType,
									   cv::Size(2 * erodeSize + 1, 2 * erodeSize + 1),
									   cv::Point(erodeSize, erodeSize));
	dilateKernel = cv::getStructuringElement(dilateShapeType,
											 cv::Size(2 * dilateSize + 1, 2 * dilateSize + 1),
											 cv::Point(dilateSize, dilateSize));
	bSub->setHistory(history);
	bSub->setDetectShadows(detectShadows);
	bSub->setVarThreshold(threshold);

	needsUpdate = false;
}

MTBackgroundSubstraction2UI::
MTBackgroundSubstraction2UI(const std::shared_ptr<MTVideoProcess>& videoProcess, ofImageType imageType) :
		MTVideoProcessUIWithImage(videoProcess, imageType)
{
}

void MTBackgroundSubstraction2UI::draw(ofxImGui::Settings& settings)
{
	auto vp = std::dynamic_pointer_cast<MTBackgroundSubstraction2>(videoProcess.lock());
	drawImage();
	ofxImGui::AddParameter<bool>(vp->isActive);
	ofxImGui::AddParameter<float>(vp->threshold);
	ofxImGui::AddParameter<int>(vp->history);
	ofxImGui::AddParameter<bool>(vp->detectShadows);
	ImGui::Separator();
	ofxImGui::AddParameter<int>(vp->erodeSize);
	ofxImGui::AddParameter<int>(vp->erodeShapeType);
	ofxImGui::AddParameter<int>(vp->dilateSize);
	ofxImGui::AddParameter<int>(vp->dilateShapeType);
}