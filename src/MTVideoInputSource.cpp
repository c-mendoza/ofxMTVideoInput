//
// Created by Cristobal Mendoza on 7/22/20.
//

#include "MTVideoInputSource.hpp"
#include "MTModel.hpp"

MTVideoInputSource::MTVideoInputSource(std::string name, std::string typeName, std::string friendlyTypeName, std::string _deviceID) : MTModel(name)
{
	addParameters(this->typeName.set("Input Type Name", typeName),
				  this->friendlyTypeName.set("Friendly Type Name", friendlyTypeName),
				  deviceID.set("Device ID", _deviceID),
				  captureSize.set("Capture Size", glm::vec2(320, 240), glm::vec2(40, 30), glm::vec2(1920, 1080)),
				  frameRate.set("Frame Rate", 30, 5, 120));


	 addEventListener(deviceID.newListener([this](std::string id) {
			setup();
	 }));

	 addEventListener(captureSize.newListener([this](glm::ivec2& size) {
			setup();
	 }));

	 addEventListener(frameRate.newListener([this](int rate) {
			setup();
	 }));
}

void MTVideoInputSource::deserialize(ofXml& serializer)
{
	 isDeserializing = true;
	 MTModel::deserialize(serializer);
	 isDeserializing = false;
}

MTVideoInputSourceUI::MTVideoInputSourceUI(std::shared_ptr<MTVideoInputSource> inputSource)
{
	this->inputSource = inputSource;
}

void MTVideoInputSourceUI::draw(ofxImGui::Settings& settings)
{
	ofxImGui::AddGroup(inputSource.lock()->getParameters(), settings);
}

MTVideoInputSourceUIWithImage::
MTVideoInputSourceUIWithImage(std::shared_ptr<MTVideoInputSource> inputSource,
						  ofImageType imageType) : MTVideoInputSourceUI(inputSource)
{
	addEventListener(inputSource->frameCapturedEvent.newListener([this](
			const std::shared_ptr<MTVideoInputSource>& source)
																		{
																			ofPixels pixels = source->getPixels();
																			outputChannel.send(std::move(pixels));
																		}));
}

MTVideoInputSourceUIWithImage::~MTVideoInputSourceUIWithImage()
{
	outputChannel.close();
}

void MTVideoInputSourceUIWithImage::draw(ofxImGui::Settings& settings)
{
	ofPixels pixels;
	while (outputChannel.tryReceive(pixels))
	{
		if (!outputImage.isAllocated() ||
		pixels.getWidth() != outputImage.getWidth() ||
		pixels.getHeight() != outputImage.getWidth())
		{
			outputImage.allocate(pixels, false); //ImGui needs GL_TEXTURE_2D
		}
		else
		{
			outputImage.loadData(pixels);
		}
	}
	ofxImGui::AddImage(outputImage, glm::vec2(outputImageWidth, outputImageHeight));
}

