#pragma once

#include <iostream>
#include <iomanip>
#include <cmath>

#define M_PI 3.14159265358979

// Vector Tools
struct vt
{
	struct Matrix4
	{
		double
			m00, m01, m02, m03,
			m10, m11, m12, m13,
			m20, m21, m22, m23,
			m30, m31, m32, m33;
		bool isEnable;
		Matrix4(bool isEnable = true) :
			m00(0.0), m01(0.0), m02(0.0), m03(0.0),
			m10(0.0), m11(0.0), m12(0.0), m13(0.0),
			m20(0.0), m21(0.0), m22(0.0), m23(0.0),
			m30(0.0), m31(0.0), m32(0.0), m33(0.0),
			isEnable(isEnable)
		{}
		Matrix4(
			double m00, double m01, double m02, double m03,
			double m10, double m11, double m12, double m13,
			double m20, double m21, double m22, double m23,
			double m30, double m31, double m32, double m33
		) :
			m00(m00), m01(m01), m02(m02), m03(m03),
			m10(m10), m11(m11), m12(m12), m13(m13),
			m20(m20), m21(m21), m22(m22), m23(m23),
			m30(m30), m31(m31), m32(m32), m33(m33),
			isEnable(true)
		{}
		Matrix4(const Matrix4& src) :
			m00(src.m00), m01(src.m01), m02(src.m02), m03(src.m03),
			m10(src.m10), m11(src.m11), m12(src.m12), m13(src.m13),
			m20(src.m20), m21(src.m21), m22(src.m22), m23(src.m23),
			m30(src.m30), m31(src.m31), m32(src.m32), m33(src.m33),
			isEnable(src.isEnable)
		{}
		Matrix4& operator=(const Matrix4& src)
		{
			m00 = src.m00; m01 = src.m01; m02 = src.m02; m03 = src.m03;
			m10 = src.m10; m11 = src.m11; m12 = src.m12; m13 = src.m13;
			m20 = src.m20; m21 = src.m21; m22 = src.m22; m23 = src.m23;
			m30 = src.m30; m31 = src.m31; m32 = src.m32; m33 = src.m33;
			isEnable = src.isEnable;
			return *this;
		}
		Matrix4 getInvert()
		{
			const double m[16] = {
				m00, m10, m20, m30,
				m01, m11, m21, m31,
				m02, m12, m22, m32,
				m03, m13, m23, m33
			};

			double invOut[16];

			double inv[16], det;
			int i;

			inv[0] = m[5] * m[10] * m[15] -
				m[5] * m[11] * m[14] -
				m[9] * m[6] * m[15] +
				m[9] * m[7] * m[14] +
				m[13] * m[6] * m[11] -
				m[13] * m[7] * m[10];

			inv[4] = -m[4] * m[10] * m[15] +
				m[4] * m[11] * m[14] +
				m[8] * m[6] * m[15] -
				m[8] * m[7] * m[14] -
				m[12] * m[6] * m[11] +
				m[12] * m[7] * m[10];

			inv[8] = m[4] * m[9] * m[15] -
				m[4] * m[11] * m[13] -
				m[8] * m[5] * m[15] +
				m[8] * m[7] * m[13] +
				m[12] * m[5] * m[11] -
				m[12] * m[7] * m[9];

			inv[12] = -m[4] * m[9] * m[14] +
				m[4] * m[10] * m[13] +
				m[8] * m[5] * m[14] -
				m[8] * m[6] * m[13] -
				m[12] * m[5] * m[10] +
				m[12] * m[6] * m[9];

			inv[1] = -m[1] * m[10] * m[15] +
				m[1] * m[11] * m[14] +
				m[9] * m[2] * m[15] -
				m[9] * m[3] * m[14] -
				m[13] * m[2] * m[11] +
				m[13] * m[3] * m[10];

			inv[5] = m[0] * m[10] * m[15] -
				m[0] * m[11] * m[14] -
				m[8] * m[2] * m[15] +
				m[8] * m[3] * m[14] +
				m[12] * m[2] * m[11] -
				m[12] * m[3] * m[10];

			inv[9] = -m[0] * m[9] * m[15] +
				m[0] * m[11] * m[13] +
				m[8] * m[1] * m[15] -
				m[8] * m[3] * m[13] -
				m[12] * m[1] * m[11] +
				m[12] * m[3] * m[9];

			inv[13] = m[0] * m[9] * m[14] -
				m[0] * m[10] * m[13] -
				m[8] * m[1] * m[14] +
				m[8] * m[2] * m[13] +
				m[12] * m[1] * m[10] -
				m[12] * m[2] * m[9];

			inv[2] = m[1] * m[6] * m[15] -
				m[1] * m[7] * m[14] -
				m[5] * m[2] * m[15] +
				m[5] * m[3] * m[14] +
				m[13] * m[2] * m[7] -
				m[13] * m[3] * m[6];

			inv[6] = -m[0] * m[6] * m[15] +
				m[0] * m[7] * m[14] +
				m[4] * m[2] * m[15] -
				m[4] * m[3] * m[14] -
				m[12] * m[2] * m[7] +
				m[12] * m[3] * m[6];

			inv[10] = m[0] * m[5] * m[15] -
				m[0] * m[7] * m[13] -
				m[4] * m[1] * m[15] +
				m[4] * m[3] * m[13] +
				m[12] * m[1] * m[7] -
				m[12] * m[3] * m[5];

			inv[14] = -m[0] * m[5] * m[14] +
				m[0] * m[6] * m[13] +
				m[4] * m[1] * m[14] -
				m[4] * m[2] * m[13] -
				m[12] * m[1] * m[6] +
				m[12] * m[2] * m[5];

			inv[3] = -m[1] * m[6] * m[11] +
				m[1] * m[7] * m[10] +
				m[5] * m[2] * m[11] -
				m[5] * m[3] * m[10] -
				m[9] * m[2] * m[7] +
				m[9] * m[3] * m[6];

			inv[7] = m[0] * m[6] * m[11] -
				m[0] * m[7] * m[10] -
				m[4] * m[2] * m[11] +
				m[4] * m[3] * m[10] +
				m[8] * m[2] * m[7] -
				m[8] * m[3] * m[6];

			inv[11] = -m[0] * m[5] * m[11] +
				m[0] * m[7] * m[9] +
				m[4] * m[1] * m[11] -
				m[4] * m[3] * m[9] -
				m[8] * m[1] * m[7] +
				m[8] * m[3] * m[5];

			inv[15] = m[0] * m[5] * m[10] -
				m[0] * m[6] * m[9] -
				m[4] * m[1] * m[10] +
				m[4] * m[2] * m[9] +
				m[8] * m[1] * m[6] -
				m[8] * m[2] * m[5];

			det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

			if (det == 0)
				return Matrix4(false);

			det = 1.0 / det;

			for (i = 0; i < 16; i++)
				invOut[i] = inv[i] * det;

			return Matrix4(
				invOut[0], invOut[4], invOut[8], invOut[12],
				invOut[1], invOut[5], invOut[9], invOut[13],
				invOut[2], invOut[6], invOut[10], invOut[14],
				invOut[3], invOut[7], invOut[11], invOut[15]
			);
		}
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
		void print()
		{
			std::cout << std::fixed << std::setprecision(5) <<
				m00 << " " << m01 << " " << m02 << " " << m03 << "\n" <<
				m10 << " " << m11 << " " << m12 << " " << m13 << "\n" <<
				m20 << " " << m21 << " " << m22 << " " << m23 << "\n" <<
				m30 << " " << m31 << " " << m32 << " " << m33 << "\n" <<
				std::endl;
		}
	};
	struct Vector4
	{
		double x, y, z, w;
		bool isEnable;
		Vector4(bool isEnable = true) :
			x(0.0), y(0.0), z(0.0), w(0.0), isEnable(isEnable) {}
		Vector4(double x, double y, double z = 0.0, double w = 1.0) :
			x(x), y(y), z(z), w(w), isEnable(true) {}
		Vector4(const Vector4& src) :
			x(src.x), y(src.y), z(src.z), w(src.w), isEnable(src.isEnable) {}
		Vector4& operator=(const Vector4& src)
		{
			x = src.x; y = src.y; z = src.z; w = src.w; isEnable = src.isEnable;
			return *this;
		}
		Vector4 operator+(const Vector4& src)
		{
			return Vector4(
				x + src.x,
				y + src.y,
				z + src.z,
				(w + src.w) / 2.0
			);
		}
		Vector4 operator+(const double num)
		{
			return Vector4(
				x + num,
				y + num,
				z + num,
				w
			);
		}
		Vector4 operator-(const Vector4& src)
		{
			return Vector4(
				x - src.x,
				y - src.y,
				z - src.z,
				(w + src.w) / 2.0
			);
		}
		Vector4 operator-(const double num)
		{
			return Vector4(
				x - num,
				y - num,
				z - num,
				w
			);
		}
		Vector4 operator*(const Vector4& src)
		{
			return Vector4(
				x * src.x,
				y * src.y,
				z * src.z,
				(w + src.w) / 2.0
			);
		}
		Vector4 operator*(const double num)
		{
			return Vector4(
				x * num,
				y * num,
				z * num,
				w
			);
		}
		Vector4 operator/(const Vector4& src)
		{
			return Vector4(
				x / src.x,
				y / src.y,
				z / src.z,
				(w + src.w) / 2.0
			);
		}
		Vector4 operator/(const double num)
		{
			return Vector4(
				x / num,
				y / num,
				z / num,
				w
			);
		}
		double length()
		{
			return std::sqrt(std::pow(x, 2.0) + std::pow(y, 2.0) + std::pow(z, 2.0));
		}
		Vector4 normal()
		{
			return (*this) / length();
		}
		Vector4 worldToScreen(Matrix4 projModelView)
		{
			Vector4 v = cross(projModelView, *this);
			return v / v.w;
		}
		Vector4 screenToWorld(Matrix4 proj, double wTmp, double hTmp)
		{
			Vector4 p = Vector4(0, 0, 0);
			p.x = (x * 2.0 / wTmp) - 1.0;
			p.y = (y * 2.0 / hTmp) - 1.0;
			p.z = z;
			p.x = p.x * p.z / proj.m00;
			p.y = p.y * p.z / proj.m11;
			return p;
		}
		static double dot(const Vector4& left, const Vector4& right)
		{
			return left.x * right.x + left.y * right.y + left.z * right.z;
		}
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
		void print()
		{
			std::cout << std::fixed << std::setprecision(5) <<
				x << " " << y << " " << z << " " << w <<
				std::endl;
		}
	};
	static Matrix4 getRotateMatrix(Vector4 axis, double rad) {
		Vector4 naxis = axis.normal();
		double x = naxis.x * std::sin(rad / 2.0);
		double y = naxis.y * std::sin(rad / 2.0);
		double z = naxis.z * std::sin(rad / 2.0);
		double w = std::cos(rad / 2.0);
		return Matrix4(
			1.0 - 2.0 * (y * y + z * z), 2.0 * (x * y + w * z), 2.0 * (x * z - w * y), 0.0,
			2.0 * (x * y - w * z), 1.0 - 2.0 * (x * x + z * z), 2.0 * (y * z + w * x), 0.0,
			2.0 * (x * z + w * y), 2.0 * (y * z - w * x), 1.0 - 2.0 * (x * x + y * y), 0.0,
			0.0, 0.0, 0.0, 1.0
		);
	}
	static double deg2rad(double deg)
	{
		return deg * (2.0 * M_PI) / 360.0;
	}
	static double rad2deg(double rad)
	{
		return rad * 360.0 / (2.0 * M_PI);
	}
	static Vector4 screenToGround(
		double cam_w, double cam_h,  // カメラの解像度
		double cam_h_fov,  // カメラの垂直画角(deg)
		double cam_pos_h,  // カメラの地面からの高さ(m)
		Vector4 p,   // カメラ座標に変換したいスクリーン座標
		Vector4 p1,  // 1点目のスクリーン座標
		Vector4 p2,  // 2点目のスクリーン座標
		Vector4 p3,  // 3点目のスクリーン座標
		Vector4 p4   // 4点目のスクリーン座標
	)
	{
		// カメラ座標の原点から画面の中心までの距離(ピクセル単位)
		const double cam_l = (0.5 * cam_h) / std::tan(0.5 * deg2rad(cam_h_fov));
		// カメラの水平画角
		const double cam_w_fov = rad2deg(2.0 * std::atan((0.5 * cam_w) / cam_l));

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
		Matrix4 r2 = view_matrix.getInvert();

		// マーカー空間でのカメラ座標の計算
		Vector4 m_cam_pos = Vector4::cross(r2, Vector4(0, 0, 0));
		// マーカー空間でのカメラの高さの計算
		double m_cam_h = m_cam_pos.y;
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
};