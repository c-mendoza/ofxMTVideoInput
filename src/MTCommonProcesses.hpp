//
// Created by Cristobal Mendoza on 3/4/18.
//

#ifndef NERVOUSSTRUCTUREOF_MTCOMMONPROCESSES_HPP
#define NERVOUSSTRUCTUREOF_MTCOMMONPROCESSES_HPP

#include <utils/ofThreadChannel.h>
#include "MTVideoProcess.hpp"
#include "MTVideoProcessUI.hpp"

class MTThresholdVideoProcess : public MTVideoProcess
{
public:
	ofParameter<int> threshold;

	MTThresholdVideoProcess(std::string name);
	MTProcessData& process(MTProcessData& input) override;
	std::unique_ptr<MTVideoProcessUI> createUI() override;

protected:

};

REGISTER_SUBCLASS(MTVideoProcess, MTThresholdVideoProcess, std::string)

class MTThresholdVideoProcessUI : public MTVideoProcessUI
{
public:
	MTThresholdVideoProcessUI(const std::shared_ptr <MTVideoProcess>& videoProcess);
	~MTThresholdVideoProcessUI() override;
	void draw(ofxImGui::Settings& settings) override;

protected:
	ofImage processImage;
	ofThreadChannel<ofPixels> outputChannel;

};

#endif //NERVOUSSTRUCTUREOF_MTCOMMONPROCESSES_HPP
