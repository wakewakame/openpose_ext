#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <OpenPoseWrapper/Examples/PreviewOpenPoseEvent.h>
#include <Utils/Vector.h>
#include <Utils/Gui.h>

class CustomOpenPoseEvent : public OpenPoseEvent
{
private:
	cv::Mat image;
	vt::ScreenToGround screenToGround;

public:
	uint8_t previewMode = 0;

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
			60.0, 6.3,
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
	int sendImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		// openposeに画像を入力
		imageInfo.inputImage = image;

		// 骨格検出をしない
		imageInfo.needOpenposeProcess = false;

		return 0;
	}
	int recieveImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		auto output = imageInfo.outputImage;

		// 指定した4点の範囲に線を描画
		screenToGround.drawAreaLine(output, 0);

		// 画像に文字を描画
		gui::text(output, "Press 'A'", { output.cols/2, output.rows/2}, gui::CENTER_CENTER, 3.0);

		// 魚眼レンズの歪みを除去
		if (previewMode == 1)
		{
			output = screenToGround.onlyFlatMat(output);
		}

		// 地面を上から見たように射影変換
		if (previewMode == 2)
		{
			output = screenToGround.translateMat(output, 0.3f);
		}

		imageInfo.outputImage = output;

		return 0;
	}
};

int main(int argc, char* argv[])
{
	// openposeのラッパークラス
	MinimumOpenPose mop;

	// 自分で定義したイベントリスナーを登録する
	auto custom = mop.addEventListener<CustomOpenPoseEvent>();

	// 出力画像のプレビューウィンドウを生成する処理の追加
	auto preview = mop.addEventListener<PreviewOpenPoseEvent>("example04_Viewport");

	// プレビューウィンドウにキーイベントを追加
	preview->addKeyboardEventListener([&](int key)
	{
		// 'A'キーが押された場合
		if (key == 'a')
		{
			// 表示モードを切り替える
			custom->previewMode = (custom->previewMode + 1) % 3;
		}
	});

	// openposeの起動
	int ret = mop.startup();

	return ret;
}
