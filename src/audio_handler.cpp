#define MINIAUDIO_IMPLEMENTATION
#include "audio_handler.h"
#include <iostream>
#include <cstring>

AudioHandler::AudioHandler()
    : recording_(false)
    , initialized_(false)
    , playback_initialized_(false) {
    memset(&capture_device_, 0, sizeof(capture_device_));
    memset(&playback_device_, 0, sizeof(playback_device_));
}

AudioHandler::~AudioHandler() {
    stop_recording();
    
    if (playback_initialized_) {
        ma_device_stop(&playback_device_);
        ma_device_uninit(&playback_device_);
    }
    
    if (initialized_) {
        ma_device_uninit(&capture_device_);
    }
}

bool AudioHandler::initialize() {
    std::cout << "miniaudio initialized successfully" << std::endl;
    
    // Enumerate available devices
    ma_context context;
    if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS) {
        std::cerr << "Context initialization error" << std::endl;
        return false;
    }
    
    ma_device_info* pPlaybackDeviceInfos;
    ma_uint32 playbackDeviceCount;
    ma_device_info* pCaptureDeviceInfos;
    ma_uint32 captureDeviceCount;
    
    if (ma_context_get_devices(&context, &pPlaybackDeviceInfos, &playbackDeviceCount, 
                                &pCaptureDeviceInfos, &captureDeviceCount) != MA_SUCCESS) {
        std::cerr << "Device enumeration error" << std::endl;
        ma_context_uninit(&context);
        return false;
    }
    
    
    // std::cout << "利用可能な音声デバイス:" << std::endl;
    // std::cout << "  キャプチャデバイス: " << captureDeviceCount << "個" << std::endl;
    // for (ma_uint32 i = 0; i < captureDeviceCount; i++) {
    //     std::cout << "    [" << i << "] " << pCaptureDeviceInfos[i].name << std::endl;
    // }
    // 
    // std::cout << "  再生デバイス: " << playbackDeviceCount << "個" << std::endl;
    // for (ma_uint32 i = 0; i < playbackDeviceCount; i++) {
    //     std::cout << "    [" << i << "] " << pPlaybackDeviceInfos[i].name << std::endl;
    // }
    
    // Select playback device (use the first device)
    if (playbackDeviceCount > 0) {
        playback_device_id_ = pPlaybackDeviceInfos[0].id;
        playback_device_selected_ = true;
        // std::cout << "使用する再生デバイス: " << pPlaybackDeviceInfos[0].name << std::endl;
    }
    
    // Initialize and start playback device (always open)
    playback_config_ = ma_device_config_init(ma_device_type_playback);
    playback_config_.playback.format   = ma_format_s16;
    playback_config_.playback.channels = CHANNELS;
    playback_config_.sampleRate        = SAMPLE_RATE_OUTPUT;
    playback_config_.dataCallback      = playback_callback;
    playback_config_.pUserData         = this;
    
    if (playback_device_selected_) {
        playback_config_.playback.pDeviceID = &playback_device_id_;
    }
    
    if (ma_device_init(NULL, &playback_config_, &playback_device_) != MA_SUCCESS) {
        std::cerr << "Failed to initialize playback device" << std::endl;
    } else {
        if (ma_device_start(&playback_device_) != MA_SUCCESS) {
            std::cerr << "Failed to start playback device" << std::endl;
            ma_device_uninit(&playback_device_);
        } else {
            playback_initialized_ = true;
        }
    }
    
    ma_context_uninit(&context);
    initialized_ = true;
    return true;
}

void AudioHandler::capture_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    AudioHandler* handler = static_cast<AudioHandler*>(pDevice->pUserData);
    
    if (pInput && handler && handler->audio_callback_) {
        const int16_t* input = static_cast<const int16_t*>(pInput);
        std::vector<int16_t> audio_data(input, input + frameCount);
        handler->audio_callback_(audio_data);
    }
    
    (void)pOutput;  // Unused
}

bool AudioHandler::start_recording(AudioCallback callback) {
    if (recording_) {
        std::cerr << "Already recording" << std::endl;
        return false;
    }
    
    audio_callback_ = std::move(callback);
    
    // Capture device configuration
    capture_config_ = ma_device_config_init(ma_device_type_capture);
    capture_config_.capture.format   = ma_format_s16;
    capture_config_.capture.channels = CHANNELS;
    capture_config_.sampleRate       = SAMPLE_RATE;
    capture_config_.dataCallback     = capture_callback;
    capture_config_.pUserData        = this;
    
    // Initialize device
    if (ma_device_init(NULL, &capture_config_, &capture_device_) != MA_SUCCESS) {
        std::cerr << "Failed to initialize capture device" << std::endl;
        std::cerr << "Hint: PulseAudio or --dummy-audio option is required in WSL environment" << std::endl;
        return false;
    }
    
    // std::cout << "入力デバイスを選択: " << capture_device_.capture.name << std::endl;
    
    // Start device
    if (ma_device_start(&capture_device_) != MA_SUCCESS) {
        std::cerr << "Failed to start capture device" << std::endl;
        ma_device_uninit(&capture_device_);
        return false;
    }
    
    recording_ = true;
    // std::cout << "録音を開始しました" << std::endl;
    return true;
}

void AudioHandler::stop_recording() {
    if (!recording_) {
        return;
    }
    
    recording_ = false;
    
    ma_device_stop(&capture_device_);
    ma_device_uninit(&capture_device_);
    
    // std::cout << "録音を停止しました" << std::endl;
}

bool AudioHandler::play_audio(const std::vector<int16_t>& audio_data, int /* sample_rate */) {
    if (audio_data.empty() || !playback_initialized_) {
        return false;
    }
    
    // Add data to buffer
    std::lock_guard<std::mutex> lock(playback_mutex_);
    playback_buffer_.insert(playback_buffer_.end(), audio_data.begin(), audio_data.end());
    
    return true;
}

void AudioHandler::playback_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    AudioHandler* handler = static_cast<AudioHandler*>(pDevice->pUserData);
    int16_t* output = static_cast<int16_t*>(pOutput);
    
    std::lock_guard<std::mutex> lock(handler->playback_mutex_);
    
    size_t frames_available = handler->playback_buffer_.size();
    size_t frames_to_copy = std::min((size_t)frameCount, frames_available);
    
    if (frames_to_copy > 0) {
        memcpy(output, handler->playback_buffer_.data(), frames_to_copy * sizeof(int16_t));
        
        // Remove played data (inefficient but using vector for simplicity)
        handler->playback_buffer_.erase(handler->playback_buffer_.begin(), handler->playback_buffer_.begin() + frames_to_copy);
    }
    
    // Fill remaining with zeros
    if (frames_to_copy < frameCount) {
        memset(output + frames_to_copy, 0, (frameCount - frames_to_copy) * sizeof(int16_t));
    }
    
    (void)pInput;
}

bool AudioHandler::is_recording() const {
    return recording_;
}
