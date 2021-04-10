//
// Created by Cristobal Mendoza on 7/16/18.
//

#include <processes/MTBackgroundSubstraction2.hpp>
#include <inputSources/MTVideoInputSourceOFVideoGrabber.hpp>
#include "ofxMTVideoInput.h"
#include "ofXml.h"
#include "processes/MTThresholdVideoProcess.hpp"
#include "processes/MTBackgroundSubstractionVideoProcess.hpp"
#include "processes/MTOpticalFlowVideoProcess.hpp"
#include "processes/MTImageAdjustmentsVideoProcess.hpp"
#include "registry.h"
#include "processes/MTMorphology.hpp"
#include "MTVideoInputSource.hpp"
#include "MTVideoInputStream.hpp"
#include "inputSources/MTVideoInputSourceRealSense.hpp"

MTVideoInput::MTVideoInput() : MTModel("VideoProcessChains")
{

}

MTVideoInput::~MTVideoInput() {
	removeAllStreams();
}

void MTVideoInput::init()
{
	if (isInit) return;
	registerVideoProcess<MTThresholdVideoProcess>("MTThresholdVideoProcess");
	registerVideoProcess<MTBackgroundSubstraction2>("MTBackgroundSubstraction2");
	registerVideoProcess<MTMorphologyVideoProcess>("MTMorphologyVideoProcess");
	registerVideoProcess<MTImageAdjustmentsVideoProcess>("MTImageAdjustmentsVideoProcess");
	registerVideoProcess<MTOpticalFlowVideoProcess>("MTOpticalFlowVideoProcess");
	registerVideoProcess<MTBackgroundSubstractionVideoProcess>("MTBackgroundSubstractionVideoProcess");

	registerInputSource<MTVideoInputSourceOFGrabber>("MTVideoInputSourceOFGrabber", []()
	{
		std::vector<MTVideoInputSourceInfo> sources;
		auto ofDevices = ofVideoGrabber().listDevices();
		for (auto device : ofDevices)
		{
			MTVideoInputSourceInfo info;
			info.name = device.deviceName;
			info.deviceID = ofToString(device.id);
			info.type = "MTVideoInputSourceOFGrabber";
			sources.push_back(info);
		}
		return sources;
	});

	registerInputSource<MTVideoInputSourceRealSense>("MTVideoInputSourceRealSense", []()
	{
		std::vector<MTVideoInputSourceInfo> sources;
		rs2::config cfg;
		for (auto device : MTVideoInputSourceRealSense::getRS2Context().query_devices())
		{
			MTVideoInputSourceInfo info;
			info.name = device.get_info(RS2_CAMERA_INFO_NAME);
			info.deviceID = device.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
			info.type = "MTVideoInputSourceRealSense";
			sources.push_back(info);
		}
		return sources;
	});
	updateInputSources(); // Probably don't need this

	// Need to stop all streams at the exitEvent instead of the destructor in order to avoid
	// a crash at app exit time:
	auto el = ofGetMainLoop()->exitEvent.newListener([this]() {
		removeAllStreams();
	});
	addEventListener(std::move(el));
	isInit = true;
}

std::shared_ptr<MTVideoInputStream> MTVideoInput::createStream(bool start)
{
	return createStream("Stream_" + ofToString(inputStreams.size() + 1), start);
}

std::shared_ptr<MTVideoInputStream> MTVideoInput::createStream(std::string name, bool start)
{
	auto stream = std::make_shared<MTVideoInputStream>(name);
	inputStreams.push_back(stream);
	parameters.add(stream->getParameters());
	MTVideoInputStreamEventArgs args;
	args.inputStream = stream;
	inputStreamAddedEvent.notify(args);
	if (start) stream->startStream();
	return stream;
}

void MTVideoInput::removeStream(int index)
{
	auto iter = inputStreams.erase(inputStreams.begin() + index);
	syncParameters();
	MTVideoInputStreamEventArgs args;
	args.inputStream = *iter;
	inputStreamRemovedEvent.notify(args);

// Rename the remaining process chains:
	for (int newDex = index; newDex < inputStreams.size(); newDex++)
	{
		inputStreams[newDex]->setName("Stream_" + ofToString(newDex));
	}

}

void MTVideoInput::removeStream(std::shared_ptr<MTVideoInputStream> stream)
{
	stream->stopStream();
	auto dex = ofFind(inputStreams, stream);
	removeStream(dex);

}

void MTVideoInput::removeAllStreams()
{
	for (auto stream : inputStreams)
	{
		stream->closeStream();
	}

	inputStreams.clear();
	parameters.clear();
	MTVideoInputStreamEventArgs args;
	args.inputStream = nullptr;
	inputStreamRemovedEvent.notify(args);
}

typename std::vector<std::shared_ptr<MTVideoInputStream>>::const_iterator MTVideoInput::begin() const
{
	return inputStreams.begin();
}

typename std::vector<std::shared_ptr<MTVideoInputStream>>::const_reverse_iterator MTVideoInput::rbegin() const
{
	return inputStreams.rbegin();
}

typename std::vector<std::shared_ptr<MTVideoInputStream>>::const_iterator MTVideoInput::end() const
{
	return inputStreams.end();
}

typename std::vector<std::shared_ptr<MTVideoInputStream>>::const_reverse_iterator MTVideoInput::rend() const
{
	return inputStreams.rend();
}

void MTVideoInput::serialize(ofXml& serializer)
{
	// Find and remove the old parameters, if any:
	auto oldParams = serializer.findFirst(this->getParameters().getEscapedName());
	if (oldParams.getName() == this->getParameters().getEscapedName()) {
		serializer.removeChild(oldParams);
	}

	syncParameters();
	MTModel::serialize(serializer);
}

void MTVideoInput::syncParameters()
{
	parameters.clear();
	for (auto& stream : inputStreams)
	{
		stream->syncParameters();
		parameters.add(stream->getParameters());
	}
}

std::istream& operator>>(std::istream& is, MTVideoInput& path)
{
	ofLogError(__PRETTY_FUNCTION__) << "is not implemented!";
	return is;
}

void MTVideoInput::deserialize(ofXml& serializer)
{
//	MTModel::deserialize(serializer);
	removeAllStreams();
	if (auto streamsXml = serializer.findFirst("//VideoProcessChains"))
	{
		auto children = streamsXml.getChildren();
//		auto bla = *(children.begin());
		for (auto& streamXml : children)
		{
			if (streamXml)
			{
				auto chain = createStream(streamXml.getName(), false);
				chain->deserialize(serializer);

				// createStream adds a parameterGroup to the parameters. We need to first remove that
				// group if we are to add the deserialized one:
				if (chain->getParameters().getName() == streamXml.getName())
				{
					parameters.remove(streamXml.getName());
				}

				parameters.add(chain->getParameters());
			}
		}
	}

	for (const auto& stream : inputStreams)
	{
		stream->startStream();
	}
}

size_t MTVideoInput::getStreamCount()
{
	return inputStreams.size();
}

std::shared_ptr<MTVideoInputSource> MTVideoInput::createInputSource(MTVideoInputSourceInfo sourceInfo)
{
	auto deviceID = sourceInfo.deviceID;
	auto inputSource = std::shared_ptr<MTVideoInputSource>(
			inputSourceRegistry.create<std::string>(sourceInfo.type,
													std::move(deviceID)));
	if (inputSource->isSetup())
	{
		return inputSource;
	}
	else
	{
		return nullptr;
	}
}


