#pragma once

#include <iostream>
#include <iomanip>
#include <cmath>

#include <opencv2/opencv.hpp>

#define M_PI 3.14159265358979

// Vector Tools
namespace vt
{
	struct Matrix4
	{
		double
			m00, m01, m02, m03,
			m10, m11, m12, m13,
			m20, m21, m22, m23,
			m30, m31, m32, m33;
		bool isEnable;
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
		Vector4 operator+(const Vector4& src);
		Vector4 operator+(const double num);
		Vector4 operator-(const Vector4& src);
		Vector4 operator-(const double num);
		Vector4 operator*(const Vector4& src);
		Vector4 operator*(const double num);
		Vector4 operator/(const Vector4& src);
		Vector4 operator/(const double num);
		operator cv::Point() const;
		operator cv::Point2f() const;
		operator cv::Point2d() const;
		double length();
		Vector4 normal();
		Vector4 worldToScreen(Matrix4 projModelView);
		Vector4 screenToWorld(Matrix4 proj, double wTmp, double hTmp);
		double dot(const Vector4& left, const Vector4& right);
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
	Matrix4 getRotateMatrix(Vector4 axis, double rad);
	double deg2rad(double deg);
	double rad2deg(double rad);
	class ScreenToGround
	{
	private:
		// ユーザー定義パラメーター
		double cam_w, cam_h;  // カメラの解像度
		double cam_h_fov;  // カメラの垂直画角(deg)
		double cam_pos_h;  // カメラの地面からの高さ(m)
		Vector4 p1, p2, p3, p4;  // スクリーン座標(1点目, 2点目, 3点目, 4点目)

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
			const Vector4& p1_tmp, const Vector4& p2_tmp, const Vector4& p3_tmp, const Vector4& p4_tmp
		);
		void setParams(
			double cam_w_tmp, double cam_h_tmp, double cam_h_fov_tmp, double cam_pos_h_tmp,
			double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4
		);
		Vector4 translate(Vector4 p);
		void drawAreaLine(cv::Mat& mat);
		cv::Mat ScreenToGround::perspective(const cv::Mat& src, float zoom = 1.0f);
	};
	class FisheyeToFlat {
	private:
		// カメラ内部パラメータ
		cv::Mat cameraMatrix;
		// 歪み補正パラメータ[k1, k2, k3, k4]
		cv::Mat distCoeffs;
		// カメラの解像度
		double cam_width = 0.0, cam_height = 0.0;
		// 入力画像の解像度
		double input_width = 0.0, input_height = 0.0;
		// キャリブレーション後のピクセルの移動位置を保持する配列
		cv::Mat map1, map2;

	public:
		FisheyeToFlat() {}
		virtual ~FisheyeToFlat() {}
		void setParams(
			double width, double height,
			double f1, double f2, double c1, double c2,
			double k1 = 0.0, double k2 = 0.0, double k3 = 0.0, double k4 = 0.0
		) {
			cam_width = width;
			cam_height = height;
			cameraMatrix = (cv::Mat_<float>(3, 3) << f1, 0.0, c1, 0.0, f2, c2, 0.0, 0.0, 1.0);
			distCoeffs = (cv::Mat_<float>(1, 4) << 0.0f, 0.0f, 0.0f, 0.0f);
		}
		//Vector4 translate(Vector4 p) {

		//}
		cv::Mat calibrate(const cv::Mat& src, float zoom = 1.0) {
			cv::Mat dst = cv::Mat::zeros(src.rows, src.cols, src.type());
			if (input_width != (double)(src.cols) || input_height != (double)(src.rows)) {
				input_width = (double)(src.cols);
				input_height = (double)(src.rows);
				cv::Mat inputCameraMatrix = cameraMatrix * input_width / cam_width;
				inputCameraMatrix.at<float>(2, 2) = 1.0;
				cv::Mat outputCameraMatrix = inputCameraMatrix.clone();
				outputCameraMatrix.at<float>(0, 0) *= zoom;
				outputCameraMatrix.at<float>(1, 1) *= zoom;
				cv::fisheye::initUndistortRectifyMap(
					inputCameraMatrix, distCoeffs, cv::Matx33d::eye(),
					outputCameraMatrix, src.size(), CV_16SC2, map1, map2
				);
			}
			cv::remap(src, dst, map1, map2, cv::INTER_LINEAR, cv::BORDER_CONSTANT);
			return dst;
		}
	};
};