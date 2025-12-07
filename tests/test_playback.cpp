#define MINIAUDIO_IMPLEMENTATION
#include "../third_party/miniaudio/miniaudio.h"
#include <iostream>
#include <cmath>
#include <thread>
#include <chrono>

// Data callback function
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    float* pOutputF32 = (float*)pOutput;
    static double phase = 0.0;
    double frequency = 440.0; // 440Hz (A4)
    double sampleRate = (double)pDevice->sampleRate;
    
    for (ma_uint32 i = 0; i < frameCount; i++) {
        float sample = (float)sin(phase * 2.0 * 3.14159265359);
        
        // Support both mono and stereo
        for (ma_uint32 c = 0; c < pDevice->playback.channels; c++) {
            pOutputF32[i * pDevice->playback.channels + c] = sample * 0.5f; // Volume 0.5
        }
        
        phase += frequency / sampleRate;
        if (phase > 1.0) phase -= 1.0;
    }
}

int main() {
    std::cout << "=== Audio Playback Test (Sine Wave 440Hz) ===" << std::endl;
    
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32; // Generate in float format
    config.playback.channels = 2;             // Stereo
    config.sampleRate        = 48000;
    config.dataCallback      = data_callback;
    
    ma_device device;
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        std::cerr << "Failed to initialize device" << std::endl;
        return 1;
    }
    
    std::cout << "Device Name: " << device.playback.name << std::endl;
    
    if (ma_device_start(&device) != MA_SUCCESS) {
        std::cerr << "Failed to start device" << std::endl;
        ma_device_uninit(&device);
        return 1;
    }
    
    std::cout << "Playing... (3 seconds)" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    ma_device_uninit(&device);
    std::cout << "Finished" << std::endl;
    
    return 0;
}
