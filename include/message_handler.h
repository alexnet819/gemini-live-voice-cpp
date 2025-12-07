#pragma once

#include <string>
#include <vector>
#include <cstdint>

/**
 * @brief Gemini Live APIのメッセージを構築・パースするクラス
 */
class MessageHandler {
public:
    /**
     * @brief セットアップメッセージを構築
     * 
     * @param model_name モデル名（例: "gemini-2.0-flash-exp"）
     * @param enable_search Google検索機能を有効にするか
     * @param temperature 生成の温度パラメータ
     * @param top_p Top-Pサンプリングパラメータ
     * @param top_k Top-Kサンプリングパラメータ
     * @param system_instruction_text システム命令のテキスト
     * @return std::string JSON形式のセットアップメッセージ
     */
    static std::string create_setup_message(
        const std::string& model_name = "gemini-2.0-flash-exp",
        bool enable_search = false,
        double temperature = 1.0,
        double top_p = 0.95,
        int top_k = 40,
        const std::string& system_instruction_text = ""
    );

    /**
     * @brief リアルタイム音声入力メッセージを構築
     * 
     * @param audio_data 音声データ（16-bit PCM, 16kHz）
     * @param mime_type MIMEタイプ（デフォルト: "audio/pcm;rate=16000"）
     * @return std::string JSON形式の音声入力メッセージ
     */
    static std::string create_audio_input_message(
        const std::vector<int16_t>& audio_data,
        const std::string& mime_type = "audio/pcm;rate=16000"
    );

    /**
     * @brief base64エンコード
     * 
     * @param data エンコードするデータ
     * @return std::string base64エンコードされた文字列
     */
    static std::string base64_encode(const std::vector<uint8_t>& data);

    /**
     * @brief base64デコード
     * 
     * @param encoded_string base64エンコードされた文字列
     * @return std::vector<uint8_t> デコードされたデータ
     */
    static std::vector<uint8_t> base64_decode(const std::string& encoded_string);

    /**
     * @brief サーバーレスポンスから音声データを抽出
     * 
     * @param json_response JSON形式のサーバーレスポンス
     * @param audio_data 抽出された音声データ（出力）
     * @return true 音声データが見つかった
     * @return false 音声データが見つからなかった
     */
    static bool extract_audio_from_response(
        const std::string& json_response,
        std::vector<int16_t>& audio_data
    );

    /**
     * @brief サーバーレスポンスから音声文字起こしテキストを抽出
     * 
     * @param json_response JSON形式のサーバーレスポンス
     * @param transcription 抽出された文字起こしテキスト（出力）
     * @return true 文字起こしが見つかった
     * @return false 文字起こしが見つからなかった
     */
    static bool extract_transcription_from_response(
        const std::string& json_response,
        std::string& transcription
    );

    /**
     * @brief サーバーレスポンスがユーザー入力の文字起こしかチェック
     * 
     * @param json_response JSON形式のサーバーレスポンス
     * @return true ユーザー入力の文字起こし
     * @return false その他のメッセージ
     */
    static bool is_user_input_transcription(const std::string& json_response);

    /**
     * @brief レスポンスがターン完了を示しているか確認
     * 
     * @param json_response JSON形式のサーバーレスポンス
     * @return true ターン完了
     * @return false ターン未完了
     */
    static bool is_turn_complete(const std::string& json_response);

    /**
     * @brief int16のベクトルをuint8のベクトルに変換
     * 
     * @param int16_data int16形式のデータ
     * @return std::vector<uint8_t> uint8形式のデータ
     */
    static std::vector<uint8_t> int16_to_uint8(const std::vector<int16_t>& int16_data);
};
