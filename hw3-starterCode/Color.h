#pragma once

struct Color
{
	double r, g, b;

	Color()
		: r(1.0)
		, g(1.0)
		, b(1.0)
	{}

	Color(double r, double g, double b)
		: r((r)), g((g)), b((b))
	{}

	Color(double x)
		: r(x), g(x), b(x)
	{}

	inline friend Color operator+ (Color lhs, const Color& rhs)
	{
		return Color(rhs.r + lhs.r, rhs.g + lhs.g, rhs.b + lhs.b);
	}
	//inline friend Color operator+(Color lhs, const Color& rhs)
	//{
	//	unsigned short r = CharClamp(rhs.r + lhs.r);
	//	unsigned short g = CharClamp(rhs.g + lhs.g);
	//	unsigned short b = CharClamp(rhs.b + lhs.b);

	//	Color p(r, g, b);
	//	return p;
	//}

	inline friend Color operator*(Color lhs, const double rhs)
	{
		return Color(lhs.r * rhs, lhs.g * rhs, lhs.b * rhs);
	}

	inline friend Color operator*(const double lhs, Color rhs) { return rhs*lhs; }
	inline friend Color operator/(Color lhs, const double rhs)
	{
		return Color(lhs.r / rhs, lhs.g / rhs, lhs.b / rhs);
	}
	inline friend Color operator/(const double lhs, Color rhs) { return rhs*lhs; }
	Color& operator*=(const double rhs) { r *= rhs; g *= rhs; b *= rhs; return *this; }
	Color& operator/=(const double rhs) { r /= rhs; g /= rhs; b /= rhs; return *this; }
	Color& operator+=(const Color& rhs) { r += rhs.r; g += rhs.g; b += rhs.b; return *this; }

	double Intensity() { return (r + g + b) / 3.0; }


private:
	/*static unsigned char CharClamp(unsigned short x) {
		if (x > 255) {
			return 255;
		}
		else
		{
			return static_cast<char>(x);
		}
	}
	static unsigned char CharClamp(int x) {
		if (x > 255) {
			return 255;
		}
		else
		{
			return static_cast<char>(x);
		}
	}
	static unsigned char CharClamp(double x) {
		if (x > 255) {
			return 255;
		}
		else
		{
			return static_cast<char>(x);
		}
	}*/

public:

	const static Color White, Grey, Gray, Black, LightGrey, LightGray, DarkGrey, DarkGray,
		Red, Green, Blue;
};
const Color Color::White = Color(1.0);
const Color Color::Black = Color(0.0);
const Color Color::Grey = Color(0.5);
const Color Color::Gray = Color::Grey;
const Color Color::DarkGrey = Color(0.25);
const Color Color::DarkGray = Color::DarkGrey;
const Color Color::LightGrey = Color(0.75);
const Color Color::LightGray = Color::LightGrey;
const Color Color::Red = Color(1.0, 0.0, 0.0);
const Color Color::Green = Color(0.0, 1.0, 0.0);
const Color Color::Blue = Color(0.0, 0.0, 1.0);

inline std::ostream& operator<<(std::ostream& os, const Color& c)
{
	os << '(' << c.r << ',' << c.g << ',' << c.b << ')';
	return os;
}