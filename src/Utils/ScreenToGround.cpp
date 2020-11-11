#pragma once

#include <Utils/Vector.h>

namespace vt
{
	ScreenToGround::ScreenToGround() {}
	ScreenToGround::~ScreenToGround() {}
	void ScreenToGround::setParams(
		float cam_w, float cam_h,
		float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4,
		float rect_width, float rect_height
	)
	{
		// パラメータに変更がなければ計算を行わないようにする
		if (
			(this->cam_w != cam_w) ||
			(this->cam_h != cam_h) ||
			(p1_ != cv::Point2f{x1, y1}) ||
			(p2_ != cv::Point2f{x2, y2}) ||
			(p3_ != cv::Point2f{x3, y3}) ||
			(p4_ != cv::Point2f{x4, y4}) ||
			(rect_size != cv::Point2f(rect_width, rect_height))
		)
		{
			// 引数に指定された値を記録する
			this->cam_w = cam_w;
			this->cam_h = cam_h;
			p1_ = cv::Point2f{ x1, y1 };
			p2_ = cv::Point2f{ x2, y2 };
			p3_ = cv::Point2f{ x3, y3 };
			p4_ = cv::Point2f{ x4, y4 };
			rect_size = cv::Point2f{ rect_width, rect_height };

			// 歪み補正後の4点
			p1 = fisheyeToFlat.translate(p1_, this->cam_w, this->cam_h);
			p2 = fisheyeToFlat.translate(p2_, this->cam_w, this->cam_h);
			p3 = fisheyeToFlat.translate(p3_, this->cam_w, this->cam_h);
			p4 = fisheyeToFlat.translate(p4_, this->cam_w, this->cam_h);

			// 透視変換行列を求める
			std::vector<cv::Point2f> srcPoint = { p1, p2, p3, p4 };
			std::vector<cv::Point2f> dstPoint = {
				cv::Point2f{ 0          , 0           },
				cv::Point2f{ rect_size.x, 0           },
				cv::Point2f{ rect_size.x, rect_size.y },
				cv::Point2f{ 0          , rect_size.y }
			};
			perspectiveTransformMatrix = cv::getPerspectiveTransform(srcPoint, dstPoint);
		}
	}
	void ScreenToGround::setCalibration(
		float cam_width, float cam_heigth, float output_scale,
		float fx, float fy, float cx, float cy, float k1, float k2, float k3, float k4
	) {
		fisheyeToFlat.setParams(
			cam_width, cam_heigth, output_scale, fx, fy, cx, cy, k1, k2, k3, k4
		);
	}
	cv::Point2f ScreenToGround::getRectSize() const
	{
		return rect_size;
	}
	cv::Point2f ScreenToGround::translate(cv::Point2f p) const
	{
		// 魚眼レンズによる歪み修正
		p = fisheyeToFlat.translate(p, cam_w, cam_h);
		
		// 行列の先頭アドレスを求める
		const double* mat = perspectiveTransformMatrix.ptr<double>(0);

		// pベクトルとmat行列の積
		cv::Point3f result{
			p.x * (float)mat[0] + p.y * (float)mat[1] + 1.0f * (float)mat[2],
			p.x * (float)mat[3] + p.y * (float)mat[4] + 1.0f * (float)mat[5],
			p.x * (float)mat[6] + p.y * (float)mat[7] + 1.0f * (float)mat[8]
		};

		result.x = result.x / result.z;
		result.y = result.y / result.z;

		return cv::Point2f{ result.x, result.y };
	}
	cv::Mat ScreenToGround::translateMat(const cv::Mat& src, float zoom, bool drawLine)
	{
		// 透視変換行列を求める
		std::vector<cv::Point2f> srcPoint = { p1, p2, p3, p4 };
		std::vector<cv::Point2f> dstPoint = {
			plot({0.f, 0.f}, src, zoom),
			plot({rect_size.x, 0.f}, src, zoom),
			plot({rect_size.x, rect_size.y}, src, zoom),
			plot({0.f, rect_size.y}, src, zoom)
		};
		auto mat = cv::getPerspectiveTransform(srcPoint, dstPoint);

		//図形変換処理
		cv::Mat dst = fisheyeToFlat.translateMat(src);
		cv::warpPerspective(dst, dst, mat, dst.size(), cv::INTER_LINEAR);

		if (drawLine) {
			cv::Point2f a1(dstPoint[0]), a2(dstPoint[1]), a3(dstPoint[2]), a4(dstPoint[3]);
			cv::circle(dst, cv::Point(a1.x, a1.y), 5, cv::Scalar(255, 0, 0), -1);
			cv::circle(dst, cv::Point(a2.x, a2.y), 5, cv::Scalar(255, 0, 0), -1);
			cv::circle(dst, cv::Point(a3.x, a3.y), 5, cv::Scalar(255, 0, 0), -1);
			cv::circle(dst, cv::Point(a4.x, a4.y), 5, cv::Scalar(255, 0, 0), -1);
			cv::line(dst, { (int)a1.x, (int)a1.y }, { (int)a2.x, (int)a2.y }, cv::Scalar{ 0, 0, 255 }, 2.0);
			cv::line(dst, { (int)a2.x, (int)a2.y }, { (int)a3.x, (int)a3.y }, cv::Scalar{ 0, 0, 255 }, 2.0);
			cv::line(dst, { (int)a3.x, (int)a3.y }, { (int)a4.x, (int)a4.y }, cv::Scalar{ 0, 0, 255 }, 2.0);
			cv::line(dst, { (int)a4.x, (int)a4.y }, { (int)a1.x, (int)a1.y }, cv::Scalar{ 0, 0, 255 }, 2.0);
		}

		return dst;
	}
	cv::Point2f ScreenToGround::plot(cv::Point2f p, const cv::Mat& src, float zoom) const
	{
		cv::Point2f reso{ (float)src.cols, (float)src.rows };
		float rate = (rect_size.x / rect_size.y) / (reso.x / reso.y);
		float scale = (rate > 1.0f) ? (reso.x / rect_size.x) : (reso.y / rect_size.y);
		scale *= zoom;

		p -= rect_size * 0.5f;
		p *= scale;
		p += reso * 0.5f;
		
		return p;
	}
	cv::Point2f ScreenToGround::onlyFlat(cv::Point2f p) {
		return fisheyeToFlat.translate(p, cam_w, cam_h);
	}
	cv::Mat ScreenToGround::onlyFlatMat(const cv::Mat& src) {
		return fisheyeToFlat.translateMat(src);
	}
};