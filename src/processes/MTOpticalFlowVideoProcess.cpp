//
//  MTOpticalFlowVideoProcess.cpp
//  nervousstructureof
//
//  Created by Cristobal Mendoza on 3/14/16.
//
//

#include <MTVideoInputStream.hpp>
#include <MTVideoProcessUI.hpp>
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
	parameters.add(useThreshold.set("Use Threshold Filter", false));
	parameters.add(threshold.set("Threshold", 0, 0, 5000));
}

void MTOpticalFlowVideoProcess::process(MTProcessData& processData)
{

	if (usefb)
	{
		curFlow = &fb;
		fb.setPyramidScale(fbPyrScale);
		fb.setNumLevels(fbLevels);
		fb.setWindowSize(fbWinSize);
		fb.setNumIterations(fbIterations);
		fb.setPolyN(fbPolyN);
		fb.setPolySigma(fbPolySigma);
		fb.setUseGaussian(fbUseGaussian);
//		fb.getFlow();
	}
	else
	{
		curFlow = &lk;
		lk.setMaxFeatures(lkMaxFeatures);
		lk.setQualityLevel(lkQualityLevel);
		lk.setMinDistance(lkMinDistance);
		lk.setWindowSize(lkWinSize);
		lk.setMaxLevel(lkMaxLevel);
	}

	// Check to see if the image size has changed. If so, reset the flow:
	if (fb.getWidth() != processData.processStream.cols ||
		fb.getHeight() != processData.processStream.rows)
	{
		// Check for the special case of the first capture frame:
		// If getWidth is 0 then we are starting up capture, so we should
		// not reset the flow
		if (fb.getWidth() != 0)
		{
			fb.resetFlow();
		}
	}

	curFlow->calcOpticalFlow(processData.processStream);

	if (useThreshold)
	{
		cv::Mat buf;
		ofxCv::imitate(buf, fb.getFlow());
		cv::pow(fb.getFlow(), 2, buf);
		auto totalScalar = cv::sum(buf);
//		float total = std::abs(totalScalar[0]) + std::abs(totalScalar[1]);
		float total = totalScalar[0] + totalScalar[1];
		if (total < threshold)
		{
			ofxCv::imitate(processOutput, fb.getFlow());
			processOutput = fb.getFlow().zeros(fb.getFlow().size(), fb.getFlow().type());
		}
		else
		{
			processOutput = fb.getFlow();
		}
	}
	else
	{
		processOutput = fb.getFlow();
	}

	processData.processResult = processOutput;
}

const cv::Vec2f& MTOpticalFlowVideoProcess::getFlowPosition(int x, int y)
{
	return fb.getFlow().at<cv::Vec2f>(y, x);
}

std::shared_ptr<MTVideoProcessUI> MTOpticalFlowVideoProcess::createUI()
{
	return std::make_shared<MTVideoProcessUIWithImage>(shared_from_this(), OF_IMAGE_COLOR);
}