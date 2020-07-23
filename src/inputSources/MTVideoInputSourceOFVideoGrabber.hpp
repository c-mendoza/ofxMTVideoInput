//
// Created by Cristobal Mendoza on 7/17/20.
//

#ifndef NERVOUSSTRUCTUREOF_MTVIDEOINPUTSOURCEOFVIDEOGRABBER_HPP
#define NERVOUSSTRUCTUREOF_MTVIDEOINPUTSOURCEOFVIDEOGRABBER_HPP

#include "MTVideoInputSource.hpp"

class MTVideoInputSourceOFGrabber : public MTVideoInputSource
{
public:
	MTVideoInputSourceOFGrabber();
	bool isFrameNew() override;
	ofPixels& getPixels() override;
	cv::Mat getCVPixels() override;
	void start() override;
	void close() override;
	void update() override;
	void setup() override;
	void setup(int width, int height, int framerate, std::string deviceID) override;

private:
	ofVideoGrabber grabber;
};

auto bogus = MTVideoInput::Instance()
		.registerInputSource<MTVideoInputSourceOFGrabber>("MTVideoInputSourceOFGrabber", []()
		{
			std::vector<MTVideoInputSourceInfo> sources;
			auto ofDevices = ofVideoGrabber().listDevices();
			for (auto device : ofDevices)
			{
				MTVideoInputSourceInfo info;
				info.name = device.deviceName;
				info.deviceID = device.id;
				info.type = "MTVideoInputSourceOFGrabber";
				sources.push_back(info);
			}
			return sources;
		});

#endif //NERVOUSSTRUCTUREOF_MTVIDEOINPUTSOURCEOFVIDEOGRABBER_HPP
