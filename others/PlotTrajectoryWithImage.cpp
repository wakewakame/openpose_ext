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
	std::string videoPath = R"(D:\思い出\Dropbox\Dropbox\SDK\openpose\video\out1.mp4)";

	// 入出力するsqlファイルのフルパス
	std::string sqlPath = videoPath + ".sqlite3";

	// 動画を読み込むクラス
	Video video;
	ret = video.open(videoPath);
	if (ret) return ret;

	// SQLファイルの読み込み、書き込みを行うクラス
	SqlOpenPose sql;
	ret = sql.open(sqlPath);
	if (ret) return ret;

	// データベースに関する情報を取得する
	SQLite::Statement infoQuery(*(sql.database),
		u8"SELECT MIN(x), MIN(y), MAX(x), MAX(y) FROM trajectory"
	);
	infoQuery.executeStep();

	// データベースに存在する座標の最小値、最大値
	cv::Point2f leftTop = { (float)infoQuery.getColumn(0).getDouble(), (float)infoQuery.getColumn(1).getDouble() };
	cv::Point2f rightBottom = { (float)infoQuery.getColumn(2).getDouble(), (float)infoQuery.getColumn(3).getDouble() };

	// 射影変換をするクラス
	vt::ScreenToGround screenToGround;

	// カメラの歪みを補正する設定
	// 広角カメラ用の設定
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
		1280, 720,
		// カメラに写っている地面の任意の4点 (左上、右上、右下、左下)
		598, 246,
		1047, 276,
		1077, 624,
		537, 601,
		// 上記の4点のうち、1点目から2点目までの長さと、2点目から3点目までの長さ (単位は任意)
		2.334, 1.800
	);

	// プレビューウィンドウを生成するクラス
	Preview preview("result");

	// フレームレートなどを表示するクラス
	PlotFrameInfo plotFrameInfo;
	PlotTrajectory plotTrajectory;

	cv::Mat frameOrg = video.next();
	cv::rotate(frameOrg, frameOrg, cv::ROTATE_180);
	frameOrg = screenToGround.translateMat(frameOrg, 0.3f, true);
	cv::Mat frame = frameOrg.clone();

	size_t count = 0;
	std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

	// 指定されたフレーム番号に映る人すべての骨格の重心を検索する
	SQLite::Statement peopleQuery(*(sql.database), u8"SELECT * FROM trajectory");
	std::map<size_t, Node> trajectoryPoint;
	size_t currentFrame = 0;
	while (peopleQuery.executeStep())
	{
		// 座標情報を取得
		size_t frameNumber = (size_t)peopleQuery.getColumn(0).getUInt();
		size_t index = (size_t)peopleQuery.getColumn(1).getUInt();
		cv::Point2f p{
			(float)peopleQuery.getColumn(2).getDouble(),
			(float)peopleQuery.getColumn(3).getDouble()
		};

		// 画面に収まるように調整
		p = screenToGround.plot(p, frame, 0.3f);

		// フレーム番号に変化があれば、以前のフレームの描画を行う
		if (frameNumber != currentFrame)
		{
			// 軌跡の描画
			plotTrajectory.plot(frame, trajectoryPoint);

			// フレーム番号の更新
			currentFrame = frameNumber;

			trajectoryPoint.clear();

			// 1000ループごとに進捗をプレビュー
			if ((count++) % 1000 == 0) {
				ret = preview.preview(frame, 1);
			}
		}

		trajectoryPoint[index] = Node{ (float)p.x, (float)p.y };

		// Escキーが押されたら終了する
		if (0x1b == ret) return 0;
	}

	float time = (float)(std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::high_resolution_clock::now() - start
	).count()) / 1000.0f;
	std::cout << "time score: " << time << " [sec]" << std::endl;

	// 最後にプレビュー
	frame = frameOrg.clone();
	plotTrajectory.plot(frame, trajectoryPoint);
	preview.preview(frame, 1);

	// キー入力があるまで待機
	cv::waitKey(0);

	return 0;
}
