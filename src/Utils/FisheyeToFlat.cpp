#pragma once

#include <Utils/Vector.h>

namespace vt
{
	FisheyeToFlat::FisheyeToFlat() {}
	FisheyeToFlat::~FisheyeToFlat() {}
	void FisheyeToFlat::setParams(
		float cam_width, float cam_height, float output_scale,
		float fx, float fy, float cx, float cy,
		float k1, float k2, float k3, float k4
	) {
		is_init = true;
		this->cam_width = cam_width; this->cam_height = cam_height;
		this->output_scale = output_scale;
		this->fx = fx; this->fy = fy; this->cx = cx; this->cy = cy;
		this->k1 = k1; this->k2 = k2; this->k3 = k3; this->k4 = k4;
		change_param = true;
	}
	cv::Point2f FisheyeToFlat::translate(cv::Point2f p, float cols, float rows) const {
		if (!is_init) return p;
		cv::Mat p_src = (cv::Mat_<cv::Vec2d>(1, 1) << cv::Vec2d(p.x, p.y));
		cv::Mat p_dst(1, 1, CV_64FC2);
		float input_width = cols;
		float input_height = rows;
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
		return cv::Point2f(result[0], result[1]);
	}
	cv::Point2f FisheyeToFlat::translate(cv::Point2f p, const cv::Mat& src) const {
		return translate(p, (float)src.cols, (float)src.rows);
	}
	cv::Mat FisheyeToFlat::translateMat(const cv::Mat& src) {
		if (!is_init) return src.clone();
		cv::Mat dst = cv::Mat::zeros(src.rows, src.cols, src.type());
		if (input_width != (float)(src.cols) || input_height != (float)(src.rows) || change_param) {
			input_width = (float)(src.cols);
			input_height = (float)(src.rows);
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