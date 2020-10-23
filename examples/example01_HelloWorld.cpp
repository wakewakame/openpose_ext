/*

openpose_ext は OpenPose をよりシンプルに扱えるようにしたライブラリです。
このライブラリでは、次のような流れでプログラムを実行します。

	1. OpenPose に画像を入力する
	2. OpenPose から骨格データが返される

また、画像の操作はおおよそ OpenCV で行っています。
頭に cv:: と付いている関数名や変数名は OpenCV です。

以下のサンプルプログラムは openpose_ext を用いた最小限のプログラムです。

*/

#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <Utils/PlotInfo.h>

int main(int argc, char* argv[])
{
	// OpenPose の初期化をする
	MinOpenPose mop(op::PoseModel::BODY_25, op::Point<int>(-1, 368));

	// OpenPose に入力する画像を用意する
	// "media/human.jpg" は入力する画像ファイルのパスを指定する
	cv::Mat image = cv::imread("media/human.jpg");

	// OpenPose で姿勢推定をする
	auto people = mop.estimate(image);

	// 姿勢推定の結果を image に描画する
	plotBone(image, people, mop);

	// できあがった画像を表示する
	cv::imshow("result", image);

	// キー入力があるまで待機する
	cv::waitKey(0);

	return 0;
}