#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <Utils/Video.h>
#include <Utils/Preview.h>
#include <Utils/VideoControllerUI.h>
#include <Utils/PlotInfo.h>
#include <Utils/SqlOpenPose.h>
#include <Utils/Tracking.h>
#include <Utils/PeopleCounter.h>
#include <Utils/Vector.h>

using People = MinOpenPose::People;
using Person = MinOpenPose::Person;
using Node = MinOpenPose::Node;

int main(int argc, char* argv[])
{
	// 関数の戻り値を入れるための一時変数
	int ret = 0;

	// 入力する映像ファイルのフルパス
	std::string videoPath = R"(C:\Users\柴田研\Videos\guest002-2020-08-26_09-00-55.mp4)";

	// コンソール引数に動画のファイルパスを指定された場合はそのパスを優先する
	if (argc == 2) videoPath = argv[1];

	// 入出力するsqlファイルのフルパス
	std::string sqlPath = videoPath + ".sqlite3";

	// openposeのラッパークラス
	MinOpenPose openpose;

	// 動画を読み込むクラス
	Video video;
	video.open(videoPath);

	// プレビューウィンドウを生成するクラス
	Preview preview("result");

	// 画面のクリックでコンソールに座標を出力する
	preview.onClick([](int x, int y) { std::cout << x << ", " << y << std::endl; });

	// フレームレートなどを表示するクラス
	PlotFrameInfo plotFrameInfo;
	PlotTrajectory plotTrajectory;

	// SQLファイルの読み込み、書き込みを行うクラス
	SqlOpenPose sql;
	ret = sql.open(sqlPath, 300);
	if (ret) return ret;

	// 骨格をトラッキングするクラス
	Tracking tracker(
		0.5f,  // 関節の信頼値がこの値以下である場合は、関節が存在しないものとして処理する
		5,     // 信頼値がconfidenceThresholdより大きい関節の数がこの値未満である場合は、その人がいないものとして処理する
		10,    // 一度トラッキングが外れた人がこのフレーム数が経過しても再発見されない場合は、消失したものとして処理する
		50.0f  // トラッキング中の人が1フレーム進んだとき、移動距離がこの値よりも大きい場合は同一人物の候補から外す
	);

	// 通行人をカウントするクラス
	PeopleCounter count(200, 250, 500, 250, 100);

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
		// カメラの垂直画角(deg)、カメラの地面からの高さ(m)
		53.267, 1.0,
		// カメラに写っている地面の任意の4点
		147, 44,
		86, 393,
		492, 529,
		635, 199
	);

	// 動画再生のコントロールをUIで行えるようにするクラス
	VideoControllerUI videoController;
	videoController.addShortcutKeys(preview, video);

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
			people = openpose.estimate(frame);

			// 結果をSQLに保存
			sql.write(frameInfo.frameNumber, frameInfo.frameTimeStamp, people);
		}

		// トラッキング
		auto tracked_people = tracker.tracking(people, sql, frameInfo.frameNumber).value();

		// 通行人のカウント
		count.update(tracker, frameInfo.frameNumber);		

		// 歩行軌跡の描画
		plotTrajectory.plot(frame, tracked_people);

		// 軌跡のみを表示したウィンドウの表示
		cv::imshow("trajectory", plotTrajectory.image);

		// 通行人のカウント状況をプレビュー
		count.drawInfo(frame, tracker);

		// 映像の上に骨格を描画
		plotBone(frame, tracked_people, openpose);  // 骨格を描画
		plotId(frame, tracked_people);  // 人のIDの描画
		plotFrameInfo.plot(frame, video);  // フレームレートとフレーム番号の描画

		// 映像を上から見たように射影変換
		frame = screenToGround.translateMat(frame, 0.3f, true);

		// プレビュー
		int ret = preview.preview(frame, 1);

		// 動画再生コントローラーの表示
		videoController.showUI(video);

		// Escキーが押されたら終了する
		if (0x1b == ret) break;
	}

	return 0;
}
