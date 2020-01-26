#include "MinOpenPose.h"

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
			datumPtr->frameNumber = frameNumber++;
			datumPtr->cvInputData = *(images.front());
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

int MinimumOpenPose::WUserInputProcessing::pushImage(std::unique_ptr<cv::Mat>&& image, size_t maxQueueSize)
{
	std::lock_guard<std::mutex> inOutLock(inOutMtx);
	if (images.size() >= maxQueueSize) return 1;
	images.push(std::move(image));
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

int MinimumOpenPose::startup(
	op::PoseModel poseModel,
	op::Point<int> netInputSize
)
{
	// 既に OpenPose が起動しているか確認
	if (isStartup()) shutdown();
	// 変数の初期化
	jobCount = 0;
	errorMessage.clear();
	// OpenPose の設定
	op::WrapperStructPose wrapperStructPose;
	// 使用する骨格モデルの選択
	wrapperStructPose.poseModel = poseModel;
	// ネットワークの解像度の設定 (16の倍数のみ指定可能, -1は縦横比に合わせて自動計算される)
	// 値は大きいほど精度が高く、処理も重い
	wrapperStructPose.netInputSize = netInputSize;
	// OpenPose に設定を適応
	opWrapper.configure(wrapperStructPose);
	// 画像入力 Worker の設定
	opWrapper.setWorker(op::WorkerType::Input, opInput, true);
	// 画像出力 Worker の設定
	opWrapper.setWorker(op::WorkerType::Output, opOutput, true);
	// OpenPose の実行
	opThread = std::thread([&] {
		try {
			opWrapper.exec();
		}
		catch(const std::exception& e) {
			std::lock_guard<std::mutex> inOutLock(inOutMtx);
			errorMessage.push_back(e.what());
		}
	});
	return 0;
}

void MinimumOpenPose::shutdown()
{
	if (!isStartup()) return;
	opInput->shutdown();
	opOutput->shutdown();
	opThread.join();
}

bool MinimumOpenPose::isStartup() { return opThread.joinable(); }

int MinimumOpenPose::pushImage(std::unique_ptr<cv::Mat>&& image, size_t maxQueueSize)
{
	if (
		(!isStartup()) ||
		(!static_cast<bool>(image)) ||
		(image->type() != CV_8UC3) ||
		(!getErrors().empty())
	) return 1;
	if (opInput->pushImage(std::move(image), maxQueueSize)) return 1;
	jobCount++;
	return 0;
}

MinimumOpenPose::ProcessState MinimumOpenPose::getProcessState()
{
	assert(opOutput->getResultsSize() > jobCount);
	if (!getErrors().empty()) return ProcessState::Error;
	else if (!isStartup()) return ProcessState::Shutdown;
	else if (opOutput->getResultsSize() < jobCount) return ProcessState::Processing;
	else if (opOutput->getResultsSize() == jobCount)
	{ 
		if (opOutput->getResultsSize() == 0) return ProcessState::WaitInput;
		else return ProcessState::Finish;
	}
}

const std::vector<std::string>& MinimumOpenPose::getErrors()
{
	opInput->getErrors(errorMessage, true);
	opOutput->getErrors(errorMessage, true);
	if (!errorMessage.empty()) shutdown();
	return errorMessage;
}

void MinimumOpenPose::resetErrors()
{
	opInput->getErrors(errorMessage, true);
	opOutput->getErrors(errorMessage, true);
	errorMessage.clear();
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