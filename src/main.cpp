#include <MinOpenPose.h>

int main(int argc, char* argv[])
{
	// Webカメラに接続開始
	cv::VideoCapture cap(0);
	if (!cap.isOpened()) return -1;

	// OpenPoseの起動
	MinimumOpenPose mop;
	mop.startup();
	
	// OpenPoseのイベントループ処理
	while (true)
	{
		// OpenPose の処理状態の確認
		switch (mop.getProcessState())
		{
		case MinimumOpenPose::ProcessState::WaitInput: // 入力待機
		{
			auto frame = std::make_unique<cv::Mat>();
			cap.read(*frame);
			mop.pushImage(std::move(frame)); // OpenPose に画像を渡す
			if (cv::waitKey(1) == 0x1b) return 0;
		}
		break;
		case MinimumOpenPose::ProcessState::Processing: // 処理中
			break;
		case MinimumOpenPose::ProcessState::Finish: // 処理の終了
		{
			auto results = mop.getResultsAndReset(); // 出力されたデータの取得
			if (!static_cast<bool>(results)) break;
			for (auto result : *results)
			{
				cv::imshow("result", result->cvOutputData);
			}
		}
		break;
		case MinimumOpenPose::ProcessState::Shutdown: // 起動していない
			break;
		case MinimumOpenPose::ProcessState::Error: // エラー発生
		{
			auto errors = mop.getErrors();
			for (auto error : errors) std::cout << error << std::endl;
			mop.resetErrors();
			return -1;
		}
		break;
		}
	}

	return 0;
}