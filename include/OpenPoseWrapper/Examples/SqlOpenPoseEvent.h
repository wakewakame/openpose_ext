#pragma once

#include <OpenPoseWrapper/OpenPoseEvent.h>
#include <Utils/Database.h>

class SqlOpenPoseEvent : public OpenPoseEvent
{
private:
	bool writeMode;
	std::string sqlPath;
	Database database;
	std::unique_ptr<SQLite::Transaction> upTransaction;
	long long saveFreq = 0;
	size_t saveCountDown = 1;
public:
	SqlOpenPoseEvent(const std::string& sqlPath, bool writeMode, long long saveFreq = 0) :
		sqlPath(sqlPath), writeMode(writeMode), saveFreq(saveFreq){}
	virtual ~SqlOpenPoseEvent() {};
	int init() override final
	{
		// sqlÇÃê∂ê¨
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
		if (upTransaction && writeMode) upTransaction->commit();
	}
	int sendImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
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
		if (writeMode)
		{
			// sqlï∂ÇÃê∂ê¨
			std::string row = "?";
			for (int colIndex = 0; colIndex < 76; colIndex++) row += ", ?";
			row = "INSERT INTO people VALUES (" + row + ")";
			// sqlï∂ÇÃílÇämíËÇ∑ÇÈ
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
						query.bind(3 + nodeIndex * 3 + 0, (double)person->second[nodeIndex].x);
						query.bind(3 + nodeIndex * 3 + 1, (double)person->second[nodeIndex].y);
						query.bind(3 + nodeIndex * 3 + 2, (double)person->second[nodeIndex].confidence);
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