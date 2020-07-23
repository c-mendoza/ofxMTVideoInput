//
// Created by Cristobal Mendoza on 7/17/20.
//

#include "MTVideoInputSourceRealSense.hpp"

bool MTVideoInputSourceRealSense::isFrameNew()
{
	return false;
}

const ofPixels& MTVideoInputSourceRealSense::getOfPixels()
{
	return ofPixels();
}

cv::Mat MTVideoInputSourceRealSense::getCVPixels()
{
	return cv::Mat();
}

void MTVideoInputSourceRealSense::start()
{
	processingThread = std::thread([&] {
		while(isRunning())
		{
			rs2::frameset data = pipeline.wait_for_frames(); // Wait for next set of frames from the camera
			rs2::frame depth_frame = data.get_depth_frame(); //Take the depth frame from the frameset
			if (!depth_frame) // Should not happen but if the pipeline is configured differently
				return;       //  it might not provide depth and we don't want to crash

			rs2::frame filtered = depth_frame; // Does not copy the frame, only adds a reference

/* Apply filters.
            The implemented flow of the filters pipeline is in the following order:
            1. apply decimation filter
            2. apply threshold filter
            3. transform the scene into disparity domain
            4. apply spatial filter
            5. apply temporal filter
            6. revert the results back (if step Disparity filter was applied
            to depth domain (each post processing block is optional and can be applied independantly).
            */

			if (decFilterOn) filtered = dec_filter.process(filtered);
			if (thrFilterOn) filtered = thr_filter.process(filtered);
			if (useDisparity) filtered = depth_to_disparity.process(filtered);
			if (spatFilterOn) filtered = spat_filter.process(filtered);
			if (tempFilterOn) filtered = temp_filter.process(filtered);

			if (useDisparity) {
				filtered = disparity_to_depth.process(filtered);
			}


			// Push filtered & original data to their respective queues
			// Note, pushing to two different queues might cause the application to display
			//  original and filtered pointclouds from different depth frames
			//  To make sure they are synchronized you need to push them together or add some
			//  synchronization mechanisms
			filtered_data.enqueue(filtered);
			original_data.enqueue(depth_frame);
		}
	});
}

void MTVideoInputSourceRealSense::stop()
{

}

void MTVideoInputSourceRealSense::update()
{
	rs2::frameset fs;
	if (pipeline.poll_for_frames(&fs))
	{
		for (const rs2::frame& f : fs)
		{
			dec_filter.get
		}

	}
}

void MTVideoInputSourceRealSense::setup(std::string deviceID, bool useTexture)
{
	setup(848, 480, 30, deviceID, useTexture);
}

void MTVideoInputSourceRealSense::setup(int width, int height, int framerate, std::string deviceID, bool useTexture)
{
	rs2::config cfg;
	cfg.enable_device(deviceID);

	processingThread.

}
