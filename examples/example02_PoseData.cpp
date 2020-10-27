/*

MinimumOpenPose では MinOpenPose::People というデータ型で骨格情報を扱います。
このサンプルでは MinOpenPose::People の扱い方についてを解説します。

*/


#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <Utils/PlotInfo.h>
#include <iostream>

int main(int argc, char* argv[])
{
	// MinimumOpenPose の初期化をする
	MinOpenPose openpose(op::PoseModel::BODY_25, op::Point<int>(-1, 368));

	// OpenPose に入力する画像を用意する
	cv::Mat image = cv::imread("media/human.jpg");

	// OpenPose で姿勢推定をする
	MinOpenPose::People people = openpose.estimate(image);

	// ここで OpenPose から people が返された。
	// people は 画面に映る人すべての骨格情報を持っている。
	// そのため、今回は映っている人数の数だけループする。

	for (auto person_itr = people.begin(); person_itr != people.end(); person_itr++)
	{
		// 1人分の骨格情報を取得する
		MinOpenPose::Person person = person_itr->second;

		// 1人分の骨格情報の中には関節の座標が配列で格納されている。
		// (BODY_25 モデルを使用している場合は25個の関節座標が入っている)
		// そのため、今度は関節の数だけループする。

		for (MinOpenPose::Node node : person)
		{
			// 関節1つ分の情報が node 変数に格納される。
			// 関節の情報には画面上の XY 座標と信頼値が格納されている。
			// 信頼値とは「関節である可能性」のような値であり、低ければ低いほど精度が低い。
			// ここでは、この3つの値をコンソールに出力する。
			std::cout
				<< "x: " << node.x << ", "  // 関節の画面上のX座標
				<< "y: " << node.y << ", "  // 関節の画面上のX座標
				<< "confidence: " << node.confidence  // 関節の信頼値
				<< std::endl;
		}
	}

	// 姿勢推定の結果を image に描画する
	plotBone(image, people, openpose);

	// できあがった画像を表示する
	cv::imshow("result", image);

	// キー入力があるまで待機する
	cv::waitKey(0);

	/**
	 
	補足

	MinOpenPose::People や MinOpenPose::Person などの中身を見たい場合は宣言や定義を確認すると良いです。
	もしこのプログラムを Visual Studio で実行している場合は People の部分を右クリックして「宣言へ移動」や「定義へ移動」などが選択できます。
	(これらのショートカットキーは F12 と Ctrl+F12 です。)
	これにより、変数や関数、クラスの中身がどうなっているのかを簡単に確認しに行くことができます。

	今回の例であれば MinOpenPose::People の正体は std::map<size_t, Person> であることがわかります。
	また MinOpenPose::Person の正体は std::vector<Node> です。
	MinOpenPose::Node の正体は単なる構造体です。

	注:　std:: から始まっている関数名やクラス名は C++ の標準ライブラリです。


	*/

	return 0;
}