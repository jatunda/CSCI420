//
//  Timer.cpp
//  password-mac
//
//  Created by Sanjay Madhav on 12/26/14.
//  Copyright (c) 2014 Sanjay Madhav. All rights reserved.
//

#include "Timer.h"

Timer::Timer()
{
}

void Timer::start()
{
	
	splits.clear();
	splits.push_back(mStart = std::chrono::high_resolution_clock::now());
}

double Timer::getElapsed() const
{
	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - mStart).count();
	return duration / 1000000000.0;
}

double Timer::split() 
{
	splits.push_back(std::chrono::high_resolution_clock::now());
	return getSplit();
}

double Timer::getSplit() const 
{
	auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(splits.back() - splits[splits.size() - 2]).count();
	return duration / 1000000000.0;
}

std::vector<std::chrono::high_resolution_clock::time_point> Timer::getSplits() const
{
	return splits;
}
