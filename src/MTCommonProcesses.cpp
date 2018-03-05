//
// Created by Cristobal Mendoza on 3/4/18.
//

#include "MTCommonProcesses.hpp"

MTThresholdVideoProcess::MTThresholdVideoProcess(std::string name) : MTVideoProcess(name)
{
	parameters.add(threshold.set("Threshold", 0, 255, 255));
}

MTProcessData& MTThresholdVideoProcess::process(MTProcessData& input)
{
	cv::threshold(input.at(MTVideoProcessStreamKey), processBuffer, threshold.get(), threshold.getMax(), CV_THRESH_BINARY);
	input[MTVideoProcessStreamKey] = processBuffer;
	input[MTVideoProcessResultKey] = processBuffer;
	return input;
}

void MTThresholdVideoProcess::drawGui(ofxImGui::Settings& settings)
{

	ofxImGui::AddGroup(getParameters(), settings);
}