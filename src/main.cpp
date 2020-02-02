#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <OpenPoseWrapper/Examples/SqlOpenPoseEvent.h>
#include <OpenPoseWrapper/Examples/VideoOpenPoseEvent.h>

int main(int argc, char* argv[])
{
	MinimumOpenPose mop;

	SqlOpenPoseEvent cope {R"(G:\思い出\Dropbox\Dropbox\SDK\openpose\研究室から貰ったデータ\openpose\video\58°.mp4)", false};
	int ret = mop.startup(cope);

	return ret;
}
