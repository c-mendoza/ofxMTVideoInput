//
// Created by Cristobal Mendoza on 7/16/18.
//

#include <processes/MTBackgroundSubstraction2.hpp>
#include "ofxMTVideoInput.h"
#include "ofXml.h"
#include "processes/MTThresholdVideoProcess.hpp"
#include "processes/MTBackgroundSubstractionVideoProcess.hpp"
#include "processes/MTOpticalFlowVideoProcess.hpp"
#include "processes/MTImageAdjustmentsVideoProcess.hpp"
#include "registry.h"
#include "processes/MTMorphology.hpp"

MTVideoInput::MTVideoInput() : MTModel("VideoProcessChains")
{}

std::shared_ptr<MTVideoInputStream> MTVideoInput::createStream()
{
	return createStream("Stream_" + ofToString(inputStreams.size() + 1));
}

std::shared_ptr<MTVideoInputStream> MTVideoInput::createStream(std::string name)
{
	auto stream = std::make_shared<MTVideoInputStream>(name);
	inputStreams.push_back(stream);
	parameters.add(stream->getParameters());
	// Fire addedVideoProcessStreamEvent
	MTVideoInputStreamEventArgs args;
	args.inputStream = stream;
	inputStreamAddedEvent.notify(args);
	stream->startStream();
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
				auto chain = createStream(streamXml.getName());
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
}

size_t MTVideoInput::getStreamCount()
{
	return inputStreams.size();
}

// Stupid shit so that we can compile with MSVC
auto r1 = ofxMTVideoInput::Registry<MTVideoProcess>::Register("MTThresholdVideoProcess", []() -> MTVideoProcess* {return new MTThresholdVideoProcess(); });
auto r2 = ofxMTVideoInput::Registry<MTVideoProcess>::Register("MTBackgroundSubstractionVideoProcess", []() -> MTVideoProcess * {return new MTBackgroundSubstractionVideoProcess(); });
auto r3 = ofxMTVideoInput::Registry<MTVideoProcess>::Register("MTOpticalFlowVideoProcess", []() -> MTVideoProcess * {return new MTOpticalFlowVideoProcess(); });
auto r4 = ofxMTVideoInput::Registry<MTVideoProcess>::Register("MTImageAdjustmentsVideoProcess", []() -> MTVideoProcess * {return new MTImageAdjustmentsVideoProcess(); });
auto r5 = ofxMTVideoInput::Registry<MTVideoProcess>::Register("MTMorphologyVideoProcess", []() -> MTVideoProcess * {return new MTMorphologyVideoProcess(); });
auto r6 = ofxMTVideoInput::Registry<MTVideoProcess>::Register("MTBackgroundSubstraction2", []() -> MTVideoProcess * {return new MTBackgroundSubstraction2(); });

//REGISTER_SUBCLASS(MTVideoProcess, MTBackgroundSubstractionVideoProcess)
//REGISTER_SUBCLASS(MTVideoProcess, MTThresholdVideoProcess)
//REGISTER_SUBCLASS(MTVideoProcess, MTOpticalFlowVideoProcess)
//REGISTER_SUBCLASS(MTVideoProcess, MTImageAdjustmentsVideoProcess)

