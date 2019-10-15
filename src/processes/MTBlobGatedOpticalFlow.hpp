////
//// Created by Cristobal Mendoza on 10/11/19.
////
//
//#ifndef NERVOUSSTRUCTUREOF_MTBLOBGATEDOPTICALFLOW_HPP
//#define NERVOUSSTRUCTUREOF_MTBLOBGATEDOPTICALFLOW_HPP
//
//#include <stdio.h>
//#include "ofxCv.h"
//#include "MTVideoProcess.hpp"
//#include "registry.h"
//
//class MTVideoProcessUI;
//
//class MTBlobGatedOpticalFlow : public MTVideoProcess
//{
//
//public:
//
//	explicit MTBlobGatedOpticalFlow();
//
//	virtual ~MTOpticalFlowVideoProcess()
//	{}
//
//	ofParameter<float> fbPolySigma;
//	ofParameter<float> fbPyrScale;
//	ofParameter<int> fbWinSize;
//	ofParameter<int> fbPolyN;
//	ofParameter<int> fbIterations;
//	ofParameter<int> fbLevels;
//	ofParameter<bool> fbUseGaussian;
//	ofFastEvent<MTVideoProcessCompleteFastEventArgs<MTOpticalFlowVideoProcess>> opticalFlowProcessCompleteFastEvent;
//
//
//	void notifyEvents() override;
//
////	std::shared_ptr<MTVideoProcessUI> createUI() override;
////	void drawGui(ofxImGui::Settings& settings) override;
////	virtual void deserialize(ofXml& serializer);
////	virtual void saveWithSerializer(ofXml& serializer);
//
//	/**
//	 * Returns a cv::Mat with the flow vectors. Input image is unchanged
//	 */
//	MTProcessData& process(MTProcessData& processData) override;
//
//	const cv::Vec2f& getFlowPosition(int x, int y);
//
//private:
//	ofxCv::FlowFarneback fb;
//	ofxCv::Flow* curFlow;
//	cv::Mat workingImage;
//
//
//};
//
//
//
//
//#endif //NERVOUSSTRUCTUREOF_MTBLOBGATEDOPTICALFLOW_HPP
