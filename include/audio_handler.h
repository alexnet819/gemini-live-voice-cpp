#pragma once

#include "miniaudio.h"
#include <functional>
#include <vector>
#include <memory>
#include <atomic>
#include <cstdint>
#include <mutex>

/**
 * @brief 音声入出力を管理するクラス（miniaudio使用）
 */
class AudioHandler {
public:
    using AudioCallback = std::function<void(const std::vector<int16_t>&)>;

    /**
     * @brief AudioHandlerのコンストラクタ
     */
    AudioHandler();

    /**
     * @brief デストラクタ
     */
    ~AudioHandler();

    /**
     * @brief miniaudioを初期化
     * 
     * @return true 初期化成功
     * @return false 初期化失敗
     */
    bool initialize();

    /**
     * @brief 音声録音を開始
     * 
     * @param callback 音声チャンクが録音されたときに呼ばれるコールバック
     * @return true 開始成功
     * @return false 開始失敗
     */
    bool start_recording(AudioCallback callback);

    /**
     * @brief 音声録音を停止
     */
    void stop_recording();

    /**
     * @brief 音声を再生
     * 
     * @param audio_data 再生する音声データ（16-bit PCM）
     * @param sample_rate サンプルレート（デフォルト: 24000Hz）
     * @return true 再生成功
     * @return false 再生失敗
     */
    bool play_audio(const std::vector<int16_t>& audio_data, int sample_rate = 24000);

    /**
     * @brief 録音中かどうかを確認
     * 
     * @return true 録音中
     * @return false 録音していない
     */
    bool is_recording() const;

private:
    // miniaudioのコールバック関数
    static void capture_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
    
    ma_device capture_device_;
    ma_device_config capture_config_;
    
    AudioCallback audio_callback_;
    std::atomic<bool> recording_;
    std::atomic<bool> initialized_;
    ma_device_id playback_device_id_;
    bool playback_device_selected_ = false;
    
    static constexpr int SAMPLE_RATE = 16000;  // 入力サンプルレート
    static constexpr int SAMPLE_RATE_OUTPUT = 24000; // 出力サンプルレート
    static constexpr int CHANNELS = 1;          // モノラル

private:
    // 再生用コールバック
    static void playback_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

    ma_device playback_device_;
    ma_device_config playback_config_;
    bool playback_initialized_ = false;
    
    // 再生用リングバッファ（簡易的な実装としてvectorとmutexを使用）
    std::vector<int16_t> playback_buffer_;
    std::mutex playback_mutex_;
};
