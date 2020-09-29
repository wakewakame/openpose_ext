#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <OpenPoseWrapper/Examples/PreviewOpenPoseEvent.h>
#include <Utils/Gui.h>

class CustomOpenPoseEvent : public OpenPoseEvent
{
public:
	int init() override final
	{
		return 0;
	}
	int sendImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		// 500x500の黒色の画像を生成
		cv::Mat image = cv::Mat::zeros(500, 500, CV_8UC3);

		// 画像に文字を描画
		gui::text(image, "Hello World", { 20, 20 });

		// openposeの入力用の変数に代入
		imageInfo.inputImage = image;

		return 0;
	}
	int recieveImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		// 画像に文字を描画
		gui::text(imageInfo.outputImage, "ABC123", { 20, 50 });

		return 0;
	}
};

int main(int argc, char* argv[])
{
	// openposeのラッパークラス
	MinimumOpenPose mop;

	// 自分で定義したイベントリスナーを登録する
	mop.addEventListener<CustomOpenPoseEvent>();

	// 出力画像のプレビューウィンドウを生成する処理の追加
	// Escで終了する
	mop.addEventListener<PreviewOpenPoseEvent>("example01_HelloWorld");

	// openposeの起動
	int ret = mop.startup();

	return ret;
}
