//
// Created by Cristobal Mendoza on 3/4/18.
//

#ifndef NERVOUSSTRUCTUREOF_MTVIDEOPROCESSVIEW_HPP
#define NERVOUSSTRUCTUREOF_MTVIDEOPROCESSVIEW_HPP

#include <utils/ofThreadChannel.h>
#include "ofxImGui.h"
#include "MTAppFrameworkUtils.hpp"

class MTVideoProcess;

class MTVideoProcessUI : public std::enable_shared_from_this<MTVideoProcessUI>,
						 public MTEventListenerStore
{
public:
	MTVideoProcessUI(std::shared_ptr<MTVideoProcess> videoProcess);

	virtual ~MTVideoProcessUI(){}

	virtual void draw(){}
	virtual void draw(ofxImGui::Settings& settings);

protected:
	std::weak_ptr<MTVideoProcess> videoProcess;
};

class MTVideoProcessUIWithImage : public MTVideoProcessUI
{
public:
	MTVideoProcessUIWithImage(std::shared_ptr<MTVideoProcess> videoProcess, ofImageType imageType);
	~MTVideoProcessUIWithImage();

	/**
	 * @brief Draws the video process output as an image via
	 * ofxImGui::AddImage.
	 * @param settings
	 */
	void draw(ofxImGui::Settings& settings) override;
	int outputImageWidth = 320;
	int outputImageHeight = 240;

	ofTexture outputImage;

protected:

	ofThreadChannel<ofPixels> outputChannel;
};

#endif //NERVOUSSTRUCTUREOF_MTVIDEOPROCESSVIEW_HPP
