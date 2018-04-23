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
//	useTransform.set("Use Transform", false);
//	processBuffer = cv::Mat
};

MTVideoProcess::~MTVideoProcess()
{}

std::unique_ptr<MTVideoProcessUI> MTVideoProcess::createUI()
{
	auto vp = shared_from_this();
	videoProcessUI = std::make_unique<MTVideoProcessUI>(vp);
	return std::move(videoProcessUI);
}
