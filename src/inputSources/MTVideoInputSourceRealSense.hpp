//
// Created by Cristobal Mendoza on 7/17/20.
//

#ifndef NERVOUSSTRUCTUREOF_MTVIDEOINPUTSOURCEREALSENSE_HPP
#define NERVOUSSTRUCTUREOF_MTVIDEOINPUTSOURCEREALSENSE_HPP

#include <MTVideoInputSource.hpp>
#include "ofxMTVideoInput.h"
#include "librealsense2/rs.hpp"
#include <unordered_map>

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

namespace ofxMTVideoInput
{
	 typedef std::pair<int, int> resolution;
	 typedef std::pair<resolution, int> res;

	 struct CaptureResolution
	 {
	 	 int width;
	 	 int height;
	 	 int fps;
	 };
}

class MTVideoInputSourceRealSense : public MTVideoInputSource,
																		public ofThread
{
public:
	 MTVideoInputSourceRealSense(std::string deviceID);
	 ~MTVideoInputSourceRealSense();
	 bool isFrameNew() override;
	 const ofPixels& getPixels() override;
	 cv::Mat getCVPixels() override;
	 void start() override;
	 void close() override;
	 void update() override;
	 void setup() override;
	 void setup(int width, int height, int framerate, std::string deviceID) override;
	 void threadedFunction() override;
	 void serialize(ofXml& serializer) override;
	 void deserialize(ofXml& serializer) override;
	 ofParameter<bool> enableDepth;
	 ofParameter<bool> enableColor;

	 ofParameter<bool> useDisparity;

	 ofParameterGroup decFilterGroup;
	 ofParameter<bool> decFilterOn;
	 ofParameter<int> decFilterMagnitude;

	 ofParameterGroup thrFilterGroup;
	 ofParameter<bool> thrFilterOn;
	 ofParameter<float> thrFilterMinDistance;
	 ofParameter<float> thrFilterMaxDistance;

	 ofParameterGroup spatFilterGroup;
	 ofParameter<bool> spatFilterOn;
	 ofParameter<float> spatFilterAlpha;
	 ofParameter<int> spatFilterDelta;
	 ofParameter<int> spatFilterMag;
	 ofParameter<int> spatFilterHolesFill;

	 ofParameterGroup tempFilterGroup;
	 ofParameter<bool> tempFilterOn;
	 ofParameter<int> tempFilterPersistence;
	 ofParameter<float> tempFilterAlpha;
	 ofParameter<int> tempFilterDelta;

	 ofParameterGroup colorizerGroup;

	 static rs2::context& getRS2Context()
	 {
			static rs2::context context;
			return context;
	 }

	 rs2::device getDeviceWithSerial(std::string serial);

private:
	 rs2::colorizer colorizer;
//	 rs2::pipeline pipeline;
	 rs2::frame_queue outputQueue;
	 rs2::frame_queue postProcessingQueue;
	 rs2::pointcloud pointcloud;
	 rs2::device device;
	 rs2::sensor sensor;
// Declare filters
	 rs2::decimation_filter dec_filter;  // Decimation - reduces depth frame density
	 rs2::threshold_filter thr_filter;   // Threshold  - removes values outside recommended range
	 rs2::spatial_filter spat_filter;    // Spatial    - edge-preserving spatial smoothing
	 rs2::temporal_filter temp_filter;   // Temporal   - reduces temporal noise
	 rs2::disparity_transform depth_to_disparity;
	 rs2::disparity_transform disparity_to_depth;

	 std::vector<rs2::filter> filters;
	 bool isFrameAvailable = false;
	 ofPixels pixels;
	 void setFilterOptions();

	 template<typename T = unsigned char>
	 void toOf(rs2::video_frame& frame, ofPixels_<T>& pix)
	 {
			memcpy(pix.getData(), (T*) frame.get_data(),
						 frame.get_width() * frame.get_height() * sizeof(T) * pix.getNumChannels());
	 }

	 void getSupportedResolutions(rs2::sensor& sensor);
	 rs2::video_stream_profile getBestProfile(int width, int height, int fps);
	 std::vector<rs2::video_stream_profile> streamProfiles;
	 void createParameterFromOption(rs2::options endpoint, rs2_option option, ofParameterGroup& parameters);
	 void setRS2Option(rs2::options endpoint, rs2_option option, float val);

	 ofThreadChannel<std::pair<rs2_option, float>> optionsChannel;
};


#endif //NERVOUSSTRUCTUREOF_MTVIDEOINPUTSOURCEREALSENSE_HPP
