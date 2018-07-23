//
//  MTVideoInputStream.cpp
//  nervousstructureof
//
//  Created by Cristobal Mendoza on 3/14/16.
//
//

#include <graphics/ofPath.h>
#include <ofMain.h>
#include "ofxMTVideoInput.h"
#include "MTApp.hpp"


MTVideoInputStream::MTVideoInputStream(std::string name) : MTModel(name)
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

	// Add a default value for the inputROI:
	inputROI->rectangle(0, 0, videoWidth, videoHeight);
	isSetup = false;

//	NSModel* m = MTApp::sharedApp->
}

MTVideoInputStream::~MTVideoInputStream()
{
	closeStream();
	//TODO:
//	NSApp::sharedApp->getNSModel()->outputWidth.removeListener(this, &MTVideoInputStream::documentSizeChanged);
//	NSApp::sharedApp->getNSModel()->outputHeight.removeListener(this, &MTVideoInputStream::documentSizeChanged);
	processWidth.removeListener(this, &MTVideoInputStream::processSizeChanged);
	processHeight.removeListener(this, &MTVideoInputStream::processSizeChanged);
	videoInputDeviceID.removeListener(this, &MTVideoInputStream::videoDeviceIDChanged);
	useVideoPlayer.removeListener(this, &MTVideoInputStream::videoPlayerStatusChanged);
	videoFilePath.removeListener(this, &MTVideoInputStream::videoFilePathChanged);

}


#pragma mark THREAD
//////////////////////////////////
//Thread
//////////////////////////////////

void MTVideoInputStream::threadedFunction()
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
			processData.processSource = videoInputImage;
			processData.processStream = workingImage;
			for (auto p : videoProcesses)
			{
				processData = p->process(processData);
				p->notifyEvents();

			}

			auto eventArgs = MTVideoInputStreamCompleteEventArgs();
			eventArgs.stream = this->shared_from_this();
			eventArgs.input = videoInputImage;
			eventArgs.result = processData.processResult;
			eventArgs.fps = fpsCounter.getFps();
			streamCompleteFastEvent.notify(this, eventArgs);
			streamCompleteEvent.notify(this, eventArgs);
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

void MTVideoInputStream::setup()
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
		p->processStream = shared_from_this();
		p->setup();
	}

	isSetup = true;
}

bool MTVideoInputStream::initializeVideoCapture()
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

void MTVideoInputStream::startStream()
{
	if (!isSetup) setup();
	isRunning = true;
	startThread();
}

void MTVideoInputStream::stopStream()
{
	isRunning = false;
	waitForThread(true, 100);
}

void MTVideoInputStream::closeStream()
{
	stopStream();
	videoGrabber.close();
	videoPlayer.close();
}

void MTVideoInputStream::setStreamRunning(bool _isRunning)
{
	isRunning = _isRunning;

	if (isRunning)
	{
		startStream();
	}
	else
	{
		stopStream();
	}

}

cv::Mat MTVideoInputStream::getProcessToWorldTransform()
{
	return processToWorldTransform;
}

cv::Mat MTVideoInputStream::getWorldToProcessTransform()
{
	return worldToProcessTransform;
}

ofPath MTVideoInputStream::getInputROI()
{
	return *inputROI.get();
}
ofPath MTVideoInputStream::getOutputRegion()
{
	return *outputRegion.get();
}

void MTVideoInputStream::setInputROI(ofPath path)
{
	lock();
	inputROI->clear();
	inputROI->append(path);
	updateTransformInternals();
	unlock();
}
void MTVideoInputStream::setOutputRegion(ofPath path)
{
	lock();
	outputRegion->clear();
	outputRegion->append(path);
	updateTransformInternals();
	unlock();
}

#pragma mark UTILITY
//////////////////////////////////
//Utility
//////////////////////////////////

void MTVideoInputStream::setProcessingResolution(int w, int h)
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

void MTVideoInputStream::setCaptureResolution(int w, int h)
{
	videoWidth.setWithoutEventNotifications(w);
	videoHeight.setWithoutEventNotifications(h);
	initializeVideoCapture();
}


#pragma mark EVENTS
//////////////////////////////////
//Event Listeners
//////////////////////////////////


void MTVideoInputStream::processSizeChanged(int& changedValue)
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

void MTVideoInputStream::videoDeviceIDChanged(int& unused)
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

void MTVideoInputStream::videoPlayerStatusChanged(bool& unused)
{
	//for now:
	isRunning = false;
	isSetup = false;
}

void MTVideoInputStream::videoFilePathChanged(std::string& newPath)
{
	//for now:
	isRunning = false;
	isSetup = false;
}

#pragma mark DATA CONTROL
//////////////////////////////////
//Data Handling
//////////////////////////////////

void MTVideoInputStream::addVideoProcess(std::shared_ptr<MTVideoProcess> process)
{
	addVideoProcessAtIndex(process, videoProcesses.size());
}

void MTVideoInputStream::addVideoProcessAtIndex(std::shared_ptr<MTVideoProcess> process, unsigned long index)
{
	lock();
	videoProcesses.insert(videoProcesses.begin() + index, process);
//	if (ofStringTimesInString(process->getName(), "_") < 1)
//	{
//		process->setName(process->getName() + "_" + ofToString(videoProcesses.size()));
//	}
	processesParameters.addAt(process->getParameters(), index);
	process->processStream = shared_from_this();
	processAddedEvent.notify(this, process);
	unlock();
}

void MTVideoInputStream::swapProcesses(size_t index1, size_t index2)
{
	// Some basic error checking:
	if (index1 >= videoProcesses.size() || index2 >= videoProcesses.size())
	{
		ofLogError(__FUNCTION__) << "Index out of range";
		return;
	}

	lock();
	videoProcesses.at(index1)->setup();
	videoProcesses.at(index2)->setup();
	std::swap(videoProcesses.at(index1), videoProcesses.at(index2));
	processesParameters.swapPositions(index1, index2);
	processOrderChangedEvent.notify(this);
	unlock();
}

std::shared_ptr<MTVideoProcess> MTVideoInputStream::getVideoProcessAtIndex(unsigned long index)
{
	return videoProcesses[index];
}


std::shared_ptr<MTVideoProcess> MTVideoInputStream::getProcessWithName(std::string name)
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

int MTVideoInputStream::getVideoProcessCount()
{
	return videoProcesses.size();
}


bool MTVideoInputStream::removeVideoProcess(std::shared_ptr<MTVideoProcess> process)
{
	lock();
	auto iter = std::find(videoProcesses.begin(), videoProcesses.end(), process);
	if (iter != videoProcesses.end())
	{
		videoProcesses.erase(iter);
		processRemovedEvent.notify(this, process);
		unlock();
		return true;
	}
	unlock();
	return false;
}

bool MTVideoInputStream::removeVideoProcessAtIndex(int index)
{
	return removeVideoProcess(videoProcesses.at(index));
}

void MTVideoInputStream::removeAllVideoProcesses()
{
	videoProcesses.clear();
}


#pragma mark OVERRIDES
//////////////////////////////////
//Class Overrides
//////////////////////////////////

void MTVideoInputStream::deserialize(ofXml& serializer)
{
	if (isThreadRunning())
	{
		stopThread();
		waitForThread(false, INFINITE_JOIN_TIMEOUT);
	}

	auto thisChainXml = serializer.findFirst("//" + getName());
	if (!thisChainXml)
	{
		ofLogError(__FUNCTION__) << "Could not find XML data for " + getName();
		return;
	}

	MTModel::deserialize(thisChainXml);

	auto processParamsXml = serializer.findFirst("//" + getName() + "/" + "Video_Processes");

	if (!processParamsXml)
	{
		ofLogError() << "MTVideoInputStream: Error loading video processes";
		return;
	}

	auto pChildren = processParamsXml.getChildren();

	for (auto& processXml : pChildren)
	{
		string name = processXml.getName();
		if (auto typenameXml = processXml.getChild("Process_Type_Name"))
		{
			std::shared_ptr<MTVideoProcess> process = MTVideoInput::getInstance().createVideoProcess(
					typenameXml.getValue());
			if (process != nullptr)
			{
				addVideoProcess(process);
				process->deserialize(processXml);
			}
			else
			{
				ofLogError("MTVideoInputStream") << "Could not find class " << typenameXml.getValue();
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
//    NSApp::sharedApp->getNSModel()->outputWidth.addListener(this, &MTVideoInputStream::documentSizeChanged);
//    NSApp::sharedApp->getNSModel()->outputHeight.addListener(this, &MTVideoInputStream::documentSizeChanged);
	processWidth.addListener(this, &MTVideoInputStream::processSizeChanged);
	processHeight.addListener(this, &MTVideoInputStream::processSizeChanged);

	videoInputDeviceID.addListener(this, &MTVideoInputStream::videoDeviceIDChanged);
	useVideoPlayer.addListener(this, &MTVideoInputStream::videoPlayerStatusChanged);
	videoFilePath.addListener(this, &MTVideoInputStream::videoFilePathChanged);

}

#pragma mark INTERNALS
//////////////////////////////////
//Internals
//////////////////////////////////

void MTVideoInputStream::updateTransformInternals()
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
			ofLogError("MTVideoInputStream", "error getting inputROI path!");
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
		ofLogError("MTVideoInputStream", "error getting output region path!");
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
