#pragma once

#include <OpenPoseWrapper/MinimumOpenPose.h>

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
	// ウィンドウ上のマウス座標
	cv::Point mouse;

	Preview(const std::string windowTitle = "result") : windowTitle(windowTitle), mouse(0, 0)
	{
		// マウス座標を更新
		addMouseEventListener([&](int event, int x, int y) {
			if (event == cv::EVENT_MOUSEMOVE) { mouse.x = x; mouse.y = y; }
		});
	}
	
	virtual ~Preview() {};

	/**
	 * 指定された画像をウィンドウとして表示する
	 * @param input 表示する画像
	 * @param delay ミリ秒単位の待機時間 (0を指定するとキー入力があるまで停止する)
	 * @withoutWaitKey trueにするとdelayによる待機時間が0秒になるが、addKeyboardEventListener関数の効果がなくなる
	 * @return 最後に入力されたキー番号が帰る
	 * @note
	 * OpenCVの仕様上、複数のウィンドウが生成された状態でキー入力をしても、どのウィンドウに対しての操作であるかを特定できない。
	 * また、複数個のPreviewクラスを生成し、それらすべてwithoutWaitKeyをtrueに設定すると、キーイベントがどのウィンドウに対して送信されるかは予想できない。
	 * そのため、1つのウィンドウのみwithoutWaitKeyをtrueに設定し、それ以外のウィンドウをfalseに設定することで一時的にこの問題を回避できる。
	 */
	int preview(const cv::Mat& input, uint32_t delay = 1, bool withoutWaitKey = false)
	{
		// ウィンドウの表示
		cv::imshow(windowTitle, input);

		// マウスイベントのコールバック関数の指定
		cv::setMouseCallback(windowTitle, [](int event, int x, int y, int flags, void* userdata) {
			// マウスイベントのリスナーの発火
			for (auto&& func : *((std::vector<std::function<void(int, int, int)>>*)userdata)) func(event, x, y);
		}, (void*)(&mouseEventListener));

		if (withoutWaitKey) return 0;

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

	// 左クリック時のコールバック関数登録
	void onClick(const std::function<void(int, int)>& func)
	{
		addMouseEventListener([func](int event, int x, int y) {
			if (event == cv::EVENT_LBUTTONDOWN) { func(x, y); }
		});
	}
};