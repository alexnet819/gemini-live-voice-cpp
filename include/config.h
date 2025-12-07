#pragma once

#include <string>
#include <vector>

/**
 * @brief アプリケーション設定を管理するクラス
 */
class Config {
public:
    /**
     * @brief デフォルトコンストラクタ（デフォルト設定を使用）
     */
    Config();

    /**
     * @brief 設定ファイルから読み込むコンストラクタ
     * 
     * @param config_path 設定ファイルのパス
     * @throw std::runtime_error ファイル読み込みまたはパースエラー時
     */
    explicit Config(const std::string& config_path);

    // モデル設定
    std::string getModelName() const { return model_name_; }
    double getTemperature() const { return temperature_; }
    double getTopP() const { return top_p_; }
    int getTopK() const { return top_k_; }
    std::vector<std::string> getResponseModalities() const { return response_modalities_; }

    // 機能設定
    bool isSearchEnabled() const { return enable_search_; }
    bool isInputAudioTranscriptionEnabled() const { return input_audio_transcription_; }
    bool isOutputAudioTranscriptionEnabled() const { return output_audio_transcription_; }

    // システム命令
    std::string getSystemInstructionText() const { return system_instruction_text_; }

    // 音声設定
    int getInputSampleRate() const { return input_sample_rate_; }
    int getOutputSampleRate() const { return output_sample_rate_; }
    int getChunkSize() const { return chunk_size_; }
    int getBufferSize() const { return buffer_size_; }
    int getMinBufferSize() const { return min_buffer_size_; }
    int getGainFactor() const { return gain_factor_; }

    /**
     * @brief 設定をコンソールに出力
     */
    void print() const;

private:
    // モデル設定
    std::string model_name_;
    double temperature_;
    double top_p_;
    int top_k_;
    std::vector<std::string> response_modalities_;

    // 機能設定
    bool enable_search_;
    bool input_audio_transcription_;
    bool output_audio_transcription_;

    // システム命令
    std::string system_instruction_text_;

    // 音声設定
    int input_sample_rate_;
    int output_sample_rate_;
    int chunk_size_;
    int buffer_size_;
    int min_buffer_size_;
    int gain_factor_;

    /**
     * @brief デフォルト設定を初期化
     */
    void setDefaults();

    /**
     * @brief 設定ファイルから読み込み
     * 
     * @param config_path 設定ファイルのパス
     */
    void loadFromFile(const std::string& config_path);
};
