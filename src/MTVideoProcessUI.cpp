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
	for (auto& param : videoProcess.lock()->getParameters())
	{
		ofxImGui::AddParameter(param);
	}
//	ofxImGui::AddGroup(videoProcess.lock()->getParameters(), settings);
}

float MTVideoProcessUIWithImage::ImageScale = 1.0f;

MTVideoProcessUIWithImage::
MTVideoProcessUIWithImage(std::shared_ptr<MTVideoProcess> videoProcess,
						  ofImageType imageType) : MTVideoProcessUI(videoProcess)
{
	auto stream = videoProcess->processStream.lock();
	addEventListener(videoProcess->processCompleteFastEvent.newListener([this](
			const MTVideoProcessCompleteFastEventArgs<MTVideoProcess>& args)
																		{
																			outputChannel.send(args.processOutput);
																		}));
}

MTVideoProcessUIWithImage::~MTVideoProcessUIWithImage()
{
	outputChannel.close();
}

void MTVideoProcessUIWithImage::draw(ofxImGui::Settings& settings)
{
	ImGui::Checkbox("Render Image", &enabled);
	if (enabled)
	{
		drawImage();
		MTVideoProcessUI::draw(settings);
	}
}

void MTVideoProcessUIWithImage::drawImage()
{
	ofAbstractHasPixels pixels;
	cv::Mat cvImage;
	while (outputChannel.tryReceive(cvImage))
	{
		auto depth = cvImage.depth();
		if (depth == CV_16U)
		{
			loadTextureData<unsigned short>(cvImage);
		}
		else if (depth == CV_32F ||
				 depth == CV_64F /*|| depth == CV_16F*/)
		{
			loadTextureData<float>(cvImage);
		}
		else
		{
			loadTextureData<unsigned char>(cvImage);
		}

	}

	if (outputImage.isAllocated())
	{
		auto ow = outputImage.getWidth() * ImageScale;
		auto w = std::min(ow, ImGui::GetContentRegionAvailWidth());
		auto ratio = w / ow;
		auto h = outputImage.getHeight() * ImageScale * ratio;
		ofxImGui::AddImage(outputImage, glm::vec2(w, h));
	}
}
