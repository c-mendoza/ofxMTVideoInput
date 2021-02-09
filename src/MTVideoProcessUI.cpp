//
// Created by Cristobal Mendoza on 3/4/18.
//

#include "MTVideoProcessUI.hpp"
#include "MTVideoProcess.hpp"
#include "ofThreadChannel.h"
#include "MTVideoInputStream.hpp"

MTVideoProcessUI::MTVideoProcessUI(std::shared_ptr<MTVideoProcess> videoProcess)
{
	this->videoProcess = videoProcess;
	name = videoProcess->getName();
}

void MTVideoProcessUI::draw(ofxImGui::Settings& settings)
{
	for (auto& param : videoProcess.lock()->getParameters()) {
		ofxImGui::AddParameter(param);
	}
//	ofxImGui::AddGroup(videoProcess.lock()->getParameters(), settings);
}

MTVideoProcessUIWithImage::
MTVideoProcessUIWithImage(std::shared_ptr<MTVideoProcess> videoProcess,
						  ofImageType imageType) : MTVideoProcessUI(videoProcess)
{
	auto stream = videoProcess->processStream.lock();
	addEventListener(videoProcess->processCompleteFastEvent.newListener([this](
			const MTVideoProcessCompleteFastEventArgs<MTVideoProcess>& args)
																		{
																			ofPixels pixels;
																			ofxCv::toOf(args.processOutput, pixels);
																			outputChannel.send(std::move(pixels));
																		}));
}

MTVideoProcessUIWithImage::~MTVideoProcessUIWithImage()
{
	outputChannel.close();
}

void MTVideoProcessUIWithImage::draw(ofxImGui::Settings& settings)
{
	drawImage();
	MTVideoProcessUI::draw(settings);
}

void MTVideoProcessUIWithImage::drawImage()
{
	ofPixels pixels;
	while (outputChannel.tryReceive(pixels))
	{
		if (!outputImage.isAllocated() ||
			outputImage.getWidth() != pixels.getWidth() ||
			outputImage.getHeight() != pixels.getHeight())
		{
			outputImage.allocate(pixels, false); //ImGui needs GL_TEXTURE_2D
		}
		else
		{
			outputImage.loadData(pixels);
		}
	}
	if (outputImage.isAllocated())
	{
		auto w = ImGui::GetContentRegionAvailWidth();
		auto ratio = w / outputImage.getWidth();
		ofxImGui::AddImage(outputImage, glm::vec2(outputImage.getWidth() * ratio, outputImage.getHeight() * ratio));
	}
}
