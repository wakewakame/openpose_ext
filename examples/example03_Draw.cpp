/*

openpose_ext では画像を OpenCV の cv::Mat を用いて処理をしています。
この画像に文字や図形を書き込みたい場合が出てくると思うので、図形を表示サンプルを用意しました。

*/

#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <Utils/Gui.h>

int main(int argc, char* argv[])
{
	// 500x500の白色の画像を生成する
	cv::Mat image = cv::Mat(500, 500, CV_8UC3, { 255, 255, 255 });

	// "Hello"という文字を(100, 50)の位置に左上を原点として0.7のサイズで赤色で表示する
	gui::text(image, "Hello", { 100, 50 }, gui::LEFT_TOP, 0.7f, { 0, 0, 255 });

	// "World"という文字を(200, 70)の位置に左上を原点として2.0のサイズで青色で表示する
	gui::text(image, "World", { 200, 70 }, gui::LEFT_TOP, 2.0f, { 255, 0, 0 });

	// 円形
	cv::circle(image, { 250, 250 }, 100, { 0, 255, 0 }, -1);
	cv::circle(image, { 270, 270 }, 100, { 100, 100, 100 }, 5);

	// 矩形
	cv::rectangle(image, { 350, 400, 100, 200 }, { 0, 0, 255 }, 10);

	// 直線
	cv::line(image, { 20, 700 }, { 300, 300 }, { 255, 0, 0 }, 6);

	// できあがった画像を表示する
	cv::imshow("result", image);

	// キー入力があるまで待機する
	cv::waitKey(0);

	return 0;
}