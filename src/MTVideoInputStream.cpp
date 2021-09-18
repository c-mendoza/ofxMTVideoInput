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
#include "MTVideoInputSource.hpp"
#include "MTVideoInputStream.hpp"
#include "MTAppFrameworkUtils.hpp"


MTVideoInputStream::MTVideoInputStream(std::string name) : MTModel(name)
{

	parameters.add(isRunning.set("Running", false),
						mirrorVideo.set("Mirror Video", true),
						flipVideo.set("Flip Video", false),
//									processingWidth.set("Process Width", 320, 120, 1920),
//									processingHeight.set("Process Height", 240, 80, 1080),
						processingSize.set("Processing Size", 1.0, 0.1, 1.0),
						useROI.set("Use ROI", false),
						outputRegion.set("Output Region", ofPath()),
						inputROI.set("Input ROI", ofPath()));
	processesParameters.setName("Video Processes");
	inputSourcesParameters.setName("Input Sources");
	parameters.add(processesParameters, inputSourcesParameters);

// Add some defaults for the regions:
	ofPath outDef;
	outDef.rectangle(0, 0, 1280, 720);
	outputRegion = outDef;
	ofPath inDef;
	inDef.rectangle(0, 0, 320, 240);
	inputROI = inDef;

	isSetup = false;


	updateTransformInternals();

	addEventListener(useROI.newListener([this](bool& val)
													{
														enqueueFunction([this]()
																			 {
																				 updateTransformInternals();
																			 });
													}));

	addEventListener(processingSize.newListener([this](float val)
															  {
																  if (!isDeserializing)
																  {
																	  enqueueFunction([this, val]()
																							{
																								setProcessingSize(val);
																							});
																  }
															  }));

//	addEventListener(outputRegion.newListener([this](ofPath& val) {
//		updateTransformInternals();
//	}));
//	addEventListener(inputROI.newListener([this](ofPath& val) {
//		updateTransformInternals();
//	}));
}


void MTVideoInputStream::setProcessingSize(float val)
{
//	 lock();
	processingSize.setWithoutEventNotifications(val);
	float change = val / prevProcessingSize;
	prevProcessingSize = val;
	if (!isDeserializing)
	{
// Transform the inputROI:
		auto newPath = ofPath(inputROI.get());
		newPath.scale(change, change);
		inputROI.set(newPath);
	}
	processingWidth = floor((float) inputWidth * processingSize);
	processingHeight = floor((float) inputHeight * processingSize);
	workingImage.create(processingHeight, processingWidth, CV_8UC1);
	processOutput.create(processingHeight, processingWidth, CV_8UC1);
	for (const auto& p : videoProcesses)
	{
		p->setProcessSize(processingWidth, processingHeight);
	}

	updateTransformInternals();
//	 unlock();
}

MTVideoInputStream::~MTVideoInputStream()
{
	closeStream();
}

void MTVideoInputStream::threadedFunction()
{
	setThreadName(this->getName());

	MTProcessData processData;

	while (isThreadRunning())
	{
		if (!isSetup || inputSource == nullptr)
		{
			yield();
			continue;
		}

		lock();

		std::function<void()> function;
		while (functionChannel.tryReceive(function))
		{
			function();
		}

		inputSource->update();
		if (inputSource->isFrameNew())
		{
			auto pixels = inputSource->getPixels();
			if (pixels.getWidth() != inputWidth || pixels.getHeight() != inputHeight)
			{
				inputWidth = pixels.getWidth();
				inputHeight = pixels.getHeight();
				setProcessingSize(processingSize);
			}

			cv::Size processSize(processingWidth, processingHeight);
			fpsCounter.newFrame();
			videoInputImage = ofxCv::toCv(static_cast<const ofPixels&>(inputSource->getPixels()));

			if (mirrorVideo.get() && flipVideo.get())
			{
				cv::flip(videoInputImage, videoInputImage, -1);
			}
			else
			{
				if (mirrorVideo.get())
				{
					cv::flip(videoInputImage, videoInputImage, 0);
				}

				if (flipVideo.get())
				{
					cv::flip(videoInputImage, videoInputImage, 1);
				}
			}


			if (processingSize != 1.0f)
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
		unlock();
		yield();
	}

	ofLogVerbose("MTVideoInput") << "Thread complete";
}

void MTVideoInputStream::setup()
{
	workingImage.create(processingHeight, processingWidth, CV_8UC1);
	processOutput.create(processingHeight, processingWidth, CV_8UC1);

	updateTransformInternals();

//Initialize processes
	for (const auto& p : videoProcesses)
	{
		p->setProcessSize(processingWidth, processingHeight);
		p->processStream = shared_from_this();
		p->setup();
	}

	isSetup = true;
}

void MTVideoInputStream::startStream()
{
	if (!isSetup) setup();
	isRunning = true;
	if (!isThreadRunning()) startThread();
}

void MTVideoInputStream::stopStream()
{
	isRunning = false;
	waitForThread(true, 100);
}

void MTVideoInputStream::closeStream()
{
	stopStream();
	if (inputSource != nullptr) inputSource->close();
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
//	ofScopedLock lock(this->mutex);
	return processToOutputTransform.clone();
}

cv::Mat MTVideoInputStream::getOutputToProcessTransform()
{
//	ofScopedLock lock(this->mutex);
	return outputToProcessTransform.clone();
}

void MTVideoInputStream::setInputSource(MTVideoInputSourceInfo sourceInfo)
{
	// So "lock()" in Linux just deadlocks if the lock is being used elsewhere... So we are doing this stupid thing:
	while (!tryLock())
	{}

	if (inputSource != nullptr) inputSource->close();
	inputSource = MTVideoInput::Instance().createInputSource(sourceInfo);
	if (inputSource != nullptr)
	{
		MTAppFramework::RemoveAllParameters(inputSourcesParameters);
		inputSourcesParameters.add(inputSource->getParameters());
		inputSource->setup();
		inputSource->start();
	}
	else
	{
		ofLogError("MTVideoInputStream") << "Could not find input source with type " << sourceInfo.type;
	}
	unlock();
}

void MTVideoInputStream::setInputSource(MTVideoInputSourceInfo sourceInfo, ofXml& serializer)
{
	// So "lock()" in Linux just deadlocks if the lock is being used elsewhere... So we are doing this stupid thing:
	while (!tryLock())
	{}

	if (inputSource != nullptr) inputSource->close();
	inputSource = MTVideoInput::Instance().createInputSource(sourceInfo);
	if (inputSource != nullptr)
	{
		// This is the deviceID prior to deserialization:
		auto foundDevID = std::string(inputSource->deviceID.get());
		MTAppFramework::RemoveAllParameters(inputSourcesParameters);
		inputSource->deserialize(serializer);
		if (inputSource->deviceID->compare(foundDevID) != 0)
		{
			ofLogWarning("MTVideoInputStream") << "Did not find deviceID " << inputSource->deviceID
														  << ". Assigning found deviceID " << foundDevID << " instead.";
			inputSource->deviceID.setWithoutEventNotifications(foundDevID);
		}
		inputSourcesParameters.add(inputSource->getParameters());
		inputSource->setup();
		inputSource->start();
	}
	else
	{
		ofLogError("MTVideoInputStream") << "Could not find input source with type " << sourceInfo.type;
	}
	unlock();

}

//////////////////////////////////
//Event Listeners
//////////////////////////////////


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

//////////////////////////////////
//Data Handling
//////////////////////////////////

void MTVideoInputStream::addVideoProcess(std::shared_ptr<MTVideoProcess> process)
{
	addVideoProcessAtIndex(process, videoProcesses.size());
}

void MTVideoInputStream::addVideoProcessAtIndex(std::shared_ptr<MTVideoProcess> process, unsigned long index)
{
	// So "lock()" in Linux just deadlocks if the lock is being used elsewhere... So we are doing this stupid thing:
	while (!tryLock())
	{}

	process->setProcessSize(processingWidth, processingHeight);
	process->setup();
	int count = std::count_if(videoProcesses.begin(), videoProcesses.end(),
									  [&process](std::shared_ptr<MTVideoProcess> p)
									  {
										  if (p->getName().find(process->getName()) != string::npos)
										  {
											  return true;
										  }
										  return false;
									  });

	if (count > 0)
	{
		auto newName = process->getName() + "_" + ofToString(count);
		process->setName(newName);
	}

	videoProcesses.insert(videoProcesses.begin() + index, process);

//	processesParameters.addAt(process->getParameters(), index);
	syncParameters();
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
	// So "lock()" in Linux just deadlocks if the lock is being used elsewhere... So we are doing this stupid thing:
	while (!tryLock())
	{}

	videoProcesses.at(index1)->setup();
	videoProcesses.at(index2)->setup();
	std::swap(videoProcesses.at(index1), videoProcesses.at(index2));
	syncParameters();
//	processesParameters.swapPositions(index1, index2);
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
	// So "lock()" in Linux just deadlocks if the lock is being used elsewhere... So we are doing this stupid thing:
	while (!tryLock())
	{}

	auto iter = std::find(videoProcesses.begin(), videoProcesses.end(), process);
	if (iter != videoProcesses.end())
	{
		videoProcesses.erase(iter);
		syncParameters();
//		processesParameters.remove()  <- TODO
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
	while (!tryLock())
	{}
	videoProcesses.clear();
	unlock();
}

//////////////////////////////////
//Class Overrides
//////////////////////////////////

void MTVideoInputStream::deserialize(ofXml& serializer)
{
	bool wasRunning = false;
	isDeserializing = true;
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

	auto chainParent = thisChainXml.getParent();
	MTModel::deserialize(chainParent);
	setProcessingSize(this->processingSize);

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
				process->deserialize(processParamsXml);
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

	auto inputParamsXml = serializer.findFirst("//" + getName() + "/" + inputSourcesParameters.getEscapedName());

	if (!inputParamsXml)
	{
		ofLogWarning() << "MTVideoInputStream: No input sources group in document.";
	}
	else
	{
		auto deviceXml = inputParamsXml.getFirstChild();

		if (!deviceXml)
		{
			ofLogWarning() << "MTVideoInputStream: No input sources in file.";
		}
		else
		{
			MTVideoInputSourceInfo info;
			info.deviceID = deviceXml.getChild("Device_ID").getValue();
			info.type = deviceXml.getChild("Input_Type_Name").getValue();
			info.name = deviceXml.getName();
//				 auto p = deviceXml.getParent();
			setInputSource(info, inputParamsXml);

		}
	}

	updateTransformInternals();

	isDeserializing = false;

	if (wasRunning)
	{
		startStream();
	}
}

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
//		for (auto command : inputROI.get().getCommands())
//		{
//			command.to.x = ofClamp(command.to.x, 0, processingWidth);
//			command.to.y = ofClamp(command.to.y, 0, processingHeight);
//		}

		auto roiPoly = inputROI.get().getOutline()[0];

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

	auto outputPoly = outputRegion.get().getOutline()[0];
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
}

// TODO: Check to see if this is still necessary
void MTVideoInputStream::syncParameters()
{
	parameters.remove(processesParameters);
	MTAppFramework::RemoveAllParameters(processesParameters);
	for (auto p : videoProcesses)
	{
		processesParameters.add(p->getParameters());
	}
	parameters.add(processesParameters);
}
