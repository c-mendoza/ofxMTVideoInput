//
// Created by cmendoza on 8/27/20.
//

#include "SpatialCam.hpp"

SpatialCam::SpatialCam(std::string spatialCamId) : MTModel("SpatialCam_" + spatialCamId)
{

}

SpatialCam::~SpatialCam()
{
	if (depthCamera) depthCamera->close();
}

bool SpatialCam::setup(std::string serialId)
{
	MTVideoInputSourceInfo info;
	info.deviceID = serialId;
	info.type = "MTVideoInputSourceRealSense";
	auto cam = MTVideoInput::Instance().createInputSource(info);
	if (cam)
	{
		depthCamera = std::dynamic_pointer_cast<MTVideoInputSourceRealSense>(cam);
		_isSetup = true;
		return true;
	}
	else
	{
		_isSetup = false;
		return false;
	}
}

bool SpatialCam::isSetup()
{
	return false;
}

void SpatialCam::serialize(ofXml& serializer)
{
	MTModel::serialize(serializer);
}

void SpatialCam::deserialize(ofXml& serializer)
{
	MTModel::deserialize(serializer);
}
