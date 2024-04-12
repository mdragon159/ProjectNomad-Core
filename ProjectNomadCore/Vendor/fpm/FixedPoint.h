#pragma once

#include <string>
#include <type_traits>
#include "fixed.hpp"

#if WITH_ENGINE // Use following necessary includes if in Unreal context
#include "CoreMinimal.h"
#include "FixedPoint.generated.h"
#else // Otherwise replace Unreal-specific code as appropriate
#include "Utilities/PlatformSupport/UnrealReplacements.h"
#endif

/**
* Port of fpm's fixed.hpp file to an Unreal-supported USTRUCT type.
*
* Needed to do this for two key reasons:
* 1. UStructs apparently don't support template types (and other advanced features)
* 2. Need to use UProperty around the internal data for easy serialization with Unreal stuff
*
* Bonus note: Value of 1 in editor is "65536"
**/
USTRUCT(BlueprintType, meta=(ShowOnlyInnerProperties))
struct THENOMADGAME_API FFixedPoint {
    GENERATED_BODY()

  private:
    UPROPERTY(EditAnywhere)
    int64 m_value = 0; // JMN: Type matches BaseType below
    
    // TODO: Remove or refactor the following template-param-turned-constant. Or at least clean up somewhat...?
    using BaseType = int64;
    using IntermediateType = int64;
    static constexpr bool EnableRounding = true;
    static constexpr unsigned int FractionBits = 16;

    // JMN: Define the base library type to "cheat" and get around some compatibility issues now that not using template types.
    //      Specifically, hardcoded constants like ::pi() and the fpm math library use template params.
    //      So easiest to continue using them via internally supporting conversion between types.
    using fpmMatchingType = fpm::fixed<BaseType, BaseType, FractionBits>;

    static_assert(std::is_integral<BaseType>::value, "BaseType must be an integral type");
    static_assert(FractionBits > 0, "FractionBits must be greater than zero");
    static_assert(FractionBits <= sizeof(BaseType) * 8 - 1,
                  "BaseType must at least be able to contain entire fraction, with space for at least one integral bit")
    ;
    // JMN: Commenting out below static asset as using same intermediate and base type.
    //      Not the safest approach but will work... as long as *very* careful of constraints.
    // static_assert(sizeof(IntermediateType) > sizeof(BaseType), "IntermediateType must be larger than BaseType");
    static_assert(std::is_signed<IntermediateType>::value == std::is_signed<BaseType>::value,
                  "IntermediateType must have same signedness as BaseType");

    // Although this value fits in the BaseType in terms of bits, if there's only one integral bit, this value
    // is incorrect (flips from positive to negative), so we must extend the size to IntermediateType.
    static constexpr IntermediateType FRACTION_MULT = IntermediateType(1) << FractionBits;

    struct raw_construct_tag {};

    constexpr FFixedPoint(BaseType val, raw_construct_tag) noexcept : m_value(val) {}

  public:
    constexpr FFixedPoint() noexcept = default;

    constexpr fpmMatchingType ToLibraryType() const {
        return fpmMatchingType::from_raw_value(raw_value());
    }

    static constexpr FFixedPoint FromLibraryType(fpmMatchingType val) {
        return from_raw_value(val.raw_value());
    }

    // Converts an integral number to the fixed-point type.
    // Like static_cast, this truncates bits that don't fit.
    template <typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
    constexpr inline explicit FFixedPoint(T val) noexcept
        : m_value(static_cast<BaseType>(val * FRACTION_MULT)) {}

    // Converts an floating-point number to the fixed-point type.
    // Like static_cast, this truncates bits that don't fit.
    template <typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
    constexpr inline explicit FFixedPoint(T val) noexcept
        : m_value(static_cast<BaseType>((EnableRounding)
                                            ? (val >= 0.0)
                                                  ? (val * FRACTION_MULT + T{0.5})
                                                  : (val * FRACTION_MULT - T{0.5})
                                            : (val * FRACTION_MULT))) {}

    // Constructs from another fixed-point type with possibly different underlying representation.
    // Like static_cast, this truncates bits that don't fit.
    template <typename B, typename I, unsigned int F, bool R>
    constexpr inline explicit FFixedPoint(fpm::fixed<B,I,F,R> val) noexcept
        : m_value(from_fixed_point<F>(val.raw_value()).raw_value())
    {}
    
    // Explicit conversion to a floating-point type
    template <typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
    constexpr inline explicit operator T() const noexcept {
        return static_cast<T>(m_value) / FRACTION_MULT;
    }

    // Explicit conversion to an integral type
    template <typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
    constexpr inline explicit operator T() const noexcept {
        return static_cast<T>(m_value / FRACTION_MULT);
    }

    // Returns the raw underlying value of this type.
    // Do not use this unless you know what you're doing.
    constexpr inline BaseType raw_value() const noexcept {
        return m_value;
    }

    //! Constructs a fixed-point number from another fixed-point number.
    //! \tparam NumFractionBits the number of bits used by the fraction in \a value.
    //! \param value the integer fixed-point number
    template <unsigned int NumFractionBits, typename T, typename std::enable_if<(NumFractionBits > FractionBits)>::type* = nullptr>
    static constexpr inline FFixedPoint from_fixed_point(T value) noexcept
    {
        fpmMatchingType result = fpmMatchingType::from_fixed_point<NumFractionBits>(value);
        return from_raw_value(result.raw_value());
    }

    template <unsigned int NumFractionBits, typename T, typename std::enable_if<(NumFractionBits <= FractionBits)>::type* = nullptr>
    static constexpr inline FFixedPoint from_fixed_point(T value) noexcept
    {
        fpmMatchingType result = fpmMatchingType::from_fixed_point<NumFractionBits>(value);
        return from_raw_value(result.raw_value());
    }

    // Constructs a fixed-point number from its raw underlying value.
    // Do not use this unless you know what you're doing.
    static constexpr inline FFixedPoint from_raw_value(BaseType value) noexcept {
        return FFixedPoint(value, raw_construct_tag{});
    }

    //
    // Constants
    //
    static constexpr FFixedPoint e() { return from_fixed_point<61>(6267931151224907085ll); }
    static constexpr FFixedPoint pi() { return from_fixed_point<61>(7244019458077122842ll); }
    static constexpr FFixedPoint half_pi() { return from_fixed_point<62>(7244019458077122842ll); }
    static constexpr FFixedPoint two_pi() { return from_fixed_point<60>(7244019458077122842ll); }

    //
    // Arithmetic member operators
    //

    constexpr inline FFixedPoint operator-() const noexcept {
        return FFixedPoint::from_raw_value(-m_value);
    }

    constexpr inline FFixedPoint& operator+=(const FFixedPoint& y) noexcept {
        m_value += y.m_value;
        return *this;
    }

    template <typename I, typename std::enable_if<std::is_integral<I>::value>::type* = nullptr>
    constexpr inline FFixedPoint& operator+=(I y) noexcept {
        m_value += y * FRACTION_MULT;
        return *this;
    }

    constexpr inline FFixedPoint& operator-=(const FFixedPoint& y) noexcept {
        m_value -= y.m_value;
        return *this;
    }

    template <typename I, typename std::enable_if<std::is_integral<I>::value>::type* = nullptr>
    constexpr inline FFixedPoint& operator-=(I y) noexcept {
        m_value -= y * FRACTION_MULT;
        return *this;
    }

    constexpr inline FFixedPoint& operator*=(const FFixedPoint& y) noexcept {
        if (EnableRounding) {
            // Normal fixed-point multiplication is: x * y / 2**FractionBits.
            // To correctly round the last bit in the result, we need one more bit of information.
            // We do this by multiplying by two before dividing and adding the LSB to the real result.
            auto value = (static_cast<IntermediateType>(m_value) * y.m_value) / (FRACTION_MULT / 2);
            m_value = static_cast<BaseType>((value / 2) + (value % 2));
        } else {
            auto value = (static_cast<IntermediateType>(m_value) * y.m_value) / FRACTION_MULT;
            m_value = static_cast<BaseType>(value);
        }
        return *this;
    }

    template <typename I, typename std::enable_if<std::is_integral<I>::value>::type* = nullptr>
    constexpr inline FFixedPoint& operator*=(I y) noexcept {
        m_value *= y;
        return *this;
    }

    constexpr inline FFixedPoint& operator/=(const FFixedPoint& y) noexcept {
        // assert(y.m_value != 0); // JMN: Commented out as no 
        if (EnableRounding) {
            // Normal fixed-point division is: x * 2**FractionBits / y.
            // To correctly round the last bit in the result, we need one more bit of information.
            // We do this by multiplying by two before dividing and adding the LSB to the real result.
            auto value = (static_cast<IntermediateType>(m_value) * FRACTION_MULT * 2) / y.m_value;
            m_value = static_cast<BaseType>((value / 2) + (value % 2));
        } else {
            auto value = (static_cast<IntermediateType>(m_value) * FRACTION_MULT) / y.m_value;
            m_value = static_cast<BaseType>(value);
        }
        return *this;
    }

    template <typename I, typename std::enable_if<std::is_integral<I>::value>::type* = nullptr>
    constexpr inline FFixedPoint& operator/=(I y) noexcept {
        m_value /= y;
        return *this;
    }

    auto operator<=>(const FFixedPoint&) const = default;
    
    // Use an explicit CalculateCRC32 even though there is only a single field as no guarantee that UStruct won't
    //      change padding or add other data members
    void CalculateCRC32(uint32_t& resultThusFar) const;
    
    std::string ToString() const {
        return std::to_string(static_cast<float>(m_value));
    }
};

//
// Addition
//

constexpr inline FFixedPoint operator+(const FFixedPoint& x, const FFixedPoint& y) noexcept
{
    return FFixedPoint(x) += y;
}

template <typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
constexpr inline FFixedPoint operator+(const FFixedPoint& x, T y) noexcept
{
    return FFixedPoint(x) += y;
}

template <typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
constexpr inline FFixedPoint operator+(T x, const FFixedPoint& y) noexcept
{
    return FFixedPoint(y) += x;
}

//
// Subtraction
//

constexpr inline FFixedPoint operator-(const FFixedPoint& x, const FFixedPoint& y) noexcept
{
    return FFixedPoint(x) -= y;
}

template <typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
constexpr inline FFixedPoint operator-(const FFixedPoint& x, T y) noexcept
{
    return FFixedPoint(x) -= y;
}

template <typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
constexpr inline FFixedPoint operator-(T x, const FFixedPoint& y) noexcept
{
    return FFixedPoint(x) -= y;
}

//
// Multiplication
//

constexpr inline FFixedPoint operator*(const FFixedPoint& x, const FFixedPoint& y) noexcept
{
    return FFixedPoint(x) *= y;
}

template <typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
constexpr inline FFixedPoint operator*(const FFixedPoint& x, T y) noexcept
{
    return FFixedPoint(x) *= y;
}

template <typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
constexpr inline FFixedPoint operator*(T x, const FFixedPoint& y) noexcept
{
    return FFixedPoint(y) *= x;
}

//
// Division
//

constexpr inline FFixedPoint operator/(const FFixedPoint& x, const FFixedPoint& y) noexcept
{
    return FFixedPoint(x) /= y;
}

template <typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
constexpr inline FFixedPoint operator/(const FFixedPoint& x, T y) noexcept
{
    return FFixedPoint(x) /= y;
}

template <typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
constexpr inline FFixedPoint operator/(T x, const FFixedPoint& y) noexcept
{
    return FFixedPoint(x) /= y;
}