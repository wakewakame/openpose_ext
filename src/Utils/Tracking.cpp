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
			// 前フレームの中で一番距離が近かった人のインデックスを取得
			uint64_t nearestPeopleIndex = 0;  // 前フレームの中で一番距離が近かった人のインデックスを格納する一時変数
			float nearestLength = -1.0f;  // 前フレームの中で一番距離が近かった長さを格納する一時変数
			bool lostFlag = true;  // 前フレームで一番距離が近かった人が検出できなかった場合にTrueになるフラグ
			// 前フレームの人数分ループ
			for (auto treeItr = backTrees.begin(); treeItr != backTrees.end(); treeItr++)
			{
				uint64_t index = treeItr->first;  // インデックスの取得
				Tree tree = treeItr->second;  // 骨格情報の取得
				// 骨格情報がnumberFramesToLostフレーム以上前のフレームであれば除外する
				if (imageInfo.frameNumber - tree.frameNumber > numberFramesToLost) continue;
				// 骨格情報が(numberFramesToLost+1)フレーム以上前のフレームであればfirstTreesから除去する
				if (imageInfo.frameNumber - tree.frameNumber > (numberFramesToLost + 1))
				{
					if (firstTrees.count(index) != 0) firstTrees.erase(index);
				}
				// 一旦骨格情報を現在のフレームに追加する
				if (currentTrees.count(index) == 0) currentTrees[index] = tree;
				// 前フレームからの移動距離を算出
				float distance = tree.distanceFrom(nodes);
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
			currentTrees[addIndex] = Tree(
				nodes,
				imageInfo.frameNumber,
				numberNodesToTrust,
				confidenceThreshold
			);
			// 初めて登場する人はfirstTreeにも追加
			if (lostFlag) firstTrees[addIndex] = currentTrees[addIndex];
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
}