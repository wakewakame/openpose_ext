#pragma once

#include <OpenPoseWrapper/OpenPoseEvent.h>
#include <Utils/Database.h>
#include <regex>

class SqlOpenPoseEvent : public OpenPoseEvent
{
private:
	bool writeMode;
	std::string videoPath;
	std::string sqlPath;
	cv::VideoCapture cap;
	Database database;
	std::unique_ptr<SQLite::Transaction> upTransaction;
	long long saveFreq = 0;
	size_t saveCountDown = 1;
public:
	SqlOpenPoseEvent(const std::string& videoPath, bool writeMode, long long saveFreq = 0) :
		videoPath(videoPath), writeMode(writeMode), saveFreq(saveFreq)
	{
		sqlPath = std::regex_replace(this->videoPath, std::regex(R"(\.[^.]*$)"), "") + ".sqlite3";
	}
	virtual ~SqlOpenPoseEvent() {};
	int init() override final
	{
		// 動画ファイルのオープン
		cap.open(videoPath);
		if (!cap.isOpened())
		{
			std::cout << "can not open \"" << videoPath << "\"" << std::endl;
			return 1;
		}

		// sqlの生成
		try
		{
			database = createDatabase(sqlPath);
			upTransaction = std::make_unique<SQLite::Transaction>(*database);
			if (writeMode)
			{
				database->exec("DROP TABLE IF EXISTS people");
				std::string row_title = "frame INTEGER, people INTEGER";
				for (int i = 0; i < 25; i++)
				{
					row_title += ", joint" + std::to_string(i) + "x REAL";
					row_title += ", joint" + std::to_string(i) + "y REAL";
					row_title += ", joint" + std::to_string(i) + "confidence REAL";
				}
				database->exec("CREATE TABLE people (" + row_title + ")");
			}
		}
		catch (const std::exception& e)
		{
			std::cout << "error : " << __FILE__ << " : L" << __LINE__ << "\n" << e.what() << std::endl;
			return 1;
		}

		return 0;
	}
	void exit() override final
	{
		cap.release();
		if (upTransaction && writeMode) upTransaction->commit();
	}
	int sendImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		if (!cap.isOpened())
		{
			std::cout << "can not open \"" << videoPath << "\"" << std::endl;
			return 1;
		}
		imageInfo.needOpenposeProcess = true;
		imageInfo.frameNumber = (size_t)cap.get(CV_CAP_PROP_POS_FRAMES);
		cap.read(imageInfo.inputImage);
		if (imageInfo.inputImage.empty())
		{
			exit();
			return 0;
		}

		if (!writeMode)
		{
			try
			{
				imageInfo.needOpenposeProcess = false;
				SQLite::Statement query(*database, "SELECT * FROM people WHERE frame=?");
				query.bind(1, (long long)imageInfo.frameNumber);
				while (query.executeStep())
				{
					size_t index = (size_t)query.getColumn(1).getInt64();
					std::vector<ImageInfo::Node> nodes;
					for (int nodeIndex = 0; nodeIndex < 25; nodeIndex++)
					{
						nodes.push_back(ImageInfo::Node{
							(float)query.getColumn(2 + nodeIndex * 3 + 0).getDouble(),
							(float)query.getColumn(2 + nodeIndex * 3 + 1).getDouble(),
							(float)query.getColumn(2 + nodeIndex * 3 + 2).getDouble()
							});
					}
					imageInfo.people[index] = std::move(nodes);
				}
			}
			catch (const std::exception& e)
			{
				std::cout << "error : " << __FILE__ << " : L" << __LINE__ << "\n" << e.what() << std::endl;
				return 1;
			}
		}

		return 0;
	}
	int recieveImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		// プレビューの表示
		if (!writeMode)
		{
			for (auto person = imageInfo.people.begin(); person != imageInfo.people.end(); person++)
			{
				for (auto node : person->second)
				{
					if (node.confidence != 0.0f)
					{
						cv::circle(
							imageInfo.outputImage,
							cv::Point{ (int)node.x, (int)node.y },
							3, cv::Scalar{ 255, 0, 0 }, -1
						);
					}
				}
			}
		}
		cv::imshow("result", imageInfo.outputImage);
		if (cv::waitKey(1) == 0x1b) exit();

		if (writeMode)
		{
			// sql文の生成
			std::string row = "?";
			for (int colIndex = 0; colIndex < 76; colIndex++) row += ", ?";
			row = "INSERT INTO people VALUES (" + row + ")";
			// sql文の値を確定する
			try
			{
				SQLite::Statement query(*database, row);
				for (auto person = imageInfo.people.begin(); person != imageInfo.people.end(); person++)
				{
					query.reset();
					query.bind(1, (long long)imageInfo.frameNumber);
					query.bind(2, (long long)person->first);
					for (size_t nodeIndex = 0; nodeIndex < person->second.size(); nodeIndex++)
					{
						query.bind(3 + nodeIndex * 3 + 0, person->second[nodeIndex].x);
						query.bind(3 + nodeIndex * 3 + 1, person->second[nodeIndex].y);
						query.bind(3 + nodeIndex * 3 + 2, person->second[nodeIndex].confidence);
					}
					(void)query.exec();
				}
				if ((saveFreq > 0) && (--saveCountDown <= 0) && upTransaction)
				{
					saveCountDown = saveFreq;
					upTransaction->commit();
					upTransaction = std::make_unique<SQLite::Transaction>(*database);
				}
			}
			catch (const std::exception& e) {
				std::cout << "error : " << __FILE__ << " : L" << __LINE__ << "\n" << e.what() << std::endl;
				return 1;
			}
		}

		return 0;
	}
	void recieveErrors(const std::vector<std::string>& errors) override final
	{
		for (auto error : errors)
			std::cout << error << std::endl;
	}
};