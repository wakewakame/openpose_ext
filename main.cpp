#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <OpenPoseWrapper/Examples/Video.h>
#include <OpenPoseWrapper/Examples/Preview.h>
#include <OpenPoseWrapper/Examples/PlotInfo.h>
//#include <OpenPoseWrapper/Examples/SqlOpenPoseEvent.h>
//#include <OpenPoseWrapper/Examples/TrackingOpenPoseEvent.h>
//#include <OpenPoseWrapper/Examples/PeopleCounterOpenPoseEvent.h>

int main(int argc, char* argv[])
{
	// 入力する映像ファイルのフルパス
	std::string videoPath = R"(media/video.mp4)";
	if (argc == 2) videoPath = argv[1];

	// 入出力するsqlファイルのフルパス
	std::string sqlPath = videoPath + ".sqlite3";

	// openposeのラッパークラス
	MinOpenPose mop;

	// 動画を読み込むクラス
	Video video;
	video.open(videoPath);

	// プレビューウィンドウを生成するクラス
	Preview preview("result");

	// 骨格などを表示するクラス
	PlotInfo plot(true, true, true);

	while (true)
	{
		// 動画の次のフレームを読み込む
		cv::Mat frame = video.next();

		// フレームがない場合は終了する
		if (frame.empty()) break;

		// 姿勢推定
		auto people = mop.estimate(frame);

		// 映像の上に骨格を描画
		plot.plotBone(frame, mop, people);
		plot.plotFrameInfo(frame, video);

		// プレビュー
		int ret = preview.preview(frame);

		// Escキーが押されたら終了する
		if (0x1b == ret) break;
	}

	/*
	// SQL入出力機能の追加
	auto sql = mop.addEventListener<SqlOpenPoseEvent>(sqlPath, 300);

	// 動画読み込み処理の追加
	auto video = mop.addEventListener<VideoOpenPoseEvent>(videoPath);

	// 骨格のトラッキング処理の追加
	auto tracker = mop.addEventListener<TrackingOpenPoseEvent>(sql);

	// 人数カウント処理の追加
	(void)mop.addEventListener<PeopleCounterOpenPoseEvent>(tracker, 579, 578, 1429, 577, 100.0, true);

	// 出力画像に骨格情報などを描画する処理の追加
	(void)mop.addEventListener<PlotInfoOpenPoseEvent>(true, true, false);

	// 自分で定義したイベントリスナーの登録
	auto custom = mop.addEventListener<CustomOpenPoseEvent>();

	// 出力画像のプレビューウィンドウを生成する処理の追加
	std::shared_ptr<PreviewOpenPoseEvent> preview;
	preview = mop.addEventListener<PreviewOpenPoseEvent>("result");

	custom->setParams(video, tracker, preview);
	*/

	return 0;
}