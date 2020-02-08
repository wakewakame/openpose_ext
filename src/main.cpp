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
public:
	CustomOpenPoseEvent() {}
	virtual ~CustomOpenPoseEvent() {}
	int sendImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final { return 0; }
	int recieveImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		op::PeopleList& people = tracking->people;

		// 人数カウント
		peopleLineCounter.setLine(579, 578, 1429, 577, 100.0);
		peopleLineCounter.update(people);
		peopleLineCounter.drawLine(imageInfo.outputImage);

		// カウントを表示
		gui::text(imageInfo.outputImage, std::string("up : ") + std::to_string(peopleLineCounter.getUpCount()), { 20, 200 });
		gui::text(imageInfo.outputImage, std::string("down : ") + std::to_string(peopleLineCounter.getDownCount()), { 20, 230 });

		// トラッキングの始点と終点を結ぶ直線を描画
		for (size_t index : people.getCurrentIndices())
		{
			auto firstTree = people.getFirstTree(index);
			auto currentTree = people.getCurrentTree(index);
			if ((!firstTree.isValid()) || (!currentTree.isValid())) continue;

			// 直線の描画
			cv::line(imageInfo.outputImage, { (int)firstTree.average().x, (int)firstTree.average().y }, { (int)currentTree.average().x, (int)currentTree.average().y }, cv::Scalar{
				(double)((int)((std::sin((double)index * 463763.0) + 1.0) * 100000.0) % 120 + 80),
				(double)((int)((std::sin((double)index * 1279.0) + 1.0) * 100000.0) % 120 + 80),
				(double)((int)((std::sin((double)index * 92763.0) + 1.0) * 100000.0) % 120 + 80)
			}, 2.0);

			// idの描画
			gui::text(imageInfo.outputImage, std::to_string(index), { (int)currentTree.average().x, (int)currentTree.average().y }, gui::CENTER_CENTER, 0.5);
		}

		return 0;
	}
	void setParams(
		std::shared_ptr<TrackingOpenPoseEvent>& trackingTmp,
		std::shared_ptr<VideoOpenPoseEvent>& videoTmp,
		std::shared_ptr<PreviewOpenPoseEvent>& previewTmp
	)
	{
		tracking = trackingTmp;
		video = videoTmp;
		preview = previewTmp;

		// マウスイベント処理
		preview->addMouseEventListener([&](int event, int x, int y) {
			switch (event)
			{
			// マウス移動時
			case cv::EVENT_MOUSEMOVE:
				break;

			// 左クリック時
			case cv::EVENT_LBUTTONDOWN:
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
			std::cout << key << std::endl;

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
