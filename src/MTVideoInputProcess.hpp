//
// Created by Cristobal Mendoza on 3/5/18.
//

#ifndef NERVOUSSTRUCTUREOF_MTVIDEOINPUTPROCESS_HPP
#define NERVOUSSTRUCTUREOF_MTVIDEOINPUTPROCESS_HPP

#include "MTVideoProcess.hpp"

class MTVideoInputProcess : public MTVideoProcess
{
public:
	MTVideoInputProcess(const std::string& name);
	~MTVideoInputProcess();

	MTProcessData& process(MTProcessData& input) override;
	virtual void start() = 0;
	virtual void stop() = 0;
	virtual void update() = 0;
	virtual bool isFrameNew() = 0;
	bool isRunning() { return _isRunning; }

protected:
	bool _isRunning = false;
};

class MTVideoCaptureProcess : public MTVideoInputProcess
{
public:
	MTVideoCaptureProcess(const std::string& name);
	void start() override;
	void stop() override;
	void update() override;
	bool isFrameNew() override;

protected:
	ofVideoGrabber videoGrabber;
};



#endif //NERVOUSSTRUCTUREOF_MTVIDEOINPUTPROCESS_HPP
