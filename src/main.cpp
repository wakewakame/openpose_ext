#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <OpenPoseWrapper/Examples/SqlOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/VideoOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/PreviewOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/TrackingOpenPoseEvent.h>
#include <regex>

int main(int argc, char* argv[])
{
	MinimumOpenPose mop;
	std::string videoPath =
		R"(G:\思い出\Dropbox\Dropbox\SDK\openpose\研究室から貰ったデータ\openpose\video\58°.mp4)";
	std::string sqlPath = std::regex_replace(videoPath, std::regex(R"(\.[^.]*$)"), "") + ".sqlite3";

	mop.on<TrackingOpenPoseEvent>();
	mop.on<SqlOpenPoseEvent>(sqlPath, false, 60);
	mop.on<VideoOpenPoseEvent>(videoPath);
	mop.on<PreviewOpenPoseEvent>();

	int ret = mop.startup();

	return ret;
}
