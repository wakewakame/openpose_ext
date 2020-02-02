#include <OpenPoseWrapper/OpenPoseEvent.h>

ExampleOpenPoseEvent::ExampleOpenPoseEvent(const std::string& videoPath) : videoPath(videoPath)
{

}

ExampleOpenPoseEvent::~ExampleOpenPoseEvent() {}

int ExampleOpenPoseEvent::init()
{
	cap.open(videoPath);
	if (!cap.isOpened())
	{
		std::cout << "can not open \"" << videoPath << "\"" << std::endl;
		return 1;
	}
	return 0;
}

void ExampleOpenPoseEvent::exit()
{
	cap.release();
}

int ExampleOpenPoseEvent::sendImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit)
{
	if (!cap.isOpened())
	{
		std::cout << "can not open \"" << videoPath << "\"" << std::endl;
		return 1;
	}
	imageInfo.needOpenposeProcess = true;
	imageInfo.frameNumber = (size_t)cap.get(CV_CAP_PROP_POS_FRAMES);
	cap.read(imageInfo.inputImage);
	return 0;
}

int ExampleOpenPoseEvent::recieveImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit)
{
	cv::imshow("result", imageInfo.outputImage);
	if (cv::waitKey(1) == 0x1b) exit();
	return 0;
}

void ExampleOpenPoseEvent::recieveErrors(const std::vector<std::string>& errors)
{
	for (auto error : errors)
		std::cout << error << std::endl;
}

std::pair<op::PoseModel, op::Point<int>> ExampleOpenPoseEvent::selectOpenposeMode()
{
	return std::pair<op::PoseModel, op::Point<int>>(
		op::PoseModel::BODY_25, op::Point<int>(-1, 368)
		);
}