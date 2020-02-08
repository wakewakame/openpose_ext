#include <Utils/Gui.h>

namespace gui
{
	cv::Size text(
		cv::Mat& image,
		const std::string& text,
		cv::Point position,
		TextMode textMode,
		double fontScale,
		cv::Scalar color
	)
	{
		const cv::HersheyFonts fontFace = cv::FONT_HERSHEY_SIMPLEX;
		const int thickness = (int)(3.0 * fontScale);
		cv::Point textPosition{ position.x, position.y };

		int baseline = 0;
		auto size = cv::getTextSize(text, fontFace, fontScale, thickness, &baseline);
		if (((int)textMode % 3) == 1) textPosition.x -= size.width / 2;
		if (((int)textMode % 3) == 2) textPosition.x -= size.width;
		if (((int)textMode / 3) == 0) textPosition.y += (size.height / 2) + baseline;
		if (((int)textMode / 3) == 1) textPosition.y += (size.height) - baseline;
		if (((int)textMode / 3) == 2) textPosition.y -= baseline;

		cv::Point shadowPosition{ textPosition.x + thickness, textPosition.y + thickness };

		cv::putText(image, text, shadowPosition, fontFace, fontScale, cv::Scalar{ 0, 0, 0, 255 }, thickness, 8);
		cv::putText(image, text, textPosition, fontFace, fontScale, color, thickness, 8);

		return size;
	}
}