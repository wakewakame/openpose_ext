#pragma once

#include <Utils/Vector.h>

namespace vt
{
	FisheyeToFlat::FisheyeToFlat() {}
	FisheyeToFlat::~FisheyeToFlat() {}
	void FisheyeToFlat::setParams(
		double cam_width, double cam_height, double output_scale,
		double fx, double fy, double cx, double cy,
		double k1, double k2, double k3, double k4
	) {
		//is_init = true;
		this->cam_width = cam_width;
		this->cam_height = cam_height;
		this->output_scale = output_scale;
		this->fx = fx; this->fy = fy;
		this->cx = cx; this->cy = cy;
		this->k1 = k1;
		this->k2 = k2;
		this->k3 = k3;
		this->k4 = k4;
		change_param = true;
	}
	Vector4 FisheyeToFlat::translate(Vector4 p, double cols, double rows) {
		if (!is_init) return p;
		cv::Mat p_src = (cv::Mat_<cv::Vec2d>(1, 1) << cv::Vec2d(p.x, p.y));
		cv::Mat p_dst(1, 1, CV_64FC2);
		double input_width = cols;
		double input_height = rows;
		cv::Mat cameraMatrix = (cv::Mat_<float>(3, 3) << fx, 0.0, cx, 0.0, fy, cy, 0.0, 0.0, 1.0);
		cv::Mat distCoeffs = (cv::Mat_<float>(1, 4) << k1, k2, k3, k4);
		cv::Mat inputCameraMatrix = cameraMatrix * input_width / cam_width;
		inputCameraMatrix.at<float>(2, 2) = 1.0;
		cv::Mat outputCameraMatrix = inputCameraMatrix.clone();
		outputCameraMatrix.at<float>(0, 0) *= output_scale;
		outputCameraMatrix.at<float>(1, 1) *= output_scale;
		cv::fisheye::undistortPoints(
			p_src, p_dst, inputCameraMatrix, distCoeffs, cv::Matx33d::eye(), outputCameraMatrix
		);
		cv::Vec2d result = p_dst.at<cv::Vec2d>(0, 0);
		return Vector4(result[0], result[1]);
	}
	Vector4 FisheyeToFlat::translate(Vector4 p, const cv::Mat& src) {
		return translate(p, (double)src.cols, (double)src.rows);
	}
	double FisheyeToFlat::calcCamWFov(double cam_w_fov, double cols, double rows) {
		if (!is_init) return cam_w_fov;
		double cam_w_fov_ = deg2rad(cam_w_fov);
		double before_width = cols;
		double after_left = translate(Vector4(0.0, cy), cols, rows).y;
		double after_right = translate(Vector4(before_width, cy), cols, rows).y;
		double after_width = after_right - after_left;
		double result = 2.0 * std::atan((before_width / after_width) * std::tan(cam_w_fov_ / 2.0));
		return rad2deg(result);
	}
	double FisheyeToFlat::calcCamWFov(double cam_w_fov, const cv::Mat& src) {
		return calcCamHFov(cam_w_fov, (double)src.cols, (double)src.rows);
	}
	double FisheyeToFlat::calcCamHFov(double cam_h_fov, double cols, double rows) {
		if (!is_init) return cam_h_fov;
		double cam_h_fov_ = deg2rad(cam_h_fov);
		double before_height = rows;
		double after_top = translate(Vector4(cx, 0.0), cols, rows).y;
		double after_bottom = translate(Vector4(cx, before_height), cols, rows).y;
		double after_height = after_bottom - after_top;
		double result = 2.0 * std::atan((before_height / after_height) * std::tan(cam_h_fov_ / 2.0));
		return rad2deg(result);
	}
	double FisheyeToFlat::calcCamHFov(double cam_h_fov, const cv::Mat& src) {
		return calcCamHFov(cam_h_fov, (double)src.cols, (double)src.rows);
	}
	cv::Mat FisheyeToFlat::translateMat(const cv::Mat& src) {
		if (!is_init) return src.clone();
		cv::Mat dst = cv::Mat::zeros(src.rows, src.cols, src.type());
		if (input_width != (double)(src.cols) || input_height != (double)(src.rows) || change_param) {
			input_width = (double)(src.cols);
			input_height = (double)(src.rows);
			cv::Mat cameraMatrix = (cv::Mat_<float>(3, 3) << fx, 0.0, cx, 0.0, fy, cy, 0.0, 0.0, 1.0);
			cv::Mat distCoeffs = (cv::Mat_<float>(1, 4) << k1, k2, k3, k4);
			cv::Mat inputCameraMatrix = cameraMatrix * input_width / cam_width;
			inputCameraMatrix.at<float>(2, 2) = 1.0;
			cv::Mat outputCameraMatrix = inputCameraMatrix.clone();
			outputCameraMatrix.at<float>(0, 0) *= output_scale;
			outputCameraMatrix.at<float>(1, 1) *= output_scale;
			cv::fisheye::initUndistortRectifyMap(
				inputCameraMatrix, distCoeffs, cv::Matx33d::eye(),
				outputCameraMatrix, src.size(), CV_16SC2, map1, map2
			);
			change_param = false;
		}
		cv::remap(src, dst, map1, map2, cv::INTER_LINEAR, cv::BORDER_CONSTANT);
		return dst;
	}
};