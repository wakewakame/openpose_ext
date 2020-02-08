#pragma once

#include <functional>
#include <vector>
#include <string>

#include <OpenPoseWrapper/OpenPoseEvent.h>

class PreviewOpenPoseEvent : public OpenPoseEvent
{
private:
	std::string windowTitle;
	std::vector<std::function<void(int)>> keyboardEventListener;
	std::vector<std::function<void(int, int, int)>> mouseEventListener;
public:
	PreviewOpenPoseEvent(std::string windowTitle = "result") : windowTitle(windowTitle){}
	virtual ~PreviewOpenPoseEvent() {};
	int init() override final { return 0; }
	void exit() override final {}
	int sendImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final { return 0; }
	int recieveImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		// ウィンドウの表示
		cv::imshow(windowTitle, imageInfo.outputImage);

		// マウスイベントのコールバック関数の指定
		cv::setMouseCallback(windowTitle, [](int event, int x, int y, int flags, void* userdata) {
			// マウスイベントのリスナーの発火
			for (auto&& func : *((std::vector<std::function<void(int, int, int)>>*)userdata)) func(event, x, y);
		}, (void*)(&mouseEventListener));

		// キー入力の取得
		int key = cv::waitKey(1);
		if (key == 0x1b)
		{
			exit();
			return 0;
		}

		// キーイベントのリスナーの発火
		if (key != -1)
		{
			for (auto&& func : keyboardEventListener) func(key);
		}
		return 0;
	}
	void recieveErrors(const std::vector<std::string>& errors) override final
	{
		for (auto&& error : errors)
			std::cout << error << std::endl;
	}
	void addMouseEventListener(const std::function<void(int, int, int)>& func)
	{
		mouseEventListener.push_back(func);
	}
	void addKeyboardEventListener(const std::function<void(int)>& func)
	{
		keyboardEventListener.push_back(func);
	}
};