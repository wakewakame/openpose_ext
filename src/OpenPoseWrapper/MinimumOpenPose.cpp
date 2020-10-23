#include "OpenPoseWrapper/MinimumOpenPose.h"

MinOpenPose::WUserInputProcessing::WUserInputProcessing(std::mutex& inOutMtx) : inOutMtx(inOutMtx) {}

void MinOpenPose::WUserInputProcessing::initializationOnThread()
{
	std::lock_guard<std::mutex> inOutLock(inOutMtx);
}

void MinOpenPose::WUserInputProcessing::work(std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>>& datumsPtr)
{
	std::lock_guard<std::mutex> inOutLock(inOutMtx);
	try
	{
		// キューに画像が入っていた場合、1枚ずつ OpenPose に送る
		datumsPtr = std::make_shared<std::vector<std::shared_ptr<op::Datum>>>();
		if (!images.empty())
		{
			auto datumPtr = std::make_shared<op::Datum>();
			datumPtr->subId = 0;
			datumPtr->subIdMax = 0;
			datumPtr->name = "output";
			datumPtr->frameNumber = images.front().second;
			datumPtr->cvInputData = images.front().first;
			images.pop();
			datumPtr->cvOutputData = datumPtr->cvInputData;
			datumsPtr->push_back(datumPtr); // datumsPtr は2つ以上の要素を持てない
		}
	}
	catch (const std::exception& e)
	{
		errorMessage.push_back(e.what());
		this->stop();
	}
}

int MinOpenPose::WUserInputProcessing::pushImage(const cv::Mat& image, size_t frameNumber, size_t maxQueueSize)
{
	std::lock_guard<std::mutex> inOutLock(inOutMtx);
	if (images.size() >= maxQueueSize) return 1;
	images.push(std::pair<cv::Mat, size_t>(image, frameNumber));
	return 0;
}

void MinOpenPose::WUserInputProcessing::getErrors(std::vector<std::string>& errorMessage, bool clearErrors)
{
	std::lock_guard<std::mutex> inOutLock(inOutMtx);
	for (auto error : this->errorMessage) errorMessage.push_back(error);
	if (clearErrors) this->errorMessage.clear();
}

void MinOpenPose::WUserInputProcessing::shutdown()
{
	std::lock_guard<std::mutex> inOutLock(inOutMtx);
	this->stop();
}

MinOpenPose::WUserOutputProcessing::WUserOutputProcessing(std::mutex& inOutMtx) : inOutMtx(inOutMtx)
{
	std::lock_guard<std::mutex> inOutLock(inOutMtx);
	results = std::make_shared<std::vector<std::shared_ptr<op::Datum>>>();
	assert(static_cast<bool>(results));
}

void MinOpenPose::WUserOutputProcessing::initializationOnThread()
{
	std::lock_guard<std::mutex> inOutLock(inOutMtx);
}

void MinOpenPose::WUserOutputProcessing::work(std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>>& datumsPtr)
{
	std::lock_guard<std::mutex> inOutLock(inOutMtx);
	assert(static_cast<bool>(results));
	if (static_cast<bool>(datumsPtr) && !datumsPtr->empty()) {
		for (auto datumPtr : *datumsPtr) results->push_back(datumPtr);
	}
}

size_t MinOpenPose::WUserOutputProcessing::getResultsSize()
{
	std::lock_guard<std::mutex> inOutLock(inOutMtx);
	assert(static_cast<bool>(results));
	return results->size();
}

std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>> MinOpenPose::WUserOutputProcessing::getResultsAndReset()
{
	std::lock_guard<std::mutex> inOutLock(inOutMtx);
	auto results = std::make_shared<std::vector<std::shared_ptr<op::Datum>>>();
	results.swap(this->results);
	assert(static_cast<bool>(results));
	return results;
}

void MinOpenPose::WUserOutputProcessing::getErrors(std::vector<std::string>& errorMessage, bool clearErrors)
{
	std::lock_guard<std::mutex> inOutLock(inOutMtx);
	for (auto error : this->errorMessage) errorMessage.push_back(error);
	if (clearErrors) this->errorMessage.clear();
}

void MinOpenPose::WUserOutputProcessing::shutdown()
{
	std::lock_guard<std::mutex> inOutLock(inOutMtx);
	this->stop();
}

MinOpenPose::MinOpenPose(op::PoseModel poseModel, op::Point<int> netInputSize)
{
	opInput = std::make_shared<WUserInputProcessing>(inOutMtx);
	opOutput = std::make_shared<WUserOutputProcessing>(inOutMtx);
	startup(poseModel, netInputSize);
}

MinOpenPose::~MinOpenPose()
{
	shutdown();
}

int MinOpenPose::startup(op::PoseModel poseModel, op::Point<int> netInputSize)
{
	// 変数の初期化
	jobCount = 0;
	errorMessage.clear();
	opWrapper = std::make_unique<op::Wrapper>();

	// 使用する骨格モデルの選択
	wrapperStructPose.poseModel = poseModel;

	// ネットワークの解像度の設定 (16の倍数のみ指定可能, -1は縦横比に合わせて自動計算される)
	// 値は大きいほど精度が高く、処理も重い
	wrapperStructPose.netInputSize = netInputSize;

	// OpenPose に設定を適応
	opWrapper->configure(wrapperStructPose);

	// 画像入力 Worker の設定
	opWrapper->setWorker(op::WorkerType::Input, opInput, true);

	// 画像出力 Worker の設定
	opWrapper->setWorker(op::WorkerType::Output, opOutput, true);

	// OpenPose を別スレッドで実行
	opThread = std::thread([&] {
		try {
			opWrapper->exec();
		}
		catch (const std::exception& e) {
			std::lock_guard<std::mutex> inOutLock(inOutMtx);
			errorMessage.push_back(e.what());
		}
	});
	return 0;
}

MinOpenPose::People MinOpenPose::estimate(const cv::Mat& inputImage)
{
	// この関数が返す予定の値
	People people;

	// 画像が空であれば処理を終了する
	if (inputImage.empty()) return people;

	// OpenPose を実行しているスレッドで姿勢推定が終了するまでループして待つ
	while (true)
	{
		// OpenPose の処理状態の確認
		const ProcessState state = getProcessState();

		// OpenPose が入力待機状態の場合
		if (ProcessState::WaitInput == state)
		{			
			// OpenPose に画像を渡す
			pushImage(inputImage, 0);

			// 処理が終わるまでループ
			continue;
		}

		// OpenPose が処理中の場合
		if (ProcessState::Processing == state)
		{
			// 処理が終わるまでループ
			continue;
		}

		// OpenPose のスレッドが終了している場合
		if (ProcessState::Shutdown == state)
		{
			// エラーが発生して終了した場合はエラー内容を出力
			for (auto err : errorMessage)
			{
				std::cout << err << std::endl;
			}

			// 処理を終了
			return people;
		}

		// OpenPose の処理が終了した場合
		if (ProcessState::Finish == state)
		{
			// 出力されたデータの取得
			auto results = getResultsAndReset();
			if (!static_cast<bool>(results)) return people;

			// 出力されるデータの数だけループする (1つしか入力していないので1回だけループするはず)
			for (auto result : *results)
			{
				// 画面内に映っている人数分ループする
				for (int personIndex = 0; personIndex < result->poseKeypoints.getSize(0); personIndex++)
				{
					Person& nodes = people[(size_t)personIndex];
					nodes.reserve(result->poseKeypoints.getSize(1));

					// 骨格の数だけループする (BODY25のモデルを使う場合は25回)
					for (int nodeIndex = 0; nodeIndex < result->poseKeypoints.getSize(1); nodeIndex++)
					{
						nodes.push_back(Node{
							result->poseKeypoints[{personIndex, nodeIndex, 0}],
							result->poseKeypoints[{personIndex, nodeIndex, 1}],
							result->poseKeypoints[{personIndex, nodeIndex, 2}]
						});
					}
				}

				// 姿勢推定のプレビュー画像
				//cv::Mat outputImage = result->cvOutputData;
			}

			return people;
		}

		// ここまで到達するこはないはず
		assert(false);
	}

	// ここまで到達するこはないはず
	assert(false);
}

void MinOpenPose::shutdown()
{
	if (!isStartup()) return;
	opInput->shutdown();
	opOutput->shutdown();
	opThread.join();
}

bool MinOpenPose::isStartup() { return opThread.joinable(); }

int MinOpenPose::pushImage(const cv::Mat& image, size_t frameNumber, size_t maxQueueSize)
{
	if (
		(!isStartup()) ||
		(image.type() != CV_8UC3)
	) return 1;
	if (opInput->pushImage(image, frameNumber, maxQueueSize)) return 1;
	jobCount++;
	return 0;
}

MinOpenPose::ProcessState MinOpenPose::getProcessState()
{
	assert(opOutput->getResultsSize() > jobCount);
	
	opInput->getErrors(errorMessage, true);
	opOutput->getErrors(errorMessage, true);
	if (!errorMessage.empty())
	{
		shutdown();
	}

	if (!isStartup()) { return ProcessState::Shutdown; }
	else if (opOutput->getResultsSize() < jobCount) { return ProcessState::Processing; }
	else if (opOutput->getResultsSize() == jobCount)
	{ 
		if (opOutput->getResultsSize() == 0) return ProcessState::WaitInput;
		else return ProcessState::Finish;
	}
}

std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>> MinOpenPose::getResultsAndReset()
{
	if (
		(!isStartup()) ||
		(getProcessState() != ProcessState::Finish)
	)
	{
		return std::make_shared<std::vector<std::shared_ptr<op::Datum>>>();
	}
	jobCount = 0;
	return opOutput->getResultsAndReset();
}