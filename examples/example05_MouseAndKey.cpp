#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <OpenPoseWrapper/Examples/PreviewOpenPoseEvent.h>
#include <Utils/Gui.h>

class CustomOpenPoseEvent : public OpenPoseEvent
{
private:
	cv::Mat image;

public:
	int init() override final
	{
		// 500x500の黒色の画像を生成
		image = cv::Mat::zeros(500, 500, CV_8UC3);

		return 0;
	}
	int sendImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		// 画像を入力
		imageInfo.inputImage = image;

		// 骨格検出をしない
		imageInfo.needOpenposeProcess = false;

		return 0;
	}
	int recieveImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		gui::text(imageInfo.outputImage, "Press 'A'", { 20, 20 });
		gui::text(imageInfo.outputImage, "Press 'Space'", { 20, 50 });
		return 0;
	}
	void registPreview(const std::shared_ptr<PreviewOpenPoseEvent> preview)
	{
		// マウスイベント処理
		preview->addMouseEventListener([&](int event, int x, int y) {
			switch (event)
			{
			// マウス移動時
			case cv::EVENT_MOUSEMOVE:
				gui::text(image, "A", { x, y });
				break;

			// 左クリック時
			case cv::EVENT_LBUTTONDOWN:
				break;

			// 右クリック時
			case cv::EVENT_RBUTTONDOWN:
				break;
			}
		});

		// キーイベント処理
		preview->addKeyboardEventListener([&](int key) {
			switch (key)
			{
			// Aキーで色を反転
			case 'a':
				cv::bitwise_not(image, image);
				break;

			// スペースキーで画像を初期化
			case 32:
				image = cv::Mat::zeros(image.rows, image.cols, CV_8UC3);
				break;
			}
		});
	}
};

int main(int argc, char* argv[])
{
	// openposeのラッパークラス
	MinimumOpenPose mop;

	// 自分で定義したイベントリスナーを登録する
	auto custom = mop.addEventListener<CustomOpenPoseEvent>();

	// 出力画像のプレビューウィンドウを生成する処理の追加
	auto preview = mop.addEventListener<PreviewOpenPoseEvent>("example05_MouseAndKeyboard");

	// 自分で定義したイベントリスナーにPreviewOpenPoseEventのインスタンスを渡す
	custom->registPreview(preview);

	// openposeの起動
	int ret = mop.startup();

	return ret;
}
