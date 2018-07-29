//
// Created by Cristobal Mendoza on 3/4/18.
//

#ifndef NERVOUSSTRUCTUREOF_OFXMTVIDEOINPUT_H
#define NERVOUSSTRUCTUREOF_OFXMTVIDEOINPUT_H

#include "MTVideoInputStream.hpp"
#include "MTVideoProcess.hpp"
#include "processes/MTThresholdVideoProcess.hpp"
#include "processes/MTBackgroundSubstractionVideoProcess.hpp"
#include "processes/MTOpticalFlowVideoProcess.hpp"
#include "processes/MTImageAdjustmentsVideoProcess.hpp"
#include "registry.h"
#include "MTModel.hpp"


class MTVideoInputStreamEventArgs;
class MTVideoProcessEventArgs;

class MTVideoInput : public MTModel
{
public:
	static MTVideoInput& getInstance()
	{
		static MTVideoInput instance; // Guaranteed to be destroyed.
		// Instantiated on first use.
		return instance;
	}

private:
	MTVideoInput();
	std::vector<std::shared_ptr<MTVideoInputStream>> inputStreams;

public:
	MTVideoInput(MTVideoInput const&) = delete;
	void operator=(MTVideoInput const&) = delete;

	std::shared_ptr<MTVideoInputStream> createStream();
	std::shared_ptr<MTVideoInputStream> createStream(std::string name);
	void removeStream(int index);
	void removeStream(std::shared_ptr<MTVideoInputStream> stream);
	void removeAllStreams();
	size_t getStreamCount();

	typename std::vector<std::shared_ptr<MTVideoInputStream>>::const_iterator begin() const;
	typename std::vector<std::shared_ptr<MTVideoInputStream>>::const_reverse_iterator rbegin() const;
	typename std::vector<std::shared_ptr<MTVideoInputStream>>::const_iterator end() const;
	typename std::vector<std::shared_ptr<MTVideoInputStream>>::const_reverse_iterator rend() const;


	/**
	 * @brief Instantiates a process from the registry.
	 * @param processTypename The type name of the process.
	 * @return a shared_ptr<MTVideoProcess> if the type name was found in the registry, or
	 * nullptr if the video process could not be instantiated.
	 */
	std::shared_ptr<MTVideoProcess> createVideoProcess(std::string processTypename)
	{
		return std::shared_ptr<MTVideoProcess>(
				ofxMTVideoInput::Registry<MTVideoProcess>::Create(processTypename));
	}

	virtual void serialize(ofXml& serializer);
	virtual void deserialize(ofXml& serializer);

	ofEvent<MTVideoInputStreamEventArgs> inputStreamAddedEvent;
	ofEvent<MTVideoInputStreamEventArgs> inputStreamRemovedEvent;

};

class MTVideoInputStreamEventArgs : public ofEventArgs
{
public:
	std::shared_ptr<MTVideoInputStream> inputStream;
};


REGISTER_SUBCLASS(MTVideoProcess, MTBackgroundSubstractionVideoProcess)
REGISTER_SUBCLASS(MTVideoProcess, MTThresholdVideoProcess)
REGISTER_SUBCLASS(MTVideoProcess, MTOpticalFlowVideoProcess)
REGISTER_SUBCLASS(MTVideoProcess, MTImageAdjustmentsVideoProcess)

#endif //NERVOUSSTRUCTUREOF_OFXMTVIDEOINPUT_H
