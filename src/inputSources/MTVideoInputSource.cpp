//
// Created by Cristobal Mendoza on 7/22/20.
//

#include "MTVideoInputSource.hpp"
#include "MTModel.hpp"

MTVideoInputSource::MTVideoInputSource(std::string name, std::string typeName) : MTModel(name)
{
	addParameters(this->typeName.set("Input Type Name", typeName),
				  deviceID.set("Device ID", "none"),
				  captureSize.set("Capture Size", glm::vec2(320, 240), glm::vec2(40, 30), glm::vec2(1920, 1080)),
				  frameRate.set("Frame Rate", 30, 5, 120));


	addEventListener(deviceID.newListener([this](std::string id) {
		setup();
	}));

	addEventListener(captureSize.newListener([this](glm::vec2& size) {
		setup();
	}));

	addEventListener(frameRate.newListener([this](int rate) {
		setup();
	}));


}