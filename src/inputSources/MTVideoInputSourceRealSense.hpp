//
// Created by Cristobal Mendoza on 7/17/20.
//

#ifndef NERVOUSSTRUCTUREOF_MTVIDEOINPUTSOURCEREALSENSE_HPP
#define NERVOUSSTRUCTUREOF_MTVIDEOINPUTSOURCEREALSENSE_HPP

#include <MTVideoInputSource.hpp>
#include "ofxMTVideoInput.h"
#include "librealsense2/rs.hpp"
#include <unordered_map>
#include <utils/ofThread.h>

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
	enum OutputMode {
		Output2D,		// Output in ofPixels
		Output3D,		// Output in rs2::points
		OutputRS2		// Output in rs2::frame
	};

	MTVideoInputSourceRealSense(std::string deviceID);
	~MTVideoInputSourceRealSense();
	bool isFrameNew() override;
	const ofPixels& getPixels() override;
	void start() override;
	void close() override;
	void update() override;
	void setup() override;
	void setup(int width, int height, int framerate, std::string deviceID) override;
	void threadedFunction() override;
	void serialize(ofXml& serializer) override;
	void deserialize(ofXml& serializer) override;

	const ofMesh& getMesh();
	ofParameter<bool> enableDepth;
	ofParameter<bool> enableColor;
	ofParameter<int> outputMode;

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
	ofParameter<bool> useColorizer;

	static rs2::context& getRS2Context()
	{
		static rs2::context context;
		return context;
	}

	bool getDeviceWithSerial(const std::string& serial, rs2::device& device);

	const rs2::points& getPoints();
	rs2::frame getRS2Frame();
	static std::vector<std::shared_ptr<rs2::device>> GetDevices();
	std::vector<rs2::video_stream_profile> getStreamProfiles();
	const rs2::depth_sensor getDepthSensor() { return sensor.as<rs2::depth_sensor>(); }

private:
	rs2::colorizer colorizer;
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
	rs2::disparity_transform disparity_to_depth = rs2::disparity_transform(false);

	std::vector<rs2::filter> filters;
	bool isFrameAvailable = false;
	ofPixels pixels;
	rs2::points points;
	rs2::frame rs2Frame;
	void setFilterOptions();

	ofMesh mesh;

	template<typename T = unsigned char>
	void toOf(rs2::video_frame& frame, ofPixels_<T>& pix)
	{
		memcpy(pix.getData(), (T*) frame.get_data(),
			   frame.get_width() * frame.get_height() * sizeof(T) * pix.getNumChannels());
	}

	void getSupportedResolutions(rs2::sensor& sensor);
	rs2::video_stream_profile getBestProfile(int width, int height, int fps);
	std::vector<rs2::video_stream_profile> streamProfiles;
	void createParameterFromOption(const rs2::options& endpoint, rs2_option option, ofParameterGroup& parameters);
	static void SetRS2Option(const rs2::options& endpoint, rs2_option option, float val);

	ofThreadChannel<std::pair<rs2_option, float>> optionsChannel;

	static std::vector<std::shared_ptr<rs2::device>> Devices;


	rs2::frame colorizedFrame;
};


#endif //NERVOUSSTRUCTUREOF_MTVIDEOINPUTSOURCEREALSENSE_HPP
