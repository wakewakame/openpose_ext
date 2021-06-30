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
#include <thread>

int main(int argc, char* argv[])
{
	/*
	example10_CountLineWebcamをコマンドラインから呼ぶときのメモ

	コマンドライン引数の説明
			example10_CountLineWebcam <rtmpのURL> <直線の開始X座標> <直線の開始Y座標><直線の終了X座標> <直線の終了Y座標> <直線の太さ>

			例: example10_CountLineWebcam "rtmp://10.0.0.1/live/guest001" 0 240 640 240 5

	なお、直線の指定を省略するとデフォルトの値が使用される
	rtmpのURLを省略すると、USB接続されているWebカメラが使用される
	*/

	cv::VideoCapture webcam;
	int startX = 0, startY = 240, endX = 1920, endY = 240, lineWeight = 0;
	if (argc >= 2) {
		// コマンドライン引数の第1引数にカメラのURLを指定できる
		webcam.open(argv[1]);
		if (argc >= 7) {
			// コマンドライン引数の第2引数: 直線の開始地点のx座標
			// コマンドライン引数の第3引数: 直線の開始地点のy座標
			// コマンドライン引数の第4引数: 直線の終了地点のx座標
			// コマンドライン引数の第5引数: 直線の終了地点のy座標
			startX = atoi(argv[2]);
			startY = atoi(argv[3]);
			endX =   atoi(argv[4]);
			endY =   atoi(argv[5]);
			lineWeight = atoi(argv[6]);
		}
	}
	else {
		webcam.open(0);
		/*
		Windowsで起動が失敗する場合
			webcam.open(0);
		の行を
			webcam.open(cv::CAP_DSHOW + 0);
		に書き換えると治るかもしれないです。
		*/
	}

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
		150.0f  // トラッキング中の人が1フレーム進んだとき、移動距離がこの値よりも大きい場合は同一人物の候補から外す
	);

	// 通行人をカウントするクラス
	PeopleCounter count(
		startX, startY,    // 直線の始点座標 (X, Y)
		endX  , endY  ,  // 直線の終点座標 (X, Y)
		lineWeight         // 直線の太さ
	);

	cv::Mat image, image2;
	bool exitFlag = false;
	// 動画の次のフレームを読み込む
	if (!webcam.read(image2)) return 0;
	// 別スレッドで動画を読み込む
	std::mutex mtx;
	std::thread th([&]() {
		while (true) {
			std::scoped_lock{ mtx };
			if (exitFlag) break;
			if (!webcam.read(image2)) break;
			if (image2.empty()) break;
		}
	});

	// 動画が終わるまでループする
	uint64_t frameNumber = 0;
	while (true)
	{
		// 別スレッドで読み込んだ動画をコピー
		{
			std::scoped_lock{ mtx };
			image = image2.clone();
		}

		// 映像が終了した場合はループを抜ける
		if (image.empty()) break;

		// 姿勢推定
		MinOpenPose::People people = openpose.estimate(image);

		// トラッキング
		auto tracked_people = tracker.tracking(people, sql, frameNumber).value();

		// 通行人のカウント
		count.update(tracker, frameNumber);

		// 通行人のカウント状況をプレビュー
		count.drawInfo(image, tracker);

		// 姿勢推定の結果を image に描画する
		plotBone(image, tracked_people, openpose);


		cv::resize(image, image, cv::Size(640, 480) );

		// 画面を更新する
		int ret = preview.preview(image);

		// Escキーが押されたら終了する
		if (0x1b == ret) break;

		frameNumber += 1;
	}

	// スレッドの終了フラグを立てる
	{
		std::scoped_lock{ mtx };
		exitFlag = true;
	}

	// スレッドの終了を待つ
	th.join();

	return 0;
}
