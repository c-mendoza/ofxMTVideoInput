//
//  MTVideoInputStream.hpp
//  Created by Cristobal Mendoza on 3/14/16.
//
//

#ifndef MTVideoProcessChain_hpp
#define MTVideoProcessChain_hpp

#include <stdio.h>
#include "MTModel.hpp"
#include "ofThread.h"
#include "ofxCv.h"
#include "MTVideoProcess.hpp"
#include "MTVideoInputSource.hpp"
#include "ofxMTVideoInput.h"


class MTVideoInputStreamCompleteEventArgs : public ofEventArgs
{
public:
	 std::shared_ptr<MTVideoInputStream> stream;
/**
 * @brief The resulting Mat after all processing completes. The data in this Mat
 * is owned by the stream, so if you need to modify the data you must clone the Mat.
 */
	 cv::Mat result;
/**
 * @brief The initial Mat before processing. This will typically be
 * the unprocessed video input image. The data in this Mat
 * is owned by the stream, so if you need to modify the data you must clone the Mat.
 */
	 cv::Mat input;

/**
 * @brief The current frames per second reading of the stream.
 */
	 double fps;
};

class MTVideoInputStream : public MTModel,
													 public ofThread,
													 public std::enable_shared_from_this<MTVideoInputStream>
{

	 friend class MTVideoInput;

public:
	 MTVideoInputStream(std::string name);
	 ~MTVideoInputStream();

//ofReadonlyParameter does not serialize correctly!
	 ofParameter<bool> isRunning;

//////////////////////////////////
//Processes
//////////////////////////////////

	 ofParameter<bool> mirrorVideo;

// These are read only because they are not thread-safe. Use the setters
// declared
	 ofReadOnlyParameter<ofPath, MTVideoInputStream> outputRegion;
	 ofReadOnlyParameter<ofPath, MTVideoInputStream> inputROI;
	 ofParameter<float> processingSize;
	 ofParameter<bool> useROI;
	 ofParameterGroup processesParameters;
	 ofParameterGroup inputSourcesParameters;

private:
	 int inputWidth = 0;
	 int inputHeight = 0;
	 int processingWidth = 0;
	 int processingHeight = 0;

public:
	 std::vector<std::shared_ptr<MTVideoProcess>> videoProcesses;

//////////////////////////////////
/// Events
//////////////////////////////////

/**
 * @brief Fires when the stream is done processing, i.e. when all of the
 * MTVideoProcess instances in the stream have completed.
 */
	 ofEvent<MTVideoInputStreamCompleteEventArgs> streamCompleteEvent;
/**
 * @brief Fires when the stream is done processing, i.e. when all of the
 * MTVideoProcess instances in the stream have completed.
 */
	 ofFastEvent<MTVideoInputStreamCompleteEventArgs> streamCompleteFastEvent;
	 ofEvent<std::shared_ptr<MTVideoProcess>> processAddedEvent;
	 ofEvent<std::shared_ptr<MTVideoProcess>> processRemovedEvent;
	 ofEvent<void> processOrderChangedEvent;

//////////////////////////////////
//Getters and Setters
//////////////////////////////////
//	cv::Mat getProcessToWorldTransform();
//	cv::Mat getWorldToProcessTransform();
	int getWidth() {
		 return processingWidth;
	}

	int getHeight() {
		 return processingHeight;
	}
/**
 * @brief Returns a copy of the output transform. This method is thread-safe.
 * @return The clone() of the cv::Mat representing the output transform
 */
	 cv::Mat getProcessToOutputTransform();

/**
 * @brief Returns a clone of the output to process transform. This method is thread-safe.
 * @return cv::Mat clone
 */
	 cv::Mat getOutputToProcessTransform();

	 void setInputROI(ofPath& path)
	 {
            enqueueFunction([this, path]() {
                inputROI = path;
                updateTransformInternals();
            });
	 }

	 void setOutputRegion(ofPath& path)
	 {
         enqueueFunction([this, path]() {
             outputRegion = path;
             updateTransformInternals();
         });
	 }

private:
	 /**
	  * @brief Sets the processing resolution by scaling the input source pixel dimensions.
	  * @param Size should be between 0.1 and 1. A value of 1 means that processing will be done
	  * at 100% of the input source resolution. 0.1 would resize the processing resolution to 10% of
	  * input source resolution.
	  */
	 void setProcessingSize(float size);
	 float prevProcessingSize = 1.0f;
	 bool isDeserializing = false;

public:
	 void setInputSource(MTVideoInputSourceInfo sourceInf);
	 void setInputSource(MTVideoInputSourceInfo sourceInf, ofXml& serializer);

	 std::weak_ptr<MTVideoInputSource> getInputSource()
	 { return inputSource; }

//////////////////////////////////
//Utility
//////////////////////////////////

protected:
	 ofFpsCounter fpsCounter;
public:
	 double getFps()
	 { return fpsCounter.getFps(); }

//////////////////////////////////
//Data Handling
//////////////////////////////////
	 void addVideoProcess(std::shared_ptr<MTVideoProcess> process);
	 void addVideoProcessAtIndex(std::shared_ptr<MTVideoProcess> process, unsigned long index);
	 void swapProcesses(size_t index1, size_t index2);
	 bool removeVideoProcess(std::shared_ptr<MTVideoProcess> process);
	 bool removeVideoProcessAtIndex(int index);
	 std::shared_ptr<MTVideoProcess> getVideoProcessAtIndex(unsigned long index);
	 std::shared_ptr<MTVideoProcess> getProcessWithName(std::string name);
	 int getVideoProcessCount();
	 void removeAllVideoProcesses();

//////////////////////////////////
//Class Overrides
//////////////////////////////////
	 virtual void deserialize(ofXml& serializer);
//	virtual void serialize(ofXml& serializer);
	 virtual void threadedFunction();

//////////////////////////////////
//Controller – ACTION
//////////////////////////////////
	 void setup();
	 void startStream();

/// Stops the processing thread. Blocks until thread exits.
	 void stopStream();
	 void setStreamRunning(bool isRunning);

/// Stops processing thread and closes video devices.
/// This method blocks until the processing thread stops.
	 void closeStream();

	 void updateTransformInternals();

protected:
//////////////////////////////////
//Event Listeners
//////////////////////////////////
//	void documentSizeChanged(int& changedValue);
	 void processSizeChanged(int& value);
	 void processTransformChanged(int& value);
//	void videoDeviceIDChanged(int& unused);
	 void videoPlayerStatusChanged(bool& unused);
	 void videoFilePathChanged(std::string& newPath);
	 bool isSetup;

	 cv::Mat workingImage;
	 cv::Mat videoInputImage;
	 cv::Mat processOutput;

//////////////////////////////////
//Internals
//////////////////////////////////
	 cv::Mat roiToProcessTransform;
	 cv::Mat processToOutputTransform;
	 cv::Mat outputToProcessTransform;
//
//////////////////////////////////
//Video Internals
//////////////////////////////////

	 bool initializeVideoCapture();
	 std::shared_ptr<MTVideoInputSource> inputSource;
private:
	 std::queue<std::function<void()>> functionQueue;
protected:
	 void enqueueFunction(std::function<void()> funct)
	 {
	 		lock();
			functionQueue.push(funct);
			unlock();
	 }

	 void syncParameters();
};

struct MTProcessData
{
/**
 * @brief The Mat that is being modified and passed to subsequent processes as the data
 * runs through the stream. This should be your basic input/output source and destination.
 */
	 cv::Mat processStream;
/**
 * @brief The result of a process. In most cases processStream should be the same as processResult,
 * but this Mat is useful to pass along a result that you might not want to end up in the stream if you want
 * to continue processing the video data. For example, the output of optical flow is not necessarily
 * useful for subsequent video processes, so you might want to publish that output in processResult, and pass
 * along any video in processStream.
 */
	 cv::Mat processResult;
/**
 * @brief The source pixels of the stream, i.e. the video pixels. You shouldn't modify this Mat.
 */
	 cv::Mat processSource;
/**
 * @brief A convenience Mat to store a mask. Completely optional.
 */
	 cv::Mat processMask;
/**
 * @brief An unordered_map for any custom Mat's that you may want to use in your own processes.
 */
	 std::unordered_map<std::string, cv::Mat> user;

	 void clear()
	 {
			using namespace cv;
			processStream = Mat();
			processResult = Mat();
			processSource = Mat();
			processMask = Mat();
	 }
};

#endif /* NSVideoProcessChain_hpp */
