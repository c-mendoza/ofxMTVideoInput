//
// Created by Cristobal Mendoza on 7/17/20.
//

#ifdef MTVI_USE_REALSENSE

#include "MTVideoInputSourceRealSense.hpp"
#include "librealsense2/hpp/rs_processing.hpp"
#include "librealsense2/hpp/rs_device.hpp"
#include "librealsense2/hpp/rs_sensor.hpp"


std::vector<std::shared_ptr<rs2::device>> MTVideoInputSourceRealSense::Devices;

MTVideoInputSourceRealSense::MTVideoInputSourceRealSense(std::string devID) :
		MTVideoInputSource("RealSense Camera", "MTVideoInputSourceRealSense", "RealSense Camera", devID)
{
	bool success = MTVideoInputSourceRealSense::getDeviceWithSerial(deviceID, device);

	if (!success)
	{
		ofLogError("MTVideoInputSourceRealSense") << "No devices found";
		// Flag that the input source is not set up so that we can detect the failure in createInputSource:
		isSetupFlag = false;
		return;
	}

	auto sensors = device.query_sensors();
	for (auto s : sensors)
	{
		if (auto ds = s.as<rs2::depth_sensor>()) // Only depth sensor for now
		{
			sensor = ds;
			auto opts = ds.get_supported_options();
			for (auto opt : opts)
			{
				if (ds.is_option_read_only(opt)) continue;
				createParameterFromOption(ds, opt, parameters);
			}
			break;
		}
	}

	auto copts = colorizer.get_supported_options();
	colorizerGroup.setName("Colorizer");
	for (auto option : copts)
	{
//		ofLogVerbose() << colorizer.get_option_name(option) << " | " << colorizer.get_option_description(option)
//					   << " step: " << colorizer.get_option_range(option).step;
		createParameterFromOption(colorizer, option, colorizerGroup);

	}

	// Fixes to avoid gui crashes:
	colorizerGroup.getFloat("Min Distance").setMin(0.01f);
	colorizerGroup.getFloat("Max Distance").setMin(0.01f);

	getSupportedResolutions(sensor);

//	captureSize.setWithoutEventNotifications(())
	decFilterGroup.setName("Decimation Filter");
	decFilterGroup.add(decFilterOn.set("Enabled", false),
					   decFilterMagnitude.set("Magnitude", 2, 1, 8)
	);

	thrFilterGroup.setName("Threshold Filter");
	thrFilterGroup.add(thrFilterOn.set("Enabled", false),
					   thrFilterMinDistance.set("Min Distance", 0.1f, 0.1f, 16.f),
					   thrFilterMaxDistance.set("Max Distance", 16.f, 0.1f, 16.f)
	);

	spatFilterGroup.setName("Spatial Filter");
	spatFilterGroup.add(spatFilterOn.set("Enabled", false),
						spatFilterAlpha.set("Alpha", 0.5f, 0.25f, 1.f),
						spatFilterDelta.set("Delta", 20, 1, 50),
						spatFilterMag.set("Magnitude", 2, 1, 5),
						spatFilterHolesFill.set("Hole Filling", 0, 0, 5)
	);

	tempFilterGroup.setName("Temporal Filter");
	tempFilterGroup.add(tempFilterOn.set("Enabled", false),
						tempFilterPersistence.set("Persistence", 3, 0, 8),
						tempFilterAlpha.set("Alpha", 0.4f, 0.0f, 1.f),
						tempFilterDelta.set("Delta", 20, 1, 100)
	);

	addParameters(useColorizer.set("Enable Colorizer", true),
				  colorizerGroup,
				  enableDepth.set("Enable Depth", true),
				  enableColor.set("Enable Color", false),
				  useDisparity.set("Use Disparity Filter", true),
				  decFilterGroup,
				  thrFilterGroup,
				  spatFilterGroup,
				  tempFilterGroup);

	addEventListener(decFilterMagnitude.newListener([this](int& val)
													{ setFilterOptions(); }));
	addEventListener(thrFilterMinDistance.newListener([this](float& val)
													  { setFilterOptions(); }));
	addEventListener(thrFilterMaxDistance.newListener([this](float& val)
													  { setFilterOptions(); }));
	addEventListener(spatFilterAlpha.newListener([this](float& val)
												 { setFilterOptions(); }));
	addEventListener(spatFilterDelta.newListener([this](int& val)
												 { setFilterOptions(); }));
	addEventListener(spatFilterMag.newListener([this](int& val)
											   { setFilterOptions(); }));
	addEventListener(spatFilterHolesFill.newListener([this](int& val)
													 { setFilterOptions(); }));
	addEventListener(tempFilterPersistence.newListener([this](int& val)
													   { setFilterOptions(); }));
	addEventListener(tempFilterAlpha.newListener([this](float& val)
												 { setFilterOptions(); }));
	addEventListener(tempFilterDelta.newListener([this](int& val)
												 { setFilterOptions(); }));
	captureSize.setWithoutEventNotifications(glm::ivec2(0, 0));

	this->setThreadName("MTVideoInputSourceRealSense");

}

bool MTVideoInputSourceRealSense::isFrameNew()
{
	if (isFrameAvailable)
	{
		isFrameAvailable = false;
		return true;
	}

	return false;
}

const ofPixels& MTVideoInputSourceRealSense::getPixels()
{
	return pixels;
}

const rs2::points& MTVideoInputSourceRealSense::getPoints()
{
	return points;
}

rs2::frame MTVideoInputSourceRealSense::getRS2Frame()
{
	return rs2Frame;
}

void MTVideoInputSourceRealSense::threadedFunction()
{
	setThreadName(deviceID.get());
	ofLogVerbose("MTVideoInputSourceRealSense") << deviceID.get() << " has ThreadID " << getThreadId();
	while (isThreadRunning())
	{
		{
			std::lock_guard<std::mutex> lck(mutex);

			std::function<void()> function;
			while(threadChannel.tryReceive(function))
			{
				function();
			}

			rs2::frame frame;
			if (postProcessingQueue.poll_for_frame(&frame))
			{ // Wait for next set of frames from the camera

				if (enableDepth)
				{
					rs2::frame filtered = frame
							.as<rs2::depth_frame>(); // Does not copy the frame, only adds a reference

					//	Apply filters.
					// The implemented flow of the filters pipeline is in the following order:
					// 1. apply decimation filter
					// 2. apply threshold filter
					// 3. transform the scene into disparity domain
					// 4. apply spatial filter
					// 5. apply temporal filter
					// 6. revert the results back (if step Disparity filter was applied
					// to depth domain (each post processing block is optional and can be applied independantly).
					if (decFilterOn) filtered = dec_filter.process(filtered);
					if (thrFilterOn) filtered = thr_filter.process(filtered);
					if (useDisparity) filtered = depth_to_disparity.process(filtered);
					if (spatFilterOn) filtered = spat_filter.process(filtered);
					if (tempFilterOn) filtered = temp_filter.process(filtered);
					if (useDisparity) filtered = disparity_to_depth.process(filtered);

					if (outputMode == Output2D)
					{
						if (useColorizer) filtered = colorizer.colorize(filtered);
						outputQueue.enqueue(filtered);
					}
					else if (outputMode == Output3D)
					{
						if (useColorizer)
						{
							colorizedFrame = colorizer.colorize(filtered);
							pointcloud.map_to(colorizedFrame);
						}
						try
						{

							outputQueue.enqueue(pointcloud.calculate(filtered));
						}
						catch (std::runtime_error& error)
						{
							ofLogError("MTVideoInputSourceRealSense") << "Pointcloud error: " << error.what();
						}
					}
					else
					{
						outputQueue.enqueue(filtered);
					}


				}
			}
		}
		yield();
	}
}

void MTVideoInputSourceRealSense::start()
{
	if (!isThreadRunning())
	{

		sensor.start([this](rs2::frame frame)
					 {
						 postProcessingQueue.enqueue(std::move(frame));
					 });
		startThread();
	}
}

void MTVideoInputSourceRealSense::close()
{
	if (isThreadRunning())
	{
		ofLogVerbose("MTVideoInputSourceRealSense") << "Stopping capture thread";
		waitForThread();
		isFrameAvailable = false;
		sensor.stop();
		sensor.close();
		sleep(100);
		rs2::frame frame;
		while (postProcessingQueue.poll_for_frame(&frame))
		{}
		while (outputQueue.poll_for_frame(&frame))
		{}
		ofLogVerbose("MTVideoInputSourceRealSense") << "Stopped";
	}
}

void MTVideoInputSourceRealSense::update()
{
	rs2::frame frame;
	if (outputQueue.poll_for_frame(&frame))
	{
		rs2Frame = frame;
		if (outputMode == Output2D)
		{
			auto vf = frame.as<rs2::video_frame>();
			toOf(vf, pixels);
		}
		else if (outputMode == Output3D)
		{
			points = frame.as<rs2::points>();
		}
//		else
//		{
//			rs2Frame = frame;
//		}

		isFrameAvailable = true;
	}
}

void MTVideoInputSourceRealSense::setup()
{
	setup(captureSize->x, captureSize->y, frameRate, deviceID);
}

void MTVideoInputSourceRealSense::setup(int width, int height, int framerate, std::string deviceID)
{
	if (isDeserializing) return;
	close();
	ofLogVerbose("MTVideoInputSourceRealSense") << "SETUP - " << this->deviceID.get();

	auto profile = getBestProfile(width, height, framerate);
	try
	{
		sensor.open(profile);
		pixels.allocate(profile.width(), profile.height(), ofImageType::OF_IMAGE_COLOR);
		captureSize.setWithoutEventNotifications(glm::ivec2(profile.width(), profile.height()));
		frameRate.setWithoutEventNotifications(profile.fps());
		start();
	}
	catch (std::runtime_error& e)
	{
		ofLogError("MTVideoInputSourceRealSense") << e.what() << " --- Device ID: " << this->deviceID.get();
	}
}

void MTVideoInputSourceRealSense::serialize(ofXml& serializer)
{
	MTModel::serialize(serializer);
}

void MTVideoInputSourceRealSense::deserialize(ofXml& serializer)
{
	MTVideoInputSource::deserialize(serializer);
	setFilterOptions();
}

void MTVideoInputSourceRealSense::setFilterOptions()
{

	threadChannel.send([this]()
					   {
						   dec_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, float(decFilterMagnitude.get()));
						   thr_filter.set_option(RS2_OPTION_MIN_DISTANCE, thrFilterMinDistance.get());
						   thr_filter.set_option(RS2_OPTION_MAX_DISTANCE, thrFilterMaxDistance.get());
						   spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, spatFilterAlpha.get());
						   spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, spatFilterDelta.get());
						   spat_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, float(spatFilterMag.get()));
						   spat_filter.set_option(RS2_OPTION_HOLES_FILL, float(spatFilterHolesFill.get()));
						   temp_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, tempFilterAlpha.get());
						   temp_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, tempFilterDelta.get());
						   temp_filter.set_option(RS2_OPTION_HOLES_FILL, float(tempFilterPersistence.get()));
					   });
}

MTVideoInputSourceRealSense::~MTVideoInputSourceRealSense()
{
	MTVideoInputSourceRealSense::close();
}

std::vector<std::shared_ptr<rs2::device>> MTVideoInputSourceRealSense::GetDevices()
{
	if (Devices.empty())
	{
		auto devList = getRS2Context().query_devices();
		for (auto dev : devList)
		{
			Devices.emplace_back(std::make_shared<rs2::device>(dev));
		}
	}

	return Devices;
}

bool MTVideoInputSourceRealSense::getDeviceWithSerial(const std::string& serial, rs2::device& dev)
{
	// Populate the static devices array:


	if (GetDevices().empty())
	{
		ofLogError("MTVideoInputSourceRealSense") << "No devices found!";
		return false;
	}

	try
	{
		for (const auto& device : GetDevices())
		{
			std::string currentSerial((device)->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
			if (currentSerial == serial)
			{
				dev = *device;
				return true;
			}
		}
	}
	catch (rs2::error& e)
	{
		ofLogError("MTVideoInputSourceRealSense") << e.what();
	}


	ofLogError("MTVideoInputSourceRealSense") << "Could not find device with serial number " << serial;
	dev = *GetDevices().front();
	deviceID.setWithoutEventNotifications(dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
	return false;
}

void MTVideoInputSourceRealSense::getSupportedResolutions(rs2::sensor& sensor)
{
	streamProfiles.clear();

	for (const auto& profile : sensor.get_stream_profiles())
	{
		if (auto vidProf = profile.as<rs2::video_stream_profile>())
		{
			if (vidProf.stream_type() == RS2_STREAM_DEPTH) // Only care about depth format for now
			{
				streamProfiles.push_back(vidProf);
//					 	 ofLogVerbose() << vidProf.width() << " x " << vidProf.height() << " " << vidProf.fps();
			}
		}
	}

	std::sort(streamProfiles.begin(), streamProfiles.end(),
			  [this](const rs2::video_stream_profile& i, const rs2::video_stream_profile& j)
			  {
				  if (i.width() < j.width()) return true;
				  if (i.width() > j.width()) return false;

				  // both widths the same
				  if (i.height() < j.height()) return true;
				  if (i.height() > j.height()) return false;

				  //width and height the same
				  if (i.fps() < j.fps()) return true;
				  if (i.fps() > j.fps()) return false;

				  // We shouldn't be here
				  ofLogVerbose("MTVideoInputSourceRealSense") << "I shouldn't be here!";
				  return false;
			  });
//
//	for (const auto& vidProf : streamProfiles)
//	{
//		ofLogVerbose() << vidProf.width() << " x " << vidProf.height() << " " << vidProf.fps() << " "
//					   << rs2_format_to_string(vidProf.format());
//
//	}
}

std::vector<rs2::video_stream_profile> MTVideoInputSourceRealSense::getStreamProfiles()
{
	return streamProfiles;
}

rs2::video_stream_profile MTVideoInputSourceRealSense::getBestProfile(int width, int height, int fps)
{
	rs2::video_stream_profile bestProfile = streamProfiles.front();
	int bWidth = width;
	int bHeight = height;
	int bestDeltaW = INT_MAX;
	int bestDeltaH = INT_MAX;
	int bestDeltaF = INT_MAX;

	for (const auto& profile : streamProfiles)
	{
		int deltaW = abs(width - profile.width());
		if (deltaW < bestDeltaW)
		{
			bestDeltaW = deltaW;
			bWidth = profile.width();
		}
	}

	for (const auto& profile : streamProfiles)
	{
		if (profile.width() == bWidth)
		{
			int deltaH = abs(height - profile.height());
			if (deltaH < bestDeltaH)
			{
				bestDeltaH = deltaH;
				bHeight = profile.height();
			}
		}
	}

	for (const auto& profile : streamProfiles)
	{
		if (profile.width() == bWidth && profile.height() == bHeight)
		{
			int deltaF = abs(fps - profile.fps());
			if (deltaF < bestDeltaF)
			{
				bestDeltaF = deltaF;
				bHeight = profile.height();
				bestProfile = profile;
			}
		}
	}

	return bestProfile;
}

void MTVideoInputSourceRealSense::createParameterFromOption(const rs2::options& endpoint, rs2_option option,
															ofParameterGroup& parameters)
{

//	 ofLogVerbose() << endpoint.get_option_name(option) << " step: " << endpoint.get_option_range(option).step;

	if (!endpoint.supports(option))
	{
		ofLogVerbose(getTypeName()) << "Sensor does not support option " << rs2_option_to_string(option);
		return;
	}

	auto range = endpoint.get_option_range(option);
	// Bool param:
	if (range.min == 0 && range.max == 1 && range.step == 1)
	{
		ofParameter<bool> bp;
		parameters.add(bp.set(endpoint.get_option_name(option), (range.def == 1.0f)));
		addEventListener(bp.newListener([this, endpoint, option](bool val)
										{
//																				 optionsChannel.send(std::move(std::make_pair(option, val ? 1.0f : 0.0f)));
											SetRS2Option(endpoint, option, val ? 1.0f : 0.0f);
										}));
	}
	else if (range.step == 1)
	{
		ofParameter<int> param;
		parameters.add(param.set(endpoint.get_option_name(option), range.def, range.min, range.max));
		addEventListener(param.newListener([this, endpoint, option](int val)
										   {
											   SetRS2Option(endpoint, option, float(val));
										   }));
	}
	else
	{
		ofParameter<float> param;
		parameters.add(param.set(endpoint.get_option_name(option), range.def, range.min, range.max));
		addEventListener(param.newListener([this, endpoint, option](float val)
										   {
											   SetRS2Option(endpoint, option, val);
										   }));
	}
//	 ofLogVerbose() << endpoint.get_option_name(opt) << " " << endpoint.get_option_description(opt) << " min: "
//									<< range.min << " max: " << range.max << " step: " << range.step << " def: " << range.def;
}

void MTVideoInputSourceRealSense::SetRS2Option(const rs2::options& endpoint, rs2_option option, float val)
{
	try
	{
		if (endpoint.supports(option)) endpoint.set_option(option, val);
	}
		// start with the most specific handlers
	catch (const rs2::camera_disconnected_error& e)
	{
		ofLogError("MTVideoInputSourceRealSense") << "Camera was disconnected! Please connect it back  "
												  << endpoint.get_option_name(option) << " " << e.what();
		// wait for connect event
	}
// continue with more general cases
	catch (const rs2::recoverable_error& e)
	{
		ofLogError("MTVideoInputSourceRealSense") << "Operation failed, please try again "
												  << endpoint.get_option_name(option) << " " << e.what();
	}
// you can also catch "anything else" raised from the library by catching rs2::error
	catch (const rs2::error& e)
	{
		ofLogError("MTVideoInputSourceRealSense") << "Some other error occurred! " << endpoint.get_option_name(option)
												  << " " << e.what();
	}
	catch (const std::exception& exc)
	{
		ofLogError("MTVideoInputSourceRealSense") << "Could not set option " <<
												  endpoint.get_option_name(option) << " " << exc.what();
	}
}

#endif


