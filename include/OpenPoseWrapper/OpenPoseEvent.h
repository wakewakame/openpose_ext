#pragma once

#include <openpose/headers.hpp>

struct ImageInfo
{
	struct Node { float x, y, confidence; };

	// send
	size_t frameNumber;
	cv::Mat inputImage;
	bool needOpenposeProcess = true;

	// recieve
	cv::Mat outputImage;
	std::vector<std::vector<Node>> people;
};

class OpenPoseEvent
{
public:
	OpenPoseEvent() {}
	virtual ~OpenPoseEvent() {}
	virtual int init() { return 0; };
	virtual void exit() {};
	virtual int sendImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) = 0;
	virtual int recieveImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) = 0;
	virtual void recieveErrors(const std::vector<std::string>& errors) {}
	virtual std::pair<op::PoseModel, op::Point<int>> selectOpenposeMode()
	{
		return std::pair<op::PoseModel, op::Point<int>>(
			op::PoseModel::BODY_25, op::Point<int>(-1, 368)
			);
	}
};

class ExampleOpenPoseEvent : public OpenPoseEvent
{
private:
	std::string videoPath;
	cv::VideoCapture cap;
public:
	ExampleOpenPoseEvent(const std::string& videoPath);
	virtual ~ExampleOpenPoseEvent();
	int init() override;
	void exit() override;
	int sendImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override;
	int recieveImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override;
	void recieveErrors(const std::vector<std::string>& errors) override;
	std::pair<op::PoseModel, op::Point<int>> selectOpenposeMode() override;
};