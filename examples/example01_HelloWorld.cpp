/*

プログラム中に出てくる MinimumOpenPose は OpenPose をよりシンプルに扱えるようにしたライブラリです。
以下のサンプルプログラムは MinimumOpenPose を用いた最小限のプログラムです。

また、画像の操作はおおよそ OpenCV で行っています。
頭に cv:: と付いている関数名や変数名は OpenCV で定義されているものです。

*/

#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <Utils/PlotInfo.h>

int main(int argc, char* argv[])
{
	// MinimumOpenPose の初期化
	MinOpenPose openpose(op::PoseModel::BODY_25, op::Point<int>(-1, 368));

	// OpenPose に入力する画像を用意する
	cv::Mat image = cv::imread("media/human.jpg");

	// OpenPose で姿勢推定をする
	auto people = openpose.estimate(image);

	// 姿勢推定の結果を image に描画する
	plotBone(image, people, openpose);

	// できあがった画像を表示する
	cv::imshow("result", image);

	// キー入力があるまで待機する
	cv::waitKey(0);

	return 0;
}