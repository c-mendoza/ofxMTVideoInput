//
// Created by Cristobal Mendoza on 3/6/18.
//

#include "MTVideoProcess.hpp"
#include "MTVideoProcessUI.hpp"

#ifndef MTBACKGROUNDSUBSTRACTIONVIDEOPROCESS_HPP
#define MTBACKGROUNDSUBSTRACTIONVIDEOPROCESS_HPP

#pragma mark Process

class MTBackgroundSubstractionVideoProcess : public MTVideoProcess
{
public:
	ofParameter<int> threshold;

	MTBackgroundSubstractionVideoProcess();
	void setup() override;
	MTProcessData& process(MTProcessData& input) override;
	std::unique_ptr<MTVideoProcessUI> createUI() override;
	cv::Ptr<cv::BackgroundSubtractor> getBackgroundSubtractor() { return bSub; }

protected:
	cv::Ptr<cv::BackgroundSubtractor> bSub;
};

#pragma mark UI

class MTBackgroundSubstractionVideoProcessUI : public MTVideoProcessUIWithImage
{
public:
	MTBackgroundSubstractionVideoProcessUI(const std::shared_ptr <MTVideoProcess>& videoProcess,
										   ofImageType imageType);
	void draw(ofxImGui::Settings& settings) override;

};

#endif //MTBACKGROUNDSUBSTRACTIONVIDEOPROCESS_HPP