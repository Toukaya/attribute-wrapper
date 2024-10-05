//
// Created by Touka on 2024/9/7.
//

#ifndef ATTR_HPP
#define ATTR_HPP
#include <initializer_list>
#include <type_traits>
#include <source_location>

namespace touka {
    namespace detail {
        constexpr bool EXCEPTIONS_ENABLED = true;
        constexpr bool ASSERT_ENABLED = true;
    }

#pragma region assert
    template<bool>
    struct assert_impl;

    template<>
    struct assert_impl<true> {
        static inline void assert_msg(const bool condition, const char* msg, const std::source_location&loc) noexcept {
            if (!condition) [[unlikely]] {
                std::printf("Assertion failed: %s\nFile: %s\nFunction: %s\nLine: %u\n",
                        msg, loc.file_name(), loc.function_name(), loc.line());
                std::abort();
            }
        }
    };

    template<>
    struct assert_impl<false> {
        static constexpr inline void assert_msg([[maybe_unused]]bool condition, [[maybe_unused]]const char* msg, [[maybe_unused]]const std::source_location&loc) noexcept {}
    };

    inline void assert_msg(bool condition, const char* msg, const std::source_location& loc = std::source_location::current()) noexcept {
        assert_impl<detail::ASSERT_ENABLED>::assert_msg(condition, msg, loc);
    }

#pragma endregion
}


namespace attr {
    template<typename T>
    class attr;

#pragma region bad_attr_access
    class bad_attr_access : public std::logic_error {
    public:
        bad_attr_access() noexcept : std::logic_error("attr::bad_attr_access exception") {};
        bad_attr_access(const bad_attr_access&) noexcept = default;
        bad_attr_access& operator=(const bad_attr_access&) noexcept = default;
        ~bad_attr_access() noexcept override;
    };

    [[noreturn]] inline void throw_bad_attr_access() noexcept(!touka::detail::EXCEPTIONS_ENABLED) {
        if constexpr (touka::detail::EXCEPTIONS_ENABLED) {
            throw bad_attr_access();
        } else {
            std::printf("bad_attr_access was thrown in -fno-exceptions mode");
            std::abort();
        }
    }
#pragma endregion

    namespace Internal {
        template<typename T>
        concept TriviallyDestructible = std::is_trivially_destructible_v<T>;

        template<typename T>
        struct attr_storage {
            using value_type = std::remove_const_t<T>;

            constexpr attr_storage() noexcept = default;

            constexpr explicit attr_storage(const value_type&v) : engaged(true) {
                ::new(std::addressof(val)) value_type(v);
            }

            constexpr explicit attr_storage(value_type&&v) : engaged(true) {
                ::new(std::addressof(val)) value_type(std::move(v));
            }

            constexpr ~attr_storage() {
                if (engaged) destruct_value();
            }

            template<typename... Args>
            constexpr explicit attr_storage(std::in_place_t, Args&&... args) : engaged(true) {
                ::new(std::addressof(val)) value_type(std::forward<Args>(args)...);
            }

            template<typename U, typename... Args>
                requires std::constructible_from<T, std::initializer_list<U> &, Args...>
            constexpr explicit
            attr_storage(std::in_place_t, std::initializer_list<U> ilist, Args&&... args) : engaged(true) {
                ::new(std::addressof(val)) value_type(ilist, std::forward<Args>(args)...);
            }

            constexpr void destruct_value() { reinterpret_cast<value_type *>(std::addressof(val))->~value_type(); }

            std::aligned_storage_t<sizeof(value_type), std::alignment_of_v<value_type>> val;
            bool engaged = false;
        };

        template<TriviallyDestructible T>
        struct attr_storage<T> {
            using value_type = std::remove_const_t<T>;

            constexpr attr_storage() noexcept = default;

            constexpr explicit attr_storage(const value_type&v) : engaged(true) {
                ::new(std::addressof(val)) value_type(v);
            }

            constexpr explicit attr_storage(value_type&&v) : engaged(true) {
                ::new(std::addressof(val)) value_type(std::move(v));
            }

            template<typename... Args>
            constexpr explicit attr_storage(std::in_place_t, Args&&... args)
                : engaged(true) { ::new(std::addressof(val)) value_type(std::forward<Args>(args)...); }

            template<typename U, typename... Args>
                requires std::constructible_from<T, std::initializer_list<U> &, Args...>
            constexpr explicit attr_storage(std::in_place_t, std::initializer_list<U> ilist, Args&&... args)
                : engaged(true) { ::new(std::addressof(val)) value_type(ilist, std::forward<Args>(args)...); }

            constexpr ~attr_storage() noexcept = default;

            constexpr void destruct_value() {
            }

            std::aligned_storage_t<sizeof(value_type), std::alignment_of_v<value_type>> val;
            bool engaged = false;
        };
    } // namespace Internal

    template<typename T, typename U>
    concept AttrConstructible =
            std::constructible_from<T, U &&> &&
            !std::same_as<std::remove_cvref_t<U>, std::in_place_t> &&
            !std::same_as<std::remove_cvref_t<U>, attr<T>>;

    template<typename T>
    class attr : private Internal::attr_storage<std::remove_cv_t<T>> {
        using BaseType = Internal::attr_storage<std::remove_cv_t<T>>;

        using BaseType::destruct_value;
        using BaseType::engaged;
        using BaseType::val;

    public:
        using value_type = T;
        using value_result_type = std::remove_volatile_t<value_type>;

        static_assert(!std::is_reference_v<value_type>, "attr of a reference type is ill-formed");
        static_assert(!std::is_same_v<value_type, std::in_place_t>, "attr of a in_place_t type is ill-formed");

        constexpr attr() noexcept = default;

        explicit constexpr attr(const value_type&value) : BaseType(value) {
        }

        explicit constexpr attr(value_type&&value) noexcept(std::is_nothrow_move_constructible_v<T>)
            : BaseType(std::move(value)) {
        }

        attr(const attr&other) : BaseType() {
            engaged = other.engaged;

            if (engaged) {
                auto* pOtherValue = std::launder(reinterpret_cast<const T *>(std::addressof(other.val)));
                ::new(std::addressof(val)) value_type(*pOtherValue);
            }
        }

        attr(attr&&other) noexcept : BaseType() {
            engaged = other.engaged;

            if (engaged) {
                auto* pOtherValue = std::launder(reinterpret_cast<T *>(std::addressof(other.val)));
                ::new(std::addressof(val)) value_type(std::move(*pOtherValue));
            }
        }

        template<typename... Args>
        constexpr explicit
        attr(std::in_place_t, Args&&... args) : BaseType(std::in_place, std::forward<Args>(args)...) {
        }

        template<typename U = value_type>
            requires AttrConstructible<T, U>
        inline constexpr explicit attr(U&& value)
            : BaseType(std::in_place, std::forward<U>(value)) {
        }

        constexpr inline explicit operator bool() const { return engaged; }

        inline attr& operator=(const attr&other) {
            auto* pOtherValue = std::launder(reinterpret_cast<const T *>(std::addressof(other.val)));
            if (engaged == other.engaged) {
                if (engaged)
                    *get_value_address() = *pOtherValue;
            }
            else {
                if (engaged) {
                    destruct_value();
                    engaged = false;
                }
                else {
                    construct_value(*pOtherValue);
                    engaged = true;
                }
            }
            return *this;
        }

        inline attr& operator=(attr&&other) noexcept(noexcept(std::is_nothrow_move_assignable_v<value_type> &&
                                                              std::is_nothrow_move_constructible_v<value_type>)) {
            auto* pOtherValue = std::launder(reinterpret_cast<T *>(std::addressof(other.val)));
            if (engaged == other.engaged) {
                if (engaged)
                    *get_value_address() = std::move(*pOtherValue);
            }
            else {
                if (engaged) {
                    destruct_value();
                    engaged = false;
                }
                else {
                    construct_value(std::move(*pOtherValue));
                    engaged = true;
                }
            }
            return *this;
        }

        template<class U>
        requires std::same_as<std::decay_t<U>, T>
        inline attr& operator=(U&&u) {
            if (engaged) {
                *get_value_address() = std::forward<U>(u);
            }
            else {
                engaged = true;
                construct_value(std::forward<U>(u));
            }

            return *this;
        }

        inline void swap(attr&other)
            noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_swappable_v<T>) {
            using std::swap;
            if (engaged == other.engaged) {
                if (engaged)
                    swap(**this, *other);
            }
            else {
                if (engaged) {
                    other.construct_value(std::move(*(value_type *)std::addressof(val)));
                    destruct_value();
                }
                else {
                    construct_value(std::move(*((value_type *)std::addressof(other.val))));
                    other.destruct_value();
                }

                swap(engaged, other.engaged);
            }
        }

        inline void reset() {
            if (engaged) {
                destruct_value();
                engaged = false;
            }
        }

        constexpr std::strong_ordering operator<=>(const attr& rhs) const {
            return static_cast<bool>(*this) == static_cast<bool>(rhs)
            ? (!static_cast<bool>(*this) ? std::strong_ordering::equal : static_cast<T>(*this) <=> static_cast<T>(rhs))
            : static_cast<bool>(*this) <=> static_cast<bool>(rhs);
        }

        constexpr auto operator<=>(const T& value) const {
            return static_cast<bool>(*this) ? *this <=> value : std::strong_ordering::less;
        }

        constexpr bool operator==(const attr& rhs) const {
            return (static_cast<bool>(*this) == static_cast<bool>(rhs)) && (!static_cast<bool>(*this) || static_cast<T>(*this) == static_cast<T>(rhs));
        }

        constexpr bool operator==(const T& value) const {
            return static_cast<bool>(*this) && *this == value;
        }

        operator T&() & { return get_value_ref(); }
        operator T&&() && { return std::move(get_value_ref()); }
        operator const T&() const & { return get_value_ref(); }
        operator const T&&() const && { return std::move(get_value_ref()); }

    private:
        template<class... Args>
        inline void construct_value(Args&&... args) {
            ::new(std::addressof(val)) value_type(std::forward<Args>(args)...);
        }

        inline T* get_value_address() {
            if constexpr (touka::detail::EXCEPTIONS_ENABLED) {
                if (!engaged) {
                    throw_bad_attr_access();
                }
            } else if constexpr (touka::detail::ASSERT_ENABLED) {
                touka::assert_msg(engaged, "no value to retrieve");
            }
            return std::bit_cast<T *>(std::addressof(val));
        }

        inline T* get_value_address() noexcept requires (!touka::detail::EXCEPTIONS_ENABLED) {
            if constexpr (touka::detail::ASSERT_ENABLED) {
                touka::assert_msg(engaged, "no value to retrieve");
            }
            return std::bit_cast<T *>(std::addressof(val));
        }

        inline const T* get_value_address() const {
            if constexpr (touka::detail::EXCEPTIONS_ENABLED) {
                if (!engaged) {
                    throw_bad_attr_access();
                }
            } else if constexpr (touka::detail::ASSERT_ENABLED) {
                touka::assert_msg(engaged, "no value to retrieve");
            }
            return std::bit_cast<T *>(std::addressof(val));
        }

        inline const T* get_value_address() const noexcept requires (!touka::detail::EXCEPTIONS_ENABLED) {
            if constexpr (touka::detail::ASSERT_ENABLED) {
                touka::assert_msg(engaged, "no value to retrieve");
            }
            return std::bit_cast<T *>(std::addressof(val));
        }

        inline value_type& get_value_ref() {
            if constexpr (touka::detail::EXCEPTIONS_ENABLED) {
                if (!engaged)
                    throw_bad_attr_access();
            } else if constexpr (touka::detail::ASSERT_ENABLED) {
                touka::assert_msg(engaged, "no value to retrieve");
            }
            return *std::bit_cast<value_type *>(std::addressof(val));
        }

        inline value_type& get_value_ref() noexcept requires (!touka::detail::EXCEPTIONS_ENABLED) {
            if constexpr (touka::detail::ASSERT_ENABLED) {
                touka::assert_msg(engaged && "no value to retrieve");
            }
            return *std::bit_cast<value_type *>(std::addressof(val));
        }

        inline const value_type& get_value_ref() const {
            if constexpr (touka::detail::EXCEPTIONS_ENABLED) {
                if (!engaged)
                    throw_bad_attr_access();
            } else if constexpr (touka::detail::ASSERT_ENABLED) {
                touka::assert_msg(engaged, "no value to retrieve");
            }
            return *std::bit_cast<value_type *>(std::addressof(val));
        }

        inline const value_type& get_value_ref() const noexcept requires (!touka::detail::EXCEPTIONS_ENABLED) {
            if constexpr (touka::detail::ASSERT_ENABLED) {
                touka::assert_msg(engaged, "no value to retrieve");
            }
            return *std::bit_cast<value_type *>(std::addressof(val));
        }

        inline value_type&& get_rvalue_ref() {
            if constexpr (touka::detail::EXCEPTIONS_ENABLED) {
                if (!engaged)
                    throw_bad_attr_access();
            } else if constexpr (touka::detail::ASSERT_ENABLED) {
                touka::assert_msg(engaged, "no value to retrieve");
            }
            return std::move(*std::bit_cast<value_type *>(std::addressof(val)));
        }

        inline value_type&& get_rvalue_ref() noexcept requires (!touka::detail::EXCEPTIONS_ENABLED) {
            if constexpr (touka::detail::ASSERT_ENABLED) {
                touka::assert_msg(engaged, "no value to retrieve");
            }
            return std::move(*std::bit_cast<value_type *>(std::addressof(val)));
        }
    };

    template <class T>
    inline constexpr bool operator==(const attr<T>& lhs, const attr<T>& rhs)
    {
        return (static_cast<bool>(lhs) != static_cast<bool>(rhs)) ? false : (static_cast<bool>(lhs) == false) ? true : lhs == rhs;
    }

    template <class T>
    inline constexpr auto operator<=>(const attr<T>& lhs, const attr<T>& rhs) {
        return (!lhs.has_value() && !rhs.has_value()) ? std::strong_ordering::equal : (!lhs.has_value()) ? std::strong_ordering::less : (!rhs.has_value()) ? std::strong_ordering::greater : lhs <=> rhs;
    }

    template <class T>
    constexpr auto operator<=>(const T& value, const attr<T>& opt) {
        return opt.has_value() ? value <=> opt : std::strong_ordering::greater;
    }

    template <class T>
    constexpr bool operator==(const T& value, const attr<T>& opt) {
        return opt.has_value() && value == opt;
    }

}

#endif //ATTR_HPP
