//
//  MTOpticalFlowVideoProcess.hpp
//  nervousstructureof
//
//  Created by Cristobal Mendoza on 3/14/16.
//
//

#ifndef NSOpticalFlowVideoProcess_hpp
#define NSOpticalFlowVideoProcess_hpp

#include <stdio.h>
#include "ofxCv.h"
#include "MTVideoProcess.hpp"
#include "registry.h"

class MTVideoProcessUI;

class MTOpticalFlowVideoProcess : public MTVideoProcess
{

public:

	explicit MTOpticalFlowVideoProcess();

	virtual ~MTOpticalFlowVideoProcess()
	{}

	ofParameter<float> learningRate;
	ofParameter<float> fbPolySigma;
	ofParameter<float> lkQualityLevel;
	ofParameter<float> fbPyrScale;
	ofParameter<int> lkMinDistance;
	ofParameter<int> lkMaxFeatures;
	ofParameter<int> lkMaxLevel;
	ofParameter<int> fbWinSize;
	ofParameter<int> fbPolyN;
	ofParameter<int> fbIterations;
	ofParameter<int> fbLevels;
	ofParameter<int> lkWinSize;
	ofParameter<bool> fbUseGaussian;
	ofParameter<bool> usefb;
	ofParameter<bool> useThreshold;
	ofParameter<int> threshold;
//	ofParameter<bool> useMorphFilter;
//	ofParameter<int> kernelSize;
	ofFastEvent<MTVideoProcessCompleteFastEventArgs<MTOpticalFlowVideoProcess>> opticalFlowProcessCompleteFastEvent;


	void notifyEvents() override;

//	std::shared_ptr<MTVideoProcessUI> createUI() override;
//	void drawGui(ofxImGui::Settings& settings) override;
//	virtual void deserialize(ofXml& serializer);
//	virtual void saveWithSerializer(ofXml& serializer);

	/**
	 * Returns a cv::Mat with the flow vectors. Input image is unchanged
	 */
	void process(MTProcessData& processData) override;

	const cv::Vec2f& getFlowPosition(int x, int y);

private:
	ofxCv::FlowFarneback fb;
	ofxCv::FlowPyrLK lk;
	ofxCv::Flow* curFlow;
	ofxCv::RunningBackground rb;
	cv::Mat fgMaskMOG2; //fg mask fg mask generated by MOG2 method
	cv::Ptr<cv::BackgroundSubtractor> pMOG2;
	cv::Mat workingImage;


};

#endif /* NSOpticalFlowVideoProcess_hpp */
