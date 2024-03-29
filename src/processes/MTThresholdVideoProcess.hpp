//
// Created by Cristobal Mendoza on 3/4/18.
//

#ifndef NERVOUSSTRUCTUREOF_MTCOMMONPROCESSES_HPP
#define NERVOUSSTRUCTUREOF_MTCOMMONPROCESSES_HPP

#include <utils/ofThreadChannel.h>
#include "MTVideoProcess.hpp"
#include "MTVideoProcessUI.hpp"
#include "ofxMTVideoInput.h"

class MTThresholdVideoProcess : public MTVideoProcess
{
public:
	ofParameter<int> threshold;

	MTThresholdVideoProcess();
	void process(MTProcessData& processData) override;
	std::shared_ptr<MTVideoProcessUI> createUI() override;
};

class MTThresholdVideoProcessUI : public MTVideoProcessUIWithImage
{
public:
	MTThresholdVideoProcessUI(const std::shared_ptr <MTVideoProcess>& videoProcess, ofImageType imageType);
	void draw(ofxImGui::Settings& settings) override;
};

#endif