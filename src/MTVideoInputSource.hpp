//
// Created by Cristobal Mendoza on 7/12/20.
//

#ifndef NERVOUSSTRUCTUREOF_MTVIDEOINPUTSOURCE_HPP
#define NERVOUSSTRUCTUREOF_MTVIDEOINPUTSOURCE_HPP

#include <stdio.h>
#include "MTModel.hpp"
#include "ofxCv.h"
#include "registry.h"


class MTCaptureEventArgs;

class MTVideoInputSource :
	 public std::enable_shared_from_this<MTVideoInputSource>, public MTModel
{

public:
/**
* @param name The friendly name for the input source. It will be used as the identifier
* in the ofParameterGroup
* @param typeName The class name of the input source in string format. It will be used by the
* Registry to associate the class with the right constructor.
*/
	 MTVideoInputSource(std::string name, std::string typeName, std::string friendlyTypeName, std::string _deviceID);

	 virtual ~MTVideoInputSource()
	 {
			this->close();
	 }

	 virtual bool isFrameNew() = 0;
	 virtual const ofPixels& getPixels() = 0;
	 virtual cv::Mat getCVPixels() = 0;
	 virtual void start() = 0;

	 virtual void close()
	 {}

	 virtual void update() = 0;
	 virtual void setup() = 0;
	 virtual void setup(int width, int height, int framerate, std::string deviceID = "") = 0;
	 ofParameter<glm::ivec2> captureSize;
	 ofParameter<int> frameRate;
	 ofParameter<std::string> deviceID;
	 ofReadOnlyParameter<std::string, MTVideoInputSource> typeName;
	 ofReadOnlyParameter<std::string, MTVideoInputSource> friendlyTypeName;
	 ofFastEvent<std::shared_ptr<MTVideoInputSource>> frameCapturedEvent;

	 bool isRunning() const
	 {
			return running.load();
	 }

	 void setRunning(bool running)
	 {
			this->running.store(running);
	 }

	 const std::string& getTypeName() const
	 {
			return typeName;
	 }

	 void deserialize(ofXml& serializer) override;

protected:
	 bool isDeserializing = false;
private:
	 std::atomic_bool running;
};

class MTVideoInputSourceUI : public std::enable_shared_from_this<MTVideoInputSourceUI>,
														 public MTEventListenerStore
{
public:
	 MTVideoInputSourceUI(std::shared_ptr<MTVideoInputSource> inputSource);

	 virtual ~MTVideoInputSourceUI()
	 {}

	 virtual void draw()
	 {}

	 virtual void draw(ofxImGui::Settings& settings);

protected:
	 std::weak_ptr<MTVideoInputSource> inputSource;
};

class MTVideoInputSourceUIWithImage : public MTVideoInputSourceUI
{
public:
	 MTVideoInputSourceUIWithImage(std::shared_ptr<MTVideoInputSource> inputSource, ofImageType imageType);
	 ~MTVideoInputSourceUIWithImage();

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

//class MTCaptureEventArgs : public ofEventArgs
//{
//public:
//	std::shared_ptr<MTVideoInputSource> inputSource;
//	ofPixels frame;
//};

#endif //NERVOUSSTRUCTUREOF_MTVIDEOINPUTSOURCE_HPP
