/*

OpenPose の処理は膨大です。
同じ動画を何度も OpenPose に入力するのは非効率です。
そのため OpenPose での解析結果をファイルとして入出力できるサンプルを用意しました。

このサンプルは、1度目の実行では OpenPose で動画を処理し、解析結果をファイルとして保存します。
2度目以降の実行では1度目の実行で生成されたファイルを読み込んで骨格データを取得します。

また、解析結果を保存するファイル名は、入力する動画の名前の末尾に".sqlite3"が付きます。
たとえば "aaa.mp4" という動画を入力した場合、その動画ファイルと同じ場所に "aaa.mp4.sqlite3" というファイルが生成されます。

このファイルの形式は sqlite3 なので DB Browser (SQLite) などで開くことができます。

*/

#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <Utils/Video.h>
#include <Utils/Preview.h>
#include <Utils/VideoControllerUI.h>
#include <Utils/PlotInfo.h>
#include <Utils/SqlOpenPose.h>

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

		// 姿勢推定の結果を image に描画する
		plotBone(image, people, openpose);

		// 画面を更新する
		preview.preview(image);
	}

	return 0;
}