#include <MinOpenPose.h>
#include <regex>
#include <Database.h>

int createTable(Database& db)
{
	db->exec("DROP TABLE IF EXISTS people");
	std::string row_title = "frame INTEGER, people INTEGER";
	for (int i = 0; i < 25; i++)
	{
		row_title += ", joint" + std::to_string(i) + "x REAL";
		row_title += ", joint" + std::to_string(i) + "y REAL";
		row_title += ", joint" + std::to_string(i) + "confidence REAL";
	}
	try { db->exec("CREATE TABLE people (" + row_title + ")"); }
	catch (const std::exception & e) {
		std::cout << "error : " << __FILE__ << " : L" << __LINE__ << "\n" << e.what() << std::endl;
		return 1;
	}
	return 0;
}

int addRow(Database& db, std::shared_ptr<std::vector<std::shared_ptr<op::Datum>>>& results)
{
	// SQL文の生成
	std::string row = "?";
	for (int colIndex = 0; colIndex < 76; colIndex++) row += ", ?";
	row = "INSERT INTO people VALUES (" + row + ")";
	// SQL文の値を確定する
	try
	{
		SQLite::Statement query(*db, row);
		for (auto result : *results)
		{

			// 画面内に映っている人数分ループする
			for (int peopleIndex = 0; peopleIndex < result->poseKeypoints.getSize(0); peopleIndex++)
			{
				query.reset();
				query.bind(1, (long long)result->frameNumber);
				query.bind(2, peopleIndex);
				for (int nodeIndex = 0; nodeIndex < result->poseKeypoints.getSize(1); nodeIndex++)
				{
					query.bind(3 + nodeIndex * 3 + 0, result->poseKeypoints[{peopleIndex, nodeIndex, 0}]);
					query.bind(3 + nodeIndex * 3 + 1, result->poseKeypoints[{peopleIndex, nodeIndex, 1}]);
					query.bind(3 + nodeIndex * 3 + 2, result->poseKeypoints[{peopleIndex, nodeIndex, 2}]);
				}
				// SQL文を実行する
				(void)query.exec();

			}
		}
	}
	catch (const std::exception & e) {
		std::cout << "error : " << __FILE__ << " : L" << __LINE__ << "\n" << e.what() << std::endl;
		return 1;
	}
	return 0;
}

struct Node
{
	float x, y, confidence;
};

std::unique_ptr<std::vector<std::vector<Node>>> getPoseKeypoints(Database& db, size_t frameNumber)
{
	auto result = std::make_unique<std::vector<std::vector<Node>>>();
	SQLite::Statement query(*db, "SELECT * FROM people WHERE frame=?");
	query.bind(1, (long long)frameNumber);
	while (query.executeStep())
	{
		std::vector<Node> nodes;
		for (int nodeIndex = 0; nodeIndex < 25; nodeIndex++)
		{
			nodes.push_back(Node{
				(float)query.getColumn(2 + nodeIndex * 3 + 0).getDouble(),
				(float)query.getColumn(2 + nodeIndex * 3 + 1).getDouble(),
				(float)query.getColumn(2 + nodeIndex * 3 + 2).getDouble()
			});
		}
		result->push_back(nodes);
	}
	return result;
}

class CustomOpenPoseEvent : public OpenPoseEvent
{
private:
	std::string path;
	cv::VideoCapture cap;
public:
	CustomOpenPoseEvent(const std::string& path) :
		path(path)
	{

	}
	virtual ~CustomOpenPoseEvent() {}
	int init() override
	{
		cap.open(path);
		if (!cap.isOpened()) return 1;
		return 0;
	}
	void exit() override
	{
		cap.release();
	}
	int sendImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override
	{
		if (!cap.isOpened()) return 1;
		imageInfo.needOpenposeProcess = true;
		imageInfo.frameNumber = (size_t)cap.get(CV_CAP_PROP_POS_FRAMES);
		cap.read(imageInfo.inputImage);
		return 0;
	}
	int recieveImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override
	{
		cv::imshow("result", imageInfo.outputImage);
		if (cv::waitKey(1) == 0x1b) exit();
		return 0;
	}
	void recieveErrors(const std::vector<std::string>& errors) override
	{
		for (auto error : errors)
			std::cout << error << std::endl;
	}
	std::pair<op::PoseModel, op::Point<int>> selectOpenposeMode() override
	{
		return std::pair<op::PoseModel, op::Point<int>>(
			op::PoseModel::BODY_25, op::Point<int>(-1, 368)
		);
	}
};

int main(int argc, char* argv[])
{
	MinimumOpenPose mop;

	CustomOpenPoseEvent cope {R"(G:\思い出\Dropbox\Dropbox\SDK\openpose\研究室から貰ったデータ\openpose\video\58°.mp4)"};
	int ret = mop.startup(cope);

	return ret;
}
