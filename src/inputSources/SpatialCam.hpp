//
// Created by cmendoza on 8/27/20.
//

#ifndef OFXMTVIDEOINPUT_SPATIALCAM_HPP
#define OFXMTVIDEOINPUT_SPATIALCAM_HPP

#include "SpatialCamWorld.hpp"
#include "ofParameter.h"

class SpatialCam : public MTModel
{
public:
	SpatialCam(std::string spatialCamId);
	~SpatialCam();

	ofParameter<float> x;
	ofParameter<float> y;
	ofParameter<float> z;
	ofParameter<float> rx;
	ofParameter<float> ry;
	ofParameter<float> rz;
	ofParameter<std::string> serialId;

	void getDepthFrame();
	bool setup(std::string serialId);
	bool isSetup();
	virtual void serialize(ofXml& serializer);
	virtual void deserialize(ofXml& serializer);

private:
	std::shared_ptr<MTVideoInputSourceRealSense> depthCamera;
	bool _isSetup = false;

};


#endif //OFXMTVIDEOINPUT_SPATIALCAM_HPP
