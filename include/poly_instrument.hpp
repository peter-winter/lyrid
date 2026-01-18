#pragma once

#include <vector>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <numeric>

#include "voice_parameters.hpp"
#include "patch.hpp"
#include "dsp/math.hpp"

namespace lyrid
{
    
class poly_instrument
{
public:
    poly_instrument(size_t max_voices, patch p)
        : max_voices_(max_voices), p_(p)
    {
        init();
    }
    
    float sample()
    {
        float sum = 0.0f;
    
        const auto& read_order = order_[read_order_idx_];
        auto& write_order = order_[write_order_idx_];
        size_t write_idx = 0;
        
        for (size_t i = 0; i < audible_count_; ++i)
        {
            size_t slot_idx = read_order[i];
            voice_parameters& params = params_[slot_idx];
            
            float sample = p_.sampler_(params_[slot_idx], get_slot_state_raw_ptr(slot_idx));
            sum += (sample * global_scaling);
            params.smoothed_power_ = alpha * sample * sample + (1 - alpha) * params.smoothed_power_;
            
            if (params.state_ == voice_state::active || params.smoothed_power_ > inaudible_amplitude)
            {
                if (write_idx != 0)
                {
                    size_t prev_slot_idx = write_order[write_idx - 1];
                    float prev_score = slot_score(prev_slot_idx);
                
                    if (prev_score < slot_score(slot_idx))
                    {
                        write_order[write_idx - 1] = slot_idx;
                        write_order[write_idx] = prev_slot_idx;
                    }
                    else
                        write_order[write_idx] = slot_idx;
                }
                else
                    write_order[write_idx] = slot_idx;
                write_idx++;
            }
            else
            {
                free_[free_count_++] = slot_idx;
                params.state_ = voice_state::free;
                p_.dstr_(get_slot_state_raw_ptr(slot_idx));
            }
        }

        audible_count_ = write_idx;
        std::swap(read_order_idx_, write_order_idx_);
        
        return sum;
    }

    size_t on(uint64_t id, float freq)
    {
        size_t idx = allocate_voice();

        auto& params = params_[idx];
        params.base_freq_ = freq;
        params.state_ = voice_state::active;
        params.id_ = id;
        p_.cnstr_(get_slot_state_raw_ptr(idx));
        return idx;
    }

    size_t off(uint64_t id)
    {
        auto& order = order_[read_order_idx_];
        
        for (size_t i = 0; i < audible_count_; ++i)
        {
            size_t idx = order[i];
            auto& params = params_[idx];
            if (params.id_ == id)
            {
                params.state_ = voice_state::releasing;
                return idx;
            }
        }
        return -1;
    }
    
private:
    size_t allocate_voice()
    {
        size_t result;
        auto& order = order_[read_order_idx_];
        if (free_count_ > 0)
        {
            result = free_[--free_count_];
            order[audible_count_++] = result;
        }
        else
            result = order[max_voices_ - 1];
        return result;
    }
    
    void init()
    {
        state_memory_.resize(p_.state_size_ * max_voices_);
        params_.resize(max_voices_);
        
        read_order_idx_ = 0;
        write_order_idx_ = 1;
        audible_count_ = 0;
        free_count_ = max_voices_;
        
        order_[0].resize(max_voices_);
        order_[1].resize(max_voices_);
        free_.resize(max_voices_);
        
        std::iota(free_.begin(), free_.end(), 0);
    }

    float slot_score(size_t slot_idx) const
    {
        return static_cast<float>(params_[slot_idx].state_ == voice_state::active) * 1000.0 + params_[slot_idx].smoothed_power_;
    }

    void* get_slot_state_raw_ptr(size_t slot_idx)
    {
        return static_cast<void*>(state_memory_.data() + slot_idx * p_.state_size_);
    }
    
    size_t max_voices_;
    patch p_;
    
    std::vector<size_t> order_[2];
    size_t read_order_idx_;
    size_t write_order_idx_;
    size_t audible_count_;
    size_t free_count_;
    std::vector<size_t> free_;
    std::vector<unsigned char> state_memory_;
    std::vector<voice_parameters> params_;
    
    constexpr static float inaudible_amplitude = 1.0e-7;
    constexpr static float alpha = 0.01f;
    constexpr static float global_scaling = 0.2f;
};

}
