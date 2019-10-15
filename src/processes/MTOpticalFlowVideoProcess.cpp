//
//  MTOpticalFlowVideoProcess.cpp
//  nervousstructureof
//
//  Created by Cristobal Mendoza on 3/14/16.
//
//

#include <MTVideoInputStream.hpp>
#include "processes/MTOpticalFlowVideoProcess.hpp"


void MTOpticalFlowVideoProcess::notifyEvents()
{
	MTVideoProcess::notifyEvents();
	auto processEventArgs =
            MTVideoProcessCompleteFastEventArgs<MTOpticalFlowVideoProcess>
                    (processOutput, this);
	ofNotifyEvent(opticalFlowProcessCompleteFastEvent, processEventArgs, this);
}

MTOpticalFlowVideoProcess::MTOpticalFlowVideoProcess() :
        MTVideoProcess("Optical Flow", "MTOpticalFlowVideoProcess")
{
	parameters.add(usefb.set("Use Farneback", true));
	parameters.add(fbPyrScale.set("fbPyrScale", .5, 0, .99));
	parameters.add(fbLevels.set("fbLevels", 4, 1, 8));
	parameters.add(fbIterations.set("fbIterations", 2, 1, 8));
	parameters.add(fbPolyN.set("fbPolyN", 7, 5, 10));
	parameters.add(fbPolySigma.set("fbPolySigma", 1.5, 1.1, 2));
	parameters.add(fbUseGaussian.set("fbUseGaussian", false));
	parameters.add(fbWinSize.set("winSize", 32, 4, 64));
}

void MTOpticalFlowVideoProcess::process(MTProcessData& processData)
{

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
	curFlow->calcOpticalFlow(processData.processStream);
	processOutput = fb.getFlow();
	processData.processResult = processOutput;
}

const cv::Vec2f& MTOpticalFlowVideoProcess::getFlowPosition(int x, int y)
{
	return fb.getFlow().at<cv::Vec2f>(y, x);
}

