#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <OpenPoseWrapper/Examples/TrackingOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/SqlOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/VideoOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/PlotInfoOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/PreviewOpenPoseEvent.h>
#include <regex>

int main(int argc, char* argv[])
{
	// openposeのラッパークラス
	MinimumOpenPose mop;


	// 入力する映像ファイルのフルパス
	std::string videoPath =
		R"(C:\Users\柴田研\Documents\VirtualUsers\17ad105\Videos\58.mp4)";
	// 入出力するsqlファイルのフルパス
	std::string sqlPath = std::regex_replace(videoPath, std::regex(R"(\.[^.]*$)"), "") + ".sqlite3";


	// トラッキング処理の追加
	auto tracking = mop.addEventListener<TrackingOpenPoseEvent>();
	// sql入出力機能の追加
	auto sql = mop.addEventListener<SqlOpenPoseEvent>(sqlPath, false, 60);
	// 動画読み込み処理の追加
	(void)mop.addEventListener<VideoOpenPoseEvent>(videoPath);
	// 出力画像に文字などを描画する処理の追加
	(void)mop.addEventListener<PlotInfoOpenPoseEvent>(true, true, true);
	// 出力画像のプレビューウィンドウを生成する処理の追加
	(void)mop.addEventListener<PreviewOpenPoseEvent>("result");


	// openposeの起動
	int ret = mop.startup();

	return ret;
}
