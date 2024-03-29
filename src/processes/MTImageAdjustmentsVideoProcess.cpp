//
// Created by Cristobal Mendoza on 3/6/18.
//

#include <MTVideoInputStream.hpp>
#include "MTImageAdjustmentsVideoProcess.hpp"
#include "MTAppFrameworkUtils.hpp"
#include <opencv2/photo.hpp>
#include "opencv2/imgproc.hpp"

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
				   contrast.set("Contrast", 0, -1, 1),
				   denoise.set("Denoise", false));
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

void MTImageAdjustmentsVideoProcess::process(MTProcessData& processData)
{
	bool flagChanged = false;

	if (processData.processStream.type() != CV_8UC1)
	{
		cv::cvtColor(processData.processStream, processBuffer, cv::COLOR_RGB2GRAY);
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

	if (denoise)
	{
//		cv::fastNlMeansDenoising(processBuffer, processBuffer);
		cv::GaussianBlur(processBuffer, processBuffer, cv::Size(7, 7), 0);
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
		processData.processResult = processOutput;
	}
}

std::shared_ptr<MTVideoProcessUI> MTImageAdjustmentsVideoProcess::createUI()
{
	return std::make_shared<MTImageAdjustmentsVideoProcessUI>(shared_from_this(), OF_IMAGE_GRAYSCALE);
}

void MTImageAdjustmentsVideoProcess::updateGammaLUT()
{
	float realGamma = std::pow(gamma, 3.0f);
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

//void MTImageAdjustmentsVideoProcessUI::draw(ofxImGui::Settings& settings)
//{
//	auto vp = videoProcess.lock();
//	MTVideoProcessUIWithImage::draw(settings);
////	MTVideoProcessUI::draw(settings);
//}
