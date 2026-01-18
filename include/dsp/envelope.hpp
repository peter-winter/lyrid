#pragma once

#include "constant.hpp"
#include <voice_parameters.hpp>

namespace lyrid
{
 
namespace dsp
{
    
enum class env_stage : uint8_t
{
    off,
    delay,
    attack,
    hold,
    decay,
    sustain,
    release
};

template<
    typename Del,
    typename Att,
    typename Hld,
    typename Dec,
    typename Sus,
    typename Rel
>
class envelope
{
public:
    static constexpr float min_attack_sec  = 0.008f;
    static constexpr float min_release_sec = 0.008f;
    static constexpr float min_decay_sec   = 0.008f;

    envelope()
    {
        del_t_ = 0.0f;
        att_t_ = 0.0f;
        hld_t_ = 0.0f;
        dec_t_ = 0.0f;
        sus_target_ = 1.0f;
        rel_t_ = 0.0f;
        stage_ = env_stage::off;
        sus_at_rel_ = 0.0f;
        time_ = 0.0f;
        active_ = false;
    };
        
    float sample(const voice_parameters& params)
    {
        bool active = (params.state_ == voice_state::active);
        if (!active_ && active)
            capture_parameters(params);
        active_ = active;
    
        float out = 0.0f;
        
        switch (stage_)
        {
            case env_stage::off:
            {
                if (active_)
                {
                    stage_ = del_t_ > 0.0f ? env_stage::delay : env_stage::attack;
                }
                break;
            }

            case env_stage::delay:
            {
                if (time_ >= del_t_)
                {
                    stage_ = env_stage::attack;
                    time_ -= del_t_;
                }
                break;
            }

            case env_stage::attack:
            {
                out = linear_segment(0.0f, 1.0f, att_t_, hld_t_ > 0.0f ? env_stage::hold : env_stage::decay);
                break;
            }

            case env_stage::hold:
            {
                out = 1.0f;
                if (time_ >= hld_t_)
                {
                    stage_ = env_stage::decay;
                    time_ -= hld_t_;
                }
                break;
            }

            case env_stage::decay:
            {
                out = linear_segment(1.0f, sus_target_, dec_t_, env_stage::sustain);
                break;
            }

            case env_stage::sustain:
            {
                time_ = 0.0f;
                out = sus_target_;

                if (!active_)
                {
                    sus_at_rel_ = out;
                    stage_ = env_stage::release;
                }
                break;
            }

            case env_stage::release:
            {
                if (active_)
                {
                    stage_ = (del_t_ > 0.0f ? env_stage::delay : env_stage::attack);
                    break;
                }

                out = linear_segment(sus_at_rel_, 0.0f, rel_t_, env_stage::off);
                break;
            }
        }

        time_ += 1.0f / sample_rate;
        
        if (out < 0.0)
            return 0.0f;
        return out;
    }
    
private:
    float linear_segment(float start, float target, float time_sec, env_stage next_stage)
    {
        if (time_sec <= 0.0f)
        {
            stage_ = next_stage;
            return target;
        }

        if (time_ >= time_sec)
        {
            stage_ = next_stage;
            time_ -= time_sec;
            return target;
        }

        float p = time_ / time_sec;
        return start + (target - start) * p;
    }
    
    void capture_parameters(const voice_parameters& params)
    {
        del_t_ = std::max(0.0f, del_.sample(params));
        att_t_ = std::max(min_attack_sec, att_.sample(params));
        hld_t_ = std::max(0.0f, hld_.sample(params));
        rel_t_ = std::max(min_release_sec, rel_.sample(params));

        sus_target_ = sus_.sample(params);

        dec_t_ = (sus_target_ != 1.0f)
            ? std::max(min_decay_sec, dec_.sample(params))
            : std::max(0.0f, dec_.sample(params));
    }
    
    float del_t_{0.0f};
    float att_t_{0.0f};
    float hld_t_{0.0f};
    float dec_t_{0.0f};
    float sus_target_{1.0f};
    float rel_t_{0.0f};

    env_stage stage_{env_stage::off};
    float sus_at_rel_{0.0f};
    
    float time_{0.0f};
    bool active_{false};
    
    Del del_;
    Att att_;
    Hld hld_;
    Dec dec_;
    Sus sus_;
    Rel rel_;
};

template<typename Att, typename Rel>
using envelope_ar = envelope<constant<0.0f>, Att, constant<0.0f>, constant<0.0f>, constant<1.0f>, Rel>;

}

}
