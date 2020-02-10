#pragma once

#include <openpose/core/common.hpp>

#include <cmath>
#include <vector>
#include <map>
#include <string>
#include <iostream>

#include <Utils/Vector.h>
#include <OpenPoseWrapper/OpenPoseEvent.h>
#include <Utils/Gui.h>

namespace op
{
	struct Node
	{
	public:
		float x;
		float y;
		float confidence;
		float confidenceThreshold;

		Node();

		Node(
			float x,
			float y,
			float confidence,
			float confidenceThreshold
		);

		Node(const Node& node);

		Node& operator = (const Node& node)
		{
			x = node.x;
			y = node.y;
			confidence = node.confidence;
			confidenceThreshold = node.confidenceThreshold;
			return *this;
		}

		bool isTrusted();

		float distanceFrom(const Node& node);
	};

	struct Tree
	{
	public:
		std::vector<Node> nodes;
		uint64_t frameNumber;
		uint64_t numberNodesToTrust;
		float confidenceThreshold;

		Tree();

		Tree(
			std::vector<Node> nodes,
			uint64_t frameNumber,
			uint64_t numberNodesToTrust,
			float confidenceThreshold,
			bool valid
		);

		Tree(const Tree& tree);

		Tree& operator = (const Tree& tree)
		{
			nodes = tree.nodes;
			frameNumber = tree.frameNumber;
			numberNodesToTrust = tree.numberNodesToTrust;
			confidenceThreshold = tree.confidenceThreshold;
			valid = tree.valid;
			return *this;
		}

		bool isTrusted();

		float distanceFrom(std::vector<Node>& nodes);

		Node average();

		bool isValid();

	private:
		bool valid;
	};

	struct PeopleList
	{
	public:
		uint64_t numberNodesToTrust;
		float confidenceThreshold;
		uint64_t numberFramesToLost;
		float distanceThreshold;

		PeopleList();

		PeopleList(
			uint64_t numberNodesToTrust,
			float confidenceThreshold,
			uint64_t numberFramesToLost,
			float distanceThreshold
		);

		// 骨格データを更新する
		void addFrame(ImageInfo& imageInfo);

		// 現在のフレームに映っているすべての人のIDを配列で取得する
		std::vector<uint64_t> getCurrentIndices();

		// 1フレーム前に映っているすべての人のIDを配列で取得する
		std::vector<uint64_t> getBackIndices();

		// 1フレーム前には映っていたが、現在のフレームでトラッキングが外れた人のIDを配列で取得する
		std::vector<uint64_t> getLostIndices();

		// 指定されたIDの人の現在のフレームの骨格を取得
		// ただし、そのIDがgetCurrentIndices()で返される配列に存在しない場合は無効な骨格データが返される
		Tree getCurrentTree(uint64_t index);

		// 指定されたIDの人の1フレーム前の骨格を取得
		// ただし、そのIDがgetBackIndices()で返される配列に存在しない場合は無効な骨格データが返される
		Tree getBackTree(uint64_t index);

		// 指定されたIDの人が初めて映りこんだときの骨格を取得
		// ただし、そのIDがgetCurrentIndices()で返される配列とgetBackIndices()で返される配列のどちらにも存在しない場合は無効な骨格データが返される
		Tree getFirstTree(uint64_t index);

	private:
		std::map<uint64_t, Tree> currentTrees;
		std::map<uint64_t, Tree> backTrees;
		std::map<uint64_t, Tree> firstTrees;
		uint64_t sumOfPeople = 0;
	};

	// 直線の上を何人の人が、どちらの方向に移動したかをカウントするクラス
	class PeopleLineCounter
	{
	private:
		// トラッキングが外れた人のカウンタ(最終的な移動方向が確定している)
		uint64_t staticUpCount = 0;  // 直線の上を上側に移動した人のカウント
		uint64_t staticDownCount = 0;  // 直線の上を下側に移動した人のカウント
		// トラッキングが外れていない人のカウンタ
		uint64_t dynamicUpCount = 0;  // 直線の上を上側に移動した人のカウント
		uint64_t dynamicDownCount = 0;  // 直線の上を下側に移動した人のカウント
		// 1フレーム前のstaticカウントとdynamicカウントの和
		uint64_t preUpCount = 0;  // 直線の上を上側に移動した人のカウント
		uint64_t preDownCount = 0;  // 直線の上を下側に移動した人のカウント
		// 人数カウントを行う基準線の始点と終点
		struct Line {
			float lineStartX, lineStartY, lineEndX, lineEndY;
		};
		std::vector<Line> lines;

		enum Event { UP, DOWN, NOTHING };

		bool isCross(vt::Vector4& p1Start, vt::Vector4& p1End, vt::Vector4& p2Start, vt::Vector4& p2End);

		Event judgeUpOrDown(op::Tree& peopleStart, op::Tree& peopleEnd, Line& line);

		Event judgeUpOrDown(op::Tree& peopleStart, op::Tree& peopleEnd);

	public:
		PeopleLineCounter(float startX = 0.0f, float startY = 0.0f, float endX = 1.0f, float endY = 1.0f);

		// 骨格を更新する
		void updateCount(PeopleList& people);

		// カウントのリセット
		void resetCount();

		// カウントの基準線の位置設定(始点X, 始点Y, 終点X, 終点Y)
		void setLine(float lineStartX, float lineStartY, float lineEndX, float lineEndY);

		// カウントの基準線の位置設定(始点X, 始点Y, 終点X, 終点Y, 線の太さ)
		void setLine(float lineStartX, float lineStartY, float lineEndX, float lineEndY, float lineWeigth);

		// 基準線を上方向に移動した人のカウントを取得
		inline uint64_t getUpCount() { return staticUpCount + dynamicUpCount; }
		// 基準線を上方向に移動した人のカウントを取得
		inline uint64_t getDownCount() { return staticDownCount + dynamicDownCount; }

		// 1フレーム前と比べ、カウントに変化があったかどうかを取得
		inline bool isChanged() { return ((preUpCount != getUpCount()) || preDownCount != getDownCount()); }

		// 基準線の描画
		void drawJudgeLine(cv::Mat& mat);

		// 人々の始点と終点を結ぶ直線の描画
		void drawPeopleLine(cv::Mat& mat, PeopleList& people, bool drawId);
	};
}