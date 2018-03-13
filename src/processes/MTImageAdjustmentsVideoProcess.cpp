//
// Created by Cristobal Mendoza on 3/6/18.
//

#include <MTVideoProcessStream.hpp>
#include "MTImageAdjustmentsVideoProcess.hpp"

#pragma mark Process

MTImageAdjustmentsVideoProcess::MTImageAdjustmentsVideoProcess() : MTVideoProcess("Image Adjustments",
																				  "MTImageAdjustmentsVideoProcess")
{
	gammaLUT = cv::Mat(1, 256, CV_8U);
	bcLUT = cv::Mat(1, 256, CV_8U);
	updateGammaLUT();
	updateBC();
	updateCLAHE();

	parameters.add(useHistogramEqualization.set("Auto Levels", false),
				   useCLAHE.set("Adaptive Auto Levels", false),
				   claheClipLimit.set("Adaptive Clip Limit", 10, 1, 30),
				   gamma.set("Gamma", 1, 0, 2),
				   brightness.set("Brightness", 0, -1, 1),
				   contrast.set("Contrast", 0, -1, 1));
	addEventListener(gamma.newListener([this](float args)
									   {
										   gammaNeedsUpdate = true;
									   }));
	addEventListener(brightness.newListener([this](float args)
											{
												bcNeedsUpdate = true;
											}));
	addEventListener(contrast.newListener([this](float args)
										  {
											  bcNeedsUpdate = true;
										  }));
	addEventListener(useCLAHE.newListener([this](bool args)
										  {
											  if (args)
											  {
												  claheNeedsUpdate = true;
												  useHistogramEqualization.setWithoutEventNotifications(false);
											  }
										  }));
	addEventListener(useHistogramEqualization.newListener([this](bool val)
														  {
															  if (val)
															  {
																  useCLAHE.setWithoutEventNotifications(false);
															  }
														  }));
	addEventListener(claheClipLimit.newListener([this](float args)
												{
													claheNeedsUpdate = true;
												}));

}

void MTImageAdjustmentsVideoProcess::setup()
{
	MTVideoProcess::setup();
}

MTProcessData& MTImageAdjustmentsVideoProcess::process(MTProcessData& processData)
{
	bool flagChanged = false;

	if (processData.processStream.type() != CV_8UC1)
	{
		cv::cvtColor(processData.processStream, processBuffer, CV_RGB2GRAY);
		processData.processStream = processBuffer;
		flagChanged = true;
	}


	if (useHistogramEqualization)
	{
		cv::equalizeHist(processData.processStream, processBuffer);
		processData.processStream = processBuffer;
	}
	else if (useCLAHE)
	{
		// For thread safety clahe needs to be updated in this thread
		if (claheNeedsUpdate)
		{
			updateCLAHE();
			claheNeedsUpdate = false;
		}
		clahe->apply(processData.processStream, processBuffer);
		processData.processStream = processBuffer;
		flagChanged = true;
	}


	if (gamma != 1)
	{
		if (gammaNeedsUpdate)
		{
			updateGammaLUT();
			gammaNeedsUpdate = false;
		}
		cv::LUT(processData.processStream, gammaLUT, processBuffer);
		flagChanged = true;
	}

	if (brightness != 0 || contrast != 0)
	{
		if (bcNeedsUpdate)
		{
			updateBC();
			bcNeedsUpdate = false;
		}
		cv::LUT(processBuffer, bcLUT, processBuffer);
		flagChanged = true;
	}

	if (flagChanged)
	{
		processOutput = processBuffer;
		processData.processStream = processOutput;
		processData.processResult = processOutput;
	}
	else
	{
		processOutput = processData.processStream;
	}
	return processData;
}

std::unique_ptr<MTVideoProcessUI> MTImageAdjustmentsVideoProcess::createUI()
{
	auto vp = shared_from_this();
	videoProcessUI = std::make_unique<MTImageAdjustmentsVideoProcessUI>(vp, OF_IMAGE_GRAYSCALE);
	return std::move(videoProcessUI);
}

void MTImageAdjustmentsVideoProcess::updateGammaLUT()
{
	float realGamma = std::pow(gamma, 3);
	uchar* p = gammaLUT.ptr();
	for (int i = 0; i < 256; ++i)
	{
		p[i] = cv::saturate_cast<uchar>(std::pow(i / 255.0, realGamma) * 255.0);
	}
}

void MTImageAdjustmentsVideoProcess::updateBC()
{
	float realBrightness = brightness * 60;
	float realContrast = (contrast + 1);
	uchar* p = bcLUT.ptr();
	for (int i = 0; i < 256; ++i)
	{
		p[i] = cv::saturate_cast<uchar>((realContrast * i) + realBrightness);
	}
}

void MTImageAdjustmentsVideoProcess::updateCLAHE()
{
	clahe = cv::createCLAHE(claheClipLimit, cv::Size(8, 8));
}


#pragma mark UI

MTImageAdjustmentsVideoProcessUI::MTImageAdjustmentsVideoProcessUI(const std::shared_ptr<MTVideoProcess>& videoProcess,
																   ofImageType imageType) :
		MTVideoProcessUIWithImage(videoProcess, imageType)
{

}

void MTImageAdjustmentsVideoProcessUI::draw(ofxImGui::Settings& settings)
{
	auto vp = videoProcess.lock();
	ofxImGui::BeginTree(vp->getName(), settings);
	MTVideoProcessUIWithImage::draw(settings);
	for (auto& param : vp->getParameters())
	{
		ofxImGui::AddParameter(param);
	}
//	MTVideoProcessUI::draw(settings);
	ofxImGui::EndTree(settings);
}
