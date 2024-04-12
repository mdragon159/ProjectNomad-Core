#pragma once

#include "SquirrelRNG.h"
#include "Math/FVectorFP.h"
#include "Physics/Model/FCollider.h"

namespace ProjectNomad {
    /**
    * Provides a stateful pseudo-random generator where each random call will result in a new value even with the same
    * inputs.
    *
    * ie, this provides two distinct functions:
    * 1. Neat wrapper methods around a deterministic, high-quality RNG source
    * 2. Automatically moves internal state "forward" so subsequent calls result in new values
    *
    * Reference by BwdYeti for Next() methods: https://gist.github.com/BwdYeti/f9d019bff90139a336b4137c09faa5e2#file-noiserand-cs-L158
    **/
    class IncrementalRandomizer {
    public:
        IncrementalRandomizer() {}
        IncrementalRandomizer(uint64_t inSeed) : mSeed(inSeed) {}
        IncrementalRandomizer(uint64_t inSeed, uint64_t inPosition) : mSeed(inSeed), mPosition(inPosition) {}

        uint64_t GetSeed() const {
            return mSeed;
        }
        uint64_t GetPosition() const {
            return mPosition;
        }

        void SetSeed(uint64_t newSeed) {
            mSeed = newSeed;
        }
        void SetPosition(uint64_t newPosition) {
            mPosition = newPosition;
        }

        
        
        uint32_t GetRandom32(uint32_t min, uint32_t max) {
            uint64_t result = GetNextRandom(min, max);
            return static_cast<uint32_t>(result); // Static cast to quiet warnings
        }
        uint64_t GetRandom64(uint64_t min, uint64_t max) {
            return GetNextRandom(min, max);
        }
        fp GetRandomFp(fp min, fp max) {
            uint64_t rawResult = GetNextRandom(min.raw_value(), max.raw_value());
            return fp::from_raw_value(rawResult);
        }
        
        FVectorFP GetRandomLocation(const FCollider& bounds) {
            if (bounds.IsBox()) {
                // Get min and max points per axis
                FVectorFP halfSize = bounds.GetBoxHalfSize();
                FVectorFP max = bounds.GetCenter() + halfSize;
                FVectorFP min = bounds.GetCenter() - halfSize;
                
                // Randomize along each axis
                fp x = GetRandomFp(min.x, max.x);
                fp y = GetRandomFp(min.y, max.y);
                fp z = GetRandomFp(min.z, max.z);

                return FVectorFP(x, y, z);
            }

            // Not yet supporting any collider outside box
            return bounds.GetCenter();
        }
        
    private:
        uint64_t GetNextRandom() {
            uint64_t result = SquirrelRNG::GetRandom(mSeed, mPosition);
            mPosition++;
            return result;
        }

        uint64_t GetNextRandom(uint64_t maxValue) {
            // Note: There are alternate methods which deal with the slight statistical issues of modulus, but we don't
            //          need em. Modulus is fast and good enough for our needs
            
            return GetNextRandom() % maxValue;
        }

        uint64_t GetNextRandom(uint64_t minValue, uint64_t maxValue) {
            // *Could* check that min is actually less than max, but exceptions are turned off in our Unreal games
            //      and passing in a logger is pretty inelegant.
            // Just going to put onus of assuring inputs make sense on the caller

            // Simple shift approach to reuse the underlying maxValue-only implementation
            uint64_t range = maxValue - minValue;
            uint64_t result = GetNextRandom(range);
            return result + minValue;
        }
        
        uint64_t mSeed = 0;
        uint64_t mPosition = 0;
    };
}
