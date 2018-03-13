//
//  MTVideoProcess.hpp
//  nervousstructureof
//
//  Created by Cristobal Mendoza on 3/14/16.
//
//

#ifndef MTVideoProcess_hpp
#define MTVideoProcess_hpp

#include <stdio.h>
//#include "ofxMTAppFramework.h"
#include "MTModel.hpp"
#include "ofxCv.h"
#include "registry.h"
//#include "MTVideoProcessUI.hpp"
//#include "ofxMTApp.hpp"

class MTVideoProcessStream;
class MTProcessData;
class MTVideoProcess;
class MTVideoProcessUI;


/**
 * @brief ProcessEventArgs clone (deep copy) the output of the process, making it "thread-safer".
 * If you need to use the output of the process on a different timeline than that of
 * the ProcessChain, use this event.
 * @tparam T The process type
 */
template<typename T>
class MTVideoProcessEventArgs : public ofEventArgs
{  //TODO: ProcessEventArgs vs. ProcessFastEventArgs!
public:
	cv::Mat processOutput;
	T* process;

	MTVideoProcessEventArgs(cv::Mat& processOutput, T* process)
	{
		this->processOutput = processOutput.clone();
		this->process = process;
	}
};

/**
 * @brief Fast Event Arguments do not copy the processOutput cv::Mat,
 * they return the actual Mat that the process worked on. They are meant
 * for immediate read-only access to the data, sparing you from an unnecessary copy.
 * If you need a copy of the data you might want to use MTVideoProcessEventArgs instead.
 * Modifying the cv::Mat while in the middle of a VideoProcessStream may result in unexpected/invalid
 * behavior.
 */
template<typename T>
class MTVideoProcessFastEventArgs : public ofEventArgs
{
public:
	cv::Mat processOutput;
	T* process;

	MTVideoProcessFastEventArgs(cv::Mat& processOutput, T* process)
	{
		this->processOutput = processOutput;
		this->process = process;
	}
};


class MTVideoProcess : public MTModel,
					   public std::enable_shared_from_this<MTVideoProcess>
{
public:

	ofParameter<bool> useTransform;
	ofParameter<std::string> processTypeName;
	std::weak_ptr<MTVideoProcessStream> processStream;
	ofFastEvent<MTVideoProcessFastEventArgs<MTVideoProcess>> processCompleteFastEvent;
	ofEvent<MTVideoProcessEventArgs<MTVideoProcess>> processCompleteEvent;

	/**
	 * @param name The friendly name for the process. It will be used as the identifier
	 * in the ofParameterGroup
	 * @param typeName The class name of the process in string format. It will be used by the
	 * Registry to associate the class with the right constructor.
	 */
	MTVideoProcess(std::string name, std::string typeName);
	~MTVideoProcess();

//	virtual void loadFromSerializer(ofXml& serializer) = 0;
//	virtual void saveWithSerializer(ofXml& serializer) = 0;
	virtual void setup(){}

	virtual MTProcessData& process(MTProcessData& input) = 0;
	virtual std::unique_ptr<MTVideoProcessUI> createUI();
	virtual void setProcessSize(int w, int h)
	{
		processWidth = w;
		processHeight = h;
		//Do I call another function here? Or do I override?

		//Should cv entities update in the "process" function?
		//Or should we change them elsewhere?
		markProcessSizeChanged = true;
	}

	virtual void setProcessTransform(ofRectangle area)
	{
		outputTarget = area;
	}

	virtual void notifyEvents()
	{
		auto processEventFastArgs = MTVideoProcessFastEventArgs<MTVideoProcess>(processOutput, this);
		ofNotifyEvent(processCompleteFastEvent, processEventFastArgs, this);

		// Only create the ProcessEventArgs if there is someone listening:
		if (processCompleteEvent.size() > 0)
		{
			auto processEventArgs = MTVideoProcessEventArgs<MTVideoProcess>(processOutput, this);
			ofNotifyEvent(processCompleteEvent, processEventArgs, this);
		}

	}

protected:
	cv::Mat processBuffer;
	cv::Mat processOutput;
	int processWidth;
	int processHeight;
	ofRectangle outputTarget;
	bool markProcessSizeChanged = false;
	std::unique_ptr<MTVideoProcessUI> videoProcessUI;

};


//Maybe at some point I should think about moving the chain
//towards some kind of inlet-outlet paradigm.
//More thought needed about this.
//template<typename InletType>
//struct Inlet
//{
//	InletType* inputValue;
//    std::string type = "";
//};


#endif /* NSVideoProcess_hpp */
