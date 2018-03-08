//
//  MTVideoProcessStream.cpp
//  nervousstructureof
//
//  Created by Cristobal Mendoza on 3/14/16.
//
//

#include <graphics/ofPath.h>
#include <ofMain.h>
#include "ofxMTVideoInput.h"
#include "MTApp.hpp"


MTVideoProcessStream::MTVideoProcessStream(std::string name) : MTModel(name)
{
	parameters.add(isRunning.set("Running", false),
				   mirrorVideo.set("Mirror Video", true),
				   videoInputDeviceID.set("Capture Device", 0, 0, 20),
				   videoWidth.set("Video Width", 320, 1, 1920),
				   videoHeight.set("Video Height", 240, 1, 1080),
				   useVideoPlayer.set("Use Player", false),
				   videoFilePath.set("File", ""),
				   processWidth.set("Process Width", 320, 120, 1920),
				   processHeight.set("Process Height", 240, 80, 1080),
				   useROI.set("Use ROI", false),
				   outputRegionString.set("Output Region", ""),
				   inputROIString.set("Input ROI", ""));
	processesParameters.setName("Video Processes");
	parameters.add(processesParameters);

	outputRegion = std::make_shared<ofPath>();
	inputROI = std::make_shared<ofPath>();
	isSetup = false;

//	NSModel* m = MTApp::sharedApp->
}

MTVideoProcessStream::~MTVideoProcessStream()
{
	closeChain();
	//TODO:
//	NSApp::sharedApp->getNSModel()->outputWidth.removeListener(this, &MTVideoProcessStream::documentSizeChanged);
//	NSApp::sharedApp->getNSModel()->outputHeight.removeListener(this, &MTVideoProcessStream::documentSizeChanged);
	processWidth.removeListener(this, &MTVideoProcessStream::processSizeChanged);
	processHeight.removeListener(this, &MTVideoProcessStream::processSizeChanged);
	videoInputDeviceID.removeListener(this, &MTVideoProcessStream::videoDeviceIDChanged);
	useVideoPlayer.removeListener(this, &MTVideoProcessStream::videoPlayerStatusChanged);
	videoFilePath.removeListener(this, &MTVideoProcessStream::videoFilePathChanged);

}


#pragma mark THREAD
//////////////////////////////////
//Thread
//////////////////////////////////

void MTVideoProcessStream::threadedFunction()
{
	setThreadName(this->getName());

	MTProcessData processData;
	while (isThreadRunning())
	{
		videoGrabber.update();
		if (videoGrabber.isFrameNew())
		{
			fpsCounter.newFrame();
			videoInputImage = ofxCv::toCv(static_cast<const ofPixels&>(videoGrabber.getPixels()));

			if (videoInputImage.cols != processWidth)
			{
				cv::Size size(processWidth, processHeight);
				cv::resize(videoInputImage, workingImage, size);
			}
			else
			{
				workingImage = videoInputImage;
			}

			if (mirrorVideo.get())
			{
				cv::flip(workingImage, workingImage, 1);
			}

			processData.clear();
			processData[MTVideoProcessSourceKey] = videoInputImage;
			processData[MTVideoProcessStreamKey] = workingImage;
			for (auto p : videoProcesses)
			{
				processData = p->process(processData);
				p->notifyEvents();

			}

			auto streamFastEventArgs =
					MTVideoProcessFastEventArgs<MTVideoProcessStream>
							(workingImage, this);
			ofNotifyEvent(processStreamCompleteFastEvent, streamFastEventArgs, this);

			// Only make the ProcessEventArgs is someone is listening:
			if (processStreamCompleteEvent.size() > 0)
			{
				auto streamEventArgs =
						MTVideoProcessEventArgs<MTVideoProcessStream>
								(workingImage, this);
				ofNotifyEvent(processStreamCompleteEvent, streamEventArgs, this);

			}
		}
		else
		{
			yield();
//            ofLogVerbose() << "No frame";
		}
	}

	if (isThreadRunning()) stopThread();
}

#pragma mark CONTROL
//////////////////////////////////
//Control
//////////////////////////////////

void MTVideoProcessStream::setup()
{
	workingImage.create(processHeight, processWidth, CV_8UC1);
	processOutput.create(processHeight, processWidth, CV_8UC1);

	if (outputRegion->getCommands().size() < 4)
	{
//        auto model = NSApp::sharedApp->getNSModel();
		outputRegion->setMode(ofPath::COMMANDS);
		outputRegion->clear();
		outputRegion->moveTo(0, 0);
		outputRegion->lineTo(processWidth, 0);
		outputRegion->lineTo(processWidth, processHeight);
		outputRegion->lineTo(0, processHeight);
	}
	useROI = false;
	updateTransformInternals();

	isSetup = true;

	//Initialize video
	if (useVideoPlayer == false)
	{
		isSetup = initializeVideoCapture();

	}
	else
	{
		if (!(isSetup = videoPlayer.load(videoFilePath)))
		{
			ofSystemAlertDialog("Error opening file: " + videoFilePath.get());
		}

		//set the size of processes etc.
	}

	//Initialize processes
	for (const auto& p : videoProcesses)
	{
		p->setProcessSize(processWidth, processHeight);
		p->processChain = shared_from_this();
		p->setup();
	}

	isSetup = true;
}

bool MTVideoProcessStream::initializeVideoCapture()
{
	auto devices = videoGrabber.listDevices();

	for (auto& device : devices)
	{
		if (device.bAvailable)
		{
			ofLogNotice() << "sysId: " << device.serialID << "id: " << device.id << ": "
						  << device.deviceName;
		}
		else
		{
			ofLogNotice() << device.id << ": " << device.deviceName << " - unavailable ";
		}
	}

	lock();
	videoGrabber.close();
	videoGrabber.setDeviceID(videoInputDeviceID);
	videoGrabber.setDesiredFrameRate(30);
	updateTransformInternals();
	bool result = videoGrabber.setup(videoWidth, videoHeight, false);
	unlock();
	return result;
}

void MTVideoProcessStream::startChain()
{
	isRunning = true;
	startThread();
}

void MTVideoProcessStream::stopChain()
{
	isRunning = false;
	waitForThread(true, 100);
}

void MTVideoProcessStream::closeChain()
{
	stopChain();
	videoGrabber.close();
	videoPlayer.close();
}

void MTVideoProcessStream::setChainRunning(bool _isRunning)
{
	isRunning = _isRunning;
	if (isSetup)
	{
		if (isRunning)
		{
			startThread();
		}
		else
		{
			stopThread();
		}
	}
	else
	{
		ofLog(OF_LOG_WARNING, "Tried to run a process chain that was not set up");
	}
}

cv::Mat MTVideoProcessStream::getProcessToWorldTransform()
{
	return processToWorldTransform;
}

cv::Mat MTVideoProcessStream::getWorldToProcessTransform()
{
	return worldToProcessTransform;
}

#pragma mark UTILITY
//////////////////////////////////
//Utility
//////////////////////////////////

void MTVideoProcessStream::setProcessingResolution(int w, int h)
{
	int bogus = 0;
	lock();
	// Transform the inputROI:
	auto transform = glm::scale(glm::vec3((float) w / processWidth, (float) h / processHeight, 1));
	for (auto command : inputROI->getCommands())
	{
		command.to = transform * glm::vec4(command.to, 1);
	}
	processWidth.setWithoutEventNotifications(w);
	processHeight.setWithoutEventNotifications(h);
	updateTransformInternals();
	unlock();
	processSizeChanged(bogus);
}

void MTVideoProcessStream::setCaptureResolution(int w, int h)
{
	videoWidth.setWithoutEventNotifications(w);
	videoHeight.setWithoutEventNotifications(h);
	initializeVideoCapture();
}


#pragma mark EVENTS
//////////////////////////////////
//Event Listeners
//////////////////////////////////


void MTVideoProcessStream::processSizeChanged(int& changedValue)
{
	lock();
	workingImage.create(processHeight, processWidth, CV_8UC1);
	processOutput.create(processHeight, processWidth, CV_8UC1);
	for (const auto& p : videoProcesses)
	{
		p->setProcessSize(processWidth, processHeight);
	}
	updateTransformInternals();
	unlock();
}

void MTVideoProcessStream::videoDeviceIDChanged(int& unused)
{
	//for now:

	waitForThread(true);
//	isRunning = false;
	isSetup = initializeVideoCapture();
	if (isSetup)
	{
		startThread();
	}
	else
	{
		// error
		ofLogError(getName()) << "Could not reinitialize video";
	}

}

void MTVideoProcessStream::videoPlayerStatusChanged(bool& unused)
{
	//for now:
	isRunning = false;
	isSetup = false;
}

void MTVideoProcessStream::videoFilePathChanged(std::string& newPath)
{
	//for now:
	isRunning = false;
	isSetup = false;
}

#pragma mark DATA CONTROL
//////////////////////////////////
//Data Handling
//////////////////////////////////

void MTVideoProcessStream::addVideoProcess(std::shared_ptr<MTVideoProcess> process)
{
	lock();
	videoProcesses.push_back(process);
	if (ofStringTimesInString(process->getName(), "_") < 1)
	{
		process->setName(process->getName() + "_" + ofToString(videoProcesses.size()));
	}
	processesParameters.add(process->getParameters());
	process->processChain = shared_from_this();
	unlock();
}

std::shared_ptr<MTVideoProcess> MTVideoProcessStream::getVideoProcessAtIndex(int index)
{
	return videoProcesses[index];
}

std::shared_ptr<MTVideoProcess> MTVideoProcessStream::getProcessWithName(std::string name)
{
	for (auto vp : videoProcesses)
	{
		if (vp->getName() == name)
		{
			return vp;
		}
	}

	return nullptr;
}

int MTVideoProcessStream::getVideoProcessCount()
{
	return videoProcesses.size();
}

#pragma mark OVERRIDES
//////////////////////////////////
//Class Overrides
//////////////////////////////////

void MTVideoProcessStream::loadFromSerializer(ofXml& serializer)
{
	if (isThreadRunning())
	{
		stopThread();
		waitForThread(false, INFINITE_JOIN_TIMEOUT);
	}


//	ofDeserialize(chainsXml, getParameters());
	auto thisChainXml = serializer.findFirst("//Nervous_Structure/VideoProcessChains/" + getName());
	if (!thisChainXml)
	{
		ofLogError(__FUNCTION__) << "Could not find XML data for " + getName();
		return;
	}

	// So ofDeserialize craps out on deeply nested parameter groups, so manually we must deserialize:
	// My own deserialization function:

	deserialize(thisChainXml);

	auto processParamsXml = serializer.findFirst(
			"//Nervous_Structure/VideoProcessChains/" + parameters.getEscapedName() + "/" + "Video_Processes");

	if (!processParamsXml)
	{
		ofLogError() << "MTVideoProcessStream: Error loading video processes";
		return;
	}

	auto pChildren = processParamsXml.getChildren();

	for (auto& processXml : pChildren)
	{

		string name = processXml.getName();
		if (auto typenameXml = processXml.getChild("Process_Type_Name"))
		{
			std::shared_ptr<MTVideoProcess> process = ofxMTVideoInput::mtCreateVideoProcess(typenameXml.getValue());
			if (process != nullptr)
			{
				addVideoProcess(process);
				process->deserialize(processXml);
			}
			else
			{
				ofLogError("MTVideoProcessStream") << "Could not find class " << typenameXml.getValue();
			}
		}
	}

	if (outputRegionString.get().substr(0, 1) == "{")
	{
		ofPath orPath = MTApp::pathFromString(outputRegionString);
		outputRegion = std::make_shared<ofPath>(orPath);
	}
	else
	{
		outputRegion = std::make_shared<ofPath>();
	}

	if (inputROIString.get().substr(0, 1) == "{")
	{
		auto irPath = MTApp::pathFromString(inputROIString);
		inputROI = std::make_shared<ofPath>(irPath);
	}
	else
	{
		inputROI = std::make_shared<ofPath>();
		inputROI->rectangle(0, 0, processWidth, processHeight);
	}

	//Add the listeners
//    NSApp::sharedApp->getNSModel()->outputWidth.addListener(this, &MTVideoProcessStream::documentSizeChanged);
//    NSApp::sharedApp->getNSModel()->outputHeight.addListener(this, &MTVideoProcessStream::documentSizeChanged);
	processWidth.addListener(this, &MTVideoProcessStream::processSizeChanged);
	processHeight.addListener(this, &MTVideoProcessStream::processSizeChanged);

	videoInputDeviceID.addListener(this, &MTVideoProcessStream::videoDeviceIDChanged);
	useVideoPlayer.addListener(this, &MTVideoProcessStream::videoPlayerStatusChanged);
	videoFilePath.addListener(this, &MTVideoProcessStream::videoFilePathChanged);

}

#pragma mark INTERNALS
//////////////////////////////////
//Internals
//////////////////////////////////

void MTVideoProcessStream::updateTransformInternals()
{
	cv::Point2f world[4];
	cv::Point2f process[4];

	if (this->useROI)
	{
		// Basic error checking:
		for (auto command : inputROI->getCommands())
		{
			command.to.x = ofClamp(command.to.x, 0, processWidth);
			command.to.y = ofClamp(command.to.y, 0, processHeight);
		}

		auto roiPoly = inputROI->getOutline()[0];

		if (roiPoly.size() != 4)
		{
			ofLogError("MTVideoProcessStream", "error getting inputROI path!");
			return;
		}
		for (int k = 0; k < 4; k++)
		{
			process[k].x = roiPoly[k].x;
			process[k].y = roiPoly[k].y;
		}
	}
	else
	{
		process[0].x = 0;
		process[0].y = 0;
		process[1].x = processWidth;
		process[1].y = 0;
		process[2].x = processWidth;
		process[2].y = processHeight;
		process[3].x = 0;
		process[3].y = processHeight;
	}


	auto worldPoly = outputRegion->getOutline()[0];
	if (worldPoly.size() != 4)
	{
		ofLogError("MTVideoProcessStream", "error getting output region path!");
		return;
	}

	for (int k = 0; k < 4; k++)
	{
		world[k].x = worldPoly[k].x;
		world[k].y = worldPoly[k].y;
	}

	processToWorldTransform = cv::getPerspectiveTransform(process, world);
	worldToProcessTransform = cv::getPerspectiveTransform(world, process);
	outputRegionString = MTApp::pathToString(*outputRegion.get());
	inputROIString = MTApp::pathToString(*inputROI.get());
}

