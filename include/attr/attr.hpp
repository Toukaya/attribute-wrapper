// property.hpp
#pragma once

#include <type_traits>
#include <utility>
#include <stdexcept>

// 属性修饰符
#define VIRTUAL virtual
#define OVERRIDE override

// 类型安全检查的辅助模板
namespace attr_detail {
    template<typename T>
    struct PropertyTypeCheck {
        static_assert(std::is_default_constructible_v<T>, "Property type must be default constructible");
        static_assert(!std::is_array_v<T>, "Array types are not supported as properties");
    };

    template<typename T, typename U>
    concept PropertyAssignable = requires(T& t, U&& u) {
        { t = std::forward<U>(u) } -> std::same_as<T&>;
    };

    // 初始化状态追踪
    template<typename T>
    class InitState {
        bool initialized = false;
        bool init_only = false;
    public:
        void markInitialized() { initialized = true; }
        bool isInitialized() const { return initialized; }
        void setInitOnly() { init_only = true; }
        bool isInitOnly() const { return init_only; }
    };
}

// 基本属性
#define PROPERTY(type, name, ...) \
private: \
    [[maybe_unused]] attr_detail::PropertyTypeCheck<type> _##name##_type_check{}; \
    type m_##name{} __VA_ARGS__; \
public: \
    class Property_##name { \
        friend class TestClass; \
        type& value; \
    public: \
        Property_##name(type& v) : value(v) {} \
        operator type() const noexcept { return value; } \
        Property_##name& operator=(const type& v) { value = v; return *this; } \
        template<typename U> \
        Property_##name& operator=(U&& v) \
            requires attr_detail::PropertyAssignable<type, U> \
        { value = std::forward<U>(v); return *this; } \
    }; \
    Property_##name name() noexcept { return Property_##name(m_##name); } \
    const type& name() const noexcept { return m_##name; } \
    TestClass& name(const type& value) { m_##name = value; return *this; } \
    template<typename U> \
    TestClass& name(U&& value) \
        requires attr_detail::PropertyAssignable<type, U> \
    { m_##name = std::forward<U>(value); return *this; }

// 只读属性
#define READONLY_PROPERTY(type, name) \
private: \
    [[maybe_unused]] attr_detail::PropertyTypeCheck<type> _##name##_type_check{}; \
    type m_##name{}; \
public: \
    class ReadOnlyProperty_##name { \
        friend class TestClass; \
        const type& value; \
    public: \
        ReadOnlyProperty_##name(const type& v) : value(v) {} \
        operator const type&() const noexcept { return value; } \
        const type* operator->() const noexcept { return &value; } \
        template<typename U> \
        bool operator==(const U& other) const { return value == other; } \
        bool empty() const { return value.empty(); } \
    }; \
    ReadOnlyProperty_##name name() const noexcept { return ReadOnlyProperty_##name(m_##name); }

// 带初始化检查的必需属性
#define REQUIRED_PROPERTY(type, name) \
private: \
    [[maybe_unused]] attr_detail::PropertyTypeCheck<type> _##name##_type_check{}; \
    type m_##name{}; \
    attr_detail::InitState<type> _##name##_init{}; \
public: \
    class RequiredProperty_##name { \
        friend class TestClass; \
        type& value; \
        attr_detail::InitState<type>& init_state; \
    public: \
        RequiredProperty_##name(type& v, attr_detail::InitState<type>& is) \
            : value(v), init_state(is) {} \
        operator type() const { \
            if (!init_state.isInitialized()) \
                throw std::runtime_error("Required property '" #name "' not initialized"); \
            return value; \
        } \
        template<typename U> \
        RequiredProperty_##name& operator=(U&& v) \
            requires attr_detail::PropertyAssignable<type, U> \
        { \
            value = std::forward<U>(v); \
            init_state.markInitialized(); \
            return *this; \
        } \
    }; \
    RequiredProperty_##name name() noexcept { return RequiredProperty_##name(m_##name, _##name##_init); } \
    const type& name() const { \
        if (!_##name##_init.isInitialized()) \
            throw std::runtime_error("Required property '" #name "' not initialized"); \
        return m_##name; \
    } \
    TestClass& name(const type& value) { \
        m_##name = value; \
        _##name##_init.markInitialized(); \
        return *this; \
    } \
    template<typename U> \
    TestClass& name(U&& value) \
        requires attr_detail::PropertyAssignable<type, U> \
    { \
        m_##name = std::forward<U>(value); \
        _##name##_init.markInitialized(); \
        return *this; \
    }

// 带逻辑的属性
#define PROPERTY_EX(type, name, get_logic, set_logic) \
private: \
    [[maybe_unused]] attr_detail::PropertyTypeCheck<type> _##name##_type_check{}; \
    type m_##name{}; \
public: \
    class PropertyEx_##name { \
        friend class TestClass; \
        TestClass* obj; \
    public: \
        PropertyEx_##name(TestClass* o) : obj(o) {} \
        operator type() const { return obj->m_##name; } \
        template<typename U> \
        PropertyEx_##name& operator=(U&& value) \
            requires attr_detail::PropertyAssignable<type, U> \
        { \
            if (value < 0) throw std::invalid_argument("Value must be non-negative"); \
            obj->m_##name = std::forward<U>(value); \
            return *this; \
        } \
    }; \
    PropertyEx_##name name() noexcept { return PropertyEx_##name(this); } \
    type name() const { return m_##name; } \
    TestClass& name(const type& value) { \
        if (value < 0) throw std::invalid_argument("Value must be non-negative"); \
        m_##name = value; \
        return *this; \
    } \
    template<typename U> \
    TestClass& name(U&& value) \
        requires attr_detail::PropertyAssignable<type, U> \
    { \
        if (value < 0) throw std::invalid_argument("Value must be non-negative"); \
        m_##name = std::forward<U>(value); \
        return *this; \
    }

// 属性实现
#define PROPERTY_IMPL(type, name, get_expr, set_expr, ...) \
private: \
    [[maybe_unused]] attr_detail::PropertyTypeCheck<type> _##name##_type_check{}; \
    type m_##name{} __VA_ARGS__; \
public: \
    inline const type& name() const noexcept { get_expr return m_##name; } \
    template<typename T> \
    inline TestClass& name(T&& value) \
        requires attr_detail::PropertyAssignable<type, T> \
    { set_expr m_##name = std::forward<T>(value); return *this; }

// 表达式主体属性
#define EXPRESSION_PROPERTY(type, name, expr) \
private: \
    [[maybe_unused]] attr_detail::PropertyTypeCheck<type> _##name##_type_check{}; \
public: \
    class ExpressionProperty_##name { \
        friend class TestClass; \
        const TestClass* obj; \
    public: \
        ExpressionProperty_##name(const TestClass* o) : obj(o) {} \
        operator type() const noexcept { return (obj->expr); } \
    }; \
    ExpressionProperty_##name name() const noexcept { return ExpressionProperty_##name(this); }

// init-only属性
#define INIT_ONLY_PROPERTY(type, name) \
private: \
    [[maybe_unused]] attr_detail::PropertyTypeCheck<type> _##name##_type_check{}; \
    type m_##name{}; \
    attr_detail::InitState<type> _##name##_init{}; \
public: \
    class InitOnlyProperty_##name { \
        friend class TestClass; \
        type& value; \
        attr_detail::InitState<type>& init_state; \
    public: \
        InitOnlyProperty_##name(type& v, attr_detail::InitState<type>& is) \
            : value(v), init_state(is) {} \
        operator type() const noexcept { return value; } \
        template<typename U> \
        InitOnlyProperty_##name& operator=(U&& v) \
            requires attr_detail::PropertyAssignable<type, U> \
        { \
            if (init_state.isInitialized()) \
                throw std::runtime_error("Cannot modify init-only property '" #name "'"); \
            value = std::forward<U>(v); \
            init_state.markInitialized(); \
            init_state.setInitOnly(); \
            return *this; \
        } \
    }; \
    InitOnlyProperty_##name name() noexcept { return InitOnlyProperty_##name(m_##name, _##name##_init); } \
    const type& name() const noexcept { return m_##name; } \
    TestClass& name(const type& value) { \
        if (_##name##_init.isInitialized()) \
            throw std::runtime_error("Cannot modify init-only property '" #name "'"); \
        m_##name = value; \
        _##name##_init.markInitialized(); \
        _##name##_init.setInitOnly(); \
        return *this; \
    } \
    template<typename U> \
    TestClass& name(U&& value) \
        requires attr_detail::PropertyAssignable<type, U> \
    { \
        if (_##name##_init.isInitialized()) \
            throw std::runtime_error("Cannot modify init-only property '" #name "'"); \
        m_##name = std::forward<U>(value); \
        _##name##_init.markInitialized(); \
        _##name##_init.setInitOnly(); \
        return *this; \
    }