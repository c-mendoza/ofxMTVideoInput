//
// Created by Cristobal Mendoza on 3/4/18.
//

#ifndef NERVOUSSTRUCTUREOF_MTVIDEOPROCESSVIEW_HPP
#define NERVOUSSTRUCTUREOF_MTVIDEOPROCESSVIEW_HPP

#include "ofxImGui.h"
#include "MTAppFrameworkUtils.hpp"

class MTVideoProcess;

class MTVideoProcessUI : public std::enable_shared_from_this<MTVideoProcessUI>,
						 public MTEventListenerStore
{
public:
	MTVideoProcessUI(std::shared_ptr<MTVideoProcess> videoProcess);

	virtual ~MTVideoProcessUI(){}

	virtual void draw(){}
	virtual void draw(ofxImGui::Settings& settings);

protected:
	std::weak_ptr<MTVideoProcess> videoProcess;
};

#endif //NERVOUSSTRUCTUREOF_MTVIDEOPROCESSVIEW_HPP
