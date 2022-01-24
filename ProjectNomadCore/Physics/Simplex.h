#pragma once

#include "Math/FPVector.h"

namespace ProjectNomad {
    // Simplex for GJK + EPA algorithm usage. Based on https://youtu.be/MDusDn8oTSE?t=277 
    class Simplex {
        static constexpr size_t MAX_POINTS = 4; // 4 supports a tetrahedron, which is the most complex simplex we'll need for 3D
        
        // Could use a vector, but logic is pretty simple so can easily use in-place array for quicker access
        std::array<FPVector, MAX_POINTS> points;
        size_t pointsSize = 0;

    public:
        Simplex() {}

        // Assignment operator from arbitrary list for easy Simplex creation from specific points
        Simplex& operator=(std::initializer_list<FPVector> list) {
            // TODO: Why this copy style...? Why std::distance?
            for (auto v = list.begin(); v != list.end(); v++) {
                points[std::distance(list.begin(), v)] = *v;
            }
            pointsSize = list.size();

            return *this;
        }

        // Note that order of points matters in GJK algorithm (or at least with the relevant implementation used)
        void pushFront(FPVector point) {
            points = { point, points[0], points[1], points[2] };
            pointsSize = std::min(pointsSize + 1, MAX_POINTS);
        }

        FPVector& operator[](uint32_t i) {
            return points[i];
        }
        size_t size() const {
            return pointsSize;
        }

        auto begin() const {
            return points.begin();
        }
        auto end() const {
            return points.end() - (MAX_POINTS - pointsSize);
        }
    };
}
