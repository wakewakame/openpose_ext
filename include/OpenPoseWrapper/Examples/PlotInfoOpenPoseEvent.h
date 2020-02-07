#pragma once

#include <OpenPoseWrapper/OpenPoseEvent.h>
#include <Utils/Gui.h>

#include <chrono>
#include <string>

class PlotInfoOpenPoseEvent : public OpenPoseEvent
{
private:
	using clock = std::chrono::high_resolution_clock;
	clock::time_point start, end;
	bool displayInfomation, displayJoints, displayId;

public:
	PlotInfoOpenPoseEvent(bool displayInfomation, bool displayJoints, bool displayId) : 
		displayInfomation(displayInfomation), displayJoints(displayJoints), displayId(displayId) {}
	virtual ~PlotInfoOpenPoseEvent() {};
	int init() override final {
		start = clock::now();
		return 0;
	}
	void exit() override final {}
	int sendImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final { return 0; }
	int recieveImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		// 骨格とidの表示
		for (auto person = imageInfo.people.begin(); person != imageInfo.people.end(); person++)
		{
			cv::Point p; size_t enableNodeSum = 0;
			for (auto node : person->second)
			{
				if (node.confidence != 0.0f)
				{
					if (displayJoints)
					{
						cv::circle(
							imageInfo.outputImage,
							cv::Point{ (int)node.x, (int)node.y },
							3, cv::Scalar{ 255, 0, 0 }, -1
						);
					}
					p.x += (int)node.x; p.y += (int)node.y; enableNodeSum++;
				}
			}
			p.x /= enableNodeSum; p.y /= enableNodeSum;

			if (displayId) gui::text(imageInfo.outputImage, std::to_string(person->first), p, gui::CENTER_CENTER, 0.5);
		}

		if (displayInfomation)
		{
			// fpsの測定
			end = clock::now();
			auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
			float fps = 1000.0f / (float)time;
			start = end;

			// fpsと動画の再生時間、フレーム番号の表示
			cv::Size ret{ 0, 0 }; int height = 20;
			ret = gui::text(imageInfo.outputImage, "fps : " + std::to_string(fps), cv::Point{ 20, height }); height += ret.height + 10;
			ret = gui::text(imageInfo.outputImage, "time : " + std::to_string(imageInfo.frameTimeStamp), cv::Point{ 20, height }); height += ret.height + 10;
			ret = gui::text(imageInfo.outputImage, "frame : " + std::to_string(imageInfo.frameNumber) + " / " + std::to_string(imageInfo.frameSum), cv::Point{ 20, height }); height += ret.height + 10;
		}

		return 0;
	}
	void recieveErrors(const std::vector<std::string>& errors) override final
	{
		for (auto error : errors)
			std::cout << error << std::endl;
	}
};