//
// Created by Cristobal Mendoza on 7/17/20.
//

#include "MTVideoInputSourceRealSense.hpp"
#include "librealsense2/hpp/rs_processing.hpp"
#include "librealsense2/hpp/rs_device.hpp"

MTVideoInputSourceRealSense::MTVideoInputSourceRealSense(std::string devID) :
	 MTVideoInputSource("RealSense Camera", "MTVideoInputSourceRealSense", "RealSense Camera", devID)
{
	 pipeline = rs2::pipeline(MTVideoInputSourceRealSense::getRS2Context());
	 device = MTVideoInputSourceRealSense::getDeviceWithSerial(deviceID);
	 frameQueue = rs2::frame_queue();
	 getSupportedResolutions(device);

	 for (auto s : device.query_sensors())
	 {
			if (s.as<rs2::depth_sensor>()) // Only depth sensor for now
			{
				 sensor = s;
				 auto opts = sensor.get_supported_options();
				 for (auto opt : opts)
				 {
						if (sensor.is_option_read_only(opt)) continue;
						createParameterFromOption(opt);
				 }
			}
	 }
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
											 spatFilterHolesFill.set("Hole Filling", 0, 0, 6)
	 );

	 tempFilterGroup.setName("Temporal Filter");
	 tempFilterGroup.add(tempFilterOn.set("Enabled", false),
											 tempFilterPersistence.set("Persistence", 3, 0, 8),
											 tempFilterAlpha.set("Alpha", 0.4f, 0.0f, 1.f),
											 tempFilterDelta.set("Delta", 20, 1, 100)
	 );

	 addParameters(enableDepth.set("Enable Depth", true),
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

cv::Mat MTVideoInputSourceRealSense::getCVPixels()
{
	 return cv::Mat();
}

void MTVideoInputSourceRealSense::threadedFunction()
{
	 while (isThreadRunning())
	 {
			std::unique_lock<std::mutex> lck(mutex);

			std::function<void()> function;
			while (functionsChannel.tryReceive(function))
			{
				 function();
			}

			rs2::frameset data = pipeline.wait_for_frames(); // Wait for next set of frames from the camera
			if (enableDepth)
			{
				 rs2::frame depth_frame = data.get_depth_frame(); //Take the depth frame from the frameset
				 if (!depth_frame) // Should not happen but if the pipeline is configured differently
						return;       //  it might not provide depth and we don't want to crash

				 rs2::frame filtered = depth_frame; // Does not copy the frame, only adds a reference

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

				 if (useDisparity)
				 {
						filtered = disparity_to_depth.process(filtered);
				 }

				 filtered = colorizer.colorize(filtered);
				 frameQueue.enqueue(filtered);
			}

			// Push filtered & original data to their respective queues
			// Note, pushing to two different queues might cause the application to display
			//  original and filtered pointclouds from different depth frames
			//  To make sure they are synchronized you need to push them together or add some
			//  synchronization mechanisms
//			filtered_data.enqueue(filtered);
//			original_data.enqueue(depth_frame);
	 }
}

void MTVideoInputSourceRealSense::start()
{
	 startThread();
}

void MTVideoInputSourceRealSense::close()
{
	 if (isThreadRunning())
	 {
			ofLogVerbose("MTVideoInputSourceRealSense") << "Stopping capture thread";
			waitForThread();
			isFrameAvailable = false;
			ofLogVerbose("MTVideoInputSourceRealSense") << "Stopped";
			pipeline.stop();
	 }
}

void MTVideoInputSourceRealSense::update()
{
	 rs2::frame frame;
	 if (frameQueue.poll_for_frame(&frame))
	 {
			auto vf = frame.as<rs2::video_frame>();
			toOf(vf, pixels);
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
	 rs2::config cfg;
//	 cfg.enable_stream();
	 cfg.enable_device(deviceID);
	 cfg.enable_stream(RS2_STREAM_DEPTH, width, height, RS2_FORMAT_Z16, framerate);
//	 device.query_sensors()[0].set_option() ///
	 auto profile = pipeline.start(cfg);
	 auto frameset = pipeline.wait_for_frames();
	 auto df = frameset.get_depth_frame();
	 pixels.allocate(df.get_width(), df.get_height(), ofImageType::OF_IMAGE_COLOR);
	 captureSize.setWithoutEventNotifications(glm::ivec2(df.get_width(), df.get_height()));
	 frameRate.setWithoutEventNotifications(profile.get_streams().front().fps());
	 start();
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
	 std::unique_lock<std::mutex> lock(mutex);

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
}

MTVideoInputSourceRealSense::~MTVideoInputSourceRealSense()
{
	 close();
}

rs2::device MTVideoInputSourceRealSense::getDeviceWithSerial(std::string serial)
{
	 auto devices = getRS2Context().query_devices();
	 auto found = std::find_if(devices.begin(), devices.end(), [&](const rs2::device device)
	 {
			std::string currentSerial((device).get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
			if (currentSerial == serial)
			{
				 return true;
			}
			return false;
	 });
	 rs2::device dev;
	 if (found != devices.end())
	 {
			dev = *found;
	 }
	 else
	 {
			ofLogError("MTVideoInputSourceRealSense") << "Could not find device with serial number " << serial;
			dev = devices.front();
	 }
	 return dev;
}

void MTVideoInputSourceRealSense::getSupportedResolutions(rs2::device device)
{
	 streamProfiles.clear();
	 for (auto sensor : device.query_sensors())
	 {
			ofLogVerbose("MTVideoInputSourceRealSense") << "Sensor ------- " << sensor.get_info(RS2_CAMERA_INFO_NAME);
			if (strcmp(sensor.get_info(RS2_CAMERA_INFO_NAME), "Stereo Module") ==
					0) // Only care about stereo module right now
			{
				 for (auto profile : sensor.get_stream_profiles())
				 {
						if (auto vidProf = profile.as<rs2::video_stream_profile>())
						{
							 if (vidProf.format() == RS2_FORMAT_Z16) // Only care about depth format for now
							 {
									streamProfiles.push_back(vidProf);
//					 	 ofLogVerbose() << vidProf.width() << " x " << vidProf.height() << " " << vidProf.fps();
							 }
						}
				 }
			}

	 }

	 std::sort(streamProfiles.begin(), streamProfiles.end(),
						 [this](rs2::video_stream_profile i, rs2::video_stream_profile j)
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
								ofLogVerbose("I SHOULDNT BE HERE");
								return false;
						 });

	 for (auto vidProf : streamProfiles)
	 {
			ofLogVerbose() << vidProf.width() << " x " << vidProf.height() << " " << vidProf.fps() << " "
										 << rs2_format_to_string(vidProf.format());

	 }
}

rs2::video_stream_profile MTVideoInputSourceRealSense::getBestProfile(int width, int height, int fps)
{
	 rs2::video_stream_profile best = streamProfiles.front();
	 int bWidth = width;
	 int bHeight = height;
	 int bFps = fps;
	 for (auto sp : streamProfiles)
	 {
	 }

	 return best;
}

void MTVideoInputSourceRealSense::createParameterFromOption(rs2_option option)
{
	 if (sensor.get() == nullptr)
	 {
			ofLogError("MTVideoInputSourceRealSense") << "Tried to create parameters before setting the sensor";
			return;
	 }

//	 ofLogVerbose() << sensor.get_option_name(option) << " step: " << sensor.get_option_range(option).step;

	 auto range = sensor.get_option_range(option);
	 // Bool param:
	 if (range.min == 0 && range.max == 1 && range.step == 1)
	 {
			ofParameter<bool> bp;
			addEventListener(bp.newListener([this, option](bool val)
																			{
				 optionsChannel.send(std::move(std::make_pair(option, val ? 1.0f : 0.0f)));
//																				 functionsChannel.send([this, option, val]()
																				 {
																						try
																						{
																							 sensor.set_option(option, val ? 1.0f : 0.0f);
																						}
																						catch (const std::exception& exc)
																						{
																							 ofLogVerbose("MTVideoInputSourceRealSense") << exc.what();
																						}
//																															 });
																				 }
																			}));
			parameters.add(bp.set(sensor.get_option_name(option), (range.def == 1.0f)));
	 }
	 else if (range.step == 1)
	 {
			ofParameter<int> param;
			addEventListener(param.newListener([this, option](int val)
																				 {
//																				 functionsChannel.send([this, option, val]()
																						{
																							 try
																							 {
																									sensor.set_option(option, float(val));
																							 }
																							 catch (const std::exception& exc)
																							 {
																									ofLogVerbose("MTVideoInputSourceRealSense") << exc.what();
																							 }
//																															 });
																						}
																				 }));
			parameters.add(param.set(sensor.get_option_name(option), range.def, range.min, range.max));
	 }
	 else
	 {
			ofParameter<float> param;
			addEventListener(param.newListener([this, option](float val)
																				 {
//																						functionsChannel.send([this, option, val]()
																						{
																							 try
																							 {
																									sensor.set_option(option, val);
																							 }
																							 catch (const std::exception& exc)
																							 {
																									ofLogVerbose("MTVideoInputSourceRealSense") << exc.what();
																							 }
//																																	});
																						}
																				 }));
			parameters.add(param.set(sensor.get_option_name(option), range.def, range.min, range.max));
	 }
//	 ofLogVerbose() << sensor.get_option_name(opt) << " " << sensor.get_option_description(opt) << " min: "
//									<< range.min << " max: " << range.max << " step: " << range.step << " def: " << range.def;
}



