#pragma once

#include <openpose/core/common.hpp>

#include <cmath>
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <openpose/utilities/openCv.hpp>
#include <Utils/Vector.h>
#include <OpenPoseWrapper/OpenPoseEvent.h>

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
		// 人数カウントを行う基準線の始点と終点
		struct Line {
			float lineStartX, lineStartY, lineEndX, lineEndY;
		};
		std::vector<Line> lines;

		enum Event { UP, DOWN, NOTHING };

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

		Event judgeUpOrDown(op::Tree& peopleStart, op::Tree& peopleEnd, Line& line)
		{
			auto startPos = peopleStart.average();  // 歩行者のトラッキングを開始した点
			auto endPos = peopleEnd.average();  // 歩行者のトラッキングを終了した点
			auto vecLine = vt::Vector4((double)line.lineEndX - (double)line.lineStartX, (double)line.lineEndY - (double)line.lineStartY);  // 人数カウントを行う基準線のベクトル
			auto vecStart = vt::Vector4((double)startPos.x - (double)line.lineStartX, (double)startPos.y - (double)line.lineStartY);  // 人数カウントを行う基準線の始点からstartPosへのベクトル
			auto vecEnd = vt::Vector4((double)endPos.x - (double)line.lineStartX, (double)endPos.y - (double)line.lineStartY);  // 人数カウントを行う基準線の始点からvecEndへのベクトル
			if (!isCross(vt::Vector4(0.0, 0.0), vecLine, vecStart, vecEnd)) return Event::NOTHING;  // startからendを結ぶ直線がvecLineの上を通過していない場合
			// 「vecLineを90度回転させた線」と「vecStart」との内積をもとに、startPosがvecLineの上側にあるかどうかを判定
			bool startIsUp = vecStart.x * vecLine.y > vecStart.y* vecLine.x;
			// 「vecLineを90度回転させた線」と「vecEnd」との内積をもとに、endPosがvecLineの上側にあるかどうかを判定
			bool endIsUp = vecEnd.x * vecLine.y > vecEnd.y* vecLine.x;
			if ((!startIsUp) && endIsUp) return Event::UP;  // 歩行者のトラッキングが人数カウントを行う基準線を超えて上に移動していた場合
			if (startIsUp && (!endIsUp)) return Event::DOWN;  // 歩行者のトラッキングが人数カウントを行う基準線を超えて下に移動していた場合
			return Event::NOTHING;  // それ以外
		}

		Event judgeUpOrDown(op::Tree& peopleStart, op::Tree& peopleEnd)
		{
			Event e = Event::NOTHING;
			for (size_t i = 0; i < lines.size(); i++)
			{
				if (i == 0) e = judgeUpOrDown(peopleStart, peopleEnd, lines[i]);
				else if (e != judgeUpOrDown(peopleStart, peopleEnd, lines[i])) return Event::NOTHING;
			}
			return e;
		}

	public:
		PeopleLineCounter(float startX = 0.0f, float startY = 0.0f, float endX = 1.0f, float endY = 1.0f)
		{
			setLine(startX, startY, endX, endY);
		}

		// 骨格を更新する
		void update(PeopleList& peopleList)
		{
			// トラッキングが外れていない人のカウンタをリセット
			dynamicUpCount = 0;
			dynamicDownCount = 0;
			// トラッキングが外れていない人の移動方向カウント
			const std::vector<uint64_t> currentIndex = peopleList.getCurrentIndices();
			for (size_t index : currentIndex)
			{
				auto e = judgeUpOrDown(peopleList.getFirstTree(index), peopleList.getCurrentTree(index));
				if (e == Event::UP) dynamicUpCount++;
				if (e == Event::DOWN) dynamicDownCount++;
			}
			// トラッキングが外れた人の移動方向カウント
			const std::vector<uint64_t> lostIndex = peopleList.getLostIndices();
			for (size_t index : lostIndex)
			{
				auto e = judgeUpOrDown(peopleList.getFirstTree(index), peopleList.getBackTree(index));
				if (e == Event::UP) staticUpCount++;
				if (e == Event::DOWN) staticDownCount++;
			}
		}

		// カウントのリセット
		inline void resetCount()
		{
			staticUpCount = 0;
			staticDownCount = 0;
			dynamicUpCount = 0;
			dynamicDownCount = 0;
		}

		// カウントの基準線の位置設定(始点X, 始点Y, 終点X, 終点Y)
		inline void setLine(float lineStartX, float lineStartY, float lineEndX, float lineEndY)
		{
			this->lines.clear();
			this->lines.push_back(Line{ lineStartX, lineStartY, lineEndX, lineEndY });
		}

		// カウントの基準線の位置設定(始点X, 始点Y, 終点X, 終点Y, 線の太さ)
		inline void setLine(float lineStartX, float lineStartY, float lineEndX, float lineEndY, float lineWeigth)
		{

			float lineVecX = lineEndX - lineStartX;
			float lineVecY = lineEndY - lineStartY;
			float lineVecLength = std::sqrt(lineVecX * lineVecX + lineVecY * lineVecY);
			float lineNormalX = -lineVecY / lineVecLength;
			float lineNormalY = lineVecX / lineVecLength;
			float moveX = lineNormalX * lineWeigth * 0.5;
			float moveY = lineNormalY * lineWeigth * 0.5;
			this->lines.clear();
			this->lines.push_back(Line{ lineStartX - moveX, lineStartY - moveY, lineEndX - moveX, lineEndY - moveY });
			this->lines.push_back(Line{ lineStartX + moveX, lineStartY + moveY, lineEndX + moveX, lineEndY + moveY });
		}

		// 基準線を上方向に移動した人のカウントを取得
		inline uint64_t getUpCount() { return staticUpCount + dynamicUpCount; }
		// 基準線を上方向に移動した人のカウントを取得
		inline uint64_t getDownCount() { return staticDownCount + dynamicDownCount; }

		// 基準線の描画
		void drawLine(cv::Mat& mat)
		{
			for (auto line : lines)
				cv::line(mat, { (int)line.lineStartX, (int)line.lineStartY }, { (int)line.lineEndX, (int)line.lineEndY }, cv::Scalar{ 255.0, 255.0, 255.0 }, 2.0);
		}
	};
}