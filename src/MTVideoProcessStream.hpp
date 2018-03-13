//
//  MTVideoProcessStream.hpp
//  Created by Cristobal Mendoza on 3/14/16.
//
//

#ifndef MTVideoProcessChain_hpp
#define MTVideoProcessChain_hpp

#include <stdio.h>
#include "MTModel.hpp"
#include "ofxCv.h"
#include "MTVideoProcess.hpp"


class MTVideoProcessStream : public MTModel,
							 public ofThread,
							 public std::enable_shared_from_this<MTVideoProcessStream>
{
public:
	MTVideoProcessStream(std::string name);
	~MTVideoProcessStream();

	//ofReadonlyParameter does not serialize correctly!
	ofParameter<bool> isRunning;

	//////////////////////////////////
	//Processes
	//////////////////////////////////

	ofParameter<bool> mirrorVideo;
	//The pixel dimensions of the video process chain:
	ofParameter<int> processWidth;
	ofParameter<int> processHeight;
	ofParameter<std::string> outputRegionString; //For serialization. Do not modify manually.
	ofParameter<std::string> inputROIString; //For serialization Do not modify manually.
	ofParameter<bool> useROI;
	ofParameterGroup processesParameters;

	std::shared_ptr<ofPath> outputRegion;
	std::shared_ptr<ofPath> inputROI;

	std::vector<std::shared_ptr<MTVideoProcess>> videoProcesses;

	//////////////////////////////////
	//Event
	//////////////////////////////////
	///This ofFastEvent is sent from the video process chain thread. Be aware of that
	///when listening for the event!
	ofFastEvent<MTVideoProcessFastEventArgs<MTVideoProcessStream>> processStreamCompleteFastEvent;
	ofEvent<MTVideoProcessEventArgs<MTVideoProcessStream>> processStreamCompleteEvent;
	ofEvent<std::shared_ptr<MTVideoProcess>> processAddedEvent;
	ofEvent<std::shared_ptr<MTVideoProcess>> processRemovedEvent;
	ofEvent<void> processOrderChangedEvent;

	//////////////////////////////////
	//Video
	//////////////////////////////////
	ofParameter<int> videoWidth;
	ofParameter<int> videoHeight;
	ofParameter<bool> useVideoPlayer;
	ofParameter<std::string> videoFilePath;
	ofParameter<int> videoInputDeviceID;

	//////////////////////////////////
	//Getters
	//////////////////////////////////
	cv::Mat getProcessToWorldTransform();
	cv::Mat getWorldToProcessTransform();

	//////////////////////////////////
	//Utility
	//////////////////////////////////

protected:
	ofFpsCounter fpsCounter;
public:
	double getFps()
	{ return fpsCounter.getFps(); }
//	///Transforms a point in process coordinates to world coordinates,
//	///using the transformation values specified in the chain's processOut
//	///members.
//	void processToWorld(ofVec2f processPoint, ofVec2f& transformedPoint);
//
//	///Transform a point from workd to process coordinates. Pass a reference
//	///to a Vec2f to get the transformed point.
//	///TODO: Limit transformation position to process-space?
//	void worldToProcess(ofVec2f worldPoint, ofVec2f& transformedPoint);

	void setCaptureResolution(int w, int h);
	void setProcessingResolution(int w, int h);
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
	//Getters
	//////////////////////////////////

	const ofVideoGrabber& getVideoGrabber()
	{
		return videoGrabber;
	}

	const ofVideoPlayer& getVideoPlayer()
	{
		return videoPlayer;
	}

	//////////////////////////////////
	//Class Overrides
	//////////////////////////////////
	virtual void loadFromSerializer(ofXml& serializer);
//	virtual void saveWithSerializer(ofXml& serializer);
	virtual void threadedFunction();

	//////////////////////////////////
	//Controller – ACTION
	//////////////////////////////////
	void setup();
	void startChain();

	/// Stops the processing thread. Blocks until thread exits.
	void stopChain();
	void setChainRunning(bool isRunning);

	/// Stops processing thread and closes video devices.
	/// This method blocks until the processing thread stops.
	void closeChain();

	void updateTransformInternals();

protected:
	//////////////////////////////////
	//Event Listeners
	//////////////////////////////////
//	void documentSizeChanged(int& changedValue);
	void processSizeChanged(int& value);
	void processTransformChanged(int& value);
	void videoDeviceIDChanged(int& unused);
	void videoPlayerStatusChanged(bool& unused);
	void videoFilePathChanged(std::string& newPath);
	bool isSetup;

	cv::Mat workingImage;
	cv::Mat videoInputImage;
	cv::Mat processOutput;

	//////////////////////////////////
	//Internals
	//////////////////////////////////
	cv::Mat worldToProcessTransform;
	cv::Mat processToWorldTransform;

	//The overall dimensions of the
	//NS Program Output. Essentially cached values:
	//Think about doing this a bit differently, perhaps listening for document size changes
//	int outputWidth;
//	int outputHeight;
//	int outputAspectRatio;
//	int outputInvWidth;
//	int outputInvHeight;
//
	//////////////////////////////////
	//Video Internals
	//////////////////////////////////
	ofVideoGrabber videoGrabber;
	ofVideoPlayer videoPlayer;

	bool initializeVideoCapture();
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
		processStream =  Mat();
		processResult = Mat();
		processSource = Mat();
		processMask = Mat();
	}
};

#endif /* NSVideoProcessChain_hpp */
