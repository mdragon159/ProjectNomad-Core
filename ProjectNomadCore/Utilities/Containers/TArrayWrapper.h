#pragma once

#ifndef WITH_ENGINE // Only use definition if not using Unreal so no conflicts with type names

#include <vector>

/**
* TArrayWrapper: Engine-independent direct replacement for TArray.
* Original purpose is to easily replace TArray in UStructs/UClasses so SimLayer can be run independently of Unreal, such
*   as for standalone unit tests.
*
* Note that using same exact name as Unreal type to facilitate easy direct replacement.
*   ...yeah this may get annoying to debug in different environments due to same name, but easier to read than using
*   even more macros everywhere.
**/
template<typename ElementType>
class TArray {
public:
    int Num() const { // Returns a signed value just like Unreal TArray (unfortunately)...
        return mData.size();
    }
    bool IsEmpty() const {
        return mData.empty();
    }
    bool IsValidIndex(int index) const {
        return index >= 0 && index < Num();
    }

    void Reserve(unsigned size) {
        mData.reserve(size);
    }
    void Add(const ElementType& element) {
        mData.push_back(element);
    }
    void RemoveAt(unsigned index) {
        mData.erase(mData.begin() + index);
    }

    // Ranged based for loop/for-each support. Also not the greatest use of auto, but eh this is the actual expectation
    auto begin() noexcept {
        return mData.begin();
    }
    auto begin() const noexcept {
        return mData.begin();
    }
    auto end() noexcept {
        return mData.end();
    }
    auto end() const noexcept {
        return mData.end();
    }

    ElementType& operator[](unsigned index) noexcept {
        return mData[index];
    }
    const ElementType& operator[](unsigned index) const noexcept {
        return mData[index];
    }
  
private:
    std::vector<ElementType> mData = {};
};

#endif // ifndef WITH_ENGINE
