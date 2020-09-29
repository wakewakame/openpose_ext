#include <OpenPoseWrapper/MinimumOpenPose.h>
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

	// 動画読み込み処理の追加
	mop.addEventListener<VideoOpenPoseEvent>(R"(media/video.mp4)");

	// 出力画像に骨格情報などを描画する処理の追加
	mop.addEventListener<PlotInfoOpenPoseEvent>(true, true, false);

	// 自分で定義したイベントリスナーの登録
	mop.addEventListener<CustomOpenPoseEvent>();

	// 出力画像のプレビューウィンドウを生成する処理の追加
	mop.addEventListener<PreviewOpenPoseEvent>("example02_PlayVideo");

	// openposeの起動
	int ret = mop.startup();

	return ret;
}
