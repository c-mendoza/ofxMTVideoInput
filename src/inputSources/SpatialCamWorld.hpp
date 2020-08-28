//
// Created by cmendoza on 8/27/20.
//

#ifndef OFXMTVIDEOINPUT_SPATIALCAMWORLD_HPP
#define OFXMTVIDEOINPUT_SPATIALCAMWORLD_HPP


#include <MTVideoInputSource.hpp>
#include <utils/ofThread.h>
#include <3d/ofEasyCam.h>
#include "MTVideoInputSourceRealSense.hpp"

class SpatialCamWorld : public MTVideoInputSource,
						public ofThread
{
public:
	SpatialCamWorld();
	~SpatialCamWorld();
	bool isFrameNew() override;
	const ofPixels& getPixels() override;
	void start() override;
	void close() override;
	void update() override;
	void setup() override;
	void setup(int width, int height, int framerate, std::string deviceID) override;
	void threadedFunction() override;
	void serialize(ofXml& serializer) override;
	void deserialize(ofXml& serializer) override;

	bool addSpatialCamera(std::shared_ptr<SpatialCam> cam);
	bool removeSpatialCamera(std::shared_ptr<SpatialCam> cam);

private:
	std::vector<std::shared_ptr<SpatialCam>> cameras;
	ofEasyCam outputCamera;
};


#endif //OFXMTVIDEOINPUT_SPATIALCAMWORLD_HPP
