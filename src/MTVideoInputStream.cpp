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


MTVideoInputStream::MTVideoInputStream(std::string name) : MTModel(name)
{

	 parameters.add(isRunning.set("Running", false),
									mirrorVideo.set("Mirror Video", true),
									processingWidth.set("Process Width", 320, 120, 1920),
									processingHeight.set("Process Height", 240, 80, 1080),
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

//Add the listeners
	 processingWidth.addListener(this, &MTVideoInputStream::processSizeChanged);
	 processingHeight.addListener(this, &MTVideoInputStream::processSizeChanged);

	 updateTransformInternals();

	 addEventListener(useROI.newListener([this](bool& val)
																			 {
																					updateTransformInternals();
																			 }));

//	addEventListener(outputRegion.newListener([this](ofPath& val) {
//		updateTransformInternals();
//	}));
//	addEventListener(inputROI.newListener([this](ofPath& val) {
//		updateTransformInternals();
//	}));
}

MTVideoInputStream::~MTVideoInputStream()
{
	 closeStream();
	 processingWidth.removeListener(this, &MTVideoInputStream::processSizeChanged);
	 processingHeight.removeListener(this, &MTVideoInputStream::processSizeChanged);
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

//		std::unique_lock<std::mutex>(mutex);
			lock();
			while (!functionQueue.empty())
			{
				 auto f = functionQueue.front();
				 f();
				 functionQueue.pop();
			}

			inputSource->update();
			if (inputSource->isFrameNew())
			{
				 cv::Size processSize(processingWidth, processingHeight);
				 fpsCounter.newFrame();
				 videoInputImage = ofxCv::toCv(static_cast<const ofPixels&>(inputSource->getPixels()));

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
			unlock();
	 }

	 ofLogVerbose("MTVideoInput") << "Thread complete";
}

void MTVideoInputStream::setup()
{
	 workingImage.create(processingHeight, processingWidth, CV_8UC1);
	 processOutput.create(processingHeight, processingWidth, CV_8UC1);

	 updateTransformInternals();

//	 if (inputSource == nullptr)
//	 {
//
//			auto sources = MTVideoInput::Instance().getInputSources();
//			if (sources.empty())
//			{
//				 isSetup = false;
//				 ofLogError("MTVideoInputStream") << "No input devices available";
//			}
//			else
//			{
//				 setInputSource(sources[0]);
//				 inputSource->setup();
//				 inputSource->start();
//			}
//	 }

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
	 inputSource->close();
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
	 lock();
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
	 lock();
	 if (inputSource != nullptr) inputSource->close();
	 inputSource = MTVideoInput::Instance().createInputSource(sourceInfo);
	 if (inputSource != nullptr)
	 {
			MTAppFramework::RemoveAllParameters(inputSourcesParameters);
			inputSource->deserialize(serializer);
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
	 processingWidth.set(w);
	 processingHeight.set(h);
	 updateTransformInternals();
	 unlock();
	 processSizeChanged(bogus);
}

void MTVideoInputStream::setCaptureResolution(int w, int h)
{
	 inputSource->captureSize = glm::vec2(w, h);
}

glm::vec2 MTVideoInputStream::getCaptureResolution()
{
	 return inputSource->captureSize.get();
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

//void MTVideoInputStream::videoDeviceIDChanged(int& unused)
//{
//	if (isThreadRunning())
//	{
////		lock();
//	}
//	isSetup = initializeVideoCapture();
//	if (!isSetup)
//	{
//		ofLogError(getName()) << "Could not reinitialize video";
//	}
//
//	if (isThreadRunning())
//	{
////		unlock();
//	}
//}

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
	 process->setProcessSize(processingWidth, processingHeight);
	 process->setup();
	 int count = std::count_if(videoProcesses.begin(), videoProcesses.end(), [&process](std::shared_ptr<MTVideoProcess> p)
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

	 lock();
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
	 std::unique_lock<std::mutex> uniqueLock(this->mutex);
	 auto iter = std::find(videoProcesses.begin(), videoProcesses.end(), process);
	 if (iter != videoProcesses.end())
	 {
			videoProcesses.erase(iter);
			syncParameters();
//		processesParameters.remove()  <- TODO
			processRemovedEvent.notify(this, process);
			return true;
	 }
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

	 if (wasRunning)
	 {
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
			for (auto command : inputROI.get().getCommands())
			{
				 command.to.x = ofClamp(command.to.x, 0, processingWidth);
				 command.to.y = ofClamp(command.to.y, 0, processingHeight);
			}

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
	 processesParameters.clear();
	 for (auto p : videoProcesses)
	 {
			processesParameters.add(p->getParameters());
	 }
	 parameters.add(processesParameters);
}
