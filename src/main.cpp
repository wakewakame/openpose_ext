#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <OpenPoseWrapper/Examples/SqlOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/VideoOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/TrackingOpenPoseEvent.h>

int main(int argc, char* argv[])
{
	MinimumOpenPose mop;

	mop.addEventListener(std::make_unique<TrackingOpenPoseEvent>());
	mop.addEventListener(std::make_unique<SqlOpenPoseEvent>(R"(C:\Users\ŽÄ“cŒ¤\Documents\VirtualUsers\17ad105\Videos\58.mp4)", false, 60));
	//mop.addEventListener(std::make_unique<VideoOpenPoseEvent>(R"(C:\Users\ŽÄ“cŒ¤\Documents\VirtualUsers\17ad105\Videos\58.mp4)"));

	int ret = mop.startup();

	return ret;
}
