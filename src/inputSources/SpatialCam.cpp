//
// Created by cmendoza on 8/27/20.
//

#include "SpatialCam.hpp"

SpatialCam::SpatialCam(std::string spatialCamId) : MTModel("SpatialCam_" + spatialCamId)
{
	auto zerov = glm::vec3(0,0,0);
	parameters.add(
		position.set("Position", zerov),
		rotation.set("Rotation", zerov, zerov, glm::vec3(TWO_PI, TWO_PI, TWO_PI)),
		serialId.set("RS Serial ID", "")
		);
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
		this->serialId = serialId;
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
	return _isSetup;
}

void SpatialCam::serialize(ofXml& serializer)
{
	MTModel::serialize(serializer);
}

void SpatialCam::deserialize(ofXml& serializer)
{
	MTModel::deserialize(serializer);
	if (serialId.get() != "")
	{
		setup(serialId.get());
	}
	
}
