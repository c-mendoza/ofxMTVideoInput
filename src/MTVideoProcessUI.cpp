//
// Created by Cristobal Mendoza on 3/4/18.
//

#include "MTVideoProcessUI.hpp"
#include "MTVideoProcess.hpp"
#include "ofThreadChannel.h"

MTVideoProcessUI::MTVideoProcessUI(std::shared_ptr<MTVideoProcess> videoProcess)
{
	this->videoProcess = videoProcess;
}
void MTVideoProcessUI::draw(ofxImGui::Settings& settings)
{
	ofxImGui::AddGroup(videoProcess.lock()->getParameters(), settings);
}