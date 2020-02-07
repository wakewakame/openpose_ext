#pragma once

#include <openpose/headers.hpp>

struct ImageInfo
{
	struct Node { float x, y, confidence; };

	// openposeに入力するデータ
	size_t frameNumber = 0;  // フレーム番号
	size_t frameSum = 0;  // 総フレーム数
	size_t frameTimeStamp = 0;  // フレームのタイムスタンプ(ミリ秒)
	cv::Mat inputImage;  // 入力画像
	bool needOpenposeProcess = true;  // 骨格検出をするかどうか

	// openposeから受け取るデータ
	cv::Mat outputImage;  // 骨格のプレビュー画像
	std::map<size_t, std::vector<Node>> people;  // 骨格データ
};

class OpenPoseEvent
{
public:
	OpenPoseEvent() {}
	virtual ~OpenPoseEvent() {}

	// openpose起動直後に呼び出される
	virtual int init() { return 0; };

	// openposeが終了する直前に呼び出される
	virtual void exit() {};

	// openposeにデータを入力するタイミングで呼び出される
	// imageInfoの入力用のメンバ変数をこの関数で書き換える
	// openposeを停止させるときは引数のexit()を呼ぶ
	virtual int sendImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) = 0;

	// openposeからデータを受け取るタイミングで呼び出される
	// imageInfoに結果が格納される
	// openposeを停止させるときは引数のexit()を呼ぶ
	virtual int recieveImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) = 0;

	// openposeがエラーを発生したタイミングで呼び出させる
	// errorsにエラー内容が格納される
	virtual void recieveErrors(const std::vector<std::string>& errors) { (void)errors; }
};