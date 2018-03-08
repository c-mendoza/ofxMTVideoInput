//
// Created by Cristobal Mendoza on 3/7/18.
//

#ifndef NERVOUSSTRUCTUREOF_MTHISTOGRAMVIDEOPROCESS_HPP
#define NERVOUSSTRUCTUREOF_MTHISTOGRAMVIDEOPROCESS_HPP

#pragma mark Process

#include <MTVideoProcess.hpp>
#include <MTVideoProcessUI.hpp>



class MTHistogramVideoProcess : public MTVideoProcess
{
public:
	ofParameter<bool> useEqualization;
	ofParameter<bool> useCLAHE;

	MTHistogramVideoProcess(std::string name);
	void setup() override;
	MTProcessData& process(MTProcessData& input) override;
	std::unique_ptr<MTVideoProcessUI> createUI() override;

protected:
	cv::Mat gammaLUT;
	cv::Mat bcLUT;
	void updateGammaLUT();
	void updateBC();
};

#pragma mark UI

class MTHistogramVideoProcessUI : public MTVideoProcessUIWithImage
{
public:
	MTImageAdjustmentsVideoProcessUI(const std::shared_ptr <MTVideoProcess>& videoProcess,
									 ofImageType imageType);
	void draw(ofxImGui::Settings& settings) override;

};




#endif //NERVOUSSTRUCTUREOF_MTHISTOGRAMVIDEOPROCESS_HPP
