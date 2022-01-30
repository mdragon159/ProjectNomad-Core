#pragma once

#include <mutex>

namespace ProjectNomad {
    ///<summary>
    /// Header-only singleton pattern from: https://codereview.stackexchange.com/a/147451
    /// Note: Downside/risk here is unique singletons per runtimes (see link above)
    /// This has several repercussions:
    ///     1. Singleton will persist between Unreal Editor runs
    ///     2. Network multiplayer in-editor will share the same singletons, thus breaking functionality
    /// Unfortunately, EOS SDK acts as a singleton (and requires global-scope for callbacks),
    /// thus the above repercussions are in play regardless if we use our own singletons.
    /// Therefore, we will allow usages of Singletons with the following guidelines:
    ///     1. For clarity, append "Singleton" to class name so that instances are ONLY retrieved via Singleton::get()
    ///     2. Always clean up Singleton state when SimLayer is destroyed
    ///     3. Network multiplayer can only be tested with standalone instances 
    ///</summary>
    template <class Type>
    struct Singleton {
    private:
        static Type& instance() {
            // Use static function scope variable to 
            // correctly define lifespan of object.
            static Type instance;
            return instance;
        }

    public:
        static Type& get() {
            // Note the constructor of std::once_flag
            // is a constexpr and is thus done at compile time
            // thus it is immune to multithread construction
            // issues as nothing is done at runtime.
            static std::once_flag flag;

            // Make sure all threads apart from one wait
            // until the first call to instance has completed.
            // This guarantees that the object is fully constructed
            // by a single thread.
            std::call_once(flag, [] { instance(); });

            // Now all threads can go get the instance.
            // as it has been constructed.
            return instance();
        }
    };
}
