#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <OpenPoseWrapper/Examples/SqlOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/VideoOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/PlotInfoOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/TrackingOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/PeopleCounterOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/PreviewOpenPoseEvent.h>
#include <regex>

class CustomOpenPoseEvent : public OpenPoseEvent
{
private:
	cv::Mat image;
	std::shared_ptr<PreviewOpenPoseEvent> preview;
	vt::ScreenToGround screenToGround;
	cv::Point mouse;
	int previewMode = 0;

public:
	int init() override final
	{
		// 画像を読み込む
		image = cv::imread(R"(media/checker.png)", 1);

		// カメラキャリブレーションで得られたパラメータの設定
		screenToGround.setCalibration(
			// カメラキャリブレーションに用いた画像の解像度(w, h), 出力画像の拡大率
			1858.0, 1044.0, 0.5,
			// カメラ内部パラメータの焦点距離と中心座標(fx, fy, cx, cy)
			1057, 1057, 935, 567,
			// カメラの歪み係数(k1, k2, k3, k4)
			0.0, 0.0, 0.0, 0.0
		);

		// 撮影位置などの指定
		screenToGround.setParams(
			// 画像の解像度
			image.cols, image.rows,
			// カメラの垂直画角(deg)と地面からの高さ(m)
			112.0, 6.3,
			// 画面上の地面の座標1
			346, 659,
			// 画面上の地面の座標2
			1056, 668,
			// 画面上の地面の座標3
			990, 202,
			// 画面上の地面の座標4
			478, 292
		);

		return 0;
	}
	int sendImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final {
		// openposeに画像を入力
		imageInfo.inputImage = image;

		// 骨格検出をしない
		imageInfo.needOpenposeProcess = false;

		return 0;
	}
	int recieveImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		// 複製
		imageInfo.outputImage = imageInfo.outputImage.clone();

		if (previewMode == 1) imageInfo.outputImage = screenToGround.onlyFlatMat(imageInfo.outputImage);
		if (previewMode == 2) imageInfo.outputImage = screenToGround.translateMat(imageInfo.outputImage, 0.3f);

		return 0;
	}
	void registPreview(const std::shared_ptr<PreviewOpenPoseEvent> preview)
	{
		if (!preview) return;
		this->preview = preview;

		// マウスイベント処理
		preview->addMouseEventListener([&](int event, int x, int y) {
			if (event == cv::EVENT_MOUSEMOVE)   { mouse.x = x; mouse.y = y; }
			if (event == cv::EVENT_LBUTTONDOWN) { std::cout << x << ", " << y << std::endl; }
		});

		// キーイベント処理
		preview->addKeyboardEventListener([&](int key) {
			if (key == 'a') { previewMode = (previewMode + 1) % 3; }
		});
	}
};

int main(int argc, char* argv[])
{
	// openposeのラッパークラス
	MinimumOpenPose mop;

	// 自分で定義したイベントリスナーの登録
	auto custom = mop.addEventListener<CustomOpenPoseEvent>();

	// 出力画像のプレビューウィンドウを生成する処理の追加
	auto preview = mop.addEventListener<PreviewOpenPoseEvent>("result");

	custom->registPreview(preview);

	// openposeの起動
	int ret = mop.startup();

	return ret;
}
