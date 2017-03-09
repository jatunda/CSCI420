#pragma once

struct HeightmapPoint
{
	HeightmapPoint()
		:row(-1), col(-1), height(0.0f)
	{

	}

	HeightmapPoint(int row, int col, float height)
		:row(row), col(col), height(height)
	{

	}

	int row, col;
	float height;

};