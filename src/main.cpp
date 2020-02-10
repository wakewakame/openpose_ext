#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <OpenPoseWrapper/Examples/SqlOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/VideoOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/PlotInfoOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/PreviewOpenPoseEvent.h>
#include <regex>

#include <Utils/Tracking.h>
class CustomOpenPoseEvent : public OpenPoseEvent
{
private:
	op::PeopleList people{
		5,         // NUMBER_NODES_TO_TRUST
		0.5f,   // CONFIDENCE_THRESHOLD
		10,      // NUMBER_FRAMES_TO_LOST
		50.0f  // DISTANCE_THRESHOLD
	};
	std::shared_ptr<VideoOpenPoseEvent> video;
	std::shared_ptr<SqlOpenPoseEvent> sql;
	std::shared_ptr<PreviewOpenPoseEvent> preview;
	op::PeopleLineCounter peopleLineCounter;
	vt::ScreenToGround screenToGround;
	cv::Point mouse;
	int previewMode;

	int checkError()
	{
		if ((!video) || (!sql))
		{
			std::cout
				<< "VideoOpenPoseEvent, SqlOpenPoseEventのいずれかが未指定です。\n"
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

		// people_with_trackingテーブルを再生成
		sql->database->exec(u8"DROP TABLE IF EXISTS people_with_tracking");
		if (sql->createTableIfNoExist(
			u8"people_with_tracking",
			u8"frame INTEGER, people INTEGER, x REAL, y REAL"
		)) return 1;
		if (sql->createIndexIfNoExist(u8"people_with_tracking", u8"frame", false)) return 1;

		// people_with_normalized_trackingテーブルを再生成
		sql->database->exec(u8"DROP TABLE IF EXISTS people_with_normalized_tracking");
		if (sql->createTableIfNoExist(
			u8"people_with_normalized_tracking",
			u8"frame INTEGER, people INTEGER, x REAL, y REAL"
		)) return 1;
		if (sql->createIndexIfNoExist(u8"people_with_normalized_tracking", u8"frame", false)) return 1;

		return 0;
	}
	int sendImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final { return 0; }
	int recieveImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		if (checkError()) return 1;

		// トラッキング
		people.addFrame(imageInfo);

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
		if (previewMode == 1) imageInfo.outputImage = screenToGround.perspective(imageInfo.outputImage, 0.3f); // プレビュー

		// people_with_tracking、people_with_normalized_trackingテーブルの更新
		try
		{
			SQLite::Statement pwtQuery(*sql->database, u8"INSERT INTO people_with_tracking VALUES (?, ?, ?, ?)");
			SQLite::Statement pwntQuery(*sql->database, u8"INSERT INTO people_with_normalized_tracking VALUES (?, ?, ?, ?)");
			for (auto&& index : people.getCurrentIndices())
			{
				auto tree = people.getCurrentTree(index);
				if (tree.frameNumber != imageInfo.frameNumber) continue;
				auto position = tree.average();

				pwtQuery.reset();
				pwtQuery.bind(1, (long long)imageInfo.frameNumber);
				pwtQuery.bind(2, (long long)index);
				pwtQuery.bind(3, (double)position.x);
				pwtQuery.bind(4, (double)position.y);
				(void)pwtQuery.exec();

				auto normal = screenToGround.translate(vt::Vector4{ position.x, position.y });
				pwntQuery.reset();
				pwntQuery.bind(1, (long long)imageInfo.frameNumber);
				pwntQuery.bind(2, (long long)index);
				pwntQuery.bind(3, (double)normal.x);
				pwntQuery.bind(4, (double)normal.y);
				(void)pwntQuery.exec();
			}
		}
		catch (const std::exception& e)
		{
			std::cout << u8"error : " << __FILE__ << u8" : L" << __LINE__ << u8"\n" << e.what() << std::endl;
			return 1;
		}

		return 0;
	}
	void setParams(
		const std::shared_ptr<VideoOpenPoseEvent> videoTmp,
		const std::shared_ptr<SqlOpenPoseEvent> sqlTmp,
		const std::shared_ptr<PreviewOpenPoseEvent> previewTmp = nullptr
	)
	{
		video = videoTmp;
		sql = sqlTmp;
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
	std::string videoPath =
		R"(C:\Users\柴田研\Documents\VirtualUsers\17ad105\Videos\IMG_1533.mp4)";
	// 入出力するsqlファイルのフルパス
	std::string sqlPath = std::regex_replace(videoPath, std::regex(R"(\.[^.]*$)"), "") + ".sqlite3";


	// sql入出力機能の追加
	auto sql = mop.addEventListener<SqlOpenPoseEvent>(sqlPath, 300);
	// 動画読み込み処理の追加
	auto video = mop.addEventListener<VideoOpenPoseEvent>(videoPath);
	// 出力画像に文字などを描画する処理の追加
	(void)mop.addEventListener<PlotInfoOpenPoseEvent>(true, true, false);
	// 自分で定義したイベントリスナーの登録
	auto custom = mop.addEventListener<CustomOpenPoseEvent>();
	// 出力画像のプレビューウィンドウを生成する処理の追加
	auto preview = mop.addEventListener<PreviewOpenPoseEvent>("result");


	custom->setParams(video, sql, preview);


	// openposeの起動
	auto start = std::chrono::high_resolution_clock::now();
	int ret = mop.startup();
	double time = (1.0 / 1000.0) * (double)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();
	std::cout << "time score : " << time << " sec." << std::endl;

	return ret;
}
