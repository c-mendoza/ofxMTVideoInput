//
// Created by Cristobal Mendoza on 3/4/18.
//

#include "MTVideoProcessUI.hpp"
#include "MTVideoProcess.hpp"
#include "ofThreadChannel.h"
#include "MTVideoProcessStream.hpp"

MTVideoProcessUI::MTVideoProcessUI(std::shared_ptr<MTVideoProcess> videoProcess)
{
	this->videoProcess = videoProcess;
}

void MTVideoProcessUI::draw(ofxImGui::Settings& settings)
{
	ofxImGui::AddGroup(videoProcess.lock()->getParameters(), settings);
}

MTVideoProcessUIWithImage::
MTVideoProcessUIWithImage(std::shared_ptr<MTVideoProcess> videoProcess,
						  ofImageType imageType) : MTVideoProcessUI(videoProcess)
{
	auto stream = videoProcess->processChain.lock();
	outputImage.allocate(stream->processWidth, stream->processHeight, imageType);
	addEventListener(videoProcess->processCompleteFastEvent.newListener([this](
			const MTVideoProcessFastEventArgs<MTVideoProcess>& args)
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
	ofPixels pixels;
	while (outputChannel.tryReceive(pixels))
	{
		outputImage.setFromPixels(pixels);
	}
	ofxImGui::AddImage(outputImage, glm::vec2(outputImageWidth, outputImageHeight));
}
