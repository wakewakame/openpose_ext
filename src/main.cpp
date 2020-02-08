#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <OpenPoseWrapper/Examples/TrackingOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/SqlOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/VideoOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/PlotInfoOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/PreviewOpenPoseEvent.h>
#include <regex>

#include <Utils/Tracking.h>
class CustomOpenPoseEvent : public OpenPoseEvent
{
private:
	std::shared_ptr<TrackingOpenPoseEvent> tracking;
	std::shared_ptr<VideoOpenPoseEvent> video;
	std::shared_ptr<PreviewOpenPoseEvent> preview;
	op::PeopleLineCounter peopleLineCounter;
	vt::ScreenToGround screenToGround;
	cv::Point mouse;
	int previewMode;
public:
	CustomOpenPoseEvent() : previewMode(0) {}
	virtual ~CustomOpenPoseEvent() {}
	int sendImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final { return 0; }
	int recieveImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		if ((!tracking) || (!video))
		{
			std::cout
				<< "TrackingOpenPoseEventもしくはVideoOpenPoseEventが未指定です。\n"
				<< "setParams関数で正しい値を指定してください。"
				<< std::endl;
			return 1;
		}

		op::PeopleList& people = tracking->people;

		// 人数カウント
		peopleLineCounter.setLine(579, 578, 1429, 577, 100.0);  // カウントの基準線の座標設定
		peopleLineCounter.updateCount(people);  // カウントの更新
		peopleLineCounter.drawJudgeLine(imageInfo.outputImage);  // 基準線の描画
		peopleLineCounter.drawPeopleLine(imageInfo.outputImage, people, true);  // 人々の始点と終点の描画

		// カウントを表示
		gui::text(imageInfo.outputImage, std::string("up : ") + std::to_string(peopleLineCounter.getUpCount()), { 20, 200 });
		gui::text(imageInfo.outputImage, std::string("down : ") + std::to_string(peopleLineCounter.getDownCount()), { 20, 230 });

		// 射影変換
		screenToGround.setParams(
			imageInfo.outputImage.cols, imageInfo.outputImage.rows, 33.3, 6.3,
			492, 436,
			863, 946,
			1335, 644,
			905, 242
		);
		screenToGround.drawAreaLine(imageInfo.outputImage);  // 射影変換に使用する4点の範囲を描画

		if (previewMode == 1) imageInfo.outputImage = screenToGround.perspective(imageInfo.outputImage);

		return 0;
	}
	void setParams(
		const std::shared_ptr<TrackingOpenPoseEvent> trackingTmp,
		const std::shared_ptr<VideoOpenPoseEvent> videoTmp,
		const std::shared_ptr<PreviewOpenPoseEvent> previewTmp = nullptr
	)
	{
		tracking = trackingTmp;
		video = videoTmp;
		preview = previewTmp;

		if (!preview) return;

		// マウスイベント処理
		preview->addMouseEventListener([&](int event, int x, int y) {
			switch (event)
			{
			// マウス移動時
			case cv::EVENT_MOUSEMOVE:
				mouse.x = x; mouse.y = y;
				break;

			// 左クリック時
			case cv::EVENT_LBUTTONDOWN:
				std::cout << x << ", " << y << std::endl;
				break;

			// 右クリック時
			case cv::EVENT_RBUTTONDOWN:
				break;

			// その他
			default:
				break;
			}
		});

		// キーイベント処理
		preview->addKeyboardEventListener([&](int key) {
			switch (key)
			{
			// Jキーで30フレーム戻る
			case 'j':
				video->seekRelative(-30);
				break;

			// Kキーで30フレーム進む
			case 'k':
				video->seekRelative(30);
				break;

			// Aキーで画面表示切替
			case 'a':
				previewMode = (previewMode + 1) % 2;
				break;

			// スペースキーで動画の再生/一時停止
			case 32:
				if (video->isPlay()) video->pause();
				else video->play();
				break;

			// その他
			default:
				break;
			}
		});
	}
};

int main(int argc, char* argv[])
{
	// openposeのラッパークラス
	MinimumOpenPose mop;


	// 入力する映像ファイルのフルパス
	std::string videoPath =
		R"(G:\思い出\Dropbox\Dropbox\SDK\openpose\video\IMG_1533.mp4)";
	// 入出力するsqlファイルのフルパス
	std::string sqlPath = std::regex_replace(videoPath, std::regex(R"(\.[^.]*$)"), "") + ".sqlite3";


	// トラッキング処理の追加
	auto tracking = mop.addEventListener<TrackingOpenPoseEvent>();
	// sql入出力機能の追加
	auto sql = mop.addEventListener<SqlOpenPoseEvent>(sqlPath, 60);
	// 動画読み込み処理の追加
	auto video = mop.addEventListener<VideoOpenPoseEvent>(videoPath);
	// 出力画像に文字などを描画する処理の追加
	(void)mop.addEventListener<PlotInfoOpenPoseEvent>(true, true, false);
	// 自分で定義したイベントリスナーの登録
	auto custom = mop.addEventListener<CustomOpenPoseEvent>();
	// 出力画像のプレビューウィンドウを生成する処理の追加
	auto preview = mop.addEventListener<PreviewOpenPoseEvent>("result");


	custom->setParams(tracking, video, preview);


	// openposeの起動
	int ret = mop.startup();

	return ret;
}
