#pragma once

#include <OpenPoseWrapper/OpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/TrackingOpenPoseEvent.h>
#include <Utils/Database.h>
#include <Utils/Vector.h>

class PeopleCounterOpenPoseEvent : public OpenPoseEvent
{
public:
	std::shared_ptr<TrackingOpenPoseEvent> tracker;
	std::shared_ptr<SqlOpenPoseEvent> sql;

	PeopleCounterOpenPoseEvent(
		const std::shared_ptr<TrackingOpenPoseEvent>& tracker,
		float lineStartX, float lineStartY, float lineEndX, float lineEndY, float lineWeigth,
		bool drawInfomation
	) :
		tracker{ tracker }, sql{ tracker->sql }, drawInfomation{ drawInfomation }
	{
		float lineVecX = lineEndX - lineStartX;
		float lineVecY = lineEndY - lineStartY;
		float lineVecLength = std::sqrt(lineVecX * lineVecX + lineVecY * lineVecY);
		float lineNormalX = -lineVecY / lineVecLength;
		float lineNormalY = lineVecX / lineVecLength;
		float moveX = lineNormalX * lineWeigth * 0.5f;
		float moveY = lineNormalY * lineWeigth * 0.5f;
		lines.clear();
		lines.push_back(Line{ lineStartX - moveX, lineStartY - moveY, lineEndX - moveX, lineEndY - moveY });
		lines.push_back(Line{ lineStartX + moveX, lineStartY + moveY, lineEndX + moveX, lineEndY + moveY });
	}
	virtual ~PeopleCounterOpenPoseEvent() {};

	int init() override final
	{
		if (checkError()) return 1;

		// people_countテーブルが存在しない場合はテーブルを生成
		std::string row_title = u8"frame INTEGER PRIMARY KEY, static_up INTEGER, static_down INTEGER, dynamic_up INTEGER, dynamic_down INTEGER";
		if (sql->createTableIfNoExist(u8"people_count", row_title)) return 1;
		if (sql->createIndexIfNoExist(u8"people_count", u8"frame", true)) return 1;

		return 0;
	}
	int sendImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		if (checkError()) return 1;
		return 0;
	}
	int recieveImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		if (checkError()) return 1;

		try
		{
			// SQLからデータを取得
			auto&& sqlData = getCountFromSql(imageInfo.frameNumber);

			// SQLにデータが存在しなかった場合は新たにカウントを取得する
			if (std::get<0>(sqlData) == -1)
			{
				// トラッキングが外れていない人のカウンタをリセット
				dynamicUpCount = 0;
				dynamicDownCount = 0;

				// トラッキングが外れていない人の移動方向カウント
				for (auto currentPerson = tracker->currentPeople.begin(); currentPerson != tracker->currentPeople.end(); currentPerson++)
				{
					auto e = judgeUpOrDown(tracker->firstPeople[currentPerson->first], currentPerson->second, lines);
					if (e == Event::UP) dynamicUpCount++;
					if (e == Event::DOWN) dynamicDownCount++;
				}

				// トラッキングが外れた人の移動方向カウント
				for (auto&& index : tracker->untrackedPeopleIndex)
				{
					auto e = judgeUpOrDown(tracker->firstPeople[index], tracker->backPeople[index], lines);
					if (e == Event::UP) staticUpCount++;
					if (e == Event::DOWN) staticDownCount++;
				}

				// SQLにデータを追加
				SQLite::Statement insertQuery(*(sql->database), u8"INSERT INTO people_count VALUES(?, ?, ?, ?, ?)");
				sql->bindAllAndExec(
					insertQuery,
					(int64_t)imageInfo.frameNumber,
					(int64_t)staticUpCount,
					(int64_t)staticDownCount,
					(int64_t)dynamicUpCount,
					(int64_t)dynamicDownCount
				);
			}
			else
			{
				staticUpCount = std::get<0>(sqlData);
				staticDownCount = std::get<1>(sqlData);
				dynamicUpCount = std::get<2>(sqlData);
				dynamicDownCount = std::get<3>(sqlData);
			}
		}
		catch (const std::exception& e)
		{
			std::cout << "error : " << __FILE__ << " : L" << __LINE__ << "\n" << e.what() << std::endl;
			return 1;
		}
		

		// トラッキングの描画を行わない場合はここで終了
		if (!drawInfomation) return 0;

		// 基準線の描画
		for (auto line : lines)
		{
			cv::line(
				imageInfo.outputImage,
				{ (int)line.lineStartX, (int)line.lineStartY },
				{ (int)line.lineEndX, (int)line.lineEndY },
				cv::Scalar{ 255.0, 255.0, 255.0 }, 2
			);
		}

		// トラッキングの始点と終点を結ぶ直線を描画
		for (auto currentPerson = tracker->currentPeople.begin(); currentPerson != tracker->currentPeople.end(); currentPerson++)
		{
			size_t index = currentPerson->first;
			auto&& firstPosition = TrackingOpenPoseEvent::getJointAverage(tracker->firstPeople[index]);
			auto&& currentPosition = TrackingOpenPoseEvent::getJointAverage(currentPerson->second);

			// 直線の描画
			cv::line(
				imageInfo.outputImage,
				{ (int)firstPosition.x, (int)firstPosition.y },
				{ (int)currentPosition.x, (int)currentPosition.y },
				cv::Scalar{
					(double)((int)((std::sin(((double)index) * 463763.0) + 1.0) * 100000.0) % 120 + 80),
					(double)((int)((std::sin(((double)index) * 1279.0) + 1.0) * 100000.0) % 120 + 80),
					(double)((int)((std::sin(((double)index) * 92763.0) + 1.0) * 100000.0) % 120 + 80)
				}, 2.0
			);

			// idの描画
			gui::text(
				imageInfo.outputImage, std::to_string(index),
				{ (int)currentPosition.x, (int)currentPosition.y },
				gui::CENTER_CENTER, 0.5
			);
		}

		// カウントを表示
		gui::text(imageInfo.outputImage, std::string("up : ") + std::to_string(getUpCount()), { 20, 200 });
		gui::text(imageInfo.outputImage, std::string("down : ") + std::to_string(getDownCount()), { 20, 230 });

		return 0;
	}

	// 基準線を上方向に移動した人のカウントを取得
	inline uint64_t getUpCount() { return staticUpCount + dynamicUpCount; }

	// 基準線を下方向に移動した人のカウントを取得
	inline uint64_t getDownCount() { return staticDownCount + dynamicDownCount; }

private:
	// トラッキングが外れた人のカウンタ(最終的な移動方向が確定している)
	uint64_t staticUpCount = 0;  // 直線の上を上側に移動した人のカウント
	uint64_t staticDownCount = 0;  // 直線の上を下側に移動した人のカウント

	// トラッキングが外れていない人のカウンタ
	uint64_t dynamicUpCount = 0;  // 直線の上を上側に移動した人のカウント
	uint64_t dynamicDownCount = 0;  // 直線の上を下側に移動した人のカウント

	// カウント情報を画面に描画するかどうか
	bool drawInfomation;

	int checkError()
	{
		if (!tracker)
		{
			std::cout
				<< "TrackingOpenPoseEventが未指定です。\n"
				<< "コンストラクタで正しい値を指定してください。"
				<< std::endl;
			return 1;
		}
		return 0;
	}

	// 直線の始点と終点
	struct Line {
		float lineStartX, lineStartY, lineEndX, lineEndY;
	};

	// 人数カウントを行う基準線
	std::vector<Line> lines;

	enum Event { UP, DOWN, NOTHING };

	// p1Startからp1Endまでを結ぶ直線とp2Startからp2Endまでを結ぶ直線が交差しているかどうかを取得
	bool isCross(vt::Vector4& p1Start, vt::Vector4& p1End, vt::Vector4& p2Start, vt::Vector4& p2End)
	{
		// p1Startからp1Endへの直線とp2Startからp2Endへの直線が交差しているかどうかを求める
		// 参考 : https://imagingsolution.blog.fc2.com/blog-entry-137.html
		double s1 = ((p2End.x - p2Start.x) * (p1Start.y - p2Start.y) - (p2End.y - p2Start.y) * (p1Start.x - p2Start.x)) / 2.0;
		double s2 = ((p2End.x - p2Start.x) * (p2Start.y - p1End.y) - (p2End.y - p2Start.y) * (p2Start.x - p1End.x)) / 2.0;
		if (s1 + s2 == 0.0) return false;
		double p = s1 / (s1 + s2);
		return (0.0 <= p && p <= 1.0);
	}

	// peopleStartからpeopleEndまでを結ぶ直線がlineと交差しているかどうか取得
	// さらに、交差している場合はどちらの方向に交差しているのかを取得
	Event judgeUpOrDown(std::vector<ImageInfo::Node>& peopleStart, std::vector<ImageInfo::Node>& peopleEnd, Line& line)
	{
		auto startPos = TrackingOpenPoseEvent::getJointAverage(peopleStart);  // 歩行者のトラッキングを開始した点
		auto endPos = TrackingOpenPoseEvent::getJointAverage(peopleEnd);  // 歩行者のトラッキングを終了した点
		auto vecLine = vt::Vector4((double)line.lineEndX - (double)line.lineStartX, (double)line.lineEndY - (double)line.lineStartY);  // 人数カウントを行う基準線のベクトル
		auto vecStart = vt::Vector4((double)startPos.x - (double)line.lineStartX, (double)startPos.y - (double)line.lineStartY);  // 人数カウントを行う基準線の始点からstartPosへのベクトル
		auto vecEnd = vt::Vector4((double)endPos.x - (double)line.lineStartX, (double)endPos.y - (double)line.lineStartY);  // 人数カウントを行う基準線の始点からvecEndへのベクトル
		if (!isCross(vt::Vector4(0.0, 0.0), vecLine, vecStart, vecEnd)) return Event::NOTHING;  // startからendを結ぶ直線がvecLineの上を通過していない場合
		// 「vecLineを90度回転させた線」と「vecStart」との内積をもとに、startPosがvecLineの上側にあるかどうかを判定
		bool startIsUp = vecStart.x * vecLine.y > vecStart.y * vecLine.x;
		// 「vecLineを90度回転させた線」と「vecEnd」との内積をもとに、endPosがvecLineの上側にあるかどうかを判定
		bool endIsUp = vecEnd.x * vecLine.y > vecEnd.y * vecLine.x;
		if ((!startIsUp) && endIsUp) return Event::UP;  // 歩行者のトラッキングが人数カウントを行う基準線を超えて上に移動していた場合
		if (startIsUp && (!endIsUp)) return Event::DOWN;  // 歩行者のトラッキングが人数カウントを行う基準線を超えて下に移動していた場合
		return Event::NOTHING;  // それ以外
	}

	// peopleStartからpeopleEndまでを結ぶ直線が全てのlinesと同一方向に交差しているかどうか取得
	// さらに、交差している場合はどちらの方向に交差しているのかを取得
	Event judgeUpOrDown(std::vector<ImageInfo::Node>& peopleStart, std::vector<ImageInfo::Node>& peopleEnd, std::vector<Line>& lines)
	{
		Event e = Event::NOTHING;
		for (size_t i = 0; i < lines.size(); i++)
		{
			if (i == 0) e = judgeUpOrDown(peopleStart, peopleEnd, lines[i]);
			else if (e != judgeUpOrDown(peopleStart, peopleEnd, lines[i])) return Event::NOTHING;
		}
		return e;
	}

	// 指定されたフレーム番号のデータをSQLから取得する
	// 存在しない場合は-1で満たされたtupleを返す
	std::tuple<int, int, int, int> getCountFromSql(size_t frame)
	{
		if (!sql->isDataExist(u8"people_count", u8"frame", frame)) return std::make_tuple(-1, -1, -1, -1);
		SQLite::Statement countQuery(*(sql->database), u8"SELECT * FROM people_count WHERE frame=?");
		countQuery.bind(1, (long long)frame);
		(void)countQuery.executeStep();

		return std::make_tuple(
			countQuery.getColumn(1).getInt(),
			countQuery.getColumn(2).getInt(),
			countQuery.getColumn(3).getInt(),
			countQuery.getColumn(4).getInt()
		);
	}
};