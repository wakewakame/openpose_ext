/*

OpenPose は画像を1枚ずつ単独で処理します。
そのため、動画を処理した場合は同一人物の骨格が今と1フレーム後でIDが振りなおされます。
このサンプルでは、フレーム間で最も移動が少なかった骨格を同一人物とみなしてIDのトラッキングを行います。

*/

#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <Utils/Video.h>
#include <Utils/Preview.h>
#include <Utils/VideoControllerUI.h>
#include <Utils/PlotInfo.h>
#include <Utils/SqlOpenPose.h>
#include <Utils/Tracking.h>

int main(int argc, char* argv[])
{
	// 入力する映像ファイルのフルパス
	std::string videoPath = R"(media/video.mp4)";

	// 入出力する SQL ファイルのフルパス
	std::string sqlPath = videoPath + ".sqlite3";

	// MinimumOpenPose の初期化をする
	MinOpenPose openpose(op::PoseModel::BODY_25, op::Point<int>(-1, 368));

	// OpenPose に入力する動画を用意する
	Video video;
	video.open(videoPath);

	// 動画をプレビューするためのウィンドウを生成する
	Preview preview("result");

	// SQL の読み書きを行うクラスの初期化
	SqlOpenPose sql;
	sql.open(sqlPath, 300);

	// 骨格をトラッキングするクラス
	Tracking tracker(
		0.5f,  // 関節の信頼値がこの値以下である場合は、関節が存在しないものとして処理する
		5,     // 信頼値がconfidenceThresholdより大きい関節の数がこの値未満である場合は、その人がいないものとして処理する
		10,    // 一度トラッキングが外れた人がこのフレーム数が経過しても再発見されない場合は、消失したものとして処理する
		50.0f  // トラッキング中の人が1フレーム進んだとき、移動距離がこの値よりも大きい場合は同一人物の候補から外す
	);

	// 歩行軌跡を描画するクラス
	PlotTrajectory trajectory;

	// 動画が終わるまでループする
	while (true)
	{
		// 動画の次のフレームを読み込む
		cv::Mat image = video.next();

		// 映像が終了した場合はループを抜ける
		if (image.empty()) break;

		// フレーム番号などの情報を取得する
		Video::FrameInfo frameInfo = video.getInfo();

		// SQLに姿勢が記録されていれば、その値を使う
		auto peopleOpt = sql.read(frameInfo.frameNumber);
		MinOpenPose::People people;
		if (peopleOpt)
		{
			people = peopleOpt.value();
		}

		// SQLに姿勢が記録されていなければ姿勢推定を行う
		else
		{
			// 姿勢推定
			people = openpose.estimate(image);

			// 結果を SQL に保存
			sql.write(frameInfo.frameNumber, frameInfo.frameTimeStamp, people);
		}

		// トラッキング
		auto tracked_people = tracker.tracking(people, sql, frameInfo.frameNumber).value();

		// 歩行軌跡を image に描画する
		trajectory.plot(image, tracked_people);

		// 姿勢推定の結果を image に描画する
		plotBone(image, tracked_people, openpose);

		// 人のIDの描画
		plotId(image, tracked_people);  // 人のIDの描画

		// 画面を更新する
		preview.preview(image);
	}

	return 0;
}