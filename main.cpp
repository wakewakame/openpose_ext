#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <OpenPoseWrapper/Examples/Video.h>
#include <OpenPoseWrapper/Examples/Preview.h>
#include <OpenPoseWrapper/Examples/PlotInfo.h>
#include <OpenPoseWrapper/Examples/SqlOpenPose.h>
#include <OpenPoseWrapper/Examples/Tracking.h>
#include <OpenPoseWrapper/Examples/PeopleCounter.h>

using People = MinOpenPose::People;
using Person = MinOpenPose::Person;
using Node = MinOpenPose::Node;

int main(int argc, char* argv[])
{
	// 入力する映像ファイルのフルパス
	// 注意 : このプログラムのファイル形式がCP932ではない場合、ファイルパスに日本語が混じっていると上手く動かない可能性がある
	std::string videoPath = R"(media/video.mp4)";
	videoPath = R"(D:\思い出\Dropbox\Dropbox\SDK\openpose\video\58.mp4)";
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
	PlotInfo plot;

	// SQLファイルの読み込み、書き込みを行うクラス
	SqlOpenPose sql;
	sql.open(sqlPath, 300);

	// 骨格をトラッキングするクラス
	Tracking tracker(
		0.5f,  // 関節の信頼値がこの値以下である場合は、関節が存在しないものとして処理する
		5,  // 信頼値がconfidenceThresholdより大きい関節の数がこの値未満である場合は、その人がいないものとして処理する
		10,  // 一度トラッキングが外れた人がこのフレーム数が経過しても再発見されない場合は、消失したものとして処理する
		50.0f  // トラッキング中の人が1フレーム進んだとき、移動距離がこの値よりも大きい場合は同一人物の候補から外す
	);

	// 通行人をカウントするクラス
	PeopleCounter count(200, 250, 500, 250, 100);

	while (true)
	{
		// 動画の次のフレームを読み込む
		cv::Mat frame = video.next();

		// フレーム番号などの情報を取得する
		Video::FrameInfo frameInfo = video.getInfo();

		// フレームがない場合は終了する
		if (frame.empty()) break;

		// SQLに姿勢が記録されていれば、その値を使う
		auto peopleOpt = sql.read(frameInfo.frameNumber);
		People people;
		if (peopleOpt) { people = peopleOpt.value(); }

		// SQLに姿勢が記録されていなければ姿勢推定を行う
		else
		{
			// 姿勢推定
			people = mop.estimate(frame);

			// 結果をSQLに保存
			sql.write(frameInfo.frameNumber, frameInfo.frameTimeStamp, people);
		}

		// トラッキング
		auto tracked_people = tracker.tracking(people, sql, frameInfo.frameNumber).value();

		// 通行人のカウント
		count.update(tracker, frameInfo.frameNumber);

		// 通行人のカウント状況をプレビュー
		count.drawInfo(frame, tracker);

		// 映像の上に骨格を描画
		plot.bone(frame, mop, tracked_people);  // 骨格を描画
		plot.id(frame, tracked_people);  // フレームレートとフレーム番号の描画
		plot.frameInfo(frame, video);  // フレームレートとフレーム番号の描画

		// プレビュー
		int ret = preview.preview(frame, 1);

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