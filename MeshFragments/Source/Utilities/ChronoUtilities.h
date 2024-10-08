#pragma once

#include <chrono>
#include <iostream>

/**
*	@file ChronoUtilities.h
*	@authors Alfonso L�pez Ruiz (alr00048@red.ujaen.es)
*	@date 09/05/2019
*/

/**
*	@brief Utilities which help us to measure execution time.
*	@author Alfonso L�pez Ruiz.
*/
namespace ChronoUtilities
{
	//!< Units we can use to return the measured time
	enum TimeUnit: int
	{
		SECONDS = 1000000000, MILLISECONDS = 1000000, MICROSECONDS = 1000, NANOSECONDS = 1
	};

	//!< Private members
	namespace
	{
		std::chrono::high_resolution_clock::time_point _initTime;
	}
	
	/**
	*	@return The current date and time in the format "YYYY-MM-DD HH:MM:SS".
	*/
	std::string getCurrentDateTime();

	/**
	*	@return Measured time in the selected time unit since the clock was started. By default the time unit is milliseconds.
	*/
	long long getDuration(const TimeUnit timeUnit = ChronoUtilities::MILLISECONDS);

	/**
	*	@brief Starts the clock so we can execute whatever we want and measure the used time.
	*/
	void initChrono();
}

inline std::string ChronoUtilities::getCurrentDateTime()
{
	time_t now = time(0);
	tm* ltm = localtime(&now);
	std::string dateTime = std::to_string(1900 + ltm->tm_year) + "_" + std::to_string(1 + ltm->tm_mon) + "_" + std::to_string(ltm->tm_mday) + "__" + std::to_string(ltm->tm_hour) + "_" + std::to_string(ltm->tm_min) + "_" + std::to_string(ltm->tm_sec);
	return dateTime;
}

inline long long ChronoUtilities::getDuration(const TimeUnit timeUnit)
{
	std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
	long long measuredTime = std::chrono::duration_cast<std::chrono::nanoseconds>(currentTime - ChronoUtilities::_initTime).count();
	
	return measuredTime / timeUnit;
}

inline void ChronoUtilities::initChrono()
{
	ChronoUtilities::_initTime = std::chrono::high_resolution_clock::now();
}
