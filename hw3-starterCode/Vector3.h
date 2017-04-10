#pragma once
#define _USE_MATH_DEFINES
#include <iostream>
#include <iomanip>
#include <math.h>
#include <cmath>

struct Vector3
{
	double x, y, z;

	Vector3()
		:x(0.0), y(0.0), z(0.0)
	{}

	Vector3(double x, double y, double z)
		:x(x), y(y), z(z)
	{}

	Vector3(float x, float y, float z)
		:x(x), y(y), z(z)
	{}

	Vector3(int x, int y, int z)
		:x(x), y(y), z(z)
	{}

	inline friend bool operator==(const Vector3& lhs, const Vector3& rhs)
	{
		return rhs.x == lhs.x && rhs.y == lhs.y && rhs.z == lhs.z;
	}

	inline friend Vector3 operator+(Vector3 lhs, const Vector3& rhs)
	{
		return Vector3(rhs.x + lhs.x, rhs.y + lhs.y, rhs.z + lhs.z);
	}
	inline friend Vector3 operator-(Vector3 lhs, const Vector3& rhs)
	{
		return Vector3(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
	}
	inline friend Vector3 operator*(Vector3 lhs, const float rhs)
	{
		return Vector3(lhs.x *rhs, lhs.y *rhs, lhs.z *rhs);
	}
	inline friend Vector3 operator/(Vector3 lhs, const float rhs)
	{
		return Vector3(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs);
	}


	inline friend Vector3 operator*(Vector3 lhs, const double rhs)
	{
		return Vector3(lhs.x *rhs, lhs.y *rhs, lhs.z *rhs);
	}
	inline friend Vector3 operator/(Vector3 lhs, const double rhs)
	{
		return Vector3(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs);
	}

	inline friend Vector3 operator*(const float lhs, Vector3 rhs) { return rhs*lhs; }
	inline friend Vector3 operator*(const double lhs, Vector3 rhs) { return rhs*lhs; }


	Vector3& operator+=(const Vector3& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
	Vector3& operator-=(const Vector3& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
	Vector3& operator*=(const float rhs) { x *= rhs; y *= rhs; z *= rhs; return *this; }
	Vector3& operator/=(const float rhs) { x /= rhs; y /= rhs; z /= rhs; return *this; }
	Vector3& operator*=(const double rhs) { x *= rhs; y *= rhs; z *= rhs; return *this; }
	Vector3& operator/=(const double rhs) { x /= rhs; y /= rhs; z /= rhs; return *this; }
	double& operator[](const int index) const
	{
		switch (index)
		{
		case 0:
			return const_cast<Vector3*>(this)->x;
		case 1:
			return const_cast<Vector3*>(this)->y;
		case 2:
			return const_cast<Vector3*>(this)->z;
		}
	}
	Vector3 Orthogonalize(const Vector3& other)
	{
		return other - ( (*this) * (Vector3::Dot(other, *this) / (Vector3::Dot(*this, *this))));
	}
	inline double Mag() const
	{
		return sqrt(MagSquared());
	}
	inline double MagSquared() const
	{
		return (x * x + y * y + z * z);
	}
	void Normalize()
	{
		double mag = Mag();
		x /= mag;
		y /= mag;
		z /= mag;
	}

	inline static Vector3 Cross(const Vector3& a, const Vector3& b)
	{
		return Vector3(a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]);
	}
	inline static Vector3 Lerp(const Vector3& p1, const Vector3& p2, float t)
	{
		return p1 + (p2-p1) * t;
	}
	inline static Vector3 Lerp(const Vector3& p1, const Vector3& p2, double t)
	{
		return p1 + (p2-p1) * t;
	}
	inline static double Dot(const Vector3& a, const Vector3& b)
	{
		return a.x*b.x + a.y*b.y + a.z*b.z;
	}
	inline static double AngleRad(const Vector3& a, const Vector3& b)
	{
		return acos(Vector3::Dot(a, b) / sqrt(a.MagSquared() * b.MagSquared()));
	}
	inline static double AngleDeg(const Vector3& a, const Vector3& b)
	{
		return Vector3::AngleRad(a, b) * 180 / M_PI;
	}
	inline static Vector3 Normalize(const Vector3&& v)
	{
		double mag = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
		return (Vector3(v.x / mag, v.y/mag, v.z/mag));
	}
	inline static Vector3 Normalize(const Vector3& v)
	{
		double mag = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
		return (Vector3(v.x / mag, v.y / mag, v.z / mag));
	}

	static const Vector3 Up, Down, Left, Right, Forward, Back, Zero, Identity;
};
const Vector3 Vector3::Up = Vector3(0.0, 1.0, 0.0);
const Vector3 Vector3::Down = Vector3(0.0, -1.0, 0.0);
const Vector3 Vector3::Right = Vector3(1.0, 0.0, 0.0);
const Vector3 Vector3::Left = Vector3(-1.0, 0.0, 0.0);
const Vector3 Vector3::Forward = Vector3(0.0, 0.0, 1.0);
const Vector3 Vector3::Back = Vector3(0.0, 0.0, -1.0);
const Vector3 Vector3::Zero = Vector3(0.0, 0.0, 0.0);
const Vector3 Vector3::Identity = Vector3(1.0, 1.0, 1.0);

inline std::ostream& operator<<(std::ostream& os, const Vector3& p)
{
	os << '(' << std::setprecision(2) << p.x << ',' << p.y << ',' << p.z << ')';
	return os;
}