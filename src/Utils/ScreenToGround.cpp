#pragma once

#include <Utils/Vector.h>

namespace vt
{
	void ScreenToGround::calcParams()
	{
		// カメラ座標の原点から画面の中心までの距離(ピクセル単位)
		cam_l = (0.5 * cam_h) / std::tan(0.5 * deg2rad(cam_h_fov));
		// カメラの水平画角
		cam_w_fov = rad2deg(2.0 * std::atan((0.5 * cam_w) / cam_l));

		// ビュー行列の計算
		// 画面上のマーカーの各辺のベクトルの計算
		Vector4 line_top = Vector4(p2.x - p1.x, p2.y - p1.y);
		Vector4 line_bottom = Vector4(p3.x - p4.x, p3.y - p4.y);
		Vector4 line_left = Vector4(p4.x - p1.x, p4.y - p1.y);
		Vector4 line_right = Vector4(p3.x - p2.x, p3.y - p2.y);
		// カメラ座標からマーカーの各点までのベクトルの計算
		Vector4 left_top = Vector4(p1.x - cam_w / 2.0, p1.y - cam_h / 2.0, cam_l);
		Vector4 right_top = Vector4(p2.x - cam_w / 2.0, p2.y - cam_h / 2.0, cam_l);
		Vector4 right_bottom = Vector4(p3.x - cam_w / 2.0, p3.y - cam_h / 2.0, cam_l);
		Vector4 left_bottom = Vector4(p4.x - cam_w / 2.0, p4.y - cam_h / 2.0, cam_l);
		// 辺とカメラ座標を通る面を画面上のマーカーの各辺で計算
		Vector4 surface_top = Vector4::cross(line_top, left_top).normal();
		Vector4 surface_bottom = Vector4::cross(line_bottom, left_bottom).normal();
		Vector4 surface_left = Vector4::cross(line_left, left_top).normal();
		Vector4 surface_right = Vector4::cross(line_right, right_top).normal();
		// 上下の面の交線と左右の面の交線を計算
		Vector4 x_vec = Vector4::cross(surface_top, surface_bottom).normal();
		Vector4 z_vec = Vector4::cross(surface_left, surface_right).normal();
		Vector4 y_vec = Vector4::cross(x_vec, z_vec).normal() * -1.0;
		// カメラの座標
		Vector4 camera_position = Vector4(0, 0, -cam_l * 4);
		// プロジェクション行列の計算
		Matrix4 projection = Matrix4(
			cam_l, 0, 0, 0,
			0, cam_l, 0, 0,
			0, 0, 1, 0,
			0, 0, 1, 0
		);
		// 回転行列の計算
		Matrix4 rotate_matrix = Matrix4(
			x_vec.x, y_vec.x, z_vec.x, 0,
			x_vec.y, y_vec.y, z_vec.y, 0,
			x_vec.z, y_vec.z, z_vec.z, 0,
			0, 0, 0, 1
		);
		// 平行移動行列の計算
		Matrix4 translate_matrix = Matrix4(
			1, 0, 0, -camera_position.x - (p1.x - cam_w / 2.0) * camera_position.z / cam_l,
			0, 1, 0, -camera_position.y - (p1.y - cam_h / 2.0) * camera_position.z / cam_l,
			0, 0, 1, -camera_position.z,
			0, 0, 0, 1
		);
		// view行列の計算
		Matrix4 view_matrix = Matrix4::cross(translate_matrix, rotate_matrix);

		// MVP行列の計算
		Matrix4 mvp = Matrix4::cross(projection, view_matrix);

		// モデルビュー行列の逆行列を計算
		r2 = view_matrix.getInvert();

		// マーカー空間でのカメラ座標の計算
		m_cam_pos = Vector4::cross(r2, Vector4(0, 0, 0));
		// マーカー空間でのカメラの高さの計算
		m_cam_h = m_cam_pos.y;
	}
	ScreenToGround::ScreenToGround() :
		cam_w(0.0), cam_h(0.0), cam_h_fov(0.0), cam_pos_h(0.0),
		cam_l(0.0), cam_w_fov(0.0), r2(), m_cam_pos(), m_cam_h(0.0)
	{}
	ScreenToGround::~ScreenToGround() {}
	void ScreenToGround::setParams(
		double cam_w_tmp, double cam_h_tmp, double cam_h_fov_tmp, double cam_pos_h_tmp,
		Vector4 p1_tmp, Vector4 p2_tmp, Vector4 p3_tmp, Vector4 p4_tmp
	)
	{
		// 魚眼レンズの歪み補正を考慮した垂直画角を計算
		cam_h_fov_tmp = fisheyeToFlat.calcCamHFov(cam_h_fov_tmp, cam_w_tmp, cam_h_tmp);
		
		// パラメータに変更がなければ計算を行わないようにする
		if (
			(cam_w != cam_w_tmp) ||
			(cam_h != cam_h_tmp) ||
			(cam_h_fov != cam_h_fov_tmp) ||
			(cam_pos_h != cam_pos_h_tmp) ||
			(p1_ != p1_tmp) ||
			(p2_ != p2_tmp) ||
			(p3_ != p3_tmp) ||
			(p4_ != p4_tmp)
			)
		{
			cam_w = cam_w_tmp;
			cam_h = cam_h_tmp;
			cam_h_fov = cam_h_fov_tmp;
			cam_pos_h = cam_pos_h_tmp;
			p1_ = p1_tmp; p2_ = p2_tmp; p3_ = p3_tmp; p4_ = p4_tmp;
			p1 = fisheyeToFlat.translate(p1_, cam_w, cam_h);
			p2 = fisheyeToFlat.translate(p2_, cam_w, cam_h);
			p3 = fisheyeToFlat.translate(p3_, cam_w, cam_h);
			p4 = fisheyeToFlat.translate(p4_, cam_w, cam_h);
			calcParams();
		}
	}
	void ScreenToGround::setParams(
		double cam_w_tmp, double cam_h_tmp, double cam_h_fov_tmp, double cam_pos_h_tmp,
		double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4
	)
	{
		setParams(
			cam_w_tmp, cam_h_tmp, cam_h_fov_tmp, cam_pos_h_tmp,
			Vector4(x1, y1), Vector4(x2, y2), Vector4(x3, y3), Vector4(x4, y4)
		);
	}
	void ScreenToGround::setCalibration(
		double cam_width, double cam_heigth, double output_scale,
		double fx, double fy, double cx, double cy, double k1, double k2, double k3, double k4
	) {
		fisheyeToFlat.setParams(
			cam_width, cam_heigth, output_scale, fx, fy, cx, cy, k1, k2, k3, k4
		);
	}
	Vector4 ScreenToGround::translate(Vector4 p)
	{
		// 魚眼レンズによる歪み修正
		p = fisheyeToFlat.translate(p, cam_w, cam_h);
		// 画面上の座標pをマーカー座標に変換
		Vector4 m_p_pos = Vector4::cross(r2, Vector4(p.x - cam_w / 2.0, p.y - cam_h / 2.0, cam_l));
		// m_cam_pos から m_p_pos へのベクトル
		Vector4 mmcvec = m_p_pos - m_cam_pos;
		// (m_cam_pos + m_p_pos * ___).y = 0 となる___を見つける
		double k = m_cam_pos.y / mmcvec.y;
		// マーカー平面での座標pの計算
		Vector4 p_p_pos = m_cam_pos - (mmcvec * k);
		// マーカー平面での座標pをメートル単位に変換
		Vector4 p_p_pos_m = p_p_pos * cam_pos_h / m_cam_h;
		return Vector4(p_p_pos_m.x, p_p_pos_m.z);
	}
	void ScreenToGround::drawAreaLine(cv::Mat& mat, uint8_t mode)
	{
		Vector4 a1, a2, a3, a4;
		if (mode == 0) { a1 = p1_; a2 = p2_; a3 = p3_; a4 = p4_; }
		if (mode == 1) { a1 = p1; a2 = p2; a3 = p3; a4 = p4; }
		cv::circle(mat, cv::Point(a1.x, a1.y), 5, cv::Scalar(255, 0, 0), -1);
		cv::circle(mat, cv::Point(a2.x, a2.y), 5, cv::Scalar(255, 0, 0), -1);
		cv::circle(mat, cv::Point(a3.x, a3.y), 5, cv::Scalar(255, 0, 0), -1);
		cv::circle(mat, cv::Point(a4.x, a4.y), 5, cv::Scalar(255, 0, 0), -1);
		cv::line(mat, { (int)a1.x, (int)a1.y }, { (int)a2.x, (int)a2.y }, cv::Scalar{ 0, 0, 255 }, 2.0);
		cv::line(mat, { (int)a2.x, (int)a2.y }, { (int)a3.x, (int)a3.y }, cv::Scalar{ 0, 0, 255 }, 2.0);
		cv::line(mat, { (int)a3.x, (int)a3.y }, { (int)a4.x, (int)a4.y }, cv::Scalar{ 0, 0, 255 }, 2.0);
		cv::line(mat, { (int)a4.x, (int)a4.y }, { (int)a1.x, (int)a1.y }, cv::Scalar{ 0, 0, 255 }, 2.0);
	}
	cv::Mat ScreenToGround::translateMat(const cv::Mat& src, float zoom)
	{
		// 指定した4点のスクリーン座標と現実の座標
		std::vector<cv::Point2f> srcPoint{ p1, p2, p3, p4 };
		std::vector<cv::Point2f> dstPoint{ p1_, p2_, p3_, p4_ };

		// 指定した4点の現実の座標をdstPointに代入し、同時にx軸、y軸の最大値と最小値を取得
		cv::Point2f min(FLT_MAX, FLT_MAX), max(FLT_MIN, FLT_MIN);
		for (auto&& d : dstPoint)
		{
			d = translate(d);
			min.x = (d.x < min.x) ? d.x : min.x;
			min.y = (d.y < min.y) ? d.y : min.y;
			max.x = (d.x > max.x) ? d.x : max.x;
			max.y = (d.y > max.y) ? d.y : max.y;
		}

		// 現実座標の値が画面に丁度収まるように拡大縮小、移動
		cv::Point2f size{ max.x - min.x, max.y - min.y };
		for (auto&& p : dstPoint) { p.x -= min.x; p.y -= min.y; }
		float rate = (size.x / size.y) / ((float)src.cols, (float)src.rows);
		float scale = (rate > 1.0f) ? ((float)src.cols / size.x) : ((float)src.rows / size.y);
		size.x *= scale; size.y *= scale;
		cv::Point2f move = (rate > 1.0f) ? cv::Point2f{ 0.0f, ((float)src.rows - size.y) / 2.0f } : cv::Point2f{ ((float)src.cols - size.x) / 2.0f, 0.0f };
		for (auto&& p : dstPoint) { p.x *= scale; p.y *= scale; p.x += move.x; p.y += move.y; }

		// 拡大縮小
		for (auto&& p : dstPoint)
		{
			p.x -= (float)src.cols / 2.0f; p.y -= (float)src.rows / 2.0f;
			p.x *= zoom; p.y *= zoom;
			p.x += (float)src.cols / 2.0f; p.y += (float)src.rows / 2.0f;
		}

		//変換行列作成
		cv::Mat r_mat = cv::getPerspectiveTransform(srcPoint, dstPoint);

		//図形変換処理
		cv::Mat dst1 = fisheyeToFlat.translateMat(src);
		cv::Mat dst2 = cv::Mat::zeros(src.rows, src.cols, src.type());
		cv::warpPerspective(dst1, dst2, r_mat, dst2.size(), cv::INTER_LINEAR);

		return dst2;
	}
	Vector4 ScreenToGround::onlyFlat(Vector4 p) {
		return fisheyeToFlat.translate(p, cam_w, cam_h);
	}
	cv::Mat ScreenToGround::onlyFlatMat(const cv::Mat& src) {
		return fisheyeToFlat.translateMat(src);
	}
};