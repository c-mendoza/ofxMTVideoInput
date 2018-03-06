//
// Created by Cristobal Mendoza on 3/5/18.
//
#include "MTVideoProcess.hpp"
#include "MTVideoProcessUI.hpp"

MTVideoProcess::MTVideoProcess(std::string name) : MTModel(name)
{
	processTypeName.set("Process Type Name", typeid(this).name());
	useTransform.set("Use Transform", false);
	parameters.add(processTypeName, useTransform);
};

MTVideoProcess::~MTVideoProcess()
{}

std::unique_ptr<MTVideoProcessUI> MTVideoProcess::createUI()
{
	auto vp = shared_from_this();
	videoProcessUI = std::make_unique<MTVideoProcessUI>(vp);
	return std::move(videoProcessUI);
}