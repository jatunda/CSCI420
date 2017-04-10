#pragma once
#include "Vector3.h"

struct Ray
{
	Vector3 position;
	Vector3 direction;

	Ray()
		: position(Vector3::Zero)
		, direction(Vector3::Zero)
	{}

	Ray(Vector3 pos, Vector3 dir)
		:position(pos), direction(dir)
	{}

	inline friend bool operator==(const Ray& lhs, const Ray& rhs)
	{
		return lhs.position == rhs.position && lhs.direction == rhs.direction;
	}

	static Ray Nothing;
};

Ray Ray::Nothing = Ray(Vector3::Zero, Vector3::Zero);