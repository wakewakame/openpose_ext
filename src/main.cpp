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
	op::PeopleList& people;
	op::PeopleLineCounter peopleLineCounter;
	cv::Point mouse;
public:
	CustomOpenPoseEvent(std::shared_ptr<TrackingOpenPoseEvent>& trackingTmp) : people(trackingTmp->people) {}
	virtual ~CustomOpenPoseEvent() {};
	int sendImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final { return 0; }
	int recieveImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		/* 人数カウント */
		peopleLineCounter.setLine(579, 578, 1429, 577, 100.0);  // 基準線の座標を更新(始点のx座標, 始点のy座標, 終点のx座標, 終点のy座標)
		peopleLineCounter.update(people);  // カウントの更新
		peopleLineCounter.drawLine(imageInfo.outputImage);  // 基準線の描画
		// 上方向のカウントを表示
		gui::text(imageInfo.outputImage, std::string("up : ") + std::to_string(peopleLineCounter.getUpCount()), { 20, 200 });
		// 下方向のカウントを表示
		gui::text(imageInfo.outputImage, std::string("down : ") + std::to_string(peopleLineCounter.getDownCount()), { 20, 230 });

		/* トラッキングの始点と終点を結ぶ直線を描画 */
		for (size_t index : people.getCurrentIndices()) {  // 現在のフレームに映っているすべての人のIDを配列で取得し、人数分ループ
			auto firstTree = people.getFirstTree(index);  // ID番目の人が最初に映ったときの骨格を取得
			auto currentTree = people.getCurrentTree(index);  // ID番目の人の現在のフレームの骨格を取得
			if ((!firstTree.isValid()) || (!currentTree.isValid())) continue;  // 取得した骨格が有効な値であるか確認
			cv::line(imageInfo.outputImage, { (int)firstTree.average().x, (int)firstTree.average().y }, { (int)currentTree.average().x, (int)currentTree.average().y }, cv::Scalar{
				(double)((int)((std::sin((double)index * 463763.0) + 1.0) * 100000.0) % 120 + 80),
				(double)((int)((std::sin((double)index * 1279.0) + 1.0) * 100000.0) % 120 + 80),
				(double)((int)((std::sin((double)index * 92763.0) + 1.0) * 100000.0) % 120 + 80)
				}, 2.0);
		}

		cv::setMouseCallback("result", [](int event, int x, int y, int flags, void* userdata) {
			if (event == cv::EVENT_MOUSEMOVE)
			{
				((cv::Point*)userdata)->x = x;
				((cv::Point*)userdata)->y = y;
				std::cout << x << ", " << y << std::endl;
			}
		}, (void*)(&mouse));

		return 0;
	}
};

int main(int argc, char* argv[])
{
	// openposeのラッパークラス
	MinimumOpenPose mop;


	// 入力する映像ファイルのフルパス
	std::string videoPath =
		R"(C:\Users\柴田研\Documents\VirtualUsers\17ad105\Videos\IMG_1533.mp4)";
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
	// 自分で定義したイベントリスナーの登録
	(void)mop.addEventListener<CustomOpenPoseEvent>(tracking);
	// 出力画像のプレビューウィンドウを生成する処理の追加
	(void)mop.addEventListener<PreviewOpenPoseEvent>("result");


	// openposeの起動
	int ret = mop.startup();

	return ret;
}
