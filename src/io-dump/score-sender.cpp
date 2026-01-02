#include "io-dump/score-sender.h"
#include "io-dump/player-status-dump-json.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "view/display-messages.h"

// libcurl static library definition for Windows
#ifdef WINDOWS
#ifndef CURL_STATICLIB
#define CURL_STATICLIB
#endif
#endif

#include <curl/curl.h>
#include <string>

#ifdef WORLD_SCORE

// デフォルトのスコアサーバーURL
static std::string score_server_url = "http://localhost:3000/submit";

/*!
 * @brief curlのレスポンスデータを受け取るコールバック関数
 * @param contents 受け取ったデータ
 * @param size データのサイズ単位
 * @param nmemb データの個数
 * @param userp ユーザーポインタ（std::string*）
 * @return 処理したバイト数
 */
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

/*!
 * @brief スコアサーバーのURLを取得
 * @return スコアサーバーのURL
 */
std::string get_score_server_url()
{
    return score_server_url;
}

/*!
 * @brief スコアサーバーのURLを設定
 * @param url 設定するURL
 */
void set_score_server_url(const std::string &url)
{
    score_server_url = url;
}

/*!
 * @brief スコアをワールドスコアサーバーに送信する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 成功した場合true、失敗した場合false
 * @details
 * libcurlを使用してHTTP POSTリクエストでJSONデータをスコアサーバーに送信する。
 * サーバーへの送信に失敗してもゲームは続行される（エラーメッセージのみ表示）。
 */
bool send_score_json(PlayerType *player_ptr)
{
    // JSON文字列を生成
    std::string json_data = dump_player_status_json(player_ptr);

    // curlの初期化
    CURL *curl = curl_easy_init();
    if (!curl) {
        msg_print("Failed to initialize CURL.");
        return false;
    }

    bool success = false;
    std::string response_string;
    std::string error_buffer;
    error_buffer.resize(CURL_ERROR_SIZE);

    // curlの設定
    curl_easy_setopt(curl, CURLOPT_URL, score_server_url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, json_data.length());

    // HTTPヘッダーの設定
    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // レスポンス受信のコールバック設定
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

    // エラーバッファの設定
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, &error_buffer[0]);

    // タイムアウト設定（10秒）
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);

    // SSL検証をスキップ（ローカルサーバー用）
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    // メッセージ表示
    msg_print("Sending score to world score server...");

    // リクエスト実行
    CURLcode res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
        long response_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        if (response_code == 200) {
            msg_print("Score successfully sent to world score server!");
            success = true;
        } else {
            msg_format("Server returned error code: %ld", response_code);
            if (!response_string.empty()) {
                msg_format("Server response: %s", response_string.c_str());
            }
        }
    } else {
        msg_format("Failed to send score: %s", error_buffer.c_str());
        msg_format("Server URL: %s", score_server_url.c_str());
    }

    // クリーンアップ
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return success;
}

#endif