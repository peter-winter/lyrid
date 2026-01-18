#include "device.hpp"
#include "global_constants.hpp"
#include "poly_instrument.hpp"

#include <stdexcept>

namespace lyrid
{
    device::device(poly_instrument& instr):
        instr_(instr)
    {
        ma_device_config config = ma_device_config_init(ma_device_type_playback);
        config.playback.format = ma_format_f32;
        config.playback.channels = 2;
        config.sampleRate = sample_rate;
        config.dataCallback = data_callback;
        config.pUserData = this;
        
        if (ma_device_init(nullptr, &config, &dev_) != MA_SUCCESS)
            throw std::runtime_error("Failed to initialize audio device");
            
        initialized_ = true;
    }

    void device::data_callback(ma_device* device_ptr, void* output_ptr, const void*, ma_uint32 frame_count)
    {
        device* dev_ptr = static_cast<device*>(device_ptr->pUserData);
        
        float* output = static_cast<float*>(output_ptr);
        for (ma_uint32 i = 0; i < frame_count; ++i)
        {
            float sample = dev_ptr->instr_.sample();
            output[i * 2 + 0] = sample;
            output[i * 2 + 1] = sample;
        }
    }

    void device::start()
    {
        if (ma_device_start(&dev_) != MA_SUCCESS)
            throw std::runtime_error("Failed to start audio device");
    }
    
    device::~device()
    {
        if (initialized_)
            ma_device_uninit(&dev_);
    }
}
