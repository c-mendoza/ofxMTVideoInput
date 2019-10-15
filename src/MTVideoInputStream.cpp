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
				   processingWidth.set("Process Width", 320, 120, 1920),
				   processingHeight.set("Process Height", 240, 80, 1080),
				   useROI.set("Use ROI", false),
				   outputRegionString.set("Output Region", ""),
				   inputROIString.set("Input ROI", ""));
	processesParameters.setName("Video Processes");
	parameters.add(processesParameters);

	outputRegion = std::make_shared<ofPath>();
	// Add a default value for the output region:
	outputRegion->rectangle(0, 0, 1280, 720);

	inputROI = std::make_shared<ofPath>();
	// Add a default value for the inputROI:
	inputROI->rectangle(0, 0, videoWidth, videoHeight);
	isSetup = false;

	//Add the listeners
	processingWidth.addListener(this, &MTVideoInputStream::processSizeChanged);
	processingHeight.addListener(this, &MTVideoInputStream::processSizeChanged);

	videoInputDeviceID.addListener(this, &MTVideoInputStream::videoDeviceIDChanged);
	useVideoPlayer.addListener(this, &MTVideoInputStream::videoPlayerStatusChanged);
	videoFilePath.addListener(this, &MTVideoInputStream::videoFilePathChanged);

	updateTransformInternals();

	addEventListener(useROI.newListener([this](bool& val)
										{
											updateTransformInternals();
										}));
}

MTVideoInputStream::~MTVideoInputStream()
{
	closeStream();
	//TODO:
//	App::sharedApp->getOMModel()->outputWidth.removeListener(this, &MTVideoInputStream::documentSizeChanged);
//	App::sharedApp->getOMModel()->outputHeight.removeListener(this, &MTVideoInputStream::documentSizeChanged);
	processingWidth.removeListener(this, &MTVideoInputStream::processSizeChanged);
	processingHeight.removeListener(this, &MTVideoInputStream::processSizeChanged);
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
		if (!isSetup)
		{
			yield();
			continue;
		}


		while (!functionQueue.empty())
		{
			auto f = functionQueue.front();
			f();
			functionQueue.pop();
		}

		videoGrabber.update();
		if (videoGrabber.isFrameNew())
		{
			cv::Size processSize(processingWidth, processingHeight);
			fpsCounter.newFrame();
			videoInputImage = ofxCv::toCv(static_cast<const ofPixels&>(videoGrabber.getPixels()));

			if (mirrorVideo.get())
			{
				cv::flip(videoInputImage, videoInputImage, 1);
			}

			if (videoInputImage.cols != processingWidth)
			{
				cv::resize(videoInputImage, workingImage, processSize);
			}
			else
			{
				workingImage = videoInputImage;
			}

			if (useROI)
			{
				cv::Mat result;
				cv::warpPerspective(workingImage,
									result,
									roiToProcessTransform,
									processSize);
				workingImage = result;
			}

			if (isRunning)
			{
				processData.clear();
				processData.processSource = videoInputImage;
				processData.processStream = workingImage;

				for (auto p : videoProcesses)
				{
					if (p->isActive)
					{
						p->process(processData);
						p->notifyEvents();
					}

				}

				auto eventArgs = MTVideoInputStreamCompleteEventArgs();
				eventArgs.stream = this->shared_from_this();
				eventArgs.input = videoInputImage;
				eventArgs.result = processData.processResult;
				eventArgs.fps = fpsCounter.getFps();
				streamCompleteFastEvent.notify(this, eventArgs);
				streamCompleteEvent.notify(this, eventArgs);
			}
		}
		else
		{
			yield();
//            ofLogVerbose() << "No frame";
		}
	}

	ofLogVerbose("MTVideoInput") <<  "Thread complete";
}

#pragma mark CONTROL
//////////////////////////////////
//Control
//////////////////////////////////

void MTVideoInputStream::setup()
{
	workingImage.create(processingHeight, processingWidth, CV_8UC1);
	processOutput.create(processingHeight, processingWidth, CV_8UC1);

	if (outputRegion->getCommands().size() < 5)
	{
//        auto model = App::sharedApp->getOMModel();
		outputRegion->setMode(ofPath::COMMANDS);
		outputRegion->clear();
		outputRegion->moveTo(0, 0);
		outputRegion->lineTo(processingWidth, 0);
		outputRegion->lineTo(processingWidth, processingHeight);
		outputRegion->lineTo(0, processingHeight);
	}
//	useROI = false;
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
		p->setProcessSize(processingWidth, processingHeight);
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


	videoGrabber.close();
	videoGrabber.setDeviceID(videoInputDeviceID);
	videoGrabber.setDesiredFrameRate(30);
	updateTransformInternals();
	bool result = videoGrabber.setup(videoWidth, videoHeight, false);
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

//cv::Mat MTVideoInputStream::getProcessToWorldTransform()
//{
//	return processToWorldTransform;
//}
//
//cv::Mat MTVideoInputStream::getWorldToProcessTransform()
//{
//	ofScopedLock lock(this->mutex);
//	return worldToProcessTransform;
//}

cv::Mat MTVideoInputStream::getProcessToOutputTransform()
{
	ofScopedLock lock(this->mutex);
	return processToOutputTransform.clone();
}

cv::Mat MTVideoInputStream::getOutputToProcessTransform()
{
	ofScopedLock lock(this->mutex);
	return outputToProcessTransform.clone();
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
	enqueueFunction([this, path]
					{
						inputROI->clear();
						inputROI->append(path);
						updateTransformInternals();
					});
}

void MTVideoInputStream::setOutputRegion(ofPath path)
{
	lock();
//	enqueueFunction([this, path]
//					{
						outputRegion->clear();
						outputRegion->append(path);
						updateTransformInternals();
//					});
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
	auto transform = glm::scale(glm::vec3((float) w / processingWidth, (float) h / processingHeight, 1));
	for (auto command : inputROI->getCommands())
	{
		command.to = transform * glm::vec4(command.to, 1);
	}
	processingWidth.setWithoutEventNotifications(w);
	processingHeight.setWithoutEventNotifications(h);
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
	enqueueFunction([this, changedValue]
					{
						workingImage.create(processingHeight, processingWidth, CV_8UC1);
						processOutput.create(processingHeight, processingWidth, CV_8UC1);
						for (const auto& p : videoProcesses)
						{
							p->setProcessSize(processingWidth, processingHeight);
						}
						updateTransformInternals();
					});
}

void MTVideoInputStream::videoDeviceIDChanged(int& unused)
{
	if (isThreadRunning())
	{
//		lock();
	}
	isSetup = initializeVideoCapture();
	if (!isSetup)
	{
		ofLogError(getName()) << "Could not reinitialize video";
	}

	if (isThreadRunning())
	{
//		unlock();
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
	bool wasRunning = false;
	if (isThreadRunning())
	{
		wasRunning = true;
		stopThread();
		waitForThread(false, INFINITE_JOIN_TIMEOUT);
	}

	auto thisChainXml = serializer.findFirst("//" + getName());
	if (!thisChainXml)
	{
		ofLogError(__FUNCTION__) << "Could not find XML data for " + getName();
		return;
	}

	// Custom deserialization to deal with nested ParameterGroups
	MTModel::deserialize(thisChainXml);

	// ofDeserialize doesn't seem to work with nested ParameterGroups
//	ofDeserialize(thisChainXml, parameters);

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
			std::shared_ptr<MTVideoProcess> process = MTVideoInput::Instance().createVideoProcess(
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
		else
		{
			ofLogError("MTVideoInputStream") << "No Process Type Name found in XML, skipping process " << name;
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
		inputROI->rectangle(0, 0, processingWidth, processingHeight);
	}

	updateTransformInternals();

	if (wasRunning) {
		startStream();
	}
}

#pragma mark INTERNALS
//////////////////////////////////
//Internals
//////////////////////////////////

void MTVideoInputStream::updateTransformInternals()
{
	cv::Point2f world[4];
	cv::Point2f process[4];
	cv::Point2f processRoi[4];
	cv::Point2f output[4];

	if (this->useROI)
	{
		// Basic error checking:
		for (auto command : inputROI->getCommands())
		{
			command.to.x = ofClamp(command.to.x, 0, processingWidth);
			command.to.y = ofClamp(command.to.y, 0, processingHeight);
		}

		auto roiPoly = inputROI->getOutline()[0];

		if (roiPoly.size() != 4)
		{
			ofLogError("MTVideoInputStream", "error getting inputROI path!");
			return;
		}
		for (int k = 0; k < 4; k++)
		{
			processRoi[k].x = roiPoly[k].x;
			processRoi[k].y = roiPoly[k].y;
		}
	}

	process[0].x = 0;
	process[0].y = 0;
	process[1].x = processingWidth;
	process[1].y = 0;
	process[2].x = processingWidth;
	process[2].y = processingHeight;
	process[3].x = 0;
	process[3].y = processingHeight;

	auto outputPoly = outputRegion->getOutline()[0];
	if (outputPoly.size() != 4)
	{
		ofLogError("MTVideoInputStream", "error getting output region path!");
		return;
	}

	for (int k = 0; k < 4; k++)
	{
		output[k].x = outputPoly[k].x;
		output[k].y = outputPoly[k].y;
	}

	roiToProcessTransform = cv::getPerspectiveTransform(processRoi, process);
	processToOutputTransform = cv::getPerspectiveTransform(process, output);
	outputToProcessTransform = cv::getPerspectiveTransform(output, process);
//	processToWorldTransform = cv::getPerspectiveTransform(process, world);
//	worldToProcessTransform = cv::getPerspectiveTransform(world, process);
	outputRegionString = MTApp::pathToString(*outputRegion.get());
	inputROIString = MTApp::pathToString(*inputROI.get());
}
