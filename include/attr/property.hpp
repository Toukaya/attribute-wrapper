//
// Zero-overhead property proxy implementation
// - MSVC: Uses native __declspec(property)
// - GCC/Clang: Uses CRTP + offsetof technique
//
// Features:
// - Zero runtime overhead
// - Compile-time type validation via templates
// - Cross-platform unified syntax
//
// Created by Claude for Touka
//

#ifndef TOUKA_PROPERTY_HPP
#define TOUKA_PROPERTY_HPP

#include <cstddef>
#include <concepts>
#include <type_traits>
#include <utility>

namespace touka {

// ═══════════════════════════════════════════════════════════════════════════
// Part 0: Compile-Time Property Validation (Zero Overhead)
// ═══════════════════════════════════════════════════════════════════════════

// Helper to extract member function pointer traits
template<typename T>
struct MemberFunctionTraits;

// Non-const member function: R (C::*)(Args...)
template<typename R, typename C, typename... Args>
struct MemberFunctionTraits<R (C::*)(Args...)> {
    using return_type = R;
    using class_type = C;
    using args_tuple = std::tuple<Args...>;
    static constexpr bool is_const = false;
    static constexpr std::size_t arity = sizeof...(Args);
};

// Const member function: R (C::*)(Args...) const
template<typename R, typename C, typename... Args>
struct MemberFunctionTraits<R (C::*)(Args...) const> {
    using return_type = R;
    using class_type = C;
    using args_tuple = std::tuple<Args...>;
    static constexpr bool is_const = true;
    static constexpr std::size_t arity = sizeof...(Args);
};

// Noexcept variants
template<typename R, typename C, typename... Args>
struct MemberFunctionTraits<R (C::*)(Args...) noexcept>
    : MemberFunctionTraits<R (C::*)(Args...)> {};

template<typename R, typename C, typename... Args>
struct MemberFunctionTraits<R (C::*)(Args...) const noexcept>
    : MemberFunctionTraits<R (C::*)(Args...) const> {};

// Property Descriptor: Zero-size type carrying compile-time property info
template<typename Owner, typename T, auto Getter, auto Setter>
struct PropertyDescriptor {
    using owner_type = Owner;
    using value_type = T;
    using getter_traits = MemberFunctionTraits<decltype(Getter)>;
    using setter_traits = MemberFunctionTraits<decltype(Setter)>;

    static constexpr auto getter = Getter;
    static constexpr auto setter = Setter;

    // Compile-time validation
    static_assert(std::is_member_function_pointer_v<decltype(Getter)>,
                  "Getter must be a member function pointer");
    static_assert(std::is_member_function_pointer_v<decltype(Setter)>,
                  "Setter must be a member function pointer");
    static_assert(std::is_same_v<typename getter_traits::class_type, Owner>,
                  "Getter must belong to Owner class");
    static_assert(std::is_same_v<typename setter_traits::class_type, Owner>,
                  "Setter must belong to Owner class");
    static_assert(getter_traits::arity == 0,
                  "Getter must take no arguments");
    static_assert(setter_traits::arity == 1,
                  "Setter must take exactly one argument");
    static_assert(std::is_convertible_v<typename getter_traits::return_type, T>,
                  "Getter return type must be convertible to property type");

    // Marker for successful validation
    static constexpr bool is_valid = true;
};

// Read-only property descriptor
template<typename Owner, typename T, auto Getter>
struct PropertyDescriptorRO {
    using owner_type = Owner;
    using value_type = T;
    using getter_traits = MemberFunctionTraits<decltype(Getter)>;

    static constexpr auto getter = Getter;

    static_assert(std::is_member_function_pointer_v<decltype(Getter)>,
                  "Getter must be a member function pointer");
    static_assert(std::is_same_v<typename getter_traits::class_type, Owner>,
                  "Getter must belong to Owner class");
    static_assert(getter_traits::arity == 0,
                  "Getter must take no arguments");
    static_assert(std::is_convertible_v<typename getter_traits::return_type, T>,
                  "Getter return type must be convertible to property type");

    static constexpr bool is_valid = true;
};

// Write-only property descriptor
template<typename Owner, typename T, auto Setter>
struct PropertyDescriptorWO {
    using owner_type = Owner;
    using value_type = T;
    using setter_traits = MemberFunctionTraits<decltype(Setter)>;

    static constexpr auto setter = Setter;

    static_assert(std::is_member_function_pointer_v<decltype(Setter)>,
                  "Setter must be a member function pointer");
    static_assert(std::is_same_v<typename setter_traits::class_type, Owner>,
                  "Setter must belong to Owner class");
    static_assert(setter_traits::arity == 1,
                  "Setter must take exactly one argument");

    static constexpr bool is_valid = true;
};

// Convenience alias templates for property definition
template<typename Owner, typename T, auto Getter, auto Setter>
using Property = PropertyDescriptor<Owner, T, Getter, Setter>;

template<typename Owner, typename T, auto Getter>
using PropertyRO = PropertyDescriptorRO<Owner, T, Getter>;

template<typename Owner, typename T, auto Setter>
using PropertyWO = PropertyDescriptorWO<Owner, T, Setter>;

} // namespace touka

// ═══════════════════════════════════════════════════════════════════════════
// MSVC: Use native __declspec(property) with template validation
// ═══════════════════════════════════════════════════════════════════════════

#if defined(_MSC_VER) && !defined(TOUKA_PROPERTY_NO_NATIVE)

// Read-Write Property (MSVC native) with compile-time validation
#define TOUKA_PROPERTY(OwnerType, PropType, PropName, GetterFunc, SetterFunc)     \
    static_assert(                                                                 \
        ::touka::PropertyDescriptor<OwnerType, PropType, GetterFunc, SetterFunc>::is_valid, \
        "Property validation failed");                                             \
    __declspec(property(get = GetterFunc, put = SetterFunc)) PropType PropName

// Read-Only Property (MSVC native)
#define TOUKA_PROPERTY_RO(OwnerType, PropType, PropName, GetterFunc)              \
    static_assert(                                                                 \
        ::touka::PropertyDescriptorRO<OwnerType, PropType, GetterFunc>::is_valid, \
        "Property validation failed");                                             \
    __declspec(property(get = GetterFunc)) PropType PropName

// Write-Only Property (MSVC native)
#define TOUKA_PROPERTY_WO(OwnerType, PropType, PropName, SetterFunc)              \
    static_assert(                                                                 \
        ::touka::PropertyDescriptorWO<OwnerType, PropType, SetterFunc>::is_valid, \
        "Property validation failed");                                             \
    __declspec(property(put = SetterFunc)) PropType PropName

// Template-based property definition (alternative syntax)
#define TOUKA_PROPERTY_DEF(PropName, DescriptorType)                              \
    static_assert(DescriptorType::is_valid, "Property validation failed");         \
    __declspec(property(get = DescriptorType::getter, put = DescriptorType::setter)) \
        typename DescriptorType::value_type PropName

#define TOUKA_PROPERTY_DEF_RO(PropName, DescriptorType)                           \
    static_assert(DescriptorType::is_valid, "Property validation failed");         \
    __declspec(property(get = DescriptorType::getter))                             \
        typename DescriptorType::value_type PropName

#define TOUKA_PROPERTY_DEF_WO(PropName, DescriptorType)                           \
    static_assert(DescriptorType::is_valid, "Property validation failed");         \
    __declspec(property(put = DescriptorType::setter))                             \
        typename DescriptorType::value_type PropName

#else // GCC / Clang: Use CRTP + offsetof implementation

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

    // Implicit conversion to T (getter) - returns by value for safe usage
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

    // Explicit get - returns decltype(auto) to preserve reference if getter returns one
    decltype(auto) get() const {
        return (owner()->*Getter)();
    }

    // Explicit get returning value (for cases where copy is needed)
    T get_value() const {
        return (owner()->*Getter)();
    }

    void set(const T& value) {
        (owner()->*Setter)(value);
    }

    void set(T&& value) {
        (owner()->*Setter)(std::move(value));
    }

private:
    // Internal helper: invoke getter directly (avoids extra copy in compound ops)
    decltype(auto) invoke_getter() const {
        return (owner()->*Getter)();
    }

public:
    // ─────────────────────────────────────────────────────────────────────
    // Compound Assignment Operators (Arithmetic)
    // Optimized: directly use getter result to avoid unnecessary copy
    // ─────────────────────────────────────────────────────────────────────

    Derived& operator+=(const T& v) requires Addable<T> {
        set(invoke_getter() + v);
        return *self();
    }

    Derived& operator-=(const T& v) requires Subtractable<T> {
        set(invoke_getter() - v);
        return *self();
    }

    Derived& operator*=(const T& v) requires Multipliable<T> {
        set(invoke_getter() * v);
        return *self();
    }

    Derived& operator/=(const T& v) requires Divisible<T> {
        set(invoke_getter() / v);
        return *self();
    }

    Derived& operator%=(const T& v) requires HasModulo<T> {
        set(invoke_getter() % v);
        return *self();
    }

    // ─────────────────────────────────────────────────────────────────────
    // Compound Assignment Operators (Bitwise)
    // ─────────────────────────────────────────────────────────────────────

    Derived& operator&=(const T& v) requires BitwiseAndable<T> {
        set(invoke_getter() & v);
        return *self();
    }

    Derived& operator|=(const T& v) requires BitwiseOrable<T> {
        set(invoke_getter() | v);
        return *self();
    }

    Derived& operator^=(const T& v) requires BitwiseXorable<T> {
        set(invoke_getter() ^ v);
        return *self();
    }

    Derived& operator<<=(const T& v) requires LeftShiftable<T> {
        set(invoke_getter() << v);
        return *self();
    }

    Derived& operator>>=(const T& v) requires RightShiftable<T> {
        set(invoke_getter() >> v);
        return *self();
    }

    // ─────────────────────────────────────────────────────────────────────
    // Increment / Decrement
    // Note: These require a copy since we need to modify the value
    // ─────────────────────────────────────────────────────────────────────

    // Prefix ++
    Derived& operator++() requires Incrementable<T> {
        T val = invoke_getter();
        ++val;
        set(std::move(val));
        return *self();
    }

    // Postfix ++
    T operator++(int) requires Incrementable<T> {
        T old = invoke_getter();
        T val = old;
        ++val;
        set(std::move(val));
        return old;
    }

    // Prefix --
    Derived& operator--() requires Decrementable<T> {
        T val = invoke_getter();
        --val;
        set(std::move(val));
        return *self();
    }

    // Postfix --
    T operator--(int) requires Decrementable<T> {
        T old = invoke_getter();
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
        return invoke_getter() <=> other;
    }

    bool operator==(const T& other) const
        requires std::equality_comparable<T>
    {
        return invoke_getter() == other;
    }

    // ─────────────────────────────────────────────────────────────────────
    // Pointer-like Access
    // ─────────────────────────────────────────────────────────────────────

    // Arrow operator for raw pointers
    T operator->() const requires std::is_pointer_v<T> {
        return invoke_getter();
    }

    // Arrow operator for smart pointers / proxy objects
    auto operator->() const requires (!std::is_pointer_v<T> && HasArrowOperator<T>) {
        return invoke_getter().operator->();
    }

    // Dereference operator
    decltype(auto) operator*() const requires Dereferenceable<T> {
        return *invoke_getter();
    }

    // ─────────────────────────────────────────────────────────────────────
    // Subscript Operator
    // ─────────────────────────────────────────────────────────────────────

    template<typename Index>
    auto operator[](Index&& i) const
        requires Subscriptable<T, Index>
    {
        // Return by value to avoid dangling reference to temporary
        return invoke_getter()[std::forward<Index>(i)];
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

    // Implicit conversion to T (getter) - returns by value for safe usage
    operator T() const {
        return (owner()->*Getter)();
    }

    // Explicit get - returns decltype(auto) to preserve reference if getter returns one
    decltype(auto) get() const {
        return (owner()->*Getter)();
    }

    // Explicit get returning value (for cases where copy is needed)
    T get_value() const {
        return (owner()->*Getter)();
    }

private:
    // Internal helper: invoke getter directly
    decltype(auto) invoke_getter() const {
        return (owner()->*Getter)();
    }

public:
    // Comparison
    auto operator<=>(const T& other) const
        requires std::three_way_comparable<T>
    {
        return invoke_getter() <=> other;
    }

    bool operator==(const T& other) const
        requires std::equality_comparable<T>
    {
        return invoke_getter() == other;
    }

    // Pointer-like access
    T operator->() const requires std::is_pointer_v<T> {
        return invoke_getter();
    }

    auto operator->() const requires (!std::is_pointer_v<T> && HasArrowOperator<T>) {
        return invoke_getter().operator->();
    }

    decltype(auto) operator*() const requires Dereferenceable<T> {
        return *invoke_getter();
    }

    // Subscript
    template<typename Index>
    auto operator[](Index&& i) const
        requires Subscriptable<T, Index>
    {
        // Return by value to avoid dangling reference to temporary
        return invoke_getter()[std::forward<Index>(i)];
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
// Part 6: Property Declaration Macros (GCC/Clang)
// ═══════════════════════════════════════════════════════════════════════════

// Internal macro to generate property struct
#define TOUKA_PROPERTY_IMPL_(OwnerType, PropType, PropName, GetterFunc, SetterFunc) \
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

// Read-Write Property with compile-time validation
#define TOUKA_PROPERTY(OwnerType, PropType, PropName, GetterFunc, SetterFunc)     \
    static_assert(                                                                 \
        ::touka::PropertyDescriptor<OwnerType, PropType, GetterFunc, SetterFunc>::is_valid, \
        "Property '" #PropName "' validation failed");                             \
    TOUKA_PROPERTY_IMPL_(OwnerType, PropType, PropName, GetterFunc, SetterFunc)

// Internal macro for read-only property
#define TOUKA_PROPERTY_RO_IMPL_(OwnerType, PropType, PropName, GetterFunc)        \
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

// Read-Only Property with compile-time validation
#define TOUKA_PROPERTY_RO(OwnerType, PropType, PropName, GetterFunc)              \
    static_assert(                                                                 \
        ::touka::PropertyDescriptorRO<OwnerType, PropType, GetterFunc>::is_valid,  \
        "Property '" #PropName "' validation failed");                             \
    TOUKA_PROPERTY_RO_IMPL_(OwnerType, PropType, PropName, GetterFunc)

// Internal macro for write-only property
#define TOUKA_PROPERTY_WO_IMPL_(OwnerType, PropType, PropName, SetterFunc)        \
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

// Write-Only Property with compile-time validation
#define TOUKA_PROPERTY_WO(OwnerType, PropType, PropName, SetterFunc)              \
    static_assert(                                                                 \
        ::touka::PropertyDescriptorWO<OwnerType, PropType, SetterFunc>::is_valid,  \
        "Property '" #PropName "' validation failed");                             \
    TOUKA_PROPERTY_WO_IMPL_(OwnerType, PropType, PropName, SetterFunc)

// ═══════════════════════════════════════════════════════════════════════════
// Part 7: Template-Based Property Definition (Alternative Syntax)
// ═══════════════════════════════════════════════════════════════════════════

// Define property using a PropertyDescriptor type
#define TOUKA_PROPERTY_DEF(PropName, DescriptorType)                              \
    static_assert(DescriptorType::is_valid, "Property validation failed");         \
    TOUKA_PROPERTY_IMPL_(                                                          \
        typename DescriptorType::owner_type,                                       \
        typename DescriptorType::value_type,                                       \
        PropName,                                                                  \
        DescriptorType::getter,                                                    \
        DescriptorType::setter)

#define TOUKA_PROPERTY_DEF_RO(PropName, DescriptorType)                           \
    static_assert(DescriptorType::is_valid, "Property validation failed");         \
    TOUKA_PROPERTY_RO_IMPL_(                                                       \
        typename DescriptorType::owner_type,                                       \
        typename DescriptorType::value_type,                                       \
        PropName,                                                                  \
        DescriptorType::getter)

#define TOUKA_PROPERTY_DEF_WO(PropName, DescriptorType)                           \
    static_assert(DescriptorType::is_valid, "Property validation failed");         \
    TOUKA_PROPERTY_WO_IMPL_(                                                       \
        typename DescriptorType::owner_type,                                       \
        typename DescriptorType::value_type,                                       \
        PropName,                                                                  \
        DescriptorType::setter)

#endif // !defined(_MSC_VER) || defined(TOUKA_PROPERTY_NO_NATIVE)

#endif // TOUKA_PROPERTY_HPP
