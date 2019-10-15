//
// Created by Cristobal Mendoza on 10/13/19.
//

#ifndef NERVOUSSTRUCTUREOF_MTBACKGROUNDSUBSTRACTION2_HPP
#define NERVOUSSTRUCTUREOF_MTBACKGROUNDSUBSTRACTION2_HPP

#include "MTVideoProcess.hpp"
#include "MTVideoProcessUI.hpp"

class MTBackgroundSubstraction2 : public MTVideoProcess
{
public:
	// History
	ofParameter<float> threshold;
	ofParameter<int> history;
	ofParameter<bool> detectShadows;
	ofParameter<bool> subtractStream;
	// Morphology
	ofParameter<int> erodeSize;
	ofParameter<int> erodeShapeType;
	ofParameter<int> dilateSize;
	ofParameter<int> dilateShapeType;

	MTBackgroundSubstraction2();
	void setup() override;
	void process(MTProcessData& processData) override;
	std::shared_ptr<MTVideoProcessUI> createUI() override;
	cv::Ptr<cv::BackgroundSubtractor> getBackgroundSubtractor() { return bSub; }

protected:
	cv::Ptr<cv::BackgroundSubtractorMOG2> bSub;
	cv::Mat erodeKernel;
	cv::Mat dilateKernel;
	cv::Mat erodeOutput;
	cv::Mat dilateOutput;
	cv::Mat bSubOutput;
	bool needsUpdate = true;

	void updateInternals();
};

class MTBackgroundSubstraction2UI : public MTVideoProcessUIWithImage
{
public:
	MTBackgroundSubstraction2UI(const std::shared_ptr <MTVideoProcess>& videoProcess,
										   ofImageType imageType);
	void draw(ofxImGui::Settings& settings) override;

};
#endif //NERVOUSSTRUCTUREOF_MTBACKGROUNDSUBSTRACTION2_HPP
