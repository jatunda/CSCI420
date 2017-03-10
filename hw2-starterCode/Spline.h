#pragma once
#define _USE_MATH_DEFINES
#include <iostream>
#include <iomanip>
#include <math.h>
#include <cmath>

// represents one control point along the spline 
struct Point
{
	double x;
	double y;
	double z;
	Point() : x(0.0), y(0.0), z(0.0) {}
	Point(double x, double y, double z)
		:x(x), y(y), z(z)
	{}
	Point(float x, float y, float z)
		:x(x), y(y), z(z)
	{}

	inline friend Point operator+(Point lhs, const Point& rhs)
	{
		Point p(rhs.x + lhs.x, rhs.y + lhs.y, rhs.z + lhs.z);
		return p;
	}
	inline friend Point operator-(Point lhs, const Point& rhs)
	{
		Point p(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
		return p;
	}
	inline friend Point operator*(Point lhs, const float rhs)
	{
		Point p(lhs.x *rhs, lhs.y *rhs, lhs.z *rhs);
		return p;
	}
	inline friend Point operator/(Point lhs, const float rhs)
	{
		Point p(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs);
		return p;
	}
	Point& operator+=(const Point& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; return *this; }
	Point& operator-=(const Point& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; return *this; }
	Point& operator*=(const float rhs) { x *= rhs; y *= rhs; z *= rhs; return *this; }
	Point& operator/=(const float rhs) { x /= rhs; y /= rhs; z /= rhs; return *this; }
	const double& operator[](const int index) const
	{
		switch (index)
		{
		case 0:
			return this->x;
		case 1:
			return this->y;
		case 2:
			return this->z;
		}
	}
	Point Orthogonalize(const Point& other)
	{
		Point proj = (*this) * static_cast<float>(Point::Dot(other, *this) / static_cast<float>(Point::Dot(*this, *this)));
		return other - proj;
	}
	inline float Mag() const
	{
		return sqrt(static_cast<float>(x * x + y * y + z * z));
	}
	void Normalize()
	{
		double mag = x * x + y * y + z * z;
		mag = sqrt(mag);
		x = x / mag;
		y = y / mag;
		z = z / mag;
	}

	inline static Point Cross( const Point& a, const Point& b)
	{
		Point p(a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[2] * b[0]);
		return p;
	}
	inline static Point Lerp(const Point& p1, const Point& p2, float t)
	{
		Point dif = p2 - p1;
		return p1 + dif * t;
	}
	inline static double Dot(const Point& a, const Point& b)
	{

		double ret = a.x * b.x + a.y*b.y + a.z*b.z;
		return ret;
	}
	inline static float AngleRad(const Point& a, const Point& b)
	{
		float angle = static_cast<float>(acos( Point::Dot(a,b) / (a.Mag() * b.Mag())));
		return angle;
	}
	inline static float AngleDeg(const Point& a, const Point& b)
	{
		float angle = static_cast<float>(acos(Point::Dot(a, b) / (a.Mag() * b.Mag())));
		return angle / static_cast<float>(M_PI * 180);
	}
	static Point Up, Down, Left, Right, Forward, Back, Zero, Identity;
	
};
Point Point::Up = Point(0.0, 1.0, 0.0);
Point Point::Down = Point(0.0, -1.0, 0.0);
Point Point::Right = Point(1.0, 0.0, 0.0);
Point Point::Left = Point(-1.0, 0.0, 0.0);
Point Point::Forward = Point(0.0, 0.0, 1.0);
Point Point::Back = Point(0.0, 0.0, -1.0);
Point Point::Zero = Point(0.0, 0.0, 0.0);
Point Point::Identity = Point(1.0, 1.0, 1.0);

std::ostream& operator<<(std::ostream& os, const Point& p) 
{
	os << '(' << std::setprecision(2) << p.x << ',' << p.y << ',' << p.z << ')'; 
	return os; 
}






// spline struct 
// contains how many control points the spline has, and an array of control points 
struct Spline
{
	int numControlPoints;
	Point * points;
};