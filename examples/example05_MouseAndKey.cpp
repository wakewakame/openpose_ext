/*

プログラムの実行中にマウスやキーボードなどを使って画面を操作するサンプルを用意しました。
マウスを移動することで画面に絵を描くことができます。
また、スペースキーで画面をリセットできます。

*/

#include <Utils/Preview.h>

int main(int argc, char* argv[])
{
	// 画像をプレビューするためのウィンドウを生成する
	Preview preview("result");

	// 500x500の白色の画像を生成する
	cv::Mat image = cv::Mat(500, 500, CV_8UC3, { 255, 255, 255 });

	// ラムダ式を用いてマウス操作イベントリスナーの登録ができる
	preview.addMouseEventListener([&](int event, int x, int y) {
		// マウスが動いたとき
		if (cv::EVENT_MOUSEMOVE == event)
		{
			// マウスの位置に円を描く
			cv::circle(image, { x, y }, 2, { 0, 0, 0 }, -1);
		}

		// 左クリックが押されたとき
		if (cv::EVENT_LBUTTONDOWN == event)
		{
			// 何もしない
		}

		// 右クリックが押されたとき
		if (cv::EVENT_RBUTTONDOWN == event)
		{
			// 何もしない
		}

		// OpenCV のマウスイベントは他にもあるが、ここでは割愛する
	});

	// ラムダ式を用いてキーボード操作イベントリスナーの登録ができる
	preview.addKeyboardEventListener([&](int key) {
		// スペースキーが押されたとき
		if (32 == key)
		{
			// 画面をリセットする
			image = cv::Mat(500, 500, CV_8UC3, { 255, 255, 255 });
		}

		// Aキーが押されたとき
		if ('a' == key)
		{
			// 何もしない
		}

		// Bキーが押されたとき
		if ('b' == key)
		{
			// 何もしない
		}

		// OpenCV のキーイベントは他にもあるが、ここでは割愛する
	});

	// Escが押されるまで無限ループする
	while (true) {
		// 画面を更新する
		int key = preview.preview(image, 33);

		// Escが押されたらループを抜ける
		if (0x1b == key) break;
	}

	return 0;
}