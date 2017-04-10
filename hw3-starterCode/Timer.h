//
//  Timer.h
//  password-mac
//
//  Simple wrapper for std::chrono timer
//
//  Created by Sanjay Madhav on 12/26/14.
//  Copyright (c) 2014 Sanjay Madhav. All rights reserved.
//
//
//
//  Edited by Eric Hsieh for split functionality
//

#pragma once

#include <chrono>
#include <vector>

class Timer
{
public:
	// Construct the timer
	Timer();
	
	// Start the timer
	void start();
	
	// Get the amount of elapsed time, in seconds since start was called
	double getElapsed() const;

	// basically the "lap" button on a normal stopwatch
	// also returns the amount of time since the last split
	double split();

	// returns the amount of time since the last split was called
	double getSplit() const;

	// returns a vector with all the splits
	std::vector<std::chrono::high_resolution_clock::time_point> getSplits() const;

private:
	std::chrono::high_resolution_clock::time_point mStart;
	std::vector<std::chrono::high_resolution_clock::time_point> splits;
};

