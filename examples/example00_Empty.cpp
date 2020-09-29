/*

openpose_extはOpenPoseをよりシンプルに扱えるようにしたライブラリです。
このライブラリでは、次のような流れでプログラムを実行します。

	1. OpenPoseに入力する画像を準備する
	2. OpenPoseで画像から骨格を解析する
	3. OpenPoseによって解析されたデータを受け取る
	4. 1に戻る

この内、1と3を自分で定義してプログラムを設計します。

以下のサンプルはopenpose_extを用いた最小限のプログラムです。

*/

#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <OpenPoseWrapper/Examples/PreviewOpenPoseEvent.h>
#include <Utils/Gui.h>

// OpenPoseEvent
class CustomOpenPoseEvent : public OpenPoseEvent
{
public:
	// 初期化時に呼び出される関数
	int init() override final
	{
		// 最初にこの関数が呼ばれる
		// 正常に処理が終了した場合は0を返す
		// エラーなどが起こった場合には1を返す
		return 0;
	}

	// OpenPoseに入力する画像を準備する関数
	int sendImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		// openposeの画像処理が行われる前にこの関数が呼ばれる
		// 引数のimageInfo.inputImageに画像を書き込むと、それがopenposeの入力画像として処理される
		// プログラムを終了させたい場合には、引数のexit関数を呼び出す
		// OpenPoseEventが複数登録されている場合は、登録した順番でこの関数が呼ばれる
		// 正常に処理が終了した場合は0を返す
		// エラーなどが起こった場合には1を返す
		return 0;
	}

	// OpenPoseによって解析されたデータを受け取る関数
	int recieveImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		// openposeの画像処理が行われた後にこの関数が呼ばれる
		// 引数のimageInfoから解析結果のデータを得ることができる
		// プログラムを終了させたい場合には、引数のexit関数を呼び出す
		// OpenPoseEventが複数登録されている場合は、登録した順番と逆の順番でこの関数が呼ばれる
		// 正常に処理が終了した場合は0を返す
		// エラーなどが起こった場合には1を返す
		return 0;
	}
};

int main(int argc, char* argv[])
{
	// openposeのラッパークラス
	MinimumOpenPose mop;

	// 自分で定義したイベントリスナーを登録する
	// addEventListenerはテンプレート引数に指定したクラスをshared_ptrでインスタンス化し、それを返す
	// addEventListenerに引数を指定した場合、テンプレート引数に指定したクラスのコンストラクタ引数として展開される
	mop.addEventListener<CustomOpenPoseEvent>();

	// openposeの起動
	int ret = mop.startup();

	return ret;
}
