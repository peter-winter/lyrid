#pragma once

#include <miniaudio.h>

namespace lyrid
{
    
class poly_instrument;

class device
{
public:
    device(poly_instrument& instr);
    ~device();
    
    void start();
    
private:
    static void data_callback(ma_device* device_ptr, void* output_ptr, const void* input_ptr, ma_uint32 frame_count);
    
    ma_device dev_;
    poly_instrument& instr_;
    bool initialized_{false};
};

}
