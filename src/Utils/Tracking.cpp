#include <Utils/Tracking.h>

namespace op
{
	Node::Node() :
		x{ 0.0f },
		y{ 0.0f },
		confidence{ 0.0f },
		confidenceThreshold{ 0.5f }
	{
	}

	Node::Node(
		float x,
		float y,
		float confidence = 0.0f,
		float confidenceThreshold = 0.5f
	) :
		x{ x },
		y{ y },
		confidence{ confidence },
		confidenceThreshold{ confidenceThreshold }
	{
	}

	Node::Node(const Node& node) :
		x{ node.x },
		y{ node.y },
		confidence{ node.confidence },
		confidenceThreshold{ node.confidenceThreshold }
	{
	}

	bool Node::isTrusted()
	{
		return confidence > confidenceThreshold;
	}

	float Node::distanceFrom(const Node& node)
	{
		float x = this->x - node.x;
		float y = this->y - node.y;
		return std::sqrtf((x * x) + (y * y));
	}

	Tree::Tree() :
		frameNumber{ 0 },
		numberNodesToTrust{ 5 },
		confidenceThreshold{ 0.5 },
		valid{ false }
	{
	}

	Tree::Tree(
		std::vector<Node> nodes,
		uint64_t frameNumber = 0,
		uint64_t numberNodesToTrust = 5,
		float confidenceThreshold = 0.5,
		bool valid = true
	) :
		nodes{ nodes },
		frameNumber{ frameNumber },
		numberNodesToTrust{ numberNodesToTrust },
		confidenceThreshold{ confidenceThreshold },
		valid{ valid }
	{
	}

	Tree::Tree(const Tree& tree) :
		nodes{ tree.nodes },
		frameNumber{ tree.frameNumber },
		numberNodesToTrust{ tree.numberNodesToTrust },
		confidenceThreshold{ tree.confidenceThreshold },
		valid{ tree.valid }
	{
	}

	bool Tree::isTrusted()
	{
		uint64_t count = 0;
		for (auto node : nodes)
		{
			if (node.isTrusted()) count++;
		}
		return count >= numberNodesToTrust;
	}

	float Tree::distanceFrom(std::vector<Node>& nodes)
	{
		uint64_t samples = 0;
		float distance = 0.0f;
		for (uint64_t index = 0; index < nodes.size(); index++)
		{
			if (this->nodes[index].isTrusted() && nodes[index].isTrusted())
			{
				distance += this->nodes[index].distanceFrom(nodes[index]);
				samples++;
			}
		}
		return (samples == 0) ? (-1.0) : (distance / samples);
	}

	Node Tree::average()
	{
		Node result{ 0.0f, 0.0f, 1.0f };
		uint64_t samples = 0;
		for (auto node : nodes)
		{
			if (!node.isTrusted()) continue;
			result.x += node.x;
			result.y += node.y;
			samples++;
		}
		if (samples == 0) return Node{ 0.0f, 0.0f, 0.0f };
		result.x /= (float)samples;
		result.y /= (float)samples;
		return result;
	}

	bool Tree::isValid() {
		return valid;
	}

	PeopleList::PeopleList() :
		numberNodesToTrust{ 5 },
		confidenceThreshold{ 0.5f },
		numberFramesToLost{ 10 },
		distanceThreshold{ 50.0f }
	{
	}

	PeopleList::PeopleList(
		uint64_t numberNodesToTrust = 5,
		float confidenceThreshold = 0.5f,
		uint64_t numberFramesToLost = 10,
		float distanceThreshold = 50.0f
	) :
		numberNodesToTrust{ numberNodesToTrust },
		confidenceThreshold{ confidenceThreshold },
		numberFramesToLost{ numberFramesToLost },
		distanceThreshold{ distanceThreshold }
	{
	}

	void PeopleList::addFrame(ImageInfo& imageInfo)
	{
		backTrees = currentTrees;  // 1フレーム前に映っていた人のすべての骨格情報を一時保存
		currentTrees.clear();  // すべての人の骨格情報を初期化
		// 現在のフレームの人数分ループ
		for (auto person = imageInfo.people.begin(); person != imageInfo.people.end(); person++)
		{
			std::vector<Node> nodes;  // 1人分の骨格情報を格納する配列
			// poseKeypointsからnodesへ骨格情報をコピー
			for (auto node : person->second)
			{
				nodes.push_back(Node(
					node.x, node.y, node.confidence,
					confidenceThreshold
				));
			}
			Tree tree = Tree(
				nodes,
				imageInfo.frameNumber,
				numberNodesToTrust,
				confidenceThreshold
			);
			if (tree.average().confidence == 0.0f) continue;
			// 前フレームの中で一番距離が近かった人のインデックスを取得
			uint64_t nearestPeopleIndex = 0;  // 前フレームの中で一番距離が近かった人のインデックスを格納する一時変数
			float nearestLength = -1.0f;  // 前フレームの中で一番距離が近かった長さを格納する一時変数
			bool lostFlag = true;  // 前フレームで一番距離が近かった人が検出できなかった場合にTrueになるフラグ
			// 前フレームの人数分ループ
			for (auto treeItr = backTrees.begin(); treeItr != backTrees.end(); treeItr++)
			{
				uint64_t index = treeItr->first;  // インデックスの取得
				Tree tree_ = treeItr->second;  // 骨格情報の取得
				// 骨格情報がnumberFramesToLostフレーム以上前のフレームであれば除外する
				if (imageInfo.frameNumber - tree_.frameNumber > numberFramesToLost) continue;
				// 骨格情報が(numberFramesToLost+1)フレーム以上前のフレームであればfirstTreesから除去する
				if (imageInfo.frameNumber - tree_.frameNumber > (numberFramesToLost + 1))
				{
					if (firstTrees.count(index) != 0) firstTrees.erase(index);
				}
				// 一旦骨格情報を現在のフレームに追加する
				if (currentTrees.count(index) == 0) currentTrees[index] = tree_;
				// 前フレームからの移動距離を算出
				float distance = tree_.distanceFrom(tree.nodes);
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
			// 前フレームで一番距離が近かった人が検出できた場合はその人のインデックスを、そうでない場合は新しいインデックスを求める
			uint64_t addIndex = lostFlag ? (sumOfPeople++) : nearestPeopleIndex;
			// 現在のフレームに人を追加
			currentTrees[addIndex] = tree;
			// 初めて登場する人はfirstTreeにも追加
			if (lostFlag) firstTrees[addIndex] = currentTrees[addIndex];
		}
	}
	void PeopleList::addFrame(ImageInfo& imageInfo, std::shared_ptr<Database>& database)
	{
		// 現在のフレームの骨格検出数が0なら終了
		if (imageInfo.people.size() == 0) return;

		// 現在のフレームのデータがに既にsqlに存在していれば終了
		if (database->isDataExist(u8"people_with_tracking", u8"frame", imageInfo.frameNumber)) return;

		// 1フレーム前からnumberFramesToLostフレーム前までの最新の骨格を取得
		SQLite::Statement query(*(database->database), u8"SELECT * FROM people_with_tracking WHERE ? <= frame AND frame < ? GROUP BY people HAVING frame = MAX(frame)");
		database->bindAll(query, (int64_t)imageInfo.frameNumber - (int64_t)numberFramesToLost, (int64_t)imageInfo.frameNumber);
		std::map<size_t, std::vector<ImageInfo::Node>> backPeople;
		while (query.executeStep())
		{
			size_t index = (size_t)query.getColumn(1).getInt64();
			for (int nodeIndex = 0; nodeIndex < 25; nodeIndex++)
			{
				backPeople[index].push_back(ImageInfo::Node{
					(float)query.getColumn(2 + nodeIndex * 3 + 0).getDouble(),
					(float)query.getColumn(2 + nodeIndex * 3 + 1).getDouble(),
					(float)query.getColumn(2 + nodeIndex * 3 + 2).getDouble()
				});
			}
		}

		// SQL文の生成
		std::string row = u8"?";
		for (int colIndex = 0; colIndex < 76; colIndex++) row += u8", ?";
		row = u8"INSERT INTO people_with_tracking VALUES (" + row + u8")";
		SQLite::Statement pwtQuery(*(database->database), row);

		// 現在のフレームの人数分ループ
		for (auto currentPerson = imageInfo.people.begin(); currentPerson != imageInfo.people.end(); currentPerson++)
		{
			// 骨格データの信頼度が閾値未満であればスキップ
			auto&& currentNodes = currentPerson->second;
			uint64_t confidenceCount = 0;
			for (auto node : currentNodes) { if (node.confidence > confidenceThreshold) confidenceCount++; }
			if (confidenceCount < numberNodesToTrust) continue;

			// 前フレームの中で一番距離が近かった人のインデックスを取得
			uint64_t nearestPeopleIndex = 0;  // 前フレームの中で一番距離が近かった人のインデックスを格納する一時変数
			float nearestLength = -1.0f;  // 前フレームの中で一番距離が近かった長さを格納する一時変数
			bool lostFlag = true;  // 前フレームで一番距離が近かった人が検出できなかった場合にTrueになるフラグ

			// 前フレームの人数分ループ
			for (auto&& backPerson = backPeople.begin(); backPerson != backPeople.end(); backPerson++)
			{
				uint64_t index = backPerson->first;  // インデックスの取得
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
				SQLite::Statement peopleCountQuery(*(database->database), "SELECT COUNT(DISTINCT people) from people_with_tracking");
				(void)peopleCountQuery.executeStep();
				addIndex = peopleCountQuery.getColumn(0).getInt();
			}

			// 現在のフレームに人を追加
			pwtQuery.reset();
			pwtQuery.bind(1, (long long)imageInfo.frameNumber);
			pwtQuery.bind(2, (long long)addIndex);
			for (size_t nodeIndex = 0; nodeIndex < currentNodes.size(); nodeIndex++)
			{
				pwtQuery.bind(3 + nodeIndex * 3 + 0, (double)currentNodes[nodeIndex].x);
				pwtQuery.bind(3 + nodeIndex * 3 + 1, (double)currentNodes[nodeIndex].y);
				pwtQuery.bind(3 + nodeIndex * 3 + 2, (double)currentNodes[nodeIndex].confidence);
			}
			(void)pwtQuery.exec();
		}
	}

	std::vector<uint64_t> PeopleList::getCurrentIndices()
	{
		std::vector<uint64_t> result;
		for (auto treeItr = currentTrees.begin(); treeItr != currentTrees.end(); treeItr++)
		{
			result.push_back(treeItr->first);
		}
		return result;
	}

	std::vector<uint64_t> PeopleList::getBackIndices()
	{
		std::vector<uint64_t> result;
		for (auto treeItr = backTrees.begin(); treeItr != backTrees.end(); treeItr++)
		{
			result.push_back(treeItr->first);
		}
		return result;
	}

	std::vector<uint64_t> PeopleList::getLostIndices()
	{
		std::vector<uint64_t> result;
		for (auto backTreeItr = backTrees.begin(); backTreeItr != backTrees.end(); backTreeItr++)
		{
			bool lostFlag = true;
			for (auto currentTreeItr = currentTrees.begin(); currentTreeItr != currentTrees.end(); currentTreeItr++)
			{
				if (backTreeItr->first == currentTreeItr->first)
				{
					lostFlag = false;
					break;
				}
			}
			if (lostFlag) result.push_back(backTreeItr->first);
		}
		return result;
	}

	Tree PeopleList::getCurrentTree(uint64_t index) {
		if (currentTrees.count(index) == 0) return Tree();
		return currentTrees[index];
	}
	Tree PeopleList::getBackTree(uint64_t index) {
		if (backTrees.count(index) == 0) return Tree();
		return backTrees[index];
	}
	Tree PeopleList::getFirstTree(uint64_t index) {
		if (firstTrees.count(index) == 0) return Tree();
		return firstTrees[index];
	}
	float PeopleList::getDistance(const std::vector<ImageInfo::Node>& nodes1, const std::vector<ImageInfo::Node>& nodes2)
	{
		uint64_t samples = 0;  // 有効な関節のサンプル数
		float distance = 0.0f;  // 有効な全関節の移動量の平均
		for (uint64_t index = 0; index < nodes1.size(); index++)
		{
			// 閾値以下の関節は無効
			if ((nodes1[index].confidence <= confidenceThreshold) && (nodes2[index].confidence <= confidenceThreshold)) continue;
			float x = nodes1[index].x - nodes2[index].x;
			float y = nodes1[index].y - nodes2[index].y;
			distance += std::sqrtf((x * x) + (y * y));
			samples++;
		}
		return (samples == 0) ? (-1.0) : (distance / samples);
	}

	bool PeopleLineCounter::isCross(vt::Vector4& p1Start, vt::Vector4& p1End, vt::Vector4& p2Start, vt::Vector4& p2End)
	{
		// p1Startからp1Endへの直線とp2Startからp2Endへの直線が交差しているかどうかを求める
		// 参考 : https://imagingsolution.blog.fc2.com/blog-entry-137.html
		double s1 = ((p2End.x - p2Start.x) * (p1Start.y - p2Start.y) - (p2End.y - p2Start.y) * (p1Start.x - p2Start.x)) / 2.0;
		double s2 = ((p2End.x - p2Start.x) * (p2Start.y - p1End.y) - (p2End.y - p2Start.y) * (p2Start.x - p1End.x)) / 2.0;
		if (s1 + s2 == 0.0) return false;
		double p = s1 / (s1 + s2);
		return (0.0 <= p && p <= 1.0);
	}

	PeopleLineCounter::Event PeopleLineCounter::judgeUpOrDown(op::Tree& peopleStart, op::Tree& peopleEnd, Line& line)
	{
		auto startPos = peopleStart.average();  // 歩行者のトラッキングを開始した点
		auto endPos = peopleEnd.average();  // 歩行者のトラッキングを終了した点
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

	PeopleLineCounter::Event PeopleLineCounter::judgeUpOrDown(op::Tree& peopleStart, op::Tree& peopleEnd)
	{
		Event e = Event::NOTHING;
		for (size_t i = 0; i < lines.size(); i++)
		{
			if (i == 0) e = judgeUpOrDown(peopleStart, peopleEnd, lines[i]);
			else if (e != judgeUpOrDown(peopleStart, peopleEnd, lines[i])) return Event::NOTHING;
		}
		return e;
	}

	PeopleLineCounter::PeopleLineCounter(float startX, float startY, float endX, float endY)
	{
		setLine(startX, startY, endX, endY);
	}

	// 骨格を更新する
	void PeopleLineCounter::updateCount(PeopleList& people)
	{
		// 1フレーム前の情報を記憶
		preUpCount = getUpCount();
		preDownCount = getDownCount();

		// トラッキングが外れていない人のカウンタをリセット
		dynamicUpCount = 0;
		dynamicDownCount = 0;

		// トラッキングが外れていない人の移動方向カウント
		const std::vector<uint64_t> currentIndex = people.getCurrentIndices();
		for (size_t index : currentIndex)
		{
			auto e = judgeUpOrDown(people.getFirstTree(index), people.getCurrentTree(index));
			if (e == Event::UP) dynamicUpCount++;
			if (e == Event::DOWN) dynamicDownCount++;
		}

		// トラッキングが外れた人の移動方向カウント
		const std::vector<uint64_t> lostIndex = people.getLostIndices();
		for (size_t index : lostIndex)
		{
			auto e = judgeUpOrDown(people.getFirstTree(index), people.getBackTree(index));
			if (e == Event::UP) staticUpCount++;
			if (e == Event::DOWN) staticDownCount++;
		}
	}

	// カウントのリセット
	void PeopleLineCounter::resetCount()
	{
		staticUpCount = 0;
		staticDownCount = 0;
		dynamicUpCount = 0;
		dynamicDownCount = 0;
	}

	// カウントの基準線の位置設定(始点X, 始点Y, 終点X, 終点Y)
	void PeopleLineCounter::setLine(float lineStartX, float lineStartY, float lineEndX, float lineEndY)
	{
		this->lines.clear();
		this->lines.push_back(Line{ lineStartX, lineStartY, lineEndX, lineEndY });
	}

	// カウントの基準線の位置設定(始点X, 始点Y, 終点X, 終点Y, 線の太さ)
	void PeopleLineCounter::setLine(float lineStartX, float lineStartY, float lineEndX, float lineEndY, float lineWeigth)
	{

		float lineVecX = lineEndX - lineStartX;
		float lineVecY = lineEndY - lineStartY;
		float lineVecLength = std::sqrt(lineVecX * lineVecX + lineVecY * lineVecY);
		float lineNormalX = -lineVecY / lineVecLength;
		float lineNormalY = lineVecX / lineVecLength;
		float moveX = lineNormalX * lineWeigth * 0.5f;
		float moveY = lineNormalY * lineWeigth * 0.5f;
		this->lines.clear();
		this->lines.push_back(Line{ lineStartX - moveX, lineStartY - moveY, lineEndX - moveX, lineEndY - moveY });
		this->lines.push_back(Line{ lineStartX + moveX, lineStartY + moveY, lineEndX + moveX, lineEndY + moveY });
	}

	// 基準線の描画
	void PeopleLineCounter::drawJudgeLine(cv::Mat& mat)
	{
		for (auto line : lines)
			cv::line(mat, { (int)line.lineStartX, (int)line.lineStartY }, { (int)line.lineEndX, (int)line.lineEndY }, cv::Scalar{ 255.0, 255.0, 255.0 }, 2);
	}

	// 人々の始点と終点を結ぶ直線の描画
	void PeopleLineCounter::drawPeopleLine(cv::Mat& mat, PeopleList& people, bool drawId)
	{
		// トラッキングの始点と終点を結ぶ直線を描画
		for (size_t index : people.getCurrentIndices())
		{
			auto firstTree = people.getFirstTree(index);
			auto currentTree = people.getCurrentTree(index);
			if ((!firstTree.isValid()) || (!currentTree.isValid())) continue;

			// 直線の描画
			cv::line(mat, { (int)firstTree.average().x, (int)firstTree.average().y }, { (int)currentTree.average().x, (int)currentTree.average().y }, cv::Scalar{
				(double)((int)((std::sin((double)index * 463763.0) + 1.0) * 100000.0) % 120 + 80),
				(double)((int)((std::sin((double)index * 1279.0) + 1.0) * 100000.0) % 120 + 80),
				(double)((int)((std::sin((double)index * 92763.0) + 1.0) * 100000.0) % 120 + 80)
				}, 2.0);

			// idの描画
			if (drawId) gui::text(
				mat, std::to_string(index),
				{ (int)currentTree.average().x, (int)currentTree.average().y },
				gui::CENTER_CENTER, 0.5
			);
		}
	}
}