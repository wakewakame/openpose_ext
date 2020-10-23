#pragma once

#include <functional>
#include <vector>
#include <string>

class Preview
{
private:
	const std::string windowTitle;
	std::vector<std::function<void(int)>> keyboardEventListener;
	std::vector<std::function<void(int, int, int)>> mouseEventListener;

public:
	Preview(const std::string windowTitle = "result") : windowTitle(windowTitle){}
	virtual ~Preview() {};

	int preview(const cv::Mat& input, uint32_t delay = 1)
	{
		// ウィンドウの表示
		cv::imshow(windowTitle, input);

		// マウスイベントのコールバック関数の指定
		cv::setMouseCallback(windowTitle, [](int event, int x, int y, int flags, void* userdata) {
			// マウスイベントのリスナーの発火
			for (auto&& func : *((std::vector<std::function<void(int, int, int)>>*)userdata)) func(event, x, y);
		}, (void*)(&mouseEventListener));

		// キー入力の取得
		int key = cv::waitKey(delay);

		// キーイベントのリスナーの発火
		if (key != -1)
		{
			for (auto&& func : keyboardEventListener) func(key);
		}

		return key;
	}
	
	// マウスイベントのコールバック関数登録
	void addMouseEventListener(const std::function<void(int, int, int)>& func)
	{
		mouseEventListener.push_back(func);
	}

	// キーイベントのコールバック関数登録
	void addKeyboardEventListener(const std::function<void(int)>& func)
	{
		keyboardEventListener.push_back(func);
	}
};