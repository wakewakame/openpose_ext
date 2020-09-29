#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <OpenPoseWrapper/Examples/SqlOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/VideoOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/PlotInfoOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/PreviewOpenPoseEvent.h>

class CustomOpenPoseEvent : public OpenPoseEvent
{
public:
	int init() override final
	{
		return 0;
	}
	int sendImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		return 0;
	}
	int recieveImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		return 0;
	}
};

int main(int argc, char* argv[])
{
	// openposeのラッパークラス
	MinimumOpenPose mop;

	// SQL入出力機能の追加
	// 指定されたパスにopenposeの解析結果を保存する
	// 既にファイルが存在している場合はopenposeの解析は行わず、ファイルのデータを用いる
	// 以下の例では、300フレームごとにmedia/video.mp4.sqlite3へ解析結果を更新する
	mop.addEventListener<SqlOpenPoseEvent>(R"(media/video.mp4.sqlite3)", 300);

	// 動画読み込み処理の追加
	mop.addEventListener<VideoOpenPoseEvent>(R"(media/video.mp4)");

	// 出力画像に骨格情報などを描画する処理の追加
	mop.addEventListener<PlotInfoOpenPoseEvent>(true, true, false);

	// 自分で定義したイベントリスナーの登録
	mop.addEventListener<CustomOpenPoseEvent>();

	// 出力画像のプレビューウィンドウを生成する処理の追加
	mop.addEventListener<PreviewOpenPoseEvent>("example03_OutputSQL");

	// openposeの起動
	int ret = mop.startup();

	return ret;
}
