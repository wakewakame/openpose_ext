#pragma once

#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <Utils/Database.h>
#include <optional>

class SqlOpenPose : public Database
{
private:
    // sqlite3形式のファイルを保存するパス
    std::string sqlPath;

    // ファイルにコミットする周期
    long long saveFreq = 0;

    // ファイルにコミットするまでのカウント
    size_t saveCountDown = 1;

    using People = MinOpenPose::People;
    using Node = MinOpenPose::Node;

public:
    SqlOpenPose() {}

    virtual ~SqlOpenPose() {};

    /**
     * OpenPoseの姿勢推定の結果をSQLite3として出力するクラス
     * @param sqlPath 出力ファイルのパス
     * @param saveFreq 指定したフレーム数ごとにファイルを更新する(たとえば300を指定するとwrite関数が300回呼ばれるごとにファイルを更新する)
     */
    int open(const std::string& sqlPath, const long long saveFreq = 0)
    {
        this->sqlPath = sqlPath;
        this->saveFreq = saveFreq;
        saveCountDown = saveFreq;

        // ファイルを開く、もしくは生成する
        int ret = create(
            sqlPath,
            SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE
        );

        if (ret)
        {
            std::cout << "ファイルの読み込み、もしくは生成に失敗しました。" << std::endl;
            return 1;
        }

        try
        {
            // peopleテーブルが存在しない場合はテーブルを生成
            std::string row_title = u8"frame INTEGER, people INTEGER";
            for (int i = 0; i < 25; i++)
            {
                row_title += u8", joint" + std::to_string(i) + u8"x REAL";
                row_title += u8", joint" + std::to_string(i) + u8"y REAL";
                row_title += u8", joint" + std::to_string(i) + u8"confidence REAL";
            }
            if (createTableIfNoExist(u8"people", row_title)) return 1;

            // timestampテーブルが存在しない場合はテーブルを生成
            if (createTableIfNoExist(u8"timestamp", u8"frame INTEGER PRIMARY KEY, timestamp INTEGER")) return 1;

            // 検索速度を高速化するため、各テーブルにIndexを生成
            if (createIndexIfNoExist(u8"people", u8"frame", false)) return 1;
            if (createIndexIfNoExist(u8"people", u8"people", false)) return 1;
            if (createIndexIfNoExist(u8"people", u8"frame", u8"people", true)) return 1;
            if (createIndexIfNoExist(u8"timestamp", u8"frame", true)) return 1;
        }
        catch (const std::exception& e)
        {
            std::cout << u8"error : " << __FILE__ << u8" : L" << __LINE__ << u8"\n" << e.what() << std::endl;
            return 1;
        }

        return 0;
    }

    std::optional<People> readBones(const size_t frameNumber)
    {
        try
        {
            // SQLにタイムスタンプが存在した場合
            if (isDataExist(u8"timestamp", u8"frame", frameNumber))
            {
                // 指定されたフレーム番号に映る人すべての骨格を検索する
                People people;
                SQLite::Statement peopleQuery(*database, u8"SELECT * FROM people WHERE frame=?");
                peopleQuery.bind(1, (long long)frameNumber);
                while (peopleQuery.executeStep())
                {
                    size_t index = (size_t)peopleQuery.getColumn(1).getInt64();
                    for (int nodeIndex = 0; nodeIndex < 25; nodeIndex++)
                    {
                        people[index].push_back(MinOpenPose::Node{
                            (float)peopleQuery.getColumn(2 + nodeIndex * 3 + 0).getDouble(),
                            (float)peopleQuery.getColumn(2 + nodeIndex * 3 + 1).getDouble(),
                            (float)peopleQuery.getColumn(2 + nodeIndex * 3 + 2).getDouble()
                            });
                    }
                }

                // 検索結果を返す
                return people;
            }
        }
        catch (const std::exception& e)
        {
            std::cout << u8"error : " << __FILE__ << u8" : L" << __LINE__ << u8"\n" << e.what() << std::endl;
        }

        // SQL上に指定されたフレームが記録されていない場合、もしくはエラーが起きた場合はnulloptを返す
        return std::nullopt;
    }

    int writeBones(const size_t frameNumber, const size_t frameTimeStamp, const People& people)
    {
        try
        {
            // SQLにタイムスタンプが存在しなかった場合はSQLにデータを追加する
            if (!isDataExist(u8"timestamp", u8"frame", frameNumber))
            {
                // peopleテーブルの更新
                std::string row = u8"?";
                for (int colIndex = 0; colIndex < 76; colIndex++) row += u8", ?";
                row = u8"INSERT INTO people VALUES (" + row + u8")";
                SQLite::Statement peopleQuery(*database, row);
                for (auto person = people.begin(); person != people.end(); person++)
                {
                    peopleQuery.reset();
                    peopleQuery.bind(1, (long long)frameNumber);
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
                row = u8"INSERT INTO timestamp VALUES (?, ?)";
                SQLite::Statement timestampQuery(*database, row);
                timestampQuery.reset();
                timestampQuery.bind(1, (long long)frameNumber);
                timestampQuery.bind(2, (long long)frameTimeStamp);
                (void)timestampQuery.exec();
            }

            // sqlのコミット
            if ((saveFreq > 0) && (--saveCountDown <= 0))
            {
                saveCountDown = saveFreq;
                commit();
            }
        }
        catch (const std::exception& e)
        {
            std::cout << "error : " << __FILE__ << " : L" << __LINE__ << "\n" << e.what() << std::endl;
            return 1;
        }

        return 0;
    }

    std::map<size_t, Node> readPoints(const std::string& tableName, const size_t frameNumber)
    {
        std::map<size_t, Node> result;
        
        try
        {
            // SQLにテーブルが存在した場合
            if (isDataExist("sqlite_master", "type", "name", "table", tableName))
            {
                // 指定されたフレーム番号に映る人すべての骨格の重心を検索する
                SQLite::Statement peopleQuery(*database, u8"SELECT * FROM " + tableName + u8" WHERE frame=?");
                peopleQuery.bind(1, (long long)frameNumber);
                while (peopleQuery.executeStep())
                {
                    size_t index = (size_t)peopleQuery.getColumn(1).getInt64();
                    float x = (float)peopleQuery.getColumn(2).getDouble();
                    float y = (float)peopleQuery.getColumn(3).getDouble();
                    result[index] = Node{ x, y, 0.0 };
                }
            }
        }
        catch (const std::exception& e)
        {
            std::cout << u8"error : " << __FILE__ << u8" : L" << __LINE__ << u8"\n" << e.what() << std::endl;
        }

        // SQL上に指定されたフレームが記録されていない場合、もしくはエラーが起きた場合はnulloptを返す
        return result;
    }

    int writePoints(const std::string& tableName, const size_t frameNumber, const std::map<size_t, Node> points)
    {
        try
        {
            // テーブルが存在しない場合はテーブルを生成
            std::string row_title = u8"frame INTEGER, people INTEGER, x REAL, y REAL";
            if (createTableIfNoExist(tableName, row_title)) return 1;

            // SQLの検索を高速化するためにIndexを作成
            if (createIndexIfNoExist(tableName, u8"frame", false)) return 1;
            if (createIndexIfNoExist(tableName, u8"people", false)) return 1;
            if (createIndexIfNoExist(tableName, u8"frame", u8"people", true)) return 1;

            // SQL文の生成
            SQLite::Statement deleteQuery(*database, u8"DELETE FROM " + tableName + u8" WHERE frame=?");
            SQLite::Statement insertQuery(*database, u8"INSERT INTO " + tableName + u8" VALUES (?, ?, ?, ?)");

            // 既にデータがあった場合は上書きするために削除
            deleteQuery.reset();
            deleteQuery.bind(1, (long long)frameNumber);
            (void)deleteQuery.exec();

            // 現在のフレームの情報をSQLに追記
            for (auto pointItr = points.begin(); pointItr != points.end(); pointItr++)
            {
                insertQuery.reset();
                insertQuery.bind(1, (long long)frameNumber);
                insertQuery.bind(2, (long long)pointItr->first);
                insertQuery.bind(3, (double)pointItr->second.x);
                insertQuery.bind(4, (double)pointItr->second.y);
                (void)insertQuery.exec();
            }

            // sqlのコミット
            if ((saveFreq > 0) && (--saveCountDown <= 0))
            {
                saveCountDown = saveFreq;
                commit();
            }
        }
        catch (const std::exception& e)
        {
            std::cout << "error : " << __FILE__ << " : L" << __LINE__ << "\n" << e.what() << std::endl;
            return 1;
        }

        return 0;
    }
};