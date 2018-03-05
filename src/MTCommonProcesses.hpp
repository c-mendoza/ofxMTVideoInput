//
// Created by Cristobal Mendoza on 3/4/18.
//

#ifndef NERVOUSSTRUCTUREOF_MTCOMMONPROCESSES_HPP
#define NERVOUSSTRUCTUREOF_MTCOMMONPROCESSES_HPP

#include "MTVideoProcess.hpp"

class MTThresholdVideoProcess : public MTVideoProcess
{
	ofParameter<int> threshold;

	MTThresholdVideoProcess(std::string name);
	MTProcessData& process(MTProcessData& input) override;
	void drawGui(ofxImGui::Settings& settings) override;
};


#endif //NERVOUSSTRUCTUREOF_MTCOMMONPROCESSES_HPP
