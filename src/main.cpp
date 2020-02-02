#include <OpenPoseWrapper/MinimumOpenPose.h>

int main(int argc, char* argv[])
{
	MinimumOpenPose mop;

	ExampleOpenPoseEvent cope {R"(G:\思い出\Dropbox\Dropbox\SDK\openpose\研究室から貰ったデータ\openpose\video\58°.mp4)"};
	int ret = mop.startup(cope);

	return ret;
}
