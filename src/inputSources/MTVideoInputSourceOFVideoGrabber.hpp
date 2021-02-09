//
// Created by Cristobal Mendoza on 7/17/20.
//

#ifndef NERVOUSSTRUCTUREOF_MTVIDEOINPUTSOURCEOFVIDEOGRABBER_HPP
#define NERVOUSSTRUCTUREOF_MTVIDEOINPUTSOURCEOFVIDEOGRABBER_HPP

#include "MTVideoInputSource.hpp"

class MTVideoInputSourceOFGrabber : public MTVideoInputSource
{
public:
	MTVideoInputSourceOFGrabber(std::string devID);
	bool isFrameNew() override;
	ofPixels& getPixels() override;
	void start() override;
	void close() override;
	void update() override;
	void setup() override;
	void setup(int width, int height, int framerate, std::string deviceID) override;

private:
	ofVideoGrabber grabber;
};


#endif //NERVOUSSTRUCTUREOF_MTVIDEOINPUTSOURCEOFVIDEOGRABBER_HPP
