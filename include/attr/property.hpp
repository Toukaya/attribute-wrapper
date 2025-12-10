//
// Zero-overhead property proxy implementation
// - MSVC: Uses native __declspec(property)
// - GCC/Clang: Uses CRTP + offsetof technique
//
// Created by Claude for Touka
//

#ifndef TOUKA_PROPERTY_HPP
#define TOUKA_PROPERTY_HPP

// ═══════════════════════════════════════════════════════════════════════════
// MSVC: Use native __declspec(property)
// ═══════════════════════════════════════════════════════════════════════════

#if defined(_MSC_VER) && !defined(TOUKA_PROPERTY_NO_NATIVE)

// Read-Write Property (MSVC native)
#define TOUKA_PROPERTY(OwnerType, PropType, PropName, GetterFunc, SetterFunc)     \
    __declspec(property(get = GetterFunc, put = SetterFunc)) PropType PropName

// Read-Only Property (MSVC native)
#define TOUKA_PROPERTY_RO(OwnerType, PropType, PropName, GetterFunc)              \
    __declspec(property(get = GetterFunc)) PropType PropName

// Write-Only Property (MSVC native)
#define TOUKA_PROPERTY_WO(OwnerType, PropType, PropName, SetterFunc)              \
    __declspec(property(put = SetterFunc)) PropType PropName

#else // GCC / Clang: Use CRTP + offsetof implementation

#include <cstddef>
#include <concepts>
#include <type_traits>
#include <utility>
#include <compare>

namespace touka {

// ═══════════════════════════════════════════════════════════════════════════
// Part 1: Compiler Compatibility Macros
// ═══════════════════════════════════════════════════════════════════════════

#define TOUKA_NO_UNIQUE_ADDRESS [[no_unique_address]]

// Suppress offsetof warnings for non-standard-layout types
#if defined(__clang__)
    #define TOUKA_OFFSETOF_BEGIN \
        _Pragma("clang diagnostic push") \
        _Pragma("clang diagnostic ignored \"-Winvalid-offsetof\"")
    #define TOUKA_OFFSETOF_END \
        _Pragma("clang diagnostic pop")
#elif defined(__GNUC__)
    #define TOUKA_OFFSETOF_BEGIN \
        _Pragma("GCC diagnostic push") \
        _Pragma("GCC diagnostic ignored \"-Winvalid-offsetof\"")
    #define TOUKA_OFFSETOF_END \
        _Pragma("GCC diagnostic pop")
#else
    #define TOUKA_OFFSETOF_BEGIN
    #define TOUKA_OFFSETOF_END
#endif

// ═══════════════════════════════════════════════════════════════════════════
// Part 2: Concept Constraints
// ═══════════════════════════════════════════════════════════════════════════

template<typename Owner, typename T, auto Getter, auto Setter>
concept PropertyAccessors = requires(Owner& o, const Owner& co, T v) {
    { (co.*Getter)() } -> std::convertible_to<T>;
    { (o.*Setter)(v) };
};

template<typename Owner, typename T, auto Getter>
concept PropertyGetter = requires(const Owner& co) {
    { (co.*Getter)() } -> std::convertible_to<T>;
};

template<typename Owner, typename T, auto Setter>
concept PropertySetter = requires(Owner& o, T v) {
    { (o.*Setter)(v) };
};

// Helper concepts for conditional operator enablement
template<typename T>
concept Incrementable = requires(T v) { ++v; };

template<typename T>
concept Decrementable = requires(T v) { --v; };

template<typename T>
concept Addable = requires(T a, T b) { a + b; };

template<typename T>
concept Subtractable = requires(T a, T b) { a - b; };

template<typename T>
concept Multipliable = requires(T a, T b) { a * b; };

template<typename T>
concept Divisible = requires(T a, T b) { a / b; };

template<typename T>
concept HasModulo = requires(T a, T b) { a % b; };

template<typename T>
concept BitwiseAndable = requires(T a, T b) { a & b; };

template<typename T>
concept BitwiseOrable = requires(T a, T b) { a | b; };

template<typename T>
concept BitwiseXorable = requires(T a, T b) { a ^ b; };

template<typename T>
concept LeftShiftable = requires(T a, T b) { a << b; };

template<typename T>
concept RightShiftable = requires(T a, T b) { a >> b; };

template<typename T>
concept Dereferenceable = std::is_pointer_v<T> || requires(T v) { *v; };

template<typename T>
concept HasArrowOperator = std::is_pointer_v<T> || requires(T v) { v.operator->(); };

template<typename T, typename Index>
concept Subscriptable = requires(T v, Index i) { v[i]; };

// ═══════════════════════════════════════════════════════════════════════════
// Part 3: PropertyBase - CRTP Base Class for Read-Write Properties
// ═══════════════════════════════════════════════════════════════════════════

template<typename Derived, typename Owner, typename T, auto Getter, auto Setter>
    requires PropertyAccessors<Owner, T, Getter, Setter>
class PropertyBase {
protected:
    Derived* self() noexcept {
        return static_cast<Derived*>(this);
    }

    const Derived* self() const noexcept {
        return static_cast<const Derived*>(this);
    }

    Owner* owner() noexcept {
        return self()->GetOwner();
    }

    const Owner* owner() const noexcept {
        return self()->GetOwner();
    }

public:
    using value_type = T;
    using owner_type = Owner;

    // Default constructor (required for aggregate member)
    PropertyBase() = default;

    // Prevent standalone copying (would break offsetof calculation)
    PropertyBase(const PropertyBase&) = delete;
    PropertyBase(PropertyBase&&) = delete;

    // Note: operator= for PropertyBase itself is deleted,
    // but operator=(const T&) is defined below for value assignment

    // ─────────────────────────────────────────────────────────────────────
    // Core Operations
    // ─────────────────────────────────────────────────────────────────────

    // Implicit conversion to T (getter)
    operator T() const {
        return (owner()->*Getter)();
    }

    // Value assignment (setter) - lvalue
    Derived& operator=(const T& value) {
        (owner()->*Setter)(value);
        return *self();
    }

    // Value assignment (setter) - rvalue
    Derived& operator=(T&& value) {
        (owner()->*Setter)(std::move(value));
        return *self();
    }

    // Explicit get/set
    T get() const {
        return (owner()->*Getter)();
    }

    void set(const T& value) {
        (owner()->*Setter)(value);
    }

    void set(T&& value) {
        (owner()->*Setter)(std::move(value));
    }

    // ─────────────────────────────────────────────────────────────────────
    // Compound Assignment Operators (Arithmetic)
    // ─────────────────────────────────────────────────────────────────────

    Derived& operator+=(const T& v) requires Addable<T> {
        set(get() + v);
        return *self();
    }

    Derived& operator-=(const T& v) requires Subtractable<T> {
        set(get() - v);
        return *self();
    }

    Derived& operator*=(const T& v) requires Multipliable<T> {
        set(get() * v);
        return *self();
    }

    Derived& operator/=(const T& v) requires Divisible<T> {
        set(get() / v);
        return *self();
    }

    Derived& operator%=(const T& v) requires HasModulo<T> {
        set(get() % v);
        return *self();
    }

    // ─────────────────────────────────────────────────────────────────────
    // Compound Assignment Operators (Bitwise)
    // ─────────────────────────────────────────────────────────────────────

    Derived& operator&=(const T& v) requires BitwiseAndable<T> {
        set(get() & v);
        return *self();
    }

    Derived& operator|=(const T& v) requires BitwiseOrable<T> {
        set(get() | v);
        return *self();
    }

    Derived& operator^=(const T& v) requires BitwiseXorable<T> {
        set(get() ^ v);
        return *self();
    }

    Derived& operator<<=(const T& v) requires LeftShiftable<T> {
        set(get() << v);
        return *self();
    }

    Derived& operator>>=(const T& v) requires RightShiftable<T> {
        set(get() >> v);
        return *self();
    }

    // ─────────────────────────────────────────────────────────────────────
    // Increment / Decrement
    // ─────────────────────────────────────────────────────────────────────

    // Prefix ++
    Derived& operator++() requires Incrementable<T> {
        T val = get();
        ++val;
        set(std::move(val));
        return *self();
    }

    // Postfix ++
    T operator++(int) requires Incrementable<T> {
        T old = get();
        T val = old;
        ++val;
        set(std::move(val));
        return old;
    }

    // Prefix --
    Derived& operator--() requires Decrementable<T> {
        T val = get();
        --val;
        set(std::move(val));
        return *self();
    }

    // Postfix --
    T operator--(int) requires Decrementable<T> {
        T old = get();
        T val = old;
        --val;
        set(std::move(val));
        return old;
    }

    // ─────────────────────────────────────────────────────────────────────
    // Comparison Operators
    // ─────────────────────────────────────────────────────────────────────

    auto operator<=>(const T& other) const
        requires std::three_way_comparable<T>
    {
        return get() <=> other;
    }

    bool operator==(const T& other) const
        requires std::equality_comparable<T>
    {
        return get() == other;
    }

    // ─────────────────────────────────────────────────────────────────────
    // Pointer-like Access
    // ─────────────────────────────────────────────────────────────────────

    // Arrow operator for raw pointers
    T operator->() const requires std::is_pointer_v<T> {
        return get();
    }

    // Arrow operator for smart pointers / proxy objects
    auto operator->() const requires (!std::is_pointer_v<T> && HasArrowOperator<T>) {
        return get().operator->();
    }

    // Dereference operator
    decltype(auto) operator*() const requires Dereferenceable<T> {
        return *get();
    }

    // ─────────────────────────────────────────────────────────────────────
    // Subscript Operator
    // ─────────────────────────────────────────────────────────────────────

    template<typename Index>
    auto operator[](Index&& i) const
        requires Subscriptable<T, Index>
    {
        // Return by value to avoid dangling reference to temporary
        return get()[std::forward<Index>(i)];
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// Part 4: PropertyBaseReadOnly - CRTP Base Class for Read-Only Properties
// ═══════════════════════════════════════════════════════════════════════════

template<typename Derived, typename Owner, typename T, auto Getter>
    requires PropertyGetter<Owner, T, Getter>
class PropertyBaseReadOnly {
protected:
    Derived* self() noexcept {
        return static_cast<Derived*>(this);
    }

    const Derived* self() const noexcept {
        return static_cast<const Derived*>(this);
    }

    const Owner* owner() const noexcept {
        return self()->GetOwner();
    }

public:
    using value_type = T;
    using owner_type = Owner;

    PropertyBaseReadOnly() = default;
    PropertyBaseReadOnly(const PropertyBaseReadOnly&) = delete;
    PropertyBaseReadOnly(PropertyBaseReadOnly&&) = delete;
    PropertyBaseReadOnly& operator=(const PropertyBaseReadOnly&) = delete;
    PropertyBaseReadOnly& operator=(PropertyBaseReadOnly&&) = delete;

    // Implicit conversion to T (getter)
    operator T() const {
        return (owner()->*Getter)();
    }

    // Explicit get
    T get() const {
        return (owner()->*Getter)();
    }

    // Comparison
    auto operator<=>(const T& other) const
        requires std::three_way_comparable<T>
    {
        return get() <=> other;
    }

    bool operator==(const T& other) const
        requires std::equality_comparable<T>
    {
        return get() == other;
    }

    // Pointer-like access
    T operator->() const requires std::is_pointer_v<T> {
        return get();
    }

    auto operator->() const requires (!std::is_pointer_v<T> && HasArrowOperator<T>) {
        return get().operator->();
    }

    decltype(auto) operator*() const requires Dereferenceable<T> {
        return *get();
    }

    // Subscript
    template<typename Index>
    auto operator[](Index&& i) const
        requires Subscriptable<T, Index>
    {
        // Return by value to avoid dangling reference to temporary
        return get()[std::forward<Index>(i)];
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// Part 5: PropertyBaseWriteOnly - CRTP Base Class for Write-Only Properties
// ═══════════════════════════════════════════════════════════════════════════

template<typename Derived, typename Owner, typename T, auto Setter>
    requires PropertySetter<Owner, T, Setter>
class PropertyBaseWriteOnly {
protected:
    Derived* self() noexcept {
        return static_cast<Derived*>(this);
    }

    Owner* owner() noexcept {
        return self()->GetOwner();
    }

public:
    using value_type = T;
    using owner_type = Owner;

    PropertyBaseWriteOnly() = default;
    PropertyBaseWriteOnly(const PropertyBaseWriteOnly&) = delete;
    PropertyBaseWriteOnly(PropertyBaseWriteOnly&&) = delete;
    PropertyBaseWriteOnly& operator=(const PropertyBaseWriteOnly&) = delete;

    // Value assignment (setter)
    Derived& operator=(const T& value) {
        (owner()->*Setter)(value);
        return *self();
    }

    Derived& operator=(T&& value) {
        (owner()->*Setter)(std::move(value));
        return *self();
    }

    // Explicit set
    void set(const T& value) {
        (owner()->*Setter)(value);
    }

    void set(T&& value) {
        (owner()->*Setter)(std::move(value));
    }
};

} // namespace touka

// ═══════════════════════════════════════════════════════════════════════════
// Part 6: Property Declaration Macros
// ═══════════════════════════════════════════════════════════════════════════

// Read-Write Property
#define TOUKA_PROPERTY(OwnerType, PropType, PropName, GetterFunc, SetterFunc)     \
    struct PropName##_property_t final                                             \
        : public ::touka::PropertyBase<                                            \
              PropName##_property_t,                                               \
              OwnerType,                                                           \
              PropType,                                                            \
              GetterFunc,                                                          \
              SetterFunc>                                                          \
    {                                                                              \
        using Base = ::touka::PropertyBase<                                        \
            PropName##_property_t, OwnerType, PropType, GetterFunc, SetterFunc>;   \
        using Base::operator=;                                                     \
                                                                                   \
        OwnerType* GetOwner() noexcept {                                           \
            TOUKA_OFFSETOF_BEGIN                                                   \
            constexpr std::size_t offset = offsetof(OwnerType, PropName);          \
            TOUKA_OFFSETOF_END                                                     \
            return reinterpret_cast<OwnerType*>(                                   \
                reinterpret_cast<char*>(this) - offset);                           \
        }                                                                          \
                                                                                   \
        const OwnerType* GetOwner() const noexcept {                               \
            TOUKA_OFFSETOF_BEGIN                                                   \
            constexpr std::size_t offset = offsetof(OwnerType, PropName);          \
            TOUKA_OFFSETOF_END                                                     \
            return reinterpret_cast<const OwnerType*>(                             \
                reinterpret_cast<const char*>(this) - offset);                     \
        }                                                                          \
    };                                                                             \
    TOUKA_NO_UNIQUE_ADDRESS PropName##_property_t PropName

// Read-Only Property
#define TOUKA_PROPERTY_RO(OwnerType, PropType, PropName, GetterFunc)              \
    struct PropName##_property_t final                                             \
        : public ::touka::PropertyBaseReadOnly<                                    \
              PropName##_property_t,                                               \
              OwnerType,                                                           \
              PropType,                                                            \
              GetterFunc>                                                          \
    {                                                                              \
        const OwnerType* GetOwner() const noexcept {                               \
            TOUKA_OFFSETOF_BEGIN                                                   \
            constexpr std::size_t offset = offsetof(OwnerType, PropName);          \
            TOUKA_OFFSETOF_END                                                     \
            return reinterpret_cast<const OwnerType*>(                             \
                reinterpret_cast<const char*>(this) - offset);                     \
        }                                                                          \
    };                                                                             \
    TOUKA_NO_UNIQUE_ADDRESS PropName##_property_t PropName

// Write-Only Property
#define TOUKA_PROPERTY_WO(OwnerType, PropType, PropName, SetterFunc)              \
    struct PropName##_property_t final                                             \
        : public ::touka::PropertyBaseWriteOnly<                                   \
              PropName##_property_t,                                               \
              OwnerType,                                                           \
              PropType,                                                            \
              SetterFunc>                                                          \
    {                                                                              \
        using Base = ::touka::PropertyBaseWriteOnly<                               \
            PropName##_property_t, OwnerType, PropType, SetterFunc>;               \
        using Base::operator=;                                                     \
                                                                                   \
        OwnerType* GetOwner() noexcept {                                           \
            TOUKA_OFFSETOF_BEGIN                                                   \
            constexpr std::size_t offset = offsetof(OwnerType, PropName);          \
            TOUKA_OFFSETOF_END                                                     \
            return reinterpret_cast<OwnerType*>(                                   \
                reinterpret_cast<char*>(this) - offset);                           \
        }                                                                          \
    };                                                                             \
    TOUKA_NO_UNIQUE_ADDRESS PropName##_property_t PropName

#endif // !defined(_MSC_VER) || defined(TOUKA_PROPERTY_NO_NATIVE)

#endif // TOUKA_PROPERTY_HPP
