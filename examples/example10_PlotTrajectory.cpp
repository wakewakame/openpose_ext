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

	// SQLファイルの読み込み、書き込みを行うクラス
	SqlOpenPose sql;
	ret = sql.open(R"(D:\思い出\Dropbox\Dropbox\SDK\openpose\video\out2.mp4.sqlite3)");
	if (ret) return ret;

	// プレビューウィンドウを生成するクラス
	Preview preview("result");

	// フレームレートなどを表示するクラス
	PlotFrameInfo plotFrameInfo;
	PlotTrajectory plotTrajectory;

	// データベースに関する情報
	size_t firstFrameNumber = 1;
	size_t lastFrameNumber = 2329;
	Node leftTop = { -0.599098145961762, -3.17585253715515 };
	Node rightBottom = { 5.36959314346314, 1.66536796092987 };

	size_t frameNumber = 0;

	cv::Mat frame = cv::Mat(720, 1280, CV_8UC3, { 0, 0, 0 });

	for (size_t frameNumber = firstFrameNumber; frameNumber <= lastFrameNumber; frameNumber++)
	{
		// フレームレートの描画
		plotFrameInfo.plotFPS(frame);

		// 軌跡情報の取得
		auto trajectoryPoint = sql.readPoints("trajectory", frameNumber);

		// 画面に収まるように調整
		for (auto personItr = trajectoryPoint.begin(); personItr != trajectoryPoint.end(); personItr++)
		{
			cv::Point2f p{ personItr->second.x, personItr->second.y };
			p.x -= leftTop.x;
			p.y -= leftTop.y;
			p.x *= (float)frame.cols / (rightBottom.x - leftTop.x);
			p.y *= (float)frame.rows / (rightBottom.y - leftTop.y);
			personItr->second = Node{ (float)p.x, (float)p.y };
		}

		// 軌跡の描画
		plotTrajectory.plot(frame, trajectoryPoint);

		// 100フレームごとに進捗をプレビュー
		if (frameNumber % 100 == 0) {
			ret = preview.preview(plotTrajectory.image, 1);
		}

		// Escキーが押されたら終了する
		if (0x1b == ret) return 0;
	}

	// 最後にプレビュー
	preview.preview(plotTrajectory.image, 1);

	// キー入力があるまで待機
	cv::waitKey(0);

	return 0;
}
