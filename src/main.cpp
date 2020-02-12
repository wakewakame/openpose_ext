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
	std::shared_ptr<VideoOpenPoseEvent> video;
	std::shared_ptr<TrackingOpenPoseEvent> tracker;
	std::shared_ptr<SqlOpenPoseEvent> sql;
	std::shared_ptr<PreviewOpenPoseEvent> preview;
	vt::ScreenToGround screenToGround;
	cv::Point mouse;
	int previewMode;

	int checkError()
	{
		if ((!video) || (!tracker))
		{
			std::cout
				<< "VideoOpenPoseEvent, TrackingOpenPoseEventのいずれかが未指定です。\n"
				<< "setParams関数で正しい値を指定してください。"
				<< std::endl;
			return 1;
		}
		return 0;
	}
public:
	CustomOpenPoseEvent() : previewMode(0) {}
	virtual ~CustomOpenPoseEvent() {}
	int init() override final
	{
		if (checkError()) return 1;

		// people_with_normalized_trackingテーブルを再生成
		if (sql->deleteTableIfExist(u8"people_with_normalized_tracking")) return 1;
		if (sql->createTableIfNoExist(u8"people_with_normalized_tracking", u8"frame INTEGER, people INTEGER, x REAL, y REAL")) return 1;
		if (sql->createIndexIfNoExist(u8"people_with_normalized_tracking", u8"frame", false)) return 1;
		if (sql->createIndexIfNoExist(u8"people_with_normalized_tracking", u8"people", false)) return 1;
		if (sql->createIndexIfNoExist(u8"people_with_normalized_tracking", u8"frame", u8"people", true)) return 1;

		return 0;
	}
	int sendImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final { return 0; }
	int recieveImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		if (checkError()) return 1;

		// 射影変換
		screenToGround.setParams(
			imageInfo.outputImage.cols, imageInfo.outputImage.rows, 33.3, 6.3,
			492, 436,
			863, 946,
			1335, 644,
			905, 242
		);
		screenToGround.drawAreaLine(imageInfo.outputImage);  // 射影変換に使用する4点の範囲を描画
		if (previewMode == 1) imageInfo.outputImage = screenToGround.perspective(imageInfo.outputImage, 0.3f); // プレビュー

		// people_with_normalized_trackingテーブルの更新
		if (!sql->isDataExist("people_with_normalized_tracking", "frame", imageInfo.frameNumber))
		{
			SQLite::Statement insertQuery(*sql->database, u8"INSERT INTO people_with_normalized_tracking VALUES (?, ?, ?, ?)");
			for (auto&& currentPerson = tracker->latestPeople.begin(); currentPerson != tracker->latestPeople.end(); currentPerson++)
			{
				auto&& position = TrackingOpenPoseEvent::getJointAverage(currentPerson->second);
				auto normal = screenToGround.translate(vt::Vector4{ position.x, position.y });
				if (sql->bindAllAndExec(insertQuery, imageInfo.frameNumber, currentPerson->first, normal.x, normal.y)) return 1;
			}
		}

		return 0;
	}
	void setParams(
		const std::shared_ptr<VideoOpenPoseEvent> videoTmp,
		const std::shared_ptr<TrackingOpenPoseEvent> trackingTmp,
		const std::shared_ptr<PreviewOpenPoseEvent> previewTmp = nullptr
	)
	{
		video = videoTmp;
		tracker = trackingTmp;
		sql = trackingTmp->sql;
		preview = previewTmp;

		if (checkError()) return;

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
	std::string videoPath = R"(G:\思い出\Dropbox\Dropbox\SDK\openpose\video\IMG_1533.mp4)";

	// 入出力するsqlファイルのフルパス
	std::string sqlPath = videoPath + ".sqlite3";



	// SQL入出力機能の追加
	auto sql = mop.addEventListener<SqlOpenPoseEvent>(sqlPath, 300);

	// 動画読み込み処理の追加
	auto video = mop.addEventListener<VideoOpenPoseEvent>(videoPath);

	// 骨格のトラッキング処理の追加
	auto tracker = mop.addEventListener<TrackingOpenPoseEvent>(sql);

	// 人数カウント処理の追加
	(void)mop.addEventListener<PeopleCounterOpenPoseEvent>(tracker, 579, 578, 1429, 577, 100.0, true);

	// 出力画像に骨格情報などを描画する処理の追加
	(void)mop.addEventListener<PlotInfoOpenPoseEvent>(true, true, false);

	// 自分で定義したイベントリスナーの登録
	auto custom = mop.addEventListener<CustomOpenPoseEvent>();

	// 出力画像のプレビューウィンドウを生成する処理の追加
	std::shared_ptr<PreviewOpenPoseEvent> preview;
	preview = mop.addEventListener<PreviewOpenPoseEvent>("result");



	custom->setParams(video, tracker, preview);



	// openposeの起動
	auto start = std::chrono::high_resolution_clock::now();
	int ret = mop.startup();
	double time = (1.0 / 1000.0) * (double)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();
	
	
	
	// 起動から終了までに要した時間の表示
	std::cout << "time score : " << time << " sec." << std::endl;



	return ret;
}
