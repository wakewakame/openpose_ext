/*

ここでは、地面を上から見たように画像を変換するプログラムを紹介します。
このプログラムは OpenPose で歩行軌跡を求めた際に、地面を上から見たような軌跡に変換したいときなどに役立ちます。

*/

#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <Utils/Vector.h>

int main(int argc, char* argv[])
{
	// 入力画像
	cv::Mat before = cv::imread("media/checker.png");

	// 射影変換をするクラス
	vt::ScreenToGround screenToGround;

	// カメラの歪みを補正する設定
	screenToGround.setCalibration(
		// カメラキャリブレーションを行った時のカメラの解像度, 出力画像の拡大率
		1920, 1080, 0.5,
		// カメラ内部パラメータの焦点距離と中心座標(fx, fy, cx, cy)
		1222.78852772764, 1214.377234799321, 967.8020317677116, 569.3667691760459,
		// カメラの歪み係数(k1, k2, k3, k4)
		-0.08809225804249926, 0.03839093574614055, -0.060501971675431955, 0.033162385302275665
	);

	// カメラの映像を、地面を上から見たような映像に射影変換する
	screenToGround.setParams(
		// カメラの解像度
		1280, 960,
		// カメラに写っている地面の任意の4点 (左上、右上、右下、左下)
		461, 334,
		1001, 243,
		1056, 669,
		348, 656,
		// 上記の4点のうち、1点目から2点目までの長さと、2点目から3点目までの長さ (単位は任意)
		100.0, 100.0
	);

	// 映像を上から見たように射影変換
	cv::Mat after = screenToGround.translateMat(before, 0.3f, true);

	// 画面上の任意の点を地面上のメートル単位での座標に変換する
	auto point = screenToGround.translate({ 1001, 243 });
	std::cout
		<< "x: " << point.x << " (%), "
		<< "y: " << point.y << " (%)"
		<< std::endl;

	// できあがった画像を表示する
	cv::imshow("before", before);
	cv::imshow("after", after);

	// キー入力があるまで待機する
	cv::waitKey(0);

	return 0;
}