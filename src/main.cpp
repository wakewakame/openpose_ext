#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <OpenPoseWrapper/Examples/SqlOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/VideoOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/PreviewOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/TrackingOpenPoseEvent.h>
#include <regex>
#include <chrono>

class PlotInfoOpenPoseEvent : public OpenPoseEvent
{
private:
	using clock = std::chrono::high_resolution_clock;
	clock::time_point start, end;
public:
	PlotInfoOpenPoseEvent() {}
	virtual ~PlotInfoOpenPoseEvent() {};
	int init() override final {
		start = clock::now();
		return 0;
	}
	void exit() override final {}
	int sendImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final { return 0; }
	int recieveImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		for (auto person = imageInfo.people.begin(); person != imageInfo.people.end(); person++)
		{
			op::Point<int> p; size_t enableNodeSum = 0;
			for (auto node : person->second)
			{
				if (node.confidence != 0.0f)
				{
					cv::circle(
						imageInfo.outputImage,
						cv::Point{ (int)node.x, (int)node.y },
						3, cv::Scalar{ 255, 0, 0 }, -1
					);
					p.x += (int)node.x; p.y += (int)node.y; enableNodeSum++;
				}
			}
			p.x /= enableNodeSum; p.y /= enableNodeSum;
			op::putTextOnCvMat(
				imageInfo.outputImage,
				std::to_string(person->first), p, { 255, 255, 255 }, true, 800
			);
		}

		end = clock::now();
		float fps = 1000.0f / (float)std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
		start = end;

		return 0;
	}
	void recieveErrors(const std::vector<std::string>& errors) override final
	{
		for (auto error : errors)
			std::cout << error << std::endl;
	}
};

int main(int argc, char* argv[])
{
	MinimumOpenPose mop;
	std::string videoPath =
		R"(G:\思い出\Dropbox\Dropbox\SDK\openpose\研究室から貰ったデータ\openpose\video\58°.mp4)";
	std::string sqlPath = std::regex_replace(videoPath, std::regex(R"(\.[^.]*$)"), "") + ".sqlite3";

	mop.on<TrackingOpenPoseEvent>();
	mop.on<SqlOpenPoseEvent>(sqlPath, false, 60);
	mop.on<VideoOpenPoseEvent>(videoPath);
	mop.on<PlotInfoOpenPoseEvent>();
	mop.on<PreviewOpenPoseEvent>();

	int ret = mop.startup();

	return ret;
}
