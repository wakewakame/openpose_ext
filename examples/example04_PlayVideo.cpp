/*

openpose_ext では画像だけでなく動画の処理も可能です。
そのため、動画の姿勢推定を行うサンプルを用意しました。

*/

#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <Utils/Video.h>
#include <Utils/Preview.h>
#include <Utils/VideoControllerUI.h>
#include <Utils/PlotInfo.h>

int main(int argc, char* argv[])
{
	// MinimumOpenPose の初期化をする
	MinOpenPose openpose(op::PoseModel::BODY_25, op::Point<int>(-1, 368));

	// OpenPose に入力する動画を用意する
	// "media/video.mp4" は入力する動画ファイルのパスを指定する
	Video video;
	video.open("media/video.mp4");

	// 動画をプレビューするためのウィンドウを生成する
	Preview preview("result");

	// 動画のスキップなどができるようにする
	VideoControllerUI videoControllUI;
	videoControllUI.addShortcutKeys(preview, video);  // ショートカットキーの追加

	// 動画が終わるまでループする
	while (true)
	{
		// 動画の次のフレームを読み込む
		cv::Mat image = video.next();

		// 映像が終了した場合はループを抜ける
		if (image.empty()) break;

		// OpenPose で姿勢推定をする
		auto people = openpose.estimate(image);

		// 姿勢推定の結果を image に描画する
		plotBone(image, people, openpose);

		// 画面を更新する
		int ret = preview.preview(image);

		// 再生の操作画面を表示する
		videoControllUI.showUI(video);

		// Escキーが押されたら終了する
		if (0x1b == ret) break;
	}

	return 0;
}