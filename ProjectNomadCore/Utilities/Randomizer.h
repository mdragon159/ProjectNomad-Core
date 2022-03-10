#pragma once

#include <random/random.hpp>

#include "Math/FPVector.h"
#include "Physics/Collider.h"

namespace ProjectNomad {
    class Randomizer {
        // Get base random alias which is auto seeded and has static API and internal state
        // Check the solid documentation (readme) for more details 
        using Random = effolkronium::random_static;
        
    public:
        Randomizer() {}

        // TODO: Use a rollback-able seed
        //       Also extensive unit tests to assure consistent behavior with reseeding
        // FUTURE: Check performance of re-seeding -> number gen.
        //          Perhaps use Lehman RNG if performance too heavy... which it likely is
        //          Useful ref: https://youtu.be/ZZY9YE7rZJw?t=871
        
        FPVector getRandomLocation(const Collider& bounds) {
            if (bounds.isBox()) {
                // Get min and max points per axis
                FPVector halfSize = bounds.getBoxHalfSize();
                FPVector max = bounds.getCenter() + halfSize;
                FPVector min = bounds.getCenter() - halfSize;
                
                // Randomize along each axis
                fp x = getRandomNumber(min.x, max.x);
                fp y = getRandomNumber(min.y, max.y);
                fp z = getRandomNumber(min.z, max.z);

                return FPVector(x, y, z);
            }

            // Not yet supporting any collider outside box
            return bounds.getCenter();
        }

        fp getRandomNumber(fp min, fp max) {
            auto rawResult = Random::get(min.raw_value(), max.raw_value());
            return fp::from_raw_value(rawResult);
        }

        uint32_t getRandomNumber(uint32_t min, uint32_t max) {
            return Random::get(min, max);
        }

    };
}
