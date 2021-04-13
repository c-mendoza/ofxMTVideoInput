//
// Created by Cristobal Mendoza on 7/22/20.
//

#include "MTVideoInputSource.hpp"
#include "MTModel.hpp"


std::function<void(std::shared_ptr<MTVideoInputSource>,
				   ofxImGui::Settings&)> MTVideoInputSource::RenderImGuiDelegate = nullptr;

MTVideoInputSource::MTVideoInputSource(std::string name, std::string typeName, std::string friendlyTypeName,
									   std::string _deviceID) : MTModel(name)
{
	addParameters(this->typeName.set("Input Type Name", typeName),
				  this->friendlyTypeName.set("Friendly Type Name", friendlyTypeName),
				  deviceID.set("Device ID", _deviceID),
				  captureSize.set("Capture Size", glm::vec2(320, 240), glm::vec2(40, 30), glm::vec2(1920, 1080)),
				  frameRate.set("Frame Rate", 30, 0, 120));


	addEventListener(deviceID.newListener([this](std::string id)
										  {
											  setup();
										  }));

	addEventListener(captureSize.newListener([this](glm::ivec2& size)
											 {
												 setup();
											 }));

	addEventListener(frameRate.newListener([this](int rate)
										   {
											   setup();
										   }));
}

void MTVideoInputSource::deserialize(ofXml& serializer)
{
	isDeserializing = true;
	MTModel::deserialize(serializer);
	isDeserializing = false;
}

void MTVideoInputSource::renderImGui(ofxImGui::Settings& settings)
{
	if (RenderImGuiDelegate)
	{
		RenderImGuiDelegate(shared_from_this(), settings);
	}
	else
	{
		ofxImGui::AddGroup(getParameters(), settings);
	}
}