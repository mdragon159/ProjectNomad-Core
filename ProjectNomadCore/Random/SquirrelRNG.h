#pragma once

#include <random/random.hpp>

#include "Math/FPVector.h"
#include "Physics/Collider.h"

namespace ProjectNomad {
    /**
    * Supply high quality, deterministic RNG in a quick way with practically no memory overhead based on noise functions.
    *
    * Based the following excellent RNG talk by Squirrel Eiserloh: https://youtu.be/LWFzPP8ZbdU
    * Javascript reference: https://gist.github.com/psema4/bee2614208944f08f5c4640ff582c611
    * C# reference: https://gist.github.com/BwdYeti/f9d019bff90139a336b4137c09faa5e2
    * Ty to BwdYeti for the direction here
    *
    * IDEA: Perhaps make templatized and provide either uint32_t or uint64_t underlying types. But unclear usage, as
    *       current intended usage (games for modern PC + console) are likely going to have default uint 64bit anyways... I think
    **/
    class SquirrelRNG {
    public:
        // This RNG is inherently stateless (which is pretty neat!)
        SquirrelRNG() = delete;
        
        static uint64_t GetRandom(uint64_t seed, uint64_t position) {
            // Approach summarized: Essentially be a very fast hash function with very good spread of values.
            //                      Hash function == a noise function here, and each "seed" is like a unique noise table
            
            uint64_t mangledBits = position;

            mangledBits *= kBitNoise1;
            mangledBits += seed;
            mangledBits ^= (mangledBits >> 8);

            mangledBits += kBitNoise2;
            mangledBits ^= (mangledBits << 8);

            mangledBits *= kBitNoise3;
            mangledBits ^= (mangledBits >> 8);

            return mangledBits;
        }

        static uint64_t GetRandom2D(uint64_t seed, uint64_t posX, uint64_t posY) {
            // Simply create a single well-mangled position from the multiple dimensions
            uint64_t position = posX + kDimensionPrime1 * posY;
            return GetRandom(seed, position);
        }

        static uint64_t GetRandom3D(uint64_t seed, uint64_t posX, uint64_t posY, uint64_t posZ) {
            // Simply create a single well-mangled position from the multiple dimensions
            uint64_t position = posX + kDimensionPrime1 * posY + kDimensionPrime2 * posZ;
            return GetRandom(seed, position);
        }

    private:
        /// Use "good" selection of prime numbers used as solid noise providers
        // Originals: The three original 32bit noises from gist references in top description
        static constexpr uint32_t kBitNoise_Half1 = 0xB5297A4D; // 0b0110_1000_1110_0011_0001_1101_1010_0100
        static constexpr uint32_t kBitNoise_Half2 = 0x68E31DA4; // 0b1011_0101_0010_1001_0111_1010_0100_1101
        static constexpr uint32_t kBitNoise_Half3 = 0x1B56C4E9; // 0b0001_1011_0101_0110_1100_0100_1110_1001
        // New: Create 64bit base binary noise as actually trying to generate 64bit random numbers by default
        //   Other primes are from https://bigprimes.org/ with 20 digits and then manually selecting ones within 64bit unsigned range
        static constexpr uint64_t kBitNoise1 = (static_cast<uint64_t>(kBitNoise_Half2) << 32) + kBitNoise_Half1;
        static constexpr uint64_t kBitNoise2 = 0xBC161CC7AD3D0E67;
        static constexpr uint64_t kBitNoise3 = 0x94D46646B8B17C1D;
        // TODO: Actually extensively validate that these prime numbers result in "good" results
        
        // Nice prime numbers to "space out" additional dimensions before combining into single input.
        // (ie, use with <x, y, z> coordinates to transform y and z so not just adding simple numbers in a more predictable manner)
        //      "The large prime numbers should be orders of magnitude different" - https://gist.github.com/BwdYeti/f9d019bff90139a336b4137c09faa5e2#file-noiserand-cs-L74
        static constexpr uint64_t kDimensionPrime1 = 198491317;
        static constexpr uint64_t kDimensionPrime2 = 6542989;
        static constexpr uint64_t kDimensionPrime3 = 357239; // Another large prime number with distinct and non-boring bits
    };
}
