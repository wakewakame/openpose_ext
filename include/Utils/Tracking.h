#pragma once

#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <Utils/SqlOpenPose.h>
#include <Utils/Database.h>
#include <optional>

class Tracking
{
private:
	using People = MinOpenPose::People;
	using Node = MinOpenPose::Node;

public:
	// (numberFramesToLost - 1)フレーム前から現在のフレームまでの間で検出された最も新しい全ての骨格
	People currentPeople;

	// 1フレーム前のcurrentPeopleの複製
	People backPeople;

	// currentPeopleもしくはbackPeopleに存在する骨格が初めて画面に映りこんだときの骨格
	People firstPeople;

	// 現在のフレームで取得できた全ての骨格
	People latestPeople;

	// backPeopleには存在するがcurrentPeopleには存在しない人全てのインデックス
	std::vector<size_t> untrackedPeopleIndex;

	/**
	 * OpenPoseではフレームごとに人のIDが変動するため、このクラスではOpenPoseで得られた骨格のトラッキングを行う
	 * @param confidenceThreshold 関節の信頼値がこの値以下である場合は、関節が存在しないものとして処理する
	 * @param numberNodesToTrust 信頼値がconfidenceThresholdより大きい関節の数がこの値未満である場合は、その人がいないものとして処理する
	 * @param numberFramesToLost 一度トラッキングが外れた人がこのフレーム数が経過しても再発見されない場合は、消失したものとして処理する
	 * @param distanceThreshold トラッキング中の人が1フレーム進んだとき、移動距離がこの値よりも大きい場合は同一人物の候補から外す
	 */
	Tracking(
		float confidenceThreshold = 0.5f,
		uint64_t numberNodesToTrust = 5,
		uint64_t numberFramesToLost = 10,
		float distanceThreshold = 50.0f
	) :
		confidenceThreshold{ confidenceThreshold },
		numberNodesToTrust{ numberNodesToTrust },
		numberFramesToLost{ numberFramesToLost },
		distanceThreshold{ distanceThreshold },
		currentPeople{},
		backPeople{},
		firstPeople{},
		untrackedPeopleIndex{}
	{
	}

	virtual ~Tracking() {};

	/**
	 * トラッキングを行う
	 * @param sql SqlOpenPoseのインスタンスを入れる
	 * @param frameNumber 現在再生中の動画のフレーム番号を指定する
	 * @param Peopleのインスタンスを入れる
	 */
	std::optional<People> tracking(const People& people, SqlOpenPose& sql, const size_t frameNumber)
	{
		// people_with_trackingテーブルが存在しない場合はテーブルを生成
		std::string row_title = u8"frame INTEGER, people INTEGER";
		for (int i = 0; i < 25; i++)
		{
			row_title += u8", joint" + std::to_string(i) + u8"x REAL";
			row_title += u8", joint" + std::to_string(i) + u8"y REAL";
			row_title += u8", joint" + std::to_string(i) + u8"confidence REAL";
		}
		if (sql.createTableIfNoExist(u8"people_with_tracking", row_title)) return std::nullopt;

		// SQLの検索を高速化するためにIndexを作成
		if (sql.createIndexIfNoExist(u8"people_with_tracking", u8"frame", false)) return std::nullopt;
		if (sql.createIndexIfNoExist(u8"people_with_tracking", u8"people", false)) return std::nullopt;
		if (sql.createIndexIfNoExist(u8"people_with_tracking", u8"frame", u8"people", true)) return std::nullopt;

		try
		{
			// SQLから必要な骨格情報を取得
			if (getPeopleFromSql(sql, frameNumber)) return std::nullopt;

			// 検出された骨格がなければ終了
			if (people.size() == 0) return people;

			// すでにSQLに骨格データが存在すれば終了
			if (isDataExist(sql, frameNumber)) return currentPeople;

			// 現在のフレームの人数分ループ
			std::map<size_t, bool> usedIndex;
			for (auto currentPerson = people.begin(); currentPerson != people.end(); currentPerson++)
			{
				// 骨格データの信頼度が閾値未満であればスキップ
				auto&& currentNodes = currentPerson->second;
				uint64_t confidenceCount = 0;
				for (auto&& node : currentNodes) { if (node.confidence > confidenceThreshold) confidenceCount++; }
				if (confidenceCount < numberNodesToTrust) continue;

				// 前フレームの中で一番距離が近かった人のインデックスを取得
				uint64_t nearestPeopleIndex = 0;  // 前フレームの中で一番距離が近かった人のインデックスを格納する一時変数
				float nearestLength = -1.0f;  // 前フレームの中で一番距離が近かった長さを格納する一時変数
				bool lostFlag = true;  // 前フレームで一番距離が近かった人が検出できなかった場合にTrueになるフラグ

				// 前フレームの人数分ループ
				for (auto backPerson = backPeople.begin(); backPerson != backPeople.end(); backPerson++)
				{
					uint64_t index = backPerson->first;  // インデックスの取得

					if (usedIndex.count(index)) continue;  // インデックスが既に使用されていた場合はスキップ

					auto&& backNodes = backPerson->second;  // 骨格情報の取得

					// 前フレームからの移動距離を算出
					float distance = getDistance(backNodes, currentNodes);

					// 移動距離が正常に算出できなかった、もしくはdistanceThresholdより大きい値であった場合は除外
					if ((distance < 0.0f) || (distance > distanceThreshold)) continue;

					// 記録更新判定
					if (lostFlag || (distance < nearestLength))
					{
						nearestPeopleIndex = index;  // 一番距離が近い人のインデックスを更新
						nearestLength = distance;  // その距離を更新
						lostFlag = false;  // フラグを折る
					}
				}

				// 前フレームで一番距離が近かった人が検出できた場合はその人のインデックスを求める
				uint64_t addIndex = nearestPeopleIndex;

				// 前フレームで一番距離が近かった人が検出できなかった場合は新しいインデックスを求める
				if (lostFlag)
				{
					// 現在SQLに登録された人の総数を取得
					SQLite::Statement peopleCountQuery(*(sql.database), "SELECT COUNT(DISTINCT people) from people_with_tracking");
					(void)peopleCountQuery.executeStep();
					addIndex = peopleCountQuery.getColumn(0).getInt();
				}

				// 使用済みインデックスへ追加
				usedIndex[addIndex] = true;

				// SQL文の生成
				std::string row = u8"?";
				for (int colIndex = 0; colIndex < 76; colIndex++) row += u8", ?";
				row = u8"INSERT INTO people_with_tracking VALUES (" + row + u8")";
				SQLite::Statement insertQuery(*(sql.database), row);

				// 現在のフレームで検出された全ての骨格データをSQLに追記
				insertQuery.reset();
				insertQuery.bind(1, (long long)frameNumber);
				insertQuery.bind(2, (long long)addIndex);
				for (size_t nodeIndex = 0; nodeIndex < currentNodes.size(); nodeIndex++)
				{
					insertQuery.bind(3 + nodeIndex * 3 + 0, (double)currentNodes[nodeIndex].x);
					insertQuery.bind(3 + nodeIndex * 3 + 1, (double)currentNodes[nodeIndex].y);
					insertQuery.bind(3 + nodeIndex * 3 + 2, (double)currentNodes[nodeIndex].confidence);
				}
				(void)insertQuery.exec();
			}

			// 再度SQLから必要な骨格情報を取得
			if (getPeopleFromSql(sql, frameNumber)) return std::nullopt;
		}
		catch (const std::exception& e)
		{
			std::cout << "error : " << __FILE__ << " : L" << __LINE__ << "\n" << e.what() << std::endl;
			return std::nullopt;
		}

		return currentPeople;
	}

	// 指定されたフレーム番号のデータがSQLに存在するかどうか
	bool isDataExist(const SqlOpenPose& sql, size_t frame) const {
		return sql.isDataExist(u8"people_with_tracking", u8"frame", frame);
	}

	// 骨格の重心を取得する
	static Node getJointAverage(const std::vector<Node>& person)
	{
		Node result{ 0.0f, 0.0f, 1.0f };
		float confidenceSum = 0.0f;
		for (auto node : person)
		{
			// 信頼値が0の骨格は座標が(0, 0)になっているため除外する
			if (node.confidence == 0.0f) continue;
			result.x += node.x * node.confidence;
			result.y += node.y * node.confidence;
			confidenceSum += node.confidence;
		}
		// 0割りを避ける
		if (confidenceSum == 0.0f) return Node{ 0.0f, 0.0f, 0.0f };
		result.x /= confidenceSum;
		result.y /= confidenceSum;
		return result;
	}

private:
	// 関節の信頼値がこの値以下である場合は、関節が存在しないものとして処理する
	float confidenceThreshold;

	// 信頼値がconfidenceThresholdより大きい関節の数がこの値未満である場合は、その人がいないものとして処理する
	uint64_t numberNodesToTrust;

	// 一度トラッキングが外れた人がこのフレーム数が経過しても再発見されない場合は、消失したものとして処理する
	uint64_t numberFramesToLost;

	// トラッキング中の人が1フレーム進んだとき、移動距離がこの値よりも大きい場合は同一人物の候補から外す
	float distanceThreshold;

	// SQLに保存されているfirstFrameNumberフレームからendFrameNumberフレームまでの間に検出された最も新しい全ての骨格をpeopleに代入
	int getLatestPeopleFromSql(const SqlOpenPose& sql, std::map<size_t, std::vector<Node>>& people, int64_t firstFrameNumber, int64_t endFrameNumber)
	{
		try
		{
			firstFrameNumber = (firstFrameNumber < 0) ? 0 : firstFrameNumber;
			endFrameNumber = (endFrameNumber < 0) ? 0 : endFrameNumber;
			SQLite::Statement peopleQuery(*(sql.database), u8"SELECT * FROM people_with_tracking WHERE ? <= frame AND frame <= ? GROUP BY people HAVING frame = MAX(frame)");
			if (sql.bindAll(peopleQuery, firstFrameNumber, endFrameNumber)) return 1;
			while (peopleQuery.executeStep())
			{
				size_t index = (size_t)peopleQuery.getColumn(1).getInt64();
				for (int nodeIndex = 0; nodeIndex < 25; nodeIndex++)
				{
					people[index].push_back(Node{
						(float)peopleQuery.getColumn(2 + nodeIndex * 3 + 0).getDouble(),
						(float)peopleQuery.getColumn(2 + nodeIndex * 3 + 1).getDouble(),
						(float)peopleQuery.getColumn(2 + nodeIndex * 3 + 2).getDouble()
					});
				}
			}
		}
		catch (const std::exception& e)
		{
			std::cout << "error : " << __FILE__ << " : L" << __LINE__ << "\n" << e.what() << std::endl;
			return 1;
		}

		return 0;
	}

	// SQLに保存されているfirstFrameNumberフレームからendFrameNumberフレームまでの間に検出された全ての骨格の、最初に画面に映った時の骨格をpeopleに代入
	int getOldestPeopleFromSql(const SqlOpenPose& sql, std::map<size_t, std::vector<Node>>& people, int64_t firstFrameNumber, int64_t endFrameNumber)
	{
		try
		{
			firstFrameNumber = (firstFrameNumber < 0) ? 0 : firstFrameNumber;
			endFrameNumber = (endFrameNumber < 0) ? 0 : endFrameNumber;
			SQLite::Statement peopleQuery(*(sql.database), u8"SELECT * FROM people_with_tracking WHERE people IN (SELECT people FROM people_with_tracking WHERE ? <= frame AND frame <= ?) GROUP BY people HAVING frame=MIN(frame)");
			if (sql.bindAll(peopleQuery, firstFrameNumber, endFrameNumber)) return 1;
			while (peopleQuery.executeStep())
			{
				size_t index = (size_t)peopleQuery.getColumn(1).getInt64();
				for (int nodeIndex = 0; nodeIndex < 25; nodeIndex++)
				{
					people[index].push_back(Node{
						(float)peopleQuery.getColumn(2 + nodeIndex * 3 + 0).getDouble(),
						(float)peopleQuery.getColumn(2 + nodeIndex * 3 + 1).getDouble(),
						(float)peopleQuery.getColumn(2 + nodeIndex * 3 + 2).getDouble()
					});
				}
			}
		}
		catch (const std::exception& e)
		{
			std::cout << "error : " << __FILE__ << " : L" << __LINE__ << "\n" << e.what() << std::endl;
			return 1;
		}

		return 0;
	}

	// SQLからcurrentPeople, backPeople, currentPeopleFirst, untrackedPeopleIdを取得
	int getPeopleFromSql(const SqlOpenPose& sql, size_t frame)
	{
		// 初期化
		backPeople.clear();
		currentPeople.clear();
		firstPeople.clear();
		latestPeople.clear();
		untrackedPeopleIndex.clear();

		// numberFramesToLostフレーム前から1フレーム前までの間で検出された最も新しい骨格を全て取得
		if (getLatestPeopleFromSql(
			sql, backPeople,
			(int64_t)frame - (int64_t)numberFramesToLost,
			(int64_t)frame - 1
		)) return 1;

		// (numberFramesToLost - 1)フレーム前から現在のフレームまでの間で検出された最も新しい骨格を全て取得
		if (getLatestPeopleFromSql(
			sql, currentPeople,
			(int64_t)frame - ((int64_t)numberFramesToLost - 1),
			(int64_t)frame
		)) return 1;

		// numberFramesToLostフレーム前から現在のフレームまでの間で検出された最も古い骨格を全て取得
		if (getOldestPeopleFromSql(
			sql, firstPeople,
			(int64_t)frame - (int64_t)numberFramesToLost,
			(int64_t)frame
		)) return 1;

		// 現在のフレームで取得できた人全てのインデックスを取得
		if (getLatestPeopleFromSql(sql, latestPeople, (int64_t)frame, (int64_t)frame)) return 1;

		// backPeopleには存在するがcurrentPeopleには存在しない人全てのインデックスを取得
		for (auto backPerson = backPeople.begin(); backPerson != backPeople.end(); backPerson++)
		{
			if (currentPeople.count(backPerson->first) == 0) untrackedPeopleIndex.push_back(backPerson->first);
		}

		return 0;
	}

	// 2つの骨格の各関節の距離差の平均を取得(信頼度がconfidenceThreshold以下の関節は計算から除外される)
	// 成功すると0.0f以上の値が返される
	// 全ての関節の信頼度がconfidenceThreshold以下だった場合は-1.0fが返される
	float getDistance(const std::vector<Node>& nodes1, const std::vector<Node>& nodes2)
	{
		uint64_t samples = 0;  // 有効な関節のサンプル数
		float distance = 0.0f;  // 有効な全関節の移動量の平均
		for (uint64_t index = 0; index < nodes1.size(); index++)
		{
			// 閾値以下の関節は無効
			if ((nodes1[index].confidence <= confidenceThreshold) || (nodes2[index].confidence <= confidenceThreshold)) continue;
			float x = nodes1[index].x - nodes2[index].x;
			float y = nodes1[index].y - nodes2[index].y;
			distance += std::sqrtf((x * x) + (y * y));
			samples++;
		}
		return (samples == 0) ? (-1.0f) : (distance / (float)samples);
	}
};