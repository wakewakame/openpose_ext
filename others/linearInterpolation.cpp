/*

このプログラムでは、既に生成されたSQLファイルの空欄の穴埋めを行います。
具体的には、people_with_trackingテーブルの信頼値が0になっている場所を線形補完します。
また、プログラムを実行するとSQLファイルが書き換えられるため、注意してください。

*/

#include <Utils/Database.h>
#include <string>



struct Node { double x, y, confidence; };

/**
 * people_with_trackingテーブル上の指定された範囲を線形補完する
 * 具体的には、start+1からend-1までのフレームを、startとendの値を用いて線形補完する
 * startに-1が指定された場合は、end-1以下の全てのフレームをendと同じ値にする
 * endに-1が指定された場合は、start+1以上の全てのフレームをstartと同じ値にする
 */
size_t linearInterpolation(
    const uint32_t person_id, const uint8_t joint_id,
    const int64_t start, Node start_node,
    const int64_t end, Node end_node,
    Database& database
);

int main(int argc, char* argv[]) {
    // .sqlite3ファイルのパス
    std::string sql_path = R"(C:\Users\0214t\Documents\github\guest003-2020-08-20_09-11-35.mp4.sqlite3)";

    // 空欄を埋めた回数の合計
    size_t changed_sum = 0;

    Database database;
    database.create(
        sql_path,
        SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE
    );

    // 骨格25点分ループ
    for (uint8_t joint_id = 0; joint_id < 25; joint_id++) {
        // SQLの骨格の列名
        const std::string joint_x = "joint" + std::to_string(joint_id) + "x";
        const std::string joint_y = "joint" + std::to_string(joint_id) + "y";
        const std::string joint_confidence = "joint" + std::to_string(joint_id) + "confidence";

        // people_with_trackingテーブルの全ての人のIDを昇順でループする
        SQLite::Statement person_query(*database.database,
            "SELECT people FROM people_with_tracking GROUP BY people ORDER BY people ASC"
        );
        while (person_query.executeStep()) {
            uint32_t person_id = person_query.getColumn(0).getUInt();

            // person_idの全てのフレーム分の骨格情報を取得する
            SQLite::Statement frame_query(*database.database,
                "SELECT frame, " + joint_x + ", " + joint_y + ", " + joint_confidence +
                " FROM people_with_tracking WHERE people=? ORDER BY frame ASC"
            );
            frame_query.bind(1, person_id);
            size_t blank_count = 0;  // 信頼値が0の数を数える
            Node last_node = { 0.0f, 0.0f, 0.0f };
            int64_t frame = -1, last_frame = -1;
            while (frame_query.executeStep()) {
                frame = frame_query.getColumn(0).getUInt();
                const Node node{
                    frame_query.getColumn(1).getDouble(),
                    frame_query.getColumn(2).getDouble(),
                    frame_query.getColumn(3).getDouble()
                };

                if (0.0f == node.confidence) { blank_count++; continue; }

                // これ以前のフレームに信頼値が0のマスがあるとき
                if (blank_count > 0) {
                    // last_frameからframeまでの間の空白を線形補完する
                    changed_sum += linearInterpolation(
                        person_id, joint_id, last_frame, last_node, frame, node, database
                    );
                }

                last_node = node; last_frame = frame;
                blank_count = 0;
            }
            if ((-1 != last_frame) && (frame != last_frame)) {
                // last_frameから末端までの間の空白を補完する
                changed_sum += linearInterpolation(
                    person_id, joint_id, last_frame, last_node, -1, Node{}, database
                );
            }
        }

        // 進捗の表示
        std::cout << "joint " << ((int)joint_id + 1) << " / 25" << std::endl;
    }

    std::cout << "finish" << std::endl;

    // 埋めた空欄の数(x, y, confidenceで1)
    std::cout << "blank : " << changed_sum << std::endl;

    return 0;
}

size_t linearInterpolation(
    const uint32_t person_id, const uint8_t joint_id,
    const int64_t start, Node start_node,
    const int64_t end, Node end_node,
    Database& database
) {
    // 空欄を埋めた回数の合計
    size_t changed_sum = 0;

    // SQLの骨格の列名
    const std::string joint_x = "joint" + std::to_string(joint_id) + "x";
    const std::string joint_y = "joint" + std::to_string(joint_id) + "y";
    const std::string joint_confidence = "joint" + std::to_string(joint_id) + "confidence";

    // people=person_id AND start < frame AND frame < end のデータを取得するSQL文の作成
    std::string query_string = "SELECT frame FROM people_with_tracking WHERE people=?";
    if (0 <= start) { query_string += " AND ?<frame"; }
    else { start_node = end_node; }
    if (0 <= end) { query_string += " AND frame<?"; }
    else { end_node = start_node; }

    // SQL実行
    SQLite::Statement blank_query(*database.database, query_string);
    uint8_t bind_index = 1;
    blank_query.bind(bind_index++, person_id);
    if (0 <= start) blank_query.bind(bind_index++, start);
    if (0 <= end)   blank_query.bind(bind_index++, end);

    // 全ての結果を取得し、空欄を穴埋め
    SQLite::Statement update_query(*database.database,
        "UPDATE people_with_tracking SET " +
        joint_x + "=?, " + joint_y + "=?, " + joint_confidence + "=? WHERE frame=? AND people=?"
    );
    while (blank_query.executeStep()) {
        const int64_t frame = blank_query.getColumn(0).getUInt();
        const double proportional = (double)(frame - start) / (double)(end - start);
        const Node node{
            start_node.x + (end_node.x - start_node.x) * proportional,
            start_node.y + (end_node.y - start_node.y) * proportional,
            start_node.confidence + (end_node.confidence - start_node.confidence) * proportional
        };

        update_query.reset();
        update_query.bind(1, node.x);
        update_query.bind(2, node.y);
        update_query.bind(3, node.confidence);
        update_query.bind(4, frame);
        update_query.bind(5, (long long)person_id);
        update_query.exec();

        changed_sum++;
    }

    return changed_sum;
}