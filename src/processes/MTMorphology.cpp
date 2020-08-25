//
// Created by Cristobal Mendoza on 10/11/19.
//

#include <MTVideoInputStream.hpp>
#include "MTMorphology.hpp"

MTMorphologyVideoProcess::
MTMorphologyVideoProcess() : MTVideoProcess("Morphology", "MTMorphologyVideoProcess")
{
	parameters.add(
			size.set("Kernel Size", 1, 1, 25),
			shapeType.set("Shape Type", 0, 0, 2),
			mode.set("Mode", 0, 0, 1)
	);

	addEventListener(size.newListener([this](int args)
									  {
										  needsUpdate = true;
									  }));
	addEventListener(shapeType.newListener([this](int args)
										   {
											   needsUpdate = true;
										   }));
}

void MTMorphologyVideoProcess::setup()
{
	MTVideoProcess::setup();
	updateInternals();
}

void MTMorphologyVideoProcess::process(MTProcessData& processData)
{
	if (needsUpdate) updateInternals();

	switch (mode)
	{
		case Erode:
			cv::erode(processData.processStream, processOutput, kernel);
			break;
		case Dilate:
			cv::dilate(processData.processStream, processOutput, kernel);
			break;
	}

	processData.processStream = processOutput;
	processData.processResult = processOutput;
}

void MTMorphologyVideoProcess::updateInternals()
{
	kernel = cv::getStructuringElement(shapeType,
									   cv::Size(2 * size + 1, 2 * size + 1),
									   cv::Point(size, size));
	needsUpdate = false;
}

std::shared_ptr<MTVideoProcessUI> MTMorphologyVideoProcess::createUI()
{
	return std::make_shared<MTMorphologyVideoProcessUI>(shared_from_this(), OF_IMAGE_GRAYSCALE);
}

void MTMorphologyVideoProcessUI::draw(ofxImGui::Settings& settings)
{
	auto vp = videoProcess.lock();
	auto mp = std::dynamic_pointer_cast<MTMorphologyVideoProcess>(vp);
	MTVideoProcessUIWithImage::draw(settings);
	ofxImGui::AddParameter<bool>(mp->isActive);
	ofxImGui::AddRadio(mp->mode, modeLabels);
	ofxImGui::AddParameter<int>(mp->size);
	ofxImGui::AddParameter<int>(mp->shapeType);
}

MTMorphologyVideoProcessUI::MTMorphologyVideoProcessUI(const std::shared_ptr<MTVideoProcess>& videoProcess,
													   ofImageType imageType) :
		MTVideoProcessUIWithImage(videoProcess, imageType)
{
	modeLabels = {"Erode", "Dilate"};
}
