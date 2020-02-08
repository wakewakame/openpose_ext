#pragma once

#include <openpose/headers.hpp>

namespace gui
{
	enum TextMode : uint8_t
	{
		LEFT_TOP = 0U, CENTER_TOP = 1U, RIGHT_TOP = 2U,
		LEFT_CENTER = 3U, CENTER_CENTER = 4U, RIGHT_CENTER = 5U,
		LEFT_BOTTOM = 6U, CENTER_BOTTOM = 7U, RIGHT_BOTTOM = 8U
	};
	cv::Size text(
		cv::Mat& image,
		const std::string& text,
		cv::Point position,
		TextMode textMode = LEFT_TOP,
		double fontScale = 0.7f,
		cv::Scalar color = cv::Scalar{ 255, 255, 255, 255 }
	);
}