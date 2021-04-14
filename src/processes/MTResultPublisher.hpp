//
// Created by Cristobal Mendoza on 4/14/2021
//

#include "MTVideoProcess.hpp"
#include "MTVideoProcessUI.hpp"

#ifndef MRESULTPUBLISHER_HPP
#define MRESULTPUBLISHER_HPP

class MTResultPublisher : public MTVideoProcess
{
public:
	ofParameter<std::string> resultId;
	MTResultPublisher();
	void setup() override;
	void process(MTProcessData& processData) override;
	std::shared_ptr<MTVideoProcessUI> createUI() override;
	void notifyEvents() override;


protected:

};

class MTResultPublisherUI : public MTVideoProcessUI
{
public:
	MTResultPublisherUI(const std::shared_ptr <MTVideoProcess>& videoProcess);
	void draw(ofxImGui::Settings& settings) override;

};

#endif //MTBACKGROUNDSUBSTRACTIONVIDEOPROCESS_HPP