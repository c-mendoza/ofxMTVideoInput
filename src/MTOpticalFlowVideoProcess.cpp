//
//  MTOpticalFlowVideoProcess.cpp
//  nervousstructureof
//
//  Created by Cristobal Mendoza on 3/14/16.
//
//

#include "MTOpticalFlowVideoProcess.hpp"

void MTOpticalFlowVideoProcess::notifyEvents()
{
	MTVideoProcess::notifyEvents();
	auto processEventArgs =
            MTVideoProcessFastEventArgs<MTOpticalFlowVideoProcess>
                    (processOutput, this);
	ofNotifyEvent(opticalFlowProcessCompleteFastEvent, processEventArgs, this);
}

MTOpticalFlowVideoProcess::MTOpticalFlowVideoProcess(std::string name) :
        MTVideoProcess(name)
{
//	processTypeName.set("Process Type Name", typeid(this).name());
	parameters.setName(this->getName());
//	parameters.add(lkMaxLevel.set("lkMaxLevel", 3, 0, 8));
//	parameters.add(lkMaxFeatures.set("lkMaxFeatures", 200, 1, 1000));
//	parameters.add(lkQualityLevel.set("lkQualityLevel", 0.01, 0.001, .02));
//	parameters.add(lkMinDistance.set("lkMinDistance", 4, 1, 16));
//	parameters.add(lkWinSize.set("lkWinSize", 8, 4, 64));
	parameters.add(usefb.set("Use Farneback", true));
	parameters.add(fbPyrScale.set("fbPyrScale", .5, 0, .99));
	parameters.add(fbLevels.set("fbLevels", 4, 1, 8));
	parameters.add(fbIterations.set("fbIterations", 2, 1, 8));
	parameters.add(fbPolyN.set("fbPolyN", 7, 5, 10));
	parameters.add(fbPolySigma.set("fbPolySigma", 1.5, 1.1, 2));
	parameters.add(fbUseGaussian.set("fbUseGaussian", false));
	parameters.add(fbWinSize.set("winSize", 32, 4, 64));
	parameters.add(useBackgroundSub.set("Use Background Substraction", false));
	parameters.add(useThreshold.set("Use Threshold", false));
	parameters.add(threshold.set("Threshold", 1, 1, 50));
	parameters.add(learningRate.set("LR", 1, 0.001, 10));

	pMOG2 = cv::createBackgroundSubtractorMOG2();
	rb.setThresholdValue(threshold);
	rb.setLearningRate(learningRate);

}

cv::Mat MTOpticalFlowVideoProcess::process(cv::Mat& image)
{
	if (useBackgroundSub)
	{
		pMOG2->apply(image, fgMaskMOG2);
		image.copyTo(workingImage, fgMaskMOG2);
	}
	else if (useThreshold)
	{
		rb.update(image, workingImage);
	}
	else
	{
		workingImage = image;
	}

//	cv::bitwise_and(image, fgMaskMOG2, workingImage);
	if(usefb) {
		curFlow = &fb;
		fb.setPyramidScale(fbPyrScale);
		fb.setNumLevels(fbLevels);
		fb.setWindowSize(fbWinSize);
		fb.setNumIterations(fbIterations);
		fb.setPolyN(fbPolyN);
		fb.setPolySigma(fbPolySigma);
		fb.setUseGaussian(fbUseGaussian);
//		fb.getFlow();
	} else {
		curFlow = &lk;
		lk.setMaxFeatures(lkMaxFeatures);
		lk.setQualityLevel(lkQualityLevel);
		lk.setMinDistance(lkMinDistance);
		lk.setWindowSize(lkWinSize);
		lk.setMaxLevel(lkMaxLevel);
	}

	// you can use Flow polymorphically
	curFlow->calcOpticalFlow(workingImage);
	processOutput = fb.getFlow();
//	notifyEvents();
	return processOutput;

}

ofVec2f MTOpticalFlowVideoProcess::getFlowPosition(int x, int y)
{
//	ofVec2f temp = fb.getFlowPosition(x, y);

	return fb.getFlow().at<ofVec2f>(y, x);
}

void MTOpticalFlowVideoProcess::drawGui(ofxImGui::Settings& settings)
{
    ofxImGui::AddGroup(getParameters(), settings);
}
