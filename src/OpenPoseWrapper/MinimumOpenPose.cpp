#include "OpenPoseWrapper/MinimumOpenPose.h"

MinimumOpenPose::WUserInputProcessing::WUserInputProcessing(std::mutex& inOutMtx) : inOutMtx(inOutMtx) {}

void MinimumOpenPose::WUserInputProcessing::initializationOnThread() {
	std::lock_guard<std::mutex> inOutLock(inOutMtx);
}

void MinimumOpenPose::WUserInputProcessing::work(std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>>& datumsPtr)
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

int MinimumOpenPose::WUserInputProcessing::pushImage(cv::Mat& image, size_t frameNumber, size_t maxQueueSize)
{
	std::lock_guard<std::mutex> inOutLock(inOutMtx);
	if (images.size() >= maxQueueSize) return 1;
	images.push(std::pair<cv::Mat, size_t>(image, frameNumber));
	return 0;
}

void MinimumOpenPose::WUserInputProcessing::getErrors(std::vector<std::string>& errorMessage, bool clearErrors)
{
	std::lock_guard<std::mutex> inOutLock(inOutMtx);
	for (auto error : this->errorMessage) errorMessage.push_back(error);
	if (clearErrors) this->errorMessage.clear();
}

void MinimumOpenPose::WUserInputProcessing::shutdown()
{
	std::lock_guard<std::mutex> inOutLock(inOutMtx);
	this->stop();
}

MinimumOpenPose::WUserOutputProcessing::WUserOutputProcessing(std::mutex& inOutMtx) : inOutMtx(inOutMtx) {
	std::lock_guard<std::mutex> inOutLock(inOutMtx);
	results = std::make_shared<std::vector<std::shared_ptr<op::Datum>>>();
	assert(static_cast<bool>(results));
}

void MinimumOpenPose::WUserOutputProcessing::initializationOnThread() {
	std::lock_guard<std::mutex> inOutLock(inOutMtx);
}

void MinimumOpenPose::WUserOutputProcessing::work(std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>>& datumsPtr)
{
	std::lock_guard<std::mutex> inOutLock(inOutMtx);
	assert(static_cast<bool>(results));
	if (static_cast<bool>(datumsPtr) && !datumsPtr->empty()) {
		for (auto datumPtr : *datumsPtr) results->push_back(datumPtr);
	}
}

size_t MinimumOpenPose::WUserOutputProcessing::getResultsSize()
{
	std::lock_guard<std::mutex> inOutLock(inOutMtx);
	assert(static_cast<bool>(results));
	return results->size();
}

std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>> MinimumOpenPose::WUserOutputProcessing::getResultsAndReset()
{
	std::lock_guard<std::mutex> inOutLock(inOutMtx);
	auto results = std::make_shared<std::vector<std::shared_ptr<op::Datum>>>();
	results.swap(this->results);
	assert(static_cast<bool>(results));
	return results;
}

void MinimumOpenPose::WUserOutputProcessing::getErrors(std::vector<std::string>& errorMessage, bool clearErrors)
{
	std::lock_guard<std::mutex> inOutLock(inOutMtx);
	for (auto error : this->errorMessage) errorMessage.push_back(error);
	if (clearErrors) this->errorMessage.clear();
}

void MinimumOpenPose::WUserOutputProcessing::shutdown()
{
	std::lock_guard<std::mutex> inOutLock(inOutMtx);
	this->stop();
}

MinimumOpenPose::MinimumOpenPose()
{
	opInput = std::make_shared<WUserInputProcessing>(inOutMtx);
	opOutput = std::make_shared<WUserOutputProcessing>(inOutMtx);
}

MinimumOpenPose::~MinimumOpenPose()
{
	shutdown();
}

int MinimumOpenPose::startup(OpenPoseEvent& openPoseEvent)
{
	// 変数の初期化
	jobCount = 0;
	errorMessage.clear();
	this->openPoseEvent = &openPoseEvent;
	auto openPoseMode = this->openPoseEvent->selectOpenposeMode();
	opWrapper = std::make_unique<op::Wrapper>();
	// OpenPose の設定
	op::WrapperStructPose wrapperStructPose;
	// 使用する骨格モデルの選択
	wrapperStructPose.poseModel = openPoseMode.first;
	// ネットワークの解像度の設定 (16の倍数のみ指定可能, -1は縦横比に合わせて自動計算される)
	// 値は大きいほど精度が高く、処理も重い
	wrapperStructPose.netInputSize = openPoseMode.second;
	// OpenPose に設定を適応
	opWrapper->configure(wrapperStructPose);
	// 画像入力 Worker の設定
	opWrapper->setWorker(op::WorkerType::Input, opInput, true);
	// 画像出力 Worker の設定
	opWrapper->setWorker(op::WorkerType::Output, opOutput, true);
	// 関数の戻り値を代入する一時的変数
	int ret = 0;
	// OpenPose の実行
	opThread = std::thread([&] {
		try {
			opWrapper->exec();
		}
		catch (const std::exception& e) {
			std::lock_guard<std::mutex> inOutLock(inOutMtx);
			errorMessage.push_back(e.what());
		}
	});
	// イベントリスナーの初期化関数を実行
	ret = this->openPoseEvent->init();
	if (ret) errorMessage.push_back("OpenPoseEvent::init returned 1.");
	// イベントループの開始
	while (true)
	{
		// OpenPose の処理状態の確認
		switch (getProcessState())
		{
		case MinimumOpenPose::ProcessState::WaitInput: // 入力待機
			ret = this->openPoseEvent->sendImageInfo(imageInfo, [this](){ shutdown(); });
			if (ret)
			{
				errorMessage.push_back("OpenPoseEvent::sendImageInfo returned 1.");
				break;
			}
			if (imageInfo.needOpenposeProcess)
			{
				pushImage(imageInfo.inputImage, imageInfo.frameNumber); // OpenPose に画像を渡す
			}
			else
			{
				imageInfo.outputImage = imageInfo.inputImage;
			}
			break;
		case MinimumOpenPose::ProcessState::Processing: // 処理中
			break;
		case MinimumOpenPose::ProcessState::Finish: // 処理の終了
			{
				auto results = getResultsAndReset(); // 出力されたデータの取得
				if (!static_cast<bool>(results)) break;
				for (auto result : *results)
				{
					imageInfo.people.clear();
					// 画面内に映っている人数分ループする
					for (int peopleIndex = 0; peopleIndex < result->poseKeypoints.getSize(0); peopleIndex++)
					{
						std::vector<ImageInfo::Node> nodes;
						for (int nodeIndex = 0; nodeIndex < result->poseKeypoints.getSize(1); nodeIndex++)
						{
							nodes.push_back(ImageInfo::Node{
								result->poseKeypoints[{peopleIndex, nodeIndex, 0}],
								result->poseKeypoints[{peopleIndex, nodeIndex, 1}],
								result->poseKeypoints[{peopleIndex, nodeIndex, 2}]
							});
						}
						imageInfo.people.emplace_back(std::move(nodes));
					}
					// 画像のコピー
					imageInfo.outputImage = result->cvOutputData;
					ret = this->openPoseEvent->recieveImageInfo(imageInfo, [this]() { shutdown(); });
					if (ret)
					{
						errorMessage.push_back("OpenPoseEvent::recieveImageInfo returned 1.");
						break;
					}
				}
			}
			break;
		case ProcessState::Shutdown:
			return (!errorMessage.empty());
			break;
		}
	}
	return 0;
}

void MinimumOpenPose::shutdown()
{
	if (!isStartup()) return;
	if (this->openPoseEvent) this->openPoseEvent->exit();
	opInput->shutdown();
	opOutput->shutdown();
	opThread.join();
}

bool MinimumOpenPose::isStartup() { return opThread.joinable(); }

int MinimumOpenPose::pushImage(cv::Mat& image, size_t frameNumber, size_t maxQueueSize)
{
	if (
		(!isStartup()) ||
		(image.type() != CV_8UC3)
	) return 1;
	if (opInput->pushImage(image, frameNumber, maxQueueSize)) return 1;
	jobCount++;
	return 0;
}

MinimumOpenPose::ProcessState MinimumOpenPose::getProcessState()
{
	assert(opOutput->getResultsSize() > jobCount);
	
	opInput->getErrors(errorMessage, true);
	opOutput->getErrors(errorMessage, true);
	if (!errorMessage.empty())
	{
		this->openPoseEvent->recieveErrors(errorMessage);
		shutdown();
	}

	if (!isStartup()) return ProcessState::Shutdown;
	else if (opOutput->getResultsSize() < jobCount) return ProcessState::Processing;
	else if (opOutput->getResultsSize() == jobCount)
	{ 
		if (opOutput->getResultsSize() == 0) return ProcessState::WaitInput;
		else return ProcessState::Finish;
	}
}

std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>> MinimumOpenPose::getResultsAndReset()
{
	if (
		(!isStartup()) ||
		(getProcessState() != ProcessState::Finish)
		) return std::make_shared<std::vector<std::shared_ptr<op::Datum>>>();
	jobCount = 0;
	return opOutput->getResultsAndReset();
}