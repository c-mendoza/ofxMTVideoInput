//
// Created by Cristobal Mendoza on 7/17/20.
//

#ifndef NERVOUSSTRUCTUREOF_MTVIDEOINPUTSOURCEREALSENSE_HPP
#define NERVOUSSTRUCTUREOF_MTVIDEOINPUTSOURCEREALSENSE_HPP

#include <MTVideoInputSource.hpp>
#include "librealsense2/rs.hpp"


/**
Class to encapsulate a filter alongside its options
*/
class RSFilter : public MTModel
{
public:
	RSFilter(const std::string name, rs2::filter& filter);
	ofParameter<std::string> filter_name;                                   //Friendly name of the filter
	ofParameter<bool> enabled;

	rs2::filter& filter;                                       //The filter in use
//	std::map<rs2_option, filter_slider_ui> supported_options;  //maps from an option supported by the filter, to the corresponding slider
	std::atomic_bool is_enabled;                               //A boolean controlled by the user that determines whether to apply the filter or not
};

class MTVideoInputSourceRealSense : public MTVideoInputSource
{
public:
	bool isFrameNew() override;
	const ofPixels& getPixels() override;
	cv::Mat getCVPixels() override;
	void start() override;
	void close() override;
	void update() override;
	void setup(std::string deviceID) override;
	void setup(int width, int height, int framerate, std::string deviceID) override;
	ofParameter<bool> decFilterOn;
	ofParameter<float> decFilterMagnitude;
	ofParameter<bool> thrFilterOn;
	ofParameter<float> thrFilterMinDistance;
	ofParameter<float> thrFilterMaxDistance;
	ofParameter<bool> spatFilterOn;
	ofParameter<float> spatFilterAlpha;
	ofParameter<float> spatFilterDelta;
	ofParameter<float> spatFilterMag;
	ofParameter<float> spatFilterHolesFill;
	ofParameter<bool> tempFilterOn;
	ofParameter<int> tempFilterPersistence;
	ofParameter<float> tempFilterAlpha;
	ofParameter<float> tempFilterDelta;
	ofParameter<bool> useDisparity;

private:
	static rs2::context context;
	rs2::colorizer colorizer;
	rs2::pipeline pipeline;
	rs2::frame_queue frameQueue;
	rs2::pointcloud pointcloud;
	// Declare filters
	rs2::decimation_filter dec_filter;  // Decimation - reduces depth frame density
	rs2::threshold_filter thr_filter;   // Threshold  - removes values outside recommended range
	rs2::spatial_filter spat_filter;    // Spatial    - edge-preserving spatial smoothing
	rs2::temporal_filter temp_filter;   // Temporal   - reduces temporal noise
	rs2::disparity_transform depth_to_disparity;
	rs2::disparity_transform disparity_to_depth;

	std::vector<rs2::filter> filters;

	std::thread processingThread;
};


#endif //NERVOUSSTRUCTUREOF_MTVIDEOINPUTSOURCEREALSENSE_HPP
