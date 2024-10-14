//
// Created by Touka on 2024/9/7.
//

#ifndef ATTR_HPP
#define ATTR_HPP
#include <initializer_list>
#include <type_traits>

namespace touka {
    template<typename Fn, typename T>
    concept GetterFn = requires(Fn&&fn, T&&value)
    {
        { std::invoke(std::forward<Fn>(fn), std::forward<T>(value)) } -> std::convertible_to<T>;
    };

    template<typename Fn, typename T>
    concept SetterFn = requires(Fn&&fn, T&&value, T&&new_value)
    {
        { std::invoke(std::forward<Fn>(fn), value, new_value) };
    };

    template<typename value_type>
    struct default_getter {
        inline decltype(auto) operator()(value_type&& val) const & {
            return std::forward<value_type>(val);
        }
        inline decltype(auto) operator()(value_type&& val) & {
            return std::forward<value_type>(val);
        }
        inline decltype(auto) operator()(value_type&& val) && {
            return std::forward<value_type>(val);
        }
        inline decltype(auto) operator()(value_type&& val) const && {
            return std::forward<value_type>(val);
        }
        inline decltype(auto) operator()(const value_type& val) const {
            return val;
        }
        inline decltype(auto) operator()(value_type& val) const {
            return val;
        }
    };

    template<typename value_type>
    struct default_setter {
        inline void operator()(value_type&value, value_type& new_value) const  {
            value = new_value;
        }
        inline void operator()(value_type&value, const value_type&new_value) const  {
            value = new_value;
        }
        inline void operator()(value_type&value, value_type&&new_value) const  {
            value = std::move(new_value);
        }
    };

    template<typename T,
        typename Getter = default_getter<T>,
        typename Setter = default_setter<T>>
    requires GetterFn<Getter, T> && SetterFn<Setter, T>
    class attr_impl;

    namespace Internal {
        template<typename T>
        concept TriviallyDestructible = std::is_trivially_destructible_v<T>;

        template<typename T>
        concept DefaultConstructible = std::is_default_constructible_v<T>;

        template<typename T>
        struct attr_storage {
            using value_type = T;
            static_assert(std::is_object_v<value_type>,
                          "instantiation of attr with a non-object type is undefined behavior");

            constexpr attr_storage() noexcept requires DefaultConstructible<T> = default;
            constexpr explicit attr_storage(const value_type&v) : val(v) {
            }
            constexpr explicit attr_storage(const volatile value_type&v) : val(v) {
            }
            constexpr explicit attr_storage(value_type&&v) : val(std::move(v)) {
            }
            constexpr ~attr_storage() = default;
            template<typename... Args>
            constexpr explicit attr_storage(std::in_place_t, Args&&... args) : val(std::forward<Args>(args)...) {
            }

            template<typename U, typename... Args>
                requires std::constructible_from<T, std::initializer_list<U> &, Args...>
            constexpr explicit
            attr_storage(std::in_place_t, std::initializer_list<U> initList, Args&&... args)
                : val(initList, std::forward<Args>(args)...) {
            }

            value_type val;
        };

        template<TriviallyDestructible T>
        struct attr_storage<T> {
            using value_type = T;
            static_assert(std::is_object_v<value_type>,
                          "instantiation of attr with a non-object type is undefined behavior");

            constexpr attr_storage() noexcept requires DefaultConstructible<T> = default;

            constexpr explicit attr_storage(const value_type&v) : val(v) {
            }

            constexpr explicit attr_storage(const volatile value_type&v) : val(v) {
            }

            constexpr explicit attr_storage(value_type&&v) : val(std::move(v)) {
            }

            template<typename... Args>
            constexpr explicit attr_storage(std::in_place_t, Args&&... args) : val(std::forward<Args>(args)...) {
            }

            template<typename U, typename... Args>
                requires std::constructible_from<T, std::initializer_list<U> &, Args...>
            constexpr explicit attr_storage(std::in_place_t, std::initializer_list<U> initList, Args&&... args)
                : val(initList, std::forward<Args>(args)...) {
            }

            value_type val;
        };
    } // namespace Internal

    template<typename T, typename U>
    concept AttrConstructible =
            std::constructible_from<T, U &&> &&
            !std::same_as<std::remove_cvref_t<U>, std::in_place_t> &&
            !std::same_as<std::remove_cvref_t<U>, attr_impl<T>>;


    template<typename T, typename Getter, typename Setter>
    requires GetterFn<Getter, T> && SetterFn<Setter, T>
    class attr_impl : private Internal::attr_storage<std::remove_cv_t<T>> {
        using BaseType = Internal::attr_storage<std::remove_cv_t<T>>;

        using BaseType::val;

    public:
        using value_type = T;
        using value_result_type = std::remove_volatile_t<value_type>;
        using GetterType = std::type_identity_t<Getter>;
        using SetterType = std::type_identity_t<Setter>;

        static_assert(!std::is_reference_v<value_type>, "attr of a reference type is ill-formed");
        static_assert(!std::is_same_v<value_type, std::in_place_t>, "attr of a in_place_t type is ill-formed");

        constexpr attr_impl() noexcept = default;
        constexpr ~attr_impl() = default;

        explicit constexpr attr_impl(const value_type&value) : BaseType(value) {
        }

        explicit constexpr attr_impl(value_type&&value) noexcept(std::is_nothrow_move_constructible_v<T>)
            : BaseType(std::move(value)) {
        }

        attr_impl(const attr_impl&other) : BaseType() {
            setter(this->val, other._get());
        }

        attr_impl(attr_impl&&other) noexcept : BaseType() {
            setter(this->val, std::move(other.val));
        }

        template<typename... Args>
        constexpr explicit attr_impl(std::in_place_t, Args&&... args) : BaseType(std::in_place, std::forward<Args>(args)...) {
        }

        template<typename U = value_type>
            requires AttrConstructible<T, U>
        constexpr explicit attr_impl(U&&value)
            : BaseType(std::in_place, std::forward<U>(value)) {
        }

        inline attr_impl& operator=(const attr_impl&other) {
            setter(this->val, other._get());
            return *this;
        }

        inline attr_impl& operator=(attr_impl&&other) noexcept(std::is_nothrow_move_assignable_v<value_type> &&
                                                     std::is_nothrow_move_constructible_v<value_type>) {
            setter(this->val, std::move(other.val));
            return *this;
        }

        template<class U>
            requires std::same_as<std::decay_t<U>, T>
        inline attr_impl& operator=(U&&u) {
            _setter(this->val, std::move(std::forward<U>(u)));
            return *this;
        }

        inline void swap(attr_impl&other)
            noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_swappable_v<T>) {
            using std::swap;
            auto tmp = _get();
            setter(this->val, other._get());
            other.setter(other.val, tmp);
        }

        operator T() const { return _get(); }

        constexpr std::strong_ordering operator<=>(const attr_impl&rhs) const {
            return _get() <=> rhs._get();
        }

        constexpr auto operator<=>(const T&value) const {
            return _get() <=> value;
        }

        constexpr bool operator==(const attr_impl&rhs) const {
            return _get() == rhs._get();
        }

        constexpr bool operator==(const T&value) const {
            return _get() == value;
        }

        class ValueGetter {
            friend class attr_impl;

        protected:
            GetterType getter;

            explicit ValueGetter() : getter(Getter{}) {}

            explicit ValueGetter(Getter g) : getter(g) {
            }

        public:

            inline auto operator()(const value_type& val) const {
                return getter(val);
            }
        };

        class ValueSetter {
            friend class attr_impl;

        protected:
            SetterType setter;

            explicit ValueSetter() : setter(Setter{}) {}

            explicit ValueSetter(Setter s) : setter(s) {
            }

        public:
            inline void operator()(value_type& val, const value_type &new_val) const  {
                setter(val, new_val);
            }
        };

    private:

        ValueGetter _getter;
        ValueSetter _setter;

        template<class... Args>
        inline void construct_value(Args&&... args) {
            ::new(std::addressof(val)) value_type(std::forward<Args>(args)...);
        }

        inline T* get_value_address() {
            return std::bit_cast<T *>(std::addressof(val));
        }

        inline const T* get_value_address() const {
            return std::bit_cast<T *>(std::addressof(val));
        }

        constexpr value_type _get() const noexcept { return _getter(this->val); }
    };

    template<class T>
    inline constexpr bool operator==(const attr_impl<T>&lhs, const attr_impl<T>&rhs) {
        return lhs == rhs;
    }

    template<class T>
    inline constexpr auto operator<=>(const attr_impl<T>&lhs, const attr_impl<T>&rhs) {
        return lhs <=> rhs;
    }

    template<class T>
    constexpr auto operator<=>(const T&value, const attr_impl<T>&opt) {
        return value <=> opt;
    }

    template<class T>
    constexpr bool operator==(const T&value, const attr_impl<T>&opt) {
        return value == opt;
    }

    template<class Tp>
    inline constexpr std::enable_if_t<std::is_move_constructible_v<Tp> && std::is_swappable_v<Tp>, void>
    swap(attr_impl<Tp>&lhs, attr_impl<Tp>&rhs) noexcept(noexcept(lhs.swap(rhs))) {
        lhs.swap(rhs);
    }

    template<typename T, auto Getter, auto Setter>
    using attr = attr_impl<T,
                std::decay_t<decltype(Getter)>,
                std::decay_t<decltype(Setter)>>;

}

namespace std {
    template<class T>
    struct hash<std::__enable_hash_helper<touka::attr_impl<T>, remove_const_t<T>>> {
        size_t operator()(const touka::attr_impl<T>&attr) const noexcept {
            return hash<std::remove_const_t<T>>()(attr._get());
        }
    };
}

#endif //ATTR_HPP
