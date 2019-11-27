//
// Created by Cristobal Mendoza on 3/5/18.
//
#include "MTVideoProcess.hpp"
#include "MTVideoProcessUI.hpp"

MTVideoProcess::MTVideoProcess(std::string name, std::string typeName) : MTModel(name)
{
	processTypeName.set("Process Type Name", typeName);
	isActive.set("Active", true);
	parameters.add(processTypeName, isActive);
	processWidth = 320;
	processHeight = 240;
};

MTVideoProcess::~MTVideoProcess()
{}


void MTVideoProcess::setup()
{
	processBuffer.create(processHeight, processWidth, CV_8UC1);
	processOutput.create(processHeight, processWidth, CV_8UC1);
}

std::shared_ptr<MTVideoProcessUI> MTVideoProcess::createUI()
{
	return std::make_shared<MTVideoProcessUI>(shared_from_this());
}
