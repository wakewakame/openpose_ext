﻿#pragma once

#include <openpose/headers.hpp>
#include <queue>
#include <mutex>
#include <thread>
#include <stdint.h>
#include <cassert>

/**
 * OpenPose のラッパークラス
 * OpenPose を別スレッドで動かし、 OpenPose の操作を簡単にする
 */
class MinOpenPose
{
private:
	/**
	 * OpenPose へ入力する画像を管理するクラス
	 */
	class WUserInputProcessing : public op::Worker<std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>>>
	{
	private:
		// OpenPose へ入力する画像を格納するキュー
		std::queue<std::pair<cv::Mat, size_t>> images;

		// 各スレッドを同期させるための mutex
		std::mutex& inOutMtx;

		// エラーメッセージ配列
		std::vector<std::string> errorMessage;

	public:
		/**
		 * コンストラクタ
		 * @param inOutMtx メンバ関数はOpenPose側のスレッドからも呼び出されるので、同期をとるためにmutexを指定する
		 */
		WUserInputProcessing(std::mutex& inOutMtx);

		/**
		 * メンバ変数の初期化用関数
		 * OpenPose 側で生成されたスレッドから呼び出される
		 */
		void initializationOnThread() override;

		/**
		 * pushImage() で追加された画像を OpenPose に1枚ずつ渡す関数
		 * OpenPose 側で生成されたスレッドから呼び出される
		 * @param datumsPtr OpenPose へ入力される前のデータ
		 */
		void work(std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>>& datumsPtr) override;

		/**
		 * OpenPose に入力する画像を追加する関数
		 * MinimumOpenPose 側から呼び出される
		 * @param image 追加する画像 (フォーマット : CV_8UC3)
		 * @param maxQueueSize 追加できる画像数の上限
		 */
		int pushImage(const cv::Mat& image, size_t frameNumber, size_t maxQueueSize);

		/**
		 * エラーを取得する関数
		 * MinimumOpenPose 側から呼び出される
		 * @param errorMessage エラーメッセージが格納される変数
		 * @param clearError クラス内のエラーメッセージを削除するフラグ
		 */
		void getErrors(std::vector<std::string>& errorMessage, bool clearErrors);

		/**
		 * スレッドを停止する関数
		 * MinimumOpenPose 側から呼び出される
		 */
		void shutdown();
	};

	/**
	 * OpenPose から出力されるデータを管理するクラス
	 */
	class WUserOutputProcessing : public op::Worker<std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>>>
	{
	private:
		// OpenPose から出力されるデータを格納するキュー
		std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>> results;
		// 各スレッドを同期させるための mutex
		std::mutex& inOutMtx;
		// エラーメッセージ配列
		std::vector<std::string> errorMessage;

	public:
		/**
		 * コンストラクタ
		 * @param inOutMtx メンバ関数はOpenPose側のスレッドからも呼び出されるので、同期をとるためにmutexを指定する
		 */
		WUserOutputProcessing(std::mutex& inOutMtx);

		/**
		 * メンバ変数の初期化用関数
		 * OpenPose 側で生成されたスレッドから呼び出される
		 */
		void initializationOnThread() override;

		/**
		 * OpenPose で処理された画像を results キューに追加する関数
		 * OpenPose 側で生成されたスレッドから呼び出される
		 * @param datumsPtr OpenPose から出力されたデータ
		 */
		void work(std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>>& datumsPtr) override;

		/**
		 * OpenPose から出力されたデータの個数を取得する関数
		 */
		size_t getResultsSize();

		/**
		 * OpenPose から出力された全てのデータを取得し、キューをリセットする関数
		 */
		std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>> getResultsAndReset();

		/**
		 * エラーを取得する関数
		 * MinimumOpenPose 側から呼び出される
		 * @param errorMessage エラーメッセージが格納される変数
		 * @param clearError クラス内のエラーメッセージを削除するフラグ
		 */
		void getErrors(std::vector<std::string>& errorMessage, bool clearErrors);

		/**
		 * スレッドを停止する関数
		 * MinimumOpenPose 側から呼び出される
		 */
		void shutdown();
	};

	/**
	 * OpenPose の処理のステータスを表す
	 */
	enum class ProcessState
	{
		//! 入力キューに何も溜まっておらず、入力待ちの状態
		WaitInput = 0,
		//! 入力キューに溜まっている画像の処理中
		Processing = 1,
		//! 入力キューの画像全ての処理が完了し
		//! getResultsAndReset() が呼び出されるのを待機している状態
		Finish = 2,
		//! スレッドが終了した状態
		Shutdown = 3
	};

	// OpenPose のラッパークラス
	std::unique_ptr<op::Wrapper> opWrapper;
	// OpenPose を実行させるスレッド
	std::thread opThread;
	// OpenPose へ入力する画像を管理するクラス
	std::shared_ptr<WUserInputProcessing> opInput;
	// OpenPose から出力される画像を管理するクラス
	std::shared_ptr<WUserOutputProcessing> opOutput;
	// 現在 OpenPose で処理中の画像の枚数
	size_t jobCount = 0;
	// OpenPose 側のスレッドで発生した例外メッセージを格納する変数
	std::vector<std::string> errorMessage;
	// OpenPose 側のスレッドと同期するための mutex
	std::mutex inOutMtx;
	// OpenPose の設定
	op::WrapperStructPose wrapperStructPose;

	/**
	 * OpenPose を開始する
	 * 既に OpenPose が開始していた場合は何も変更しない
	 * この関数はコンストラクタで呼ばれる
	 */
	int startup(op::PoseModel poseModel = op::PoseModel::BODY_25, op::Point<int> netInputSize = op::Point<int>(-1, 368));

	/**
	 * OpenPose を終了する
	 * 既に OpenPose が終了していた場合は何も変更しない
	 * この関数はデストラクタでも呼ばれる
	 */
	void shutdown();

	/**
	 * OpenPose の起動状態を取得する
	 * @return 起動している場合はtrueが返る。起動していない場合はfalseが返る。
	 */
	bool isStartup();

	/**
	 * キューに画像を追加する関数
	 * @param image 追加する画像
	 * @param maxQueueSize キューの上限
	 * @return キューの追加に成功すると 0 が返り、失敗すると 1 が返る
	 */
	int pushImage(const cv::Mat& image, size_t frameNumber, size_t maxQueueSize = 128);

	/**
	 * OpenPose の処理のステータスを取得する
	 * @return OpenPose の処理のステータス
	 */
	ProcessState getProcessState();

	/**
	 * pushImage() で追加された画像の処理結果を取得し、キューをリセットする関数
	 * getProcessState() が ProcessState::Finish を返すときのみ有効
	 * @return 処理結果
	 */
	std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>> getResultsAndReset();

public:
	/**
	 * @param poseModel 姿勢推定に用いるモデルを選択する
	 * @param netInputSize 姿勢推定を行うネットワークの解像度を指定する (片方に-1を指定すると入力される画像のアスペクト比から自動計算される)
	 */
	MinOpenPose(op::PoseModel poseModel = op::PoseModel::BODY_25, op::Point<int> netInputSize = op::Point<int>(-1, 368));
	virtual ~MinOpenPose();

	struct Node { float x, y, confidence; };
	using Person = std::vector<Node>;
	using People = std::map<size_t, Person>;
	People estimate(const cv::Mat& inputImage);

	op::WrapperStructPose getConfig() const { return wrapperStructPose;  }
};