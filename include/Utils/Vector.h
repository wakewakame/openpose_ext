#pragma once

#include <iostream>
#include <iomanip>
#include <cmath>

#include <opencv2/opencv.hpp>

#define M_PI 3.14159265358979

// Vector Tools
namespace vt
{
	// 4x4行列の構造体
	struct Matrix4
	{
		double
			m00, m01, m02, m03,
			m10, m11, m12, m13,
			m20, m21, m22, m23,
			m30, m31, m32, m33;
		bool isEnable;  // 
		Matrix4(bool isEnable = true);
		Matrix4(
			double m00, double m01, double m02, double m03,
			double m10, double m11, double m12, double m13,
			double m20, double m21, double m22, double m23,
			double m30, double m31, double m32, double m33
		);
		Matrix4(const Matrix4& src);
		Matrix4& operator=(const Matrix4& src);
		Matrix4 getInvert();
		static Matrix4 cross(const Matrix4& left, const Matrix4& right)
		{
			return Matrix4(
				left.m00 * right.m00 + left.m01 * right.m10 + left.m02 * right.m20 + left.m03 * right.m30,
				left.m00 * right.m01 + left.m01 * right.m11 + left.m02 * right.m21 + left.m03 * right.m31,
				left.m00 * right.m02 + left.m01 * right.m12 + left.m02 * right.m22 + left.m03 * right.m32,
				left.m00 * right.m03 + left.m01 * right.m13 + left.m02 * right.m23 + left.m03 * right.m33,
				left.m10 * right.m00 + left.m11 * right.m10 + left.m12 * right.m20 + left.m13 * right.m30,
				left.m10 * right.m01 + left.m11 * right.m11 + left.m12 * right.m21 + left.m13 * right.m31,
				left.m10 * right.m02 + left.m11 * right.m12 + left.m12 * right.m22 + left.m13 * right.m32,
				left.m10 * right.m03 + left.m11 * right.m13 + left.m12 * right.m23 + left.m13 * right.m33,
				left.m20 * right.m00 + left.m21 * right.m10 + left.m22 * right.m20 + left.m23 * right.m30,
				left.m20 * right.m01 + left.m21 * right.m11 + left.m22 * right.m21 + left.m23 * right.m31,
				left.m20 * right.m02 + left.m21 * right.m12 + left.m22 * right.m22 + left.m23 * right.m32,
				left.m20 * right.m03 + left.m21 * right.m13 + left.m22 * right.m23 + left.m23 * right.m33,
				left.m30 * right.m00 + left.m31 * right.m10 + left.m32 * right.m20 + left.m33 * right.m30,
				left.m30 * right.m01 + left.m31 * right.m11 + left.m32 * right.m21 + left.m33 * right.m31,
				left.m30 * right.m02 + left.m31 * right.m12 + left.m32 * right.m22 + left.m33 * right.m32,
				left.m30 * right.m03 + left.m31 * right.m13 + left.m32 * right.m23 + left.m33 * right.m33
			);
		}
		void print();
	};
	// 4次元ベクトルの構造体
	struct Vector4
	{
		double x, y, z, w;
		bool isEnable;
		Vector4(bool isEnable = true);
		Vector4(double x, double y, double z = 0.0, double w = 1.0);
		Vector4(const Vector4& src);
		Vector4(const cv::Point& src);
		Vector4(const cv::Point2f& src);
		Vector4(const cv::Point2d& src);
		Vector4& operator=(const Vector4& src);
		bool operator==(const Vector4& src) const;
		bool operator!=(const Vector4& src) const;
		Vector4 operator+(const Vector4& src) const;
		Vector4 operator+(const double num) const;
		Vector4 operator-(const Vector4& src) const;
		Vector4 operator-(const double num) const;
		Vector4 operator*(const Vector4& src) const;
		Vector4 operator*(const double num) const;
		Vector4 operator/(const Vector4& src) const;
		Vector4 operator/(const double num) const;
		operator cv::Point() const;
		operator cv::Point2f() const;
		operator cv::Point2d() const;
		double length() const;
		Vector4 normal() const;
		Vector4 worldToScreen(const Matrix4 projModelView) const;
		Vector4 screenToWorld(const Matrix4 proj, const double wTmp, const double hTmp) const;
		double dot(const Vector4& left, const Vector4& right) const;
		static Vector4 cross(const Vector4& left, const Vector4& right)
		{
			return Vector4(
				left.y * right.z - left.z * right.y,
				left.z * right.x - left.x * right.z,
				left.x * right.y - left.y * right.x,
				left.w * right.w
			);
		}
		static Vector4 cross(const Matrix4& mat, const Vector4& vec)
		{
			return Vector4(
				mat.m00 * vec.x + mat.m01 * vec.y + mat.m02 * vec.z + mat.m03 * vec.w,
				mat.m10 * vec.x + mat.m11 * vec.y + mat.m12 * vec.z + mat.m13 * vec.w,
				mat.m20 * vec.x + mat.m21 * vec.y + mat.m22 * vec.z + mat.m23 * vec.w,
				mat.m30 * vec.x + mat.m31 * vec.y + mat.m32 * vec.z + mat.m33 * vec.w
			);
		}
		void print();
	};
	// クォータニオン回転
	Matrix4 getRotateMatrix(Vector4 axis, double rad);
	double deg2rad(double deg);
	double rad2deg(double rad);
	class FisheyeToFlat
	{
	private:
		// メンバ変数(fx, fy, cx, cy, k1, k2, k3, k4)の意味については以下のURLを参照
		//   http://opencv.jp/opencv-2.1/cpp/camera_calibration_and_3d_reconstruction.html
		double fx = 0.0, fy = 0.0;  // カメラの内部パラメータ行列の焦点距離
		double cx = 0.0, cy = 0.0;  // カメラの内部パラメータ行列の主点
		double k1 = 0.0, k2 = 0.0, k3 = 0.0, k4 = 0.0;  // カメラの歪み係数(distortion coefficients)
		double cam_width = 0.0, cam_height = 0.0;  // カメラキャリブレーションに用いた画像の解像度
		double input_width = 0.0, input_height = 0.0;  // 入力画像の解像度
		double output_scale = 1.0;  // 出力画像の拡大率
		cv::Mat map1, map2;  // キャリブレーション後のピクセルの移動位置を保持する配列
		bool change_param = false;  // パラメータ変更フラグ

	public:
		bool is_init = false;
		FisheyeToFlat();
		virtual ~FisheyeToFlat();
		void setParams(
			double cam_width, double cam_height, double output_scale,
			double fx, double fy, double cx, double cy,
			double k1 = 0.0, double k2 = 0.0, double k3 = 0.0, double k4 = 0.0
		);
		Vector4 translate(Vector4 p, double cols, double rows) const;
		Vector4 translate(Vector4 p, const cv::Mat& src) const;
		// キャリブレーション後のカメラの水平画角を計算する(余白も含めた画角)
		double calcCamWFov(double cam_w_fov, double cols, double rows) const;
		double calcCamWFov(double cam_w_fov, const cv::Mat& src) const;
		// キャリブレーション後のカメラの垂直画角を計算する(余白も含めた画角)
		double calcCamHFov(double cam_h_fov, double cols, double rows) const;
		double calcCamHFov(double cam_h_fov, const cv::Mat& src) const;
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
		double cam_w, cam_h;  // カメラの解像度
		double cam_h_fov;  // カメラの垂直画角(deg)
		double cam_pos_h;  // カメラの地面からの高さ(m)
		Vector4 p1_, p2_, p3_, p4_;  // カメラキャリブレーション前のスクリーン座標(1点目, 2点目, 3点目, 4点目)
		Vector4 p1, p2, p3, p4;  // カメラキャリブレーション後のスクリーン座標(1点目, 2点目, 3点目, 4点目)
		FisheyeToFlat fisheyeToFlat;  // 魚眼レンズの歪み補正を行うクラス

		// 計算により求まるパラメーター
		double cam_l;  // カメラ座標の原点から画面の中心までの距離(ピクセル単位)
		double cam_w_fov;  // カメラの水平画角
		Matrix4 r2;  // モデルビュー行列の逆行列
		Vector4 m_cam_pos;  // マーカー空間でのカメラ座標
		double m_cam_h;  // マーカー空間でのカメラの高さの計算

		void calcParams();

	public:
		ScreenToGround();

		virtual ~ScreenToGround();

		void setParams(
			double cam_w_tmp, double cam_h_tmp, double cam_h_fov_tmp, double cam_pos_h_tmp,
			Vector4 p1_tmp, Vector4 p2_tmp, Vector4 p3_tmp, Vector4 p4_tmp
		);

		/**
		 * 射影変換に必要なパラメーターを入力する関数
		 * @param cam_w_tmp, cam_h_tmp 入力画像の解像度
		 * @param cam_h_fov_tmp カメラの垂直画角(deg)
		 * @param cam_pos_h_tmp カメラの地面からの高さ(m)
		 * @param x1, y1 カメラに写っている地面の任意の点1
		 * @param x2, y2 カメラに写っている地面の任意の点2
		 * @param x3, y3 カメラに写っている地面の任意の点3
		 * @param x4, y4 カメラに写っている地面の任意の点4
		 * @note
		 * このクラスでは、x1やy1などで指定した4つの点を長方形になるように引き伸ばす処理を行う。
		 * これにより、カメラの画像を上から見たような画像に変換する。
		 */
		void setParams(
			double cam_w_tmp, double cam_h_tmp, double cam_h_fov_tmp, double cam_pos_h_tmp,
			double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4
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
			double cam_width, double cam_heigth, double output_scale,
			double fx, double fy, double cx, double cy, double k1, double k2, double k3, double k4
		);

		/**
		 * 1つの点を座標変換する
		 * @param p 画像上の任意の座標 (x, y, z, w の内 x, y のみを扱う)
		 * @return 変換後の座標
		 */
		Vector4 translate(Vector4 p) const;

		/**
		 * 画像を変換する
		 * @param src 入力する画像
		 * @param zoom 拡大率
		 * @param drawLine trueにするとsetParamsで指定した4点に直線を描画を行う
		 * @return 変換後の画像
		 */
		cv::Mat ScreenToGround::translateMat(const cv::Mat& src, float zoom = 1.0f, bool drawLine = false);

		/**
		 * 座標変換で歪み補正のみを行う
		 * @param 画像上の任意の座標 (x, y, z, w の内 x, y のみを扱う)
		 * @return 変換後の座標
		 */
		Vector4 ScreenToGround::onlyFlat(Vector4 p);

		/**
		 * 画像変換で歪み補正のみを行う
		 * @param 入力する画像
		 * @return 変換後の画像
		 */
		cv::Mat ScreenToGround::onlyFlatMat(const cv::Mat& src);

		/**
		 * setParamsで指定した4点に直線を描画を行う
		 * @param mat 入力する画像
		 * @param mode 0を指定すると歪み補正前の位置に直線を描く 1を指定すると歪み補正後の位置に直線を描く
		 */
		void drawAreaLine(cv::Mat& mat, uint8_t mode);
	};
};