//
// Created by Cristobal Mendoza on 3/4/18.
//

#ifndef NERVOUSSTRUCTUREOF_OFXMTVIDEOINPUT_H
#define NERVOUSSTRUCTUREOF_OFXMTVIDEOINPUT_H

#include "MTVideoProcess.hpp"
#include "MTModel.hpp"
#include "registry.h"

class MTVideoInputStreamEventArgs;
class MTVideoInputStream;
class MTVideoInputSource;

struct MTVideoInputSourceInfo
{
	std::string name;
	std::string type;
	std::string deviceID;
};

typedef std::function<std::vector<MTVideoInputSourceInfo>()> ProviderFunction;

class MTVideoInput : public MTModel
{
public:
	static MTVideoInput& Instance()
	{
		static MTVideoInput instance; // Guaranteed to be destroyed.
		// Instantiated on first use.
		return instance;
	}

	MTVideoInput(MTVideoInput const&) = delete;
	void operator=(MTVideoInput const&) = delete;

	void init();

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

	std::vector<MTVideoInputSourceInfo> getInputSources()
	{ return inputSources; }


	/**
	 * @brief Instantiates a process from the registry.
	 * @param processTypename The type name of the process.
	 * @return a shared_ptr<MTVideoProcess> if the type name was found in the registry, or
	 * nullptr if the video process could not be instantiated.
	 */
	std::shared_ptr<MTVideoProcess> createVideoProcess(std::string processTypename)
	{
		return std::shared_ptr<MTVideoProcess>(
				videoProcessRegistry.create(processTypename));
	}

	template<typename T>
	bool registerVideoProcess(std::string vpName)
	{
		bool success = videoProcessRegistry.registerClass(vpName, []() -> MTVideoProcess*
		{ return new T(); });
		if (!success) ofLogError() << "Error registering video process " << vpName;
		return success;
	}

	/**
 * @brief Instantiates a process from the registry.
 * @param processTypename The type name of the process.
 * @return a shared_ptr<MTVideoProcess> if the type name was found in the registry, or
 * nullptr if the video process could not be instantiated.
 */
	std::shared_ptr<MTVideoInputSource> createInputSource(MTVideoInputSourceInfo inputSourceInfo);

	template<typename T>
	bool registerInputSource(std::string sourceClassName, ProviderFunction provider)
	{
		bool success = inputSourceRegistry.registerClass(sourceClassName, [](std::string arg) -> MTVideoInputSource*
		{ return new T(arg); });
		if (!success)
		{
			ofLogError() << "Error registering input source " << sourceClassName;
			return false;
		}

		addInputSourcesProvider(provider);
		return true;
	}

	std::vector<std::string>& getVideoProcessNames()
	{
		return videoProcessRegistry.getNames();
	}

	virtual void serialize(ofXml& serializer);
	virtual void deserialize(ofXml& serializer);

	ofEvent<MTVideoInputStreamEventArgs> inputStreamAddedEvent;
	ofEvent<MTVideoInputStreamEventArgs> inputStreamRemovedEvent;

private:
	MTVideoInput();
	std::vector<std::shared_ptr<MTVideoInputStream>> inputStreams;
	void syncParameters();
	ofxMTVideoInput::Registry<MTVideoProcess> videoProcessRegistry;
	ofxMTVideoInput::Registry<MTVideoInputSource, std::string> inputSourceRegistry;
	std::vector<ProviderFunction> providerFunctions;
	std::vector<MTVideoInputSourceInfo> inputSources;
	bool isInit = false;

	void addInputSourcesProvider(ProviderFunction function)
	{
		providerFunctions.push_back(function);
		updateInputSources();
	}

	void updateInputSources()
	{
		inputSources.clear();

		for (auto f : providerFunctions)
		{
			auto sources = f();
			for (auto source : sources)
			{
				inputSources.push_back(source);
			}
		}
	}

};

class MTVideoInputStreamEventArgs : public ofEventArgs
{
public:
	std::shared_ptr<MTVideoInputStream> inputStream;
};


#endif //NERVOUSSTRUCTUREOF_OFXMTVIDEOINPUT_H
