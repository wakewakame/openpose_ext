#pragma once

#include <OpenPoseWrapper/OpenPoseEvent.h>

class PreviewOpenPoseEvent : public OpenPoseEvent
{
private:
	std::string windowTitle;
public:
	PreviewOpenPoseEvent(std::string windowTitle = "result") : windowTitle(windowTitle){}
	virtual ~PreviewOpenPoseEvent() {};
	int init() override final { return 0; }
	void exit() override final {}
	int sendImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final { return 0; }
	int recieveImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		for (auto person = imageInfo.people.begin(); person != imageInfo.people.end(); person++)
		{
			for (auto node : person->second)
			{
				if (node.confidence != 0.0f)
				{
					cv::circle(
						imageInfo.outputImage,
						cv::Point{ (int)node.x, (int)node.y },
						3, cv::Scalar{ 255, 0, 0 }, -1
					);
				}
			}
		}

		cv::imshow(windowTitle, imageInfo.outputImage);
		if (cv::waitKey(1) == 0x1b) exit();
		return 0;
	}
	void recieveErrors(const std::vector<std::string>& errors) override final
	{
		for (auto error : errors)
			std::cout << error << std::endl;
	}
};