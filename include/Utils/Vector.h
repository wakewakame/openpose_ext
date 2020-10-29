#pragma once

#include <opencv2/opencv.hpp>

#define M_PI 3.14159265358979

// Vector Tools
namespace vt
{
	class FisheyeToFlat
	{
	private:
		// メンバ変数(fx, fy, cx, cy, k1, k2, k3, k4)の意味については以下のURLを参照
		// http://opencv.jp/opencv-2.1/cpp/camera_calibration_and_3d_reconstruction.html
		float fx = 0.0, fy = 0.0;  // カメラの内部パラメータ行列の焦点距離
		float cx = 0.0, cy = 0.0;  // カメラの内部パラメータ行列の主点
		float k1 = 0.0, k2 = 0.0, k3 = 0.0, k4 = 0.0;  // カメラの歪み係数(distortion coefficients)
		float cam_width = 0.0, cam_height = 0.0;  // カメラキャリブレーションに用いた画像の解像度
		float input_width = 0.0, input_height = 0.0;  // 入力画像の解像度
		float output_scale = 1.0;  // 出力画像の拡大率
		cv::Mat map1, map2;  // 歪み補正後のピクセルの移動位置を保持する配列
		bool change_param = false;  // パラメータ変更フラグ

	public:
		bool is_init = false;
		FisheyeToFlat();
		virtual ~FisheyeToFlat();
		void setParams(
			float cam_width, float cam_height, float output_scale,
			float fx, float fy, float cx, float cy,
			float k1 = 0.0, float k2 = 0.0, float k3 = 0.0, float k4 = 0.0
		);
		cv::Point2f translate(cv::Point2f p, float cols, float rows) const;
		cv::Point2f translate(cv::Point2f p, const cv::Mat& src) const;
		cv::Mat translateMat(const cv::Mat& src);
	};

	/**
	 * このクラスでは画面上の指定された4つの点を長方形になるように引き伸ばす処理を行う
	 * これにより、カメラの画像を「地面を上から見たような画像」に変換する
	 * また、カメラの映像が魚眼レンズなどで歪んでいる場合は歪み補正も同時に行うことができる
	 */
	class ScreenToGround
	{
	private:
		// ユーザー定義パラメーター
		float cam_w = 1.0f, cam_h = 1.0f;  // カメラの解像度
		cv::Point2f p1_, p2_, p3_, p4_;  // 歪み補正前のスクリーン座標 (左上, 右上, 右下, 左下)
		cv::Point2f p1, p2, p3, p4;  // 歪み補正後のスクリーン座標 (左上, 右上, 右下, 左下)
		cv::Point2f rect_size;  // p1からp4が囲う矩形のサイズ (p1 から p2 までの距離, p2 から p3 までの距離)
		cv::Mat perspectiveTransformMatrix;  // 透視変換行列
		FisheyeToFlat fisheyeToFlat;  // 魚眼レンズの歪み補正を行うクラス

	public:
		ScreenToGround();

		virtual ~ScreenToGround();

		/**
		 * 射影変換に必要なパラメーターを入力する関数
		 * @param cam_w, cam_h 入力画像の解像度
		 * @param x1, y1 カメラに写っている地面の任意の点1 (左上)
		 * @param x2, y2 カメラに写っている地面の任意の点2 (右上)
		 * @param x3, y3 カメラに写っている地面の任意の点3 (右下)
		 * @param x4, y4 カメラに写っている地面の任意の点4 (左下)
		 * @param rect_width (x1, y1) から (x2, y2) までの長さを指定する (単位は任意)
		 * @param rect_height (x2, y2) から (x3, y3) までの長さを指定する (単位は任意)
		 * @note
		 * このクラスでは、x1やy1などで指定した4つの点を長方形になるように引き伸ばす処理を行う。
		 * これにより、カメラの画像を上から見たような画像に変換する。
		 */
		void setParams(
			float cam_w, float cam_h,
			float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4,
			float rect_width = 1.0, float rect_height = 1.0
		);

		/**
		 * カメラの歪み補正を行うパラメーターを入力する関数
		 * @param cam_width, cam_heigth カメラキャリブレーションを行った時のカメラの解像度
		 * @param output_scale 歪み補正後の画像の拡大率
		 * @param fx, fy カメラ内部パラメータの焦点距離 (ピクセル単位)
		 * @param cx, cy カメラ内部パラメータの主点位置 (ピクセル単位)
		 * @param k1, k2, k3, k4 半径方向の歪み係数
		 * @note
		 * 焦点距離や主点位置、半径方向の歪み係数については、カメラキャリブレーションで得られた値を入力してください。
		 * カメラキャリブレーションについては以下のサイトが参考になります。
		 * http://opencv.jp/opencv-2.1/cpp/camera_calibration_and_3d_reconstruction.html
		 * 具体的な方法については以下のサイトが参考になります。
		 * https://medium.com/@kennethjiang/calibrate-fisheye-lens-using-opencv-part-2-13990f1b157f
		 * 
		 * Todo: カメラキャリブレーションについての説明をもっと分かりやすくする
		 */
		void setCalibration(
			float cam_width, float cam_heigth, float output_scale,
			float fx, float fy, float cx, float cy, float k1, float k2, float k3, float k4
		);

		/**
		 * 指定した4点の縦横サイズを取得する (setParams で指定した rect_width と rect_height が帰ってくる)
		 * @return rect_width に横幅 rect_height に縦幅が代入される
		 */
		cv::Point2f getRectSize() const;

		/**
		 * 1つの点を座標変換する
		 * @param p 画像上の任意の座標
		 * @return 変換後の座標
		 */
		cv::Point2f translate(cv::Point2f p) const;

		/**
		 * 画像を変換する
		 * @param src 入力する画像
		 * @param zoom 拡大率
		 * @param drawLine trueにするとsetParamsで指定した4点に直線を描画を行う
		 * @return 変換後の画像
		 */
		cv::Mat ScreenToGround::translateMat(const cv::Mat& src, float zoom = 1.0f, bool drawLine = false);

		/**
		 * translateで変換した座標をtranslateMatで表示される座標に変換する
		 * @param p translateで変換した座標
		 * @return translateMatで表示される座標
		 */
		cv::Point2f plot(cv::Point2f p, const cv::Mat& src, float zoom) const;

		/**
		 * 座標変換で歪み補正のみを行う
		 * @param 画像上の任意の座標 (x, y, z, w の内 x, y のみを扱う)
		 * @return 変換後の座標
		 */
		cv::Point2f ScreenToGround::onlyFlat(cv::Point2f p);

		/**
		 * 画像変換で歪み補正のみを行う
		 * @param 入力する画像
		 * @return 変換後の画像
		 */
		cv::Mat ScreenToGround::onlyFlatMat(const cv::Mat& src);
	};
};