#pragma once

// represents one control point along the spline 
struct Point
{
	double x;
	double y;
	double z;
	Point() : x(0.0), y(0.0), z(0.0) {}
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
	Point& Orthogonalize(const Point& other)
	{
		Point proj = (*this) * (Point::Dot(other, *this) / Point::Dot(*this, *this));
		return other - proj;
	}
	void Normalize()
	{
		double mag = x * x + y * y + z * z;
		mag = sqrt(mag);
		x = x / mag;
		y = y / mag;
		z = z / mag;
	}

	static Point& Cross( const Point& a, const Point& b)
	{
		Point p(a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[2] * b[0]);
		return p;
	}
	static Point& Lerp(const Point& p1, const Point& p2, float t)
	{
		Point dif = p2 - p1;
		return p1 + dif * t;
	}
	static double& Dot(const Point& a, const Point& b)
	{

		double ret = a.x * b.x + a.y*b.y + a.z*b.z;
		return ret;
	}
};





// spline struct 
// contains how many control points the spline has, and an array of control points 
struct Spline
{
	int numControlPoints;
	Point * points;
};