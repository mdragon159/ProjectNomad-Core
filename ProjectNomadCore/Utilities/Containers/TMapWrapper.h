#pragma once

#ifndef WITH_ENGINE // Only use definition if not using Unreal so no conflicts with type names

#include <unordered_map>

/**
* TMapWrapper: Engine-independent direct replacement for TMap.
* Original purpose is to easily replace TMap in UStructs/UClasses so SimLayer can be run independently of Unreal, such
*   as for standalone unit tests.
*
* Note that using same exact name as Unreal type to facilitate easy direct replacement.
*   ...yeah this may get annoying to debug in different environments due to same name, but easier to read than using
*   even more macros everywhere.
**/
template<typename KeyType, typename ValueType>
class TMap {
public:
    unsigned Num() const {
        return mData.size();
    }
    bool IsEmpty() const {
        return mData.empty();
    }

    void Reserve(unsigned size) {
        mData.reserve(size);
    }
    void Add(const KeyType& key, const ValueType& value) {
        mData.insert_or_assign(key, value);
    }
    void Remove(const KeyType& key) {
        mData.erase(key);
    }

    bool Contains(const KeyType& key) const {
        return mData.contains(key);
    }

    // FindChecked assumes the caller checked that the key already exists
    ValueType& FindChecked(const KeyType& key) {
        return mData.at[key];
    }
    const ValueType& FindChecked(const KeyType& key) const {
        return mData.at[key];
    }
    
    ValueType FindRef(const KeyType& key) const {
        if (mData.contains(key)) {
            return mData.at(key);
        }

        // Unreal's TMap::FindRef falls back to default construction if key could not be found
        return ValueType();
    }

    // Not a big fan of the default Find methods but nice to have them implemented just in case anyone decides to use
    //      em in future.
    ValueType* Find(const KeyType& key) {
        if (mData.cntains(key)) {
            return mData.at[key];
        }

        return nullptr;
    }
    const ValueType* Find(const KeyType& key) const {
        if (mData.cntains(key)) {
            return mData.at[key];
        }

        return nullptr;
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
  
  private:
    std::unordered_map<KeyType, ValueType> mData = {};
};

#endif // ifndef WITH_ENGINE