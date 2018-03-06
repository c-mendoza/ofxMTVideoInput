//
// Created by Cristobal Mendoza on 3/4/18.
//

#ifndef NERVOUSSTRUCTUREOF_OFXMTVIDEOINPUT_H
#define NERVOUSSTRUCTUREOF_OFXMTVIDEOINPUT_H

#include "MTVideoProcessStream.hpp"
#include "MTVideoProcess.hpp"
#include "MTCommonProcesses.hpp"
#include "registry.h"

namespace ofxMTVideoInput
{

	/**
	 * @brief Instantiates a process from the registry.
	 * @param processTypename The type name of the process.
	 * @param nameId A friendly name for the process. This name will be used as the MTModel name
	 * for serialization.
	 * @return a shared_ptr<MTVideoProcess> if the type name was found in the registry, or
	 * nullptr if the video process could not be instantiated.
	 */
	std::shared_ptr<MTVideoProcess> mtCreateVideoProcess(std::string processTypename, std::string nameId)
	{
		return shared_ptr<MTVideoProcess>(
				mtRegistry::Registry<MTVideoProcess, string>::Create(processTypename, nameId));
	}
};

#endif //NERVOUSSTRUCTUREOF_OFXMTVIDEOINPUT_H
