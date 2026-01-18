#pragma once

#include "math.hpp"
#include <voice_parameters.hpp>

namespace lyrid
{
 
namespace dsp
{
    
template<typename Freq>
struct sine
{
    sine():
        phase_(0.0)
    {}
 
    float sample(const voice_parameters& params)
    {
        double increment = 2 * std::numbers::pi * freq_.sample(params) / sample_rate;
        phase_ += increment;
        return std::sin(phase_);
    }

    Freq freq_;
    double phase_;
};

template<typename Freq>
struct square
{
    square():
        time_(0.0), val_(-1.0f)
    {}
 
    float sample(const voice_parameters& params)
    {
        float f = freq_.sample(params);
        time_ += 1.0 / sample_rate;
        float half_period = 1.0 / (f * 2);
        
        if (time_ >= half_period)
        {
            time_ -= half_period;
            val_ = -val_;
        }
        return val_;
    }

    Freq freq_;
    float time_;
    float val_;
};

template<typename Freq>
struct saw
{
    saw():
        phase_(0.0)
    {}
 
    float sample(const voice_parameters& params)
    {
        float f = freq_.sample(params);
        float increment = f / sample_rate;
        phase_ += increment;
        if (phase_ >= 1.0) 
            phase_ -= 1.0;
        return 2.0 * phase_ - 1.0;
    }

    Freq freq_;
    float phase_;
};

template<typename Freq>
struct triangle
{
    triangle():
        phase_(0.0)
    {}
 
    float sample(const voice_parameters& params)
    {
        float f = freq_.sample(params);
        float increment = f / sample_rate;
        phase_ += increment;
        if (phase_ >= 1.0)
            phase_ -= 1.0;

        if (phase_ < 0.5)
            return 4.0 * phase_ - 1.0;
        else
            return -4.0 * phase_ + 3.0;
    }

    Freq freq_;
    float phase_;
};

class white_noise
{
public:
    white_noise():
        state_(0x853c49e6748fea9bULL)
    {}

    inline float sample(const voice_parameters&)
    {
        return sample();
    }
    
    float sample()
    {
        // Xorshift* (Daniel Lemire / Sebastiano Vigna)
        // Magic constant 0x2545F4914F6CDD1DULL from "Fast Random Integer Generation in an Interval" (PCG paper)
        uint64_t x = state_;
        x ^= x >> 12;
        x ^= x << 25;
        x ^= x >> 27;
        state_ = x;

        // Convert to float in [-1, 1) — extremely high quality uniform distribution
        uint64_t bits = (x * 0x2545F4914F6CDD1DULL) >> 32;
        return static_cast<float>(static_cast<int32_t>(bits)) * (1.0f / 0x80000000);
    }
    
private:
    uint64_t state_;
};

class pink_noise
{
public:
    pink_noise():
        b_{0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}
    {}

    float sample(const voice_parameters&)
    {
        float white = white_gen_.sample();

        b_[0] = b_[0] * 0.99886f + white * 0.0555179f;
        b_[1] = b_[1] * 0.99332f + white * 0.0750759f;
        b_[2] = b_[2] * 0.96900f + white * 0.1538520f;
        b_[3] = b_[3] * 0.86650f + white * 0.3104856f;
        b_[4] = b_[4] * 0.55000f + white * 0.5329522f;
        b_[5] = b_[5] * 0.31000f + white * -0.5329522f;
        b_[6] = b_[6] * 0.11500f + white * -0.0963792f;

        return (arr_sum(b_) + white * 0.5362f) * 0.11f;
    }

private:
    white_noise white_gen_;
    std::array<float, 7> b_;
};

}

}

