/*
このサンプルでは、画面上に引いた直線の上を何人の人がどの方向に移動したかをカウントします。
example08_CountLine.cppとの違いは入力に動画ではなくWebカメラを使用している点です。
解析結果は openpose_ext/build/bin/media の中に記録開始時刻のファイル名で保存されます。
*/

#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <Utils/Video.h>
#include <Utils/Preview.h>
#include <Utils/VideoControllerUI.h>
#include <Utils/PlotInfo.h>
#include <Utils/SqlOpenPose.h>
#include <Utils/Tracking.h>
#include <Utils/PeopleCounter.h>
#include <time.h>

int main(int argc, char* argv[])
{
	// webカメラを開く
	cv::VideoCapture webcam(0);
	if (!webcam.isOpened()) {
		std::cout << "failed to open the web camera" << std::endl;
		return 0;
	}

	// 現在時刻を取得する (ファイル名に使用する)
	time_t timer = time(NULL); tm ptm;
	localtime_s(&ptm, &timer);
	char time_c_str[256] = { '\0' }; strftime(time_c_str, sizeof(time_c_str), "%Y-%m-%d_%H-%M-%S", &ptm);
	std::string time(time_c_str);
	std::cout << time << std::endl;

	// 出力する SQL ファイルのフルパスを録画開始時刻にする
	std::string sqlPath = R"(media/webcam_)" + time + R"(.sqlite3)";

	// OpenPose の初期化をする
	MinOpenPose openpose(op::PoseModel::BODY_25, op::Point<int>(-1, 368));

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

	// 通行人をカウントするクラス
	PeopleCounter count(
		250, 0,    // 直線の始点座標 (X, Y)
		250, 500,  // 直線の終点座標 (X, Y)
		10         // 直線の太さ
	);

	// 動画が終わるまでループする
	uint64_t frameNumber = 0;
	while (true)
	{
		// 動画の次のフレームを読み込む
		cv::Mat image;
		if (!webcam.read(image)) { break; }

		// 映像が終了した場合はループを抜ける
		if (image.empty()) break;

		// 姿勢推定
		MinOpenPose::People people = openpose.estimate(image);

		// 結果を SQL に保存
		sql.writeBones(frameNumber, frameNumber, people);

		// トラッキング
		auto tracked_people = tracker.tracking(people, sql, frameNumber).value();

		// 通行人のカウント
		count.update(tracker, frameNumber);

		// 通行人のカウント状況をプレビュー
		count.drawInfo(image, tracker);

		// 姿勢推定の結果を image に描画する
		plotBone(image, tracked_people, openpose);

		// 人のIDの描画
		plotId(image, tracked_people);  // 人のIDの描画

		// 画面を更新する
		int ret = preview.preview(image);

		// Escキーが押されたら終了する
		if (0x1b == ret) break;

		frameNumber += 1;
	}

	return 0;
}