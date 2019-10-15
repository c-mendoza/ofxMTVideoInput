//
// Created by Cristobal Mendoza on 10/11/19.
//

#ifndef NERVOUSSTRUCTUREOF_MTMORPHOLOGYVIDEOPROCESS_HPP
#define NERVOUSSTRUCTUREOF_MTMORPHOLOGYVIDEOPROCESS_HPP

#include <stdio.h>
#include <MTVideoProcessUI.hpp>
#include "ofxCv.h"
#include "MTVideoProcess.hpp"
#include "registry.h"

class MTMorphologyVideoProcess : public MTVideoProcess
{
public:

	ofParameter<int> size = 0;
	ofParameter<int> mode;
	ofParameter<int> shapeType;

	enum MorphologyMode
	{
		Erode = 0,
		Dilate
	};


//	int const max_kernel_size = 21;

	MTMorphologyVideoProcess();
	void setup() override;
	void process(MTProcessData& processData) override;
	std::shared_ptr<MTVideoProcessUI> createUI() override;

protected:
	cv::Mat kernel;

	void updateInternals();
	bool needsUpdate = false;
};

#pragma mark UI

class MTMorphologyVideoProcessUI : public MTVideoProcessUIWithImage
{
public:
	MTMorphologyVideoProcessUI(const std::shared_ptr <MTVideoProcess>& videoProcess,
									 ofImageType imageType);
	void draw(ofxImGui::Settings& settings) override;

	std::vector<std::string> modeLabels;
};

#endif //NERVOUSSTRUCTUREOF_MTMORPHOLOGYVIDEOPROCESS_HPP
