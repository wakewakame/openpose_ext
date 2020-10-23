#pragma once

#include <OpenPoseWrapper/MinimumOpenPose.h>

class Video
{
private:
	cv::VideoCapture videoCapture;
	bool play_, needUpdate;
	cv::Mat buffer;

public:
	struct FrameInfo{
		size_t frameNumber, frameSum, frameTimeStamp;
	};
	Video() : play_(true), needUpdate(false) {}
	virtual ~Video() {};

	// 動画ファイルを開く
	int open(const std::string& videoPath)
	{
		// 動画ファイルを開く
		videoCapture.open(videoPath);

		// エラーの確認
		if (!videoCapture.isOpened())
		{
			std::cout << videoPath << "を開けませんでした。" << std::endl;
			return 1;
		}

		return 0;
	}

	// 動画の次のフレームを取得する
	cv::Mat next()
	{
		cv::Mat ret;

		// 動画を開いていない場合は処理を終了
		if (!videoCapture.isOpened()) return ret;

		// 一時停止状態かつ、画面を更新する必要がなければ以前取得したフレームを返す
		if ((!play_) && (!needUpdate)) return buffer;
		needUpdate = false;

		// 次のフレームを取得
		videoCapture.read(ret);
		buffer = ret;

		return ret;
	}

	// 動画の再生状態を取得
	FrameInfo getInfo() const
	{
		FrameInfo ret;

		// 現在の再生位置(フレーム単位)
		ret.frameNumber = (size_t)videoCapture.get(cv::CAP_PROP_POS_FRAMES);
		
		// 全フレームの枚数
		ret.frameSum = (size_t)videoCapture.get(cv::CAP_PROP_FRAME_COUNT);
		
		// 現在の再生位置(ミリ秒単位)
		ret.frameTimeStamp = (size_t)videoCapture.get(cv::CAP_PROP_POS_MSEC);

		return ret;
	}

	// 動画を再生する
	void play() { play_ = true; }
	
	// 動画を一時停止する
	void pause() { play_ = false; }

	// 動画が再生状態かどうか
	bool isPlay() const { return (videoCapture.isOpened() && (play_)); }

	// 動画の再生位置を指定のフレーム番号まで移動する
	void seekAbsolute(long long frame)
	{
		if (videoCapture.isOpened())
		{
			size_t frameSum = (size_t)videoCapture.get(cv::CAP_PROP_FRAME_COUNT);
			if (frame < 0) frame = 0;
			if (frame >= frameSum) frame = frameSum - 1;
			videoCapture.set(CV_CAP_PROP_POS_FRAMES, (double)frame);
			needUpdate = true;
		}
	}

	// 動画の再生位置を指定したフレーム数分移動する
	void seekRelative(long long frame)
	{
		if (videoCapture.isOpened())
		{
			long long frameNumber = (size_t)videoCapture.get(cv::CAP_PROP_POS_FRAMES);
			seekAbsolute(frameNumber + frame);
		}
	}
};