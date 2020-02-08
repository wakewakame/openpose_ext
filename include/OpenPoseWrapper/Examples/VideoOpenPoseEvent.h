#pragma once

#include <OpenPoseWrapper/OpenPoseEvent.h>

class VideoOpenPoseEvent : public OpenPoseEvent
{
private:
	std::string videoPath;
	cv::VideoCapture videoCapture;
	bool play_, needUpdate;
public:
	VideoOpenPoseEvent(const std::string& videoPath) : videoPath(videoPath), play_(true), needUpdate(false) {}
	virtual ~VideoOpenPoseEvent() {};
	int init() override final
	{
		videoCapture.open(videoPath);
		if (!videoCapture.isOpened())
		{
			std::cout << "can not open \"" << videoPath << "\"" << std::endl;
			return 1;
		}
		return 0;
	}
	void exit() override final
	{
		videoCapture.release();
	}
	int sendImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		if (!videoCapture.isOpened())
		{
			std::cout << "can not open \"" << videoPath << "\"" << std::endl;
			return 1;
		}
		if (play_ || needUpdate)
		{
			imageInfo.needOpenposeProcess = true;
			imageInfo.frameNumber = (size_t)videoCapture.get(cv::CAP_PROP_POS_FRAMES);
			imageInfo.frameSum = (size_t)videoCapture.get(cv::CAP_PROP_FRAME_COUNT);
			imageInfo.frameTimeStamp = (size_t)videoCapture.get(cv::CAP_PROP_POS_MSEC);
			videoCapture.read(imageInfo.inputImage);
			if (imageInfo.inputImage.empty()) exit();
			needUpdate = false;
		}
		else
		{
			imageInfo.needOpenposeProcess = false;
		}
		return 0;
	}
	int recieveImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final { return 0; }
	void play() { play_ = true; }
	void pause() { play_ = false; }
	bool isPlay() { return (videoCapture.isOpened() && (play_)); }
	void seekAbsolute(long long frame)
	{
		if (videoCapture.isOpened())
		{
			size_t frameSum = (size_t)videoCapture.get(cv::CAP_PROP_FRAME_COUNT);
			if (frame < 0) frame = 0;
			if (frame >= frameSum) frame = frameSum - 1;
			videoCapture.set(CV_CAP_PROP_POS_FRAMES, (double)frame);
			needUpdate = true;
		}
	}
	void seekRelative(long long frame)
	{
		if (videoCapture.isOpened())
		{
			long long frameNumber = (size_t)videoCapture.get(cv::CAP_PROP_POS_FRAMES);
			seekAbsolute(frameNumber + frame);
		}
	}
};