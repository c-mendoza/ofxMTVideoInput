//
// Created by Cristobal Mendoza on 3/6/18.
//

#ifndef NERVOUSSTRUCTUREOF_MTIMAGEADJUSTMENTS_HPP
#define NERVOUSSTRUCTUREOF_MTIMAGEADJUSTMENTS_HPP


#pragma mark Process

#include <MTVideoProcess.hpp>
#include <MTVideoProcessUI.hpp>



class MTImageAdjustmentsVideoProcess : public MTVideoProcess
{
public:
	ofParameter<float> gamma;
	ofParameter<float> brightness;
	ofParameter<float> contrast;
	ofParameter<bool> useHistogramEqualization;
	ofParameter<bool> useCLAHE;
	ofParameter<float> claheClipLimit;

	MTImageAdjustmentsVideoProcess();
	void setup() override;
	MTProcessData& process(MTProcessData& input) override;
	std::unique_ptr<MTVideoProcessUI> createUI() override;

protected:
	cv::Mat gammaLUT;
	cv::Mat bcLUT;
	cv::Ptr<cv::CLAHE> clahe;
	void updateGammaLUT();
	void updateBC();
	void updateCLAHE();
};

#pragma mark UI

class MTImageAdjustmentsVideoProcessUI : public MTVideoProcessUIWithImage
{
public:
	MTImageAdjustmentsVideoProcessUI(const std::shared_ptr <MTVideoProcess>& videoProcess,
										   ofImageType imageType);
	void draw(ofxImGui::Settings& settings) override;

};


#endif //NERVOUSSTRUCTUREOF_MTIMAGEADJUSTMENTS_HPP
