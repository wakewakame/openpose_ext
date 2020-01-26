﻿#pragma once

#include <openpose/headers.hpp>
#include <queue>
#include <mutex>
#include <thread>
#include <stdint.h>
#include <cassert>

/**
 * OpenPose のラッパークラス\n
 * OpenPose を別スレッドで動かし、 OpenPose の操作を簡単にする
 */
class MinimumOpenPose
{
private:
	/**
	 * OpenPose へ入力する画像を管理するクラス
	 */
	class WUserInputProcessing : public op::Worker<std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>>>
	{
	private:
		// OpenPose へ入力する画像を格納するキュー
		std::queue<std::unique_ptr<cv::Mat>> images;
		// 各スレッドを同期させるための mutex
		std::mutex& inOutMtx;
		// 入力画像のカウンタ
		uint64_t frameNumber = 0;
		// エラーメッセージ配列
		std::vector<std::string> errorMessage;

	public:
		/**
		 * コンストラクタ
		 * @param inOutMtx 
		 */
		WUserInputProcessing(std::mutex& inOutMtx);

		/**
		 * メンバ変数の初期化用関数\n
		 * OpenPose 側で生成されたスレッドから呼び出される
		 */
		void initializationOnThread() override;

		/**
		 * pushImage() で追加された画像を OpenPose に1枚ずつ渡す関数\n
		 * OpenPose 側で生成されたスレッドから呼び出される
		 * @param datumsPtr OpenPose へ入力される前のデータ
		 */
		void work(std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>>& datumsPtr) override;

		/**
		 * OpenPose に入力する画像を追加する関数\n
		 * MinimumOpenPose 側から呼び出される
		 * @param image 追加する画像 (フォーマット : CV_8UC3)
		 * @param maxQueueSize 追加できる画像数の上限
		 */
		int pushImage(std::unique_ptr<cv::Mat>&& image, size_t maxQueueSize);

		/**
		 * エラーを取得する関数\n
		 * MinimumOpenPose 側から呼び出される
		 * @param errorMessage エラーメッセージが格納される変数
		 * @param clearError クラス内のエラーメッセージを削除するフラグ
		 */
		void getErrors(std::vector<std::string>& errorMessage, bool clearErrors);

		/**
		 * スレッドを停止する関数\n
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
		 * @param inOutMtx
		 */
		WUserOutputProcessing(std::mutex& inOutMtx);

		/**
		 * メンバ変数の初期化用関数\n
		 * OpenPose 側で生成されたスレッドから呼び出される
		 */
		void initializationOnThread() override;

		/**
		 * OpenPose で処理された画像を results キューに追加する関数\n
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
		 * エラーを取得する関数\n
		 * MinimumOpenPose 側から呼び出される
		 * @param errorMessage エラーメッセージが格納される変数
		 * @param clearError クラス内のエラーメッセージを削除するフラグ
		 */
		void getErrors(std::vector<std::string>& errorMessage, bool clearErrors);

		/**
		 * スレッドを停止する関数\n
		 * MinimumOpenPose 側から呼び出される
		 */
		void shutdown();
	};

	// OpenPose のラッパークラス
	op::Wrapper opWrapper;
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

public:
	/**
	 * OpenPose の処理のステータスを表す
	 */
	enum ProcessState : uint8_t
	{
		//! 入力キューに何も溜まっておらず、入力待ちの状態
		WaitInput = 0,
		//! 入力キューに溜まっている画像の処理中
		Processing = 1,
		//! 入力キューの画像全ての処理が完了し
		//! getResultsAndReset() が呼び出されるのを待機している状態
		Finish = 2,
		//! 正常にスレッドが終了した状態
		Shutdown = 3,
		//! 入力キューの画像の処理中に例外が発生し、スレッドが終了した状態
		Error = 4
	};

	MinimumOpenPose();
	virtual ~MinimumOpenPose();

	/**
	 * OpenPose を起動する
	 * @param poseModel 検出する関節の数などのポーズモデルを指定
	 * @param netInputSize ネットワークの解像度を指定(幅、高さともに16の倍数でなければならない)
	 * @return 起動が成功すると0が返る。失敗すると1が返る。
	 */
	int startup(
		op::PoseModel poseModel = op::PoseModel::BODY_25,
		op::Point<int> netInputSize = op::Point<int>(-1, 368)
	);

	/**
	 * OpenPose を終了する\n
	 * 既に OpenPose が終了していた場合は何も変更しない\n
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
	int pushImage(std::unique_ptr<cv::Mat>&& image, size_t maxQueueSize = 128);

	/**
	 * OpenPose の処理のステータスを取得する
	 * @return OpenPose の処理のステータス
	 */
	ProcessState getProcessState();

	/**
	 * OpenPose 側のスレッドで発生した例外メッセージを取得する
	 */
	const std::vector<std::string>& getErrors();

	/**
 * OpenPose 側のスレッドで発生した例外メッセージをすべて削除する
 */
	void resetErrors();

	/**
	 * pushImage() で追加された画像の処理結果を取得し、キューをリセットする関数\n
	 * getProcessState() が ProcessState::Finish を返すときのみ有効
	 * @return 処理結果
	 */
	std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>> getResultsAndReset();
};