#pragma once

#include <Utils/Vector.h>

namespace vt
{
	Matrix4::Matrix4(bool isEnable) :
		m00(0.0), m01(0.0), m02(0.0), m03(0.0),
		m10(0.0), m11(0.0), m12(0.0), m13(0.0),
		m20(0.0), m21(0.0), m22(0.0), m23(0.0),
		m30(0.0), m31(0.0), m32(0.0), m33(0.0),
		isEnable(isEnable)
	{}
	Matrix4::Matrix4(
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
	Matrix4::Matrix4(const Matrix4& src) :
		m00(src.m00), m01(src.m01), m02(src.m02), m03(src.m03),
		m10(src.m10), m11(src.m11), m12(src.m12), m13(src.m13),
		m20(src.m20), m21(src.m21), m22(src.m22), m23(src.m23),
		m30(src.m30), m31(src.m31), m32(src.m32), m33(src.m33),
		isEnable(src.isEnable)
	{}
	Matrix4& Matrix4::operator=(const Matrix4& src)
	{
		m00 = src.m00; m01 = src.m01; m02 = src.m02; m03 = src.m03;
		m10 = src.m10; m11 = src.m11; m12 = src.m12; m13 = src.m13;
		m20 = src.m20; m21 = src.m21; m22 = src.m22; m23 = src.m23;
		m30 = src.m30; m31 = src.m31; m32 = src.m32; m33 = src.m33;
		isEnable = src.isEnable;
		return *this;
	}
	Matrix4 Matrix4::getInvert()
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
	void Matrix4::print()
	{
		std::cout << std::fixed << std::setprecision(5) <<
			m00 << " " << m01 << " " << m02 << " " << m03 << "\n" <<
			m10 << " " << m11 << " " << m12 << " " << m13 << "\n" <<
			m20 << " " << m21 << " " << m22 << " " << m23 << "\n" <<
			m30 << " " << m31 << " " << m32 << " " << m33 << "\n" <<
			std::endl;
	}

	Vector4::Vector4(bool isEnable) :
		x(0.0), y(0.0), z(0.0), w(0.0), isEnable(isEnable) {}
	Vector4::Vector4(double x, double y, double z, double w) :
		x(x), y(y), z(z), w(w), isEnable(true) {}
	Vector4::Vector4(const Vector4& src) :
		x(src.x), y(src.y), z(src.z), w(src.w), isEnable(src.isEnable) {}
	Vector4::Vector4(const cv::Point& src) :
		x((double)src.x), y((double)src.y), z(0.0), w(0.0), isEnable(true) {}
	Vector4::Vector4(const cv::Point2f& src) :
		x((double)src.x), y((double)src.y), z(0.0), w(0.0), isEnable(true) {}
	Vector4::Vector4(const cv::Point2d& src) :
		x(src.x), y(src.y), z(0.0), w(0.0), isEnable(true) {}
	Vector4& Vector4::operator=(const Vector4& src)
	{
		x = src.x; y = src.y; z = src.z; w = src.w; isEnable = src.isEnable;
		return *this;
	}
	bool Vector4::operator==(const Vector4& src) const
	{
		return ((x == src.x) && (y == src.y) && (z == src.z) && (w == src.w) && (isEnable == src.isEnable));
	}
	bool Vector4::operator!=(const Vector4& src) const
	{
		return !((*this) == src);
	}
	Vector4 Vector4::operator+(const Vector4& src) const
	{
		return Vector4(
			x + src.x,
			y + src.y,
			z + src.z,
			(w + src.w) / 2.0
		);
	}
	Vector4 Vector4::operator+(const double num) const
	{
		return Vector4(
			x + num,
			y + num,
			z + num,
			w
		);
	}
	Vector4 Vector4::operator-(const Vector4& src) const
	{
		return Vector4(
			x - src.x,
			y - src.y,
			z - src.z,
			(w + src.w) / 2.0
		);
	}
	Vector4 Vector4::operator-(const double num) const
	{
		return Vector4(
			x - num,
			y - num,
			z - num,
			w
		);
	}
	Vector4 Vector4::operator*(const Vector4& src) const
	{
		return Vector4(
			x * src.x,
			y * src.y,
			z * src.z,
			(w + src.w) / 2.0
		);
	}
	Vector4 Vector4::operator*(const double num) const
	{
		return Vector4(
			x * num,
			y * num,
			z * num,
			w
		);
	}
	Vector4 Vector4::operator/(const Vector4& src) const
	{
		return Vector4(
			x / src.x,
			y / src.y,
			z / src.z,
			(w + src.w) / 2.0
		);
	}
	Vector4 Vector4::operator/(const double num) const
	{
		return Vector4(
			x / num,
			y / num,
			z / num,
			w
		);
	}
	Vector4::operator cv::Point() const
	{
		return cv::Point{ (int)x, (int)y };
	}
	Vector4::operator cv::Point2f() const
	{
		return cv::Point2f{ (float)x, (float)y };
	}
	Vector4::operator cv::Point2d() const
	{
		return cv::Point2d{ x, y };
	}
	double Vector4::length() const
	{
		return std::sqrt(std::pow(x, 2.0) + std::pow(y, 2.0) + std::pow(z, 2.0));
	}
	Vector4 Vector4::normal() const
	{
		return (*this) / length();
	}
	Vector4 Vector4::worldToScreen(const Matrix4 projModelView) const
	{
		Vector4 v = cross(projModelView, *this);
		return v / v.w;
	}
	Vector4 Vector4::screenToWorld(const Matrix4 proj, const double wTmp, const double hTmp) const
	{
		Vector4 p = Vector4(0, 0, 0);
		p.x = (x * 2.0 / wTmp) - 1.0;
		p.y = (y * 2.0 / hTmp) - 1.0;
		p.z = z;
		p.x = p.x * p.z / proj.m00;
		p.y = p.y * p.z / proj.m11;
		return p;
	}
	double Vector4::dot(const Vector4& left, const Vector4& right) const
	{
		return left.x * right.x + left.y * right.y + left.z * right.z;
	}
	void Vector4::print()
	{
		std::cout << std::fixed << std::setprecision(5) <<
			x << " " << y << " " << z << " " << w <<
			std::endl;
	}

	Matrix4 getRotateMatrix(Vector4 axis, double rad) {
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
	double deg2rad(double deg)
	{
		return deg * (2.0 * M_PI) / 360.0;
	}
	double rad2deg(double rad)
	{
		return rad * 360.0 / (2.0 * M_PI);
	}
};