//
// Created by Cristobal Mendoza on 3/4/18.
//

#ifndef NERVOUSSTRUCTUREOF_MTVIDEOPROCESSVIEW_HPP
#define NERVOUSSTRUCTUREOF_MTVIDEOPROCESSVIEW_HPP

#include <utils/ofThreadChannel.h>
#include "ofxImGui.h"
#include "MTAppFrameworkUtils.hpp"
#include "ofxCv.h"

class MTVideoProcess;

class MTVideoProcessUI : public std::enable_shared_from_this<MTVideoProcessUI>,
						 public MTEventListenerStore
{
public:
	MTVideoProcessUI(std::shared_ptr<MTVideoProcess> videoProcess);

	virtual ~MTVideoProcessUI()
	{}

	virtual void draw()
	{}

	virtual void draw(ofxImGui::Settings& settings);

	std::string getName()
	{ return name; };

protected:
	std::weak_ptr<MTVideoProcess> videoProcess;
private:
	std::string name;
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
	void drawImage();
	int outputImageWidth = 320;
	int outputImageHeight = 240;

	ofTexture outputImage;

protected:

	ofThreadChannel<cv::Mat> outputChannel;

private:

	template<class T>
	void loadTextureData(cv::Mat cvImage)
	{
		ofPixels_<T> pixels;
		ofxCv::toOf(cvImage, pixels);

		if (!outputImage.isAllocated() ||
			outputImage.getWidth() != cvImage.cols ||
			outputImage.getHeight() != cvImage.rows)
		{
			outputImage.allocate(pixels, false); //ImGui needs GL_TEXTURE_2D
		}
		else
		{
			outputImage.loadData(pixels);
		}
	}
};

#endif //NERVOUSSTRUCTUREOF_MTVIDEOPROCESSVIEW_HPP
