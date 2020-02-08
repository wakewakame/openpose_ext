#pragma once

#include <OpenPoseWrapper/OpenPoseEvent.h>
#include <Utils/Database.h>

class SqlOpenPoseEvent : public OpenPoseEvent
{
private:
	std::string sqlPath;
	Database database;
	std::unique_ptr<SQLite::Transaction> upTransaction;
	long long saveFreq = 0;
	size_t saveCountDown = 1;
public:
	SqlOpenPoseEvent(const std::string& sqlPath, long long saveFreq = 0) :
		sqlPath(sqlPath), saveFreq(saveFreq){}
	virtual ~SqlOpenPoseEvent() {};
	int init() override final
	{
		// sqlの生成
		try
		{
			// ファイルのオープン
			database = createDatabase(
				sqlPath,
				SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE
			);

			// peopleテーブルが存在しない場合はテーブルを生成
			if (!database->tableExists("people"))
			{
				std::string row_title = "frame INTEGER, people INTEGER";
				for (int i = 0; i < 25; i++)
				{
					row_title += ", joint" + std::to_string(i) + "x REAL";
					row_title += ", joint" + std::to_string(i) + "y REAL";
					row_title += ", joint" + std::to_string(i) + "confidence REAL";
				}
				database->exec("CREATE TABLE people (" + row_title + ")");
				database->exec("CREATE INDEX idx_frame_on_people ON people(frame)");
			}

			// timestampテーブルが存在しない場合はテーブルを生成
			if (!database->tableExists("timestamp"))
			{
				std::string row_title = "frame INTEGER PRIMARY KEY, timestamp INTEGER";
				database->exec("CREATE TABLE timestamp (" + row_title + ")");
				database->exec("CREATE INDEX idx_frame_on_timestamp ON timestamp(frame)");
			}

			// トランザクションの開始
			upTransaction = std::make_unique<SQLite::Transaction>(*database);
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
		if (upTransaction) upTransaction->commit();
	}
	int sendImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		try
		{
			// SQLにタイムスタンプが存在するか確認
			SQLite::Statement timestampQuery(*database, "SELECT count(*) FROM timestamp WHERE frame=?");
			timestampQuery.bind(1, (long long)imageInfo.frameNumber);
			(void)timestampQuery.executeStep();
			// SQLにタイムスタンプが存在した場合はその時刻の骨格データを使用する
			if (1 == timestampQuery.getColumn(0).getInt())
			{
				imageInfo.needOpenposeProcess = false;
				SQLite::Statement peopleQuery(*database, "SELECT * FROM people WHERE frame=?");
				peopleQuery.bind(1, (long long)imageInfo.frameNumber);
				while (peopleQuery.executeStep())
				{
					size_t index = (size_t)peopleQuery.getColumn(1).getInt64();
					std::vector<ImageInfo::Node> nodes;
					for (int nodeIndex = 0; nodeIndex < 25; nodeIndex++)
					{
						nodes.push_back(ImageInfo::Node{
							(float)peopleQuery.getColumn(2 + nodeIndex * 3 + 0).getDouble(),
							(float)peopleQuery.getColumn(2 + nodeIndex * 3 + 1).getDouble(),
							(float)peopleQuery.getColumn(2 + nodeIndex * 3 + 2).getDouble()
						});
					}
					imageInfo.people[index] = std::move(nodes);
				}
			}
		}
		catch (const std::exception& e)
		{
			std::cout << "error : " << __FILE__ << " : L" << __LINE__ << "\n" << e.what() << std::endl;
			return 1;
		}

		return 0;
	}
	int recieveImageInfo(ImageInfo& imageInfo, std::function<void(void)> exit) override final
	{
		// OpenPoseが骨格検出を行ったか確認
		if (imageInfo.needOpenposeProcess)
		{
			try
			{
				// peopleテーブルの更新
				std::string row = "?";
				for (int colIndex = 0; colIndex < 76; colIndex++) row += ", ?";
				row = "INSERT INTO people VALUES (" + row + ")";
				SQLite::Statement peopleQuery(*database, row);
				for (auto person = imageInfo.people.begin(); person != imageInfo.people.end(); person++)
				{
					peopleQuery.reset();
					peopleQuery.bind(1, (long long)imageInfo.frameNumber);
					peopleQuery.bind(2, (long long)person->first);
					for (size_t nodeIndex = 0; nodeIndex < person->second.size(); nodeIndex++)
					{
						peopleQuery.bind(3 + nodeIndex * 3 + 0, (double)person->second[nodeIndex].x);
						peopleQuery.bind(3 + nodeIndex * 3 + 1, (double)person->second[nodeIndex].y);
						peopleQuery.bind(3 + nodeIndex * 3 + 2, (double)person->second[nodeIndex].confidence);
					}
					(void)peopleQuery.exec();
				}

				// timestampテーブルの更新
				row = "INSERT INTO timestamp VALUES (?, ?)";
				SQLite::Statement timestampQuery(*database, row);
				timestampQuery.reset();
				timestampQuery.bind(1, (long long)imageInfo.frameNumber);
				timestampQuery.bind(2, (long long)imageInfo.frameTimeStamp);
				(void)timestampQuery.exec();

				// sqlのセーブ
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