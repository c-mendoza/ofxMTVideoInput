//
// Created by Cristobal Mendoza on 3/5/18.
//

#include "MTVideoInputProcess.hpp"

MTVideoInputProcess::MTVideoInputProcess(const std::string& name) : MTVideoProcess(name)
{}

MTProcessData& MTVideoInputProcess::process(MTProcessData& input)
{
	return input;
}

///////////////////////////////////////////////////////
#pragma mark MTVideoCaptureProcess
///////////////////////////////////////////////////////

MTVideoCaptureProcess::MTVideoCaptureProcess(const std::string& name) : MTVideoInputProcess(name)
{}

void MTVideoCaptureProcess::start()
{

}

void MTVideoCaptureProcess::stop()
{

}

void MTVideoCaptureProcess::update()
{

}

bool MTVideoCaptureProcess::isFrameNew()
{
	return false;
}

///////////////////////////////////////////////////////
#pragma mark MTVideoFileProcess
///////////////////////////////////////////////////////