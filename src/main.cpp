#include <iostream>
#include <string>
#include <thread>
#include <chrono>

#include "patch_wrapper.hpp"
#include "device.hpp"
#include "poly_instrument.hpp"

#include "dsp/wave_generators.hpp"
#include "dsp/base_freq.hpp"
#include "dsp/envelope.hpp"
#include "dsp/constant.hpp"
#include "dsp/volume.hpp"
#include "dsp/mix.hpp"
#include "dsp/detune.hpp"
#include "dsp/polynomial.hpp"

using namespace lyrid;
using namespace lyrid::dsp;

int main()
{
    using lfo = sine<constant<7.0f>>;
    using vibrato = linear<lfo, base_freq, constant<5.0f>>;
    patch p = wrap<
        volume
        <
            mix
            < 
                saw<detune<vibrato, constant<-8.0f>>>, 
                saw<detune<vibrato, constant<-5.0f>>>, 
                saw<detune<vibrato, constant<-2.0f>>>, 
                saw<vibrato>, 
                saw<detune<vibrato, constant<1.0f>>>, 
                saw<detune<vibrato, constant<3.0f>>>, 
                saw<detune<vibrato, constant<7.0f>>>, 
                saw<detune<vibrato, constant<9.0f>>>
            >,
            envelope_ar<constant<0.5f>, constant<5.0f>>
        >
    >();
    
    try
    {
        poly_instrument instrument(16, p);
        device dev(instrument);
        
        std::string line;
    
        std::cout << "Press ENTER to start.\n";
        std::getline(std::cin, line);
        
        dev.start();
        
        size_t id = 1;
        
        std::vector<float> freqs{130.813, 164.814, 195.998};
        
        for (int i = 0; i < freqs.size(); ++i, ++id)
        {
            size_t idx = instrument.on(id, freqs[i]);
            std::cout << "Note ON " << id << " at " << idx << "\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(4000));
            
            idx = instrument.off(id);
            std::cout << "Note OFF " << id << " at " << idx << "\n";
        }
            
        std::cout << "ENTER to quit\n";
        std::getline(std::cin, line);
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << e.what() << "\n";
    }
    
    return 0;
}
