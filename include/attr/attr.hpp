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

    struct InaccessibleT {
        struct InaccessibleTag {
            explicit InaccessibleTag() = default;
        };

        constexpr explicit InaccessibleT(InaccessibleTag, InaccessibleTag) noexcept {
        }
    };

    inline constexpr InaccessibleT inaccessible_t{InaccessibleT::InaccessibleTag{}, InaccessibleT::InaccessibleTag{}};

    enum class AccessSpecifiers : uint8_t {
        Public = 0,
        Private,
    };

    template<typename Getter>
    struct make_getter {
        auto operator()(Getter&&g, AccessSpecifiers access = AccessSpecifiers::Public) {
            return std::tuple{std::forward<Getter>(g), std::move(access)};
        }
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
        typename Setter = default_setter<T>,
        AccessSpecifiers GetterAccess = AccessSpecifiers::Public,
        AccessSpecifiers SetterAccess = AccessSpecifiers::Public>
    requires GetterFn<Getter, T> && SetterFn<Setter, T>
    class attr;

    namespace Internal {
        template<typename T>
        concept TriviallyDestructible = std::is_trivially_destructible_v<T>;

        template<typename T>
        concept DefaultConstructible = std::is_default_constructible_v<T>;

        template<typename From, typename To>
        concept reference_bindable = requires {
            requires std::is_reference_v<To>;
            requires std::is_reference_v<From> || std::is_convertible_v<From, To>;
            requires (std::is_lvalue_reference_v<To> &&
                      (std::is_lvalue_reference_v<From> ||
                       std::is_convertible_v<std::add_pointer_t<std::remove_reference_t<From>>,
                                             std::add_pointer_t<std::remove_reference_t<To>>> ||
                       std::is_same_v<std::remove_cvref_t<From>,
                                      std::reference_wrapper<std::remove_reference_t<To>>> ||
                       std::is_same_v<std::remove_cvref_t<From>,
                                      std::reference_wrapper<std::remove_const_t<std::remove_reference_t<To>>>>)) ||
                     (std::is_rvalue_reference_v<To> &&
                      !std::is_lvalue_reference_v<From> &&
                      std::is_convertible_v<std::add_pointer_t<std::remove_reference_t<From>>,
                                            std::add_pointer_t<std::remove_reference_t<To>>>);
        };

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
            !std::same_as<std::remove_cvref_t<U>, attr<T>>;


    template<typename T, typename Getter,
            typename Setter,
            AccessSpecifiers GetterAccess,
            AccessSpecifiers SetterAccess>
    requires GetterFn<Getter, T> && SetterFn<Setter, T>
    class attr : private Internal::attr_storage<std::remove_cv_t<T>> {
        using BaseType = Internal::attr_storage<std::remove_cv_t<T>>;

        using BaseType::val;

    public:
        using value_type = T;
        using value_result_type = std::remove_volatile_t<value_type>;
        using GetterType = std::type_identity_t<Getter>;
        using SetterType = std::type_identity_t<Setter>;

        static_assert(!std::is_reference_v<value_type>, "attr of a reference type is ill-formed");
        static_assert(!std::is_same_v<value_type, std::in_place_t>, "attr of a in_place_t type is ill-formed");

        constexpr attr() noexcept = default;
        constexpr ~attr() = default;

        explicit constexpr attr(const value_type&value) : BaseType(value) {
        }

        explicit constexpr attr(value_type&&value) noexcept(std::is_nothrow_move_constructible_v<T>)
            : BaseType(std::move(value)) {
        }

        explicit constexpr attr(value_type&&value, make_getter<Getter>&&getter) noexcept(std::is_nothrow_move_constructible_v<T>)
            : BaseType(std::move(value)) {
        }

        attr(const attr&other) : BaseType() {
            setter(this->val, other._get());
        }

        attr(attr&&other) noexcept : BaseType() {
            setter(this->val, std::move(other.val));
        }

        template<typename... Args>
        constexpr explicit attr(std::in_place_t, Args&&... args) : BaseType(std::in_place, std::forward<Args>(args)...) {
        }

        template<typename U = value_type>
            requires AttrConstructible<T, U>
        constexpr explicit attr(U&&value)
            : BaseType(std::in_place, std::forward<U>(value)) {
        }

        inline attr& operator=(const attr&other) {
            setter(this->val, other._get());
            return *this;
        }

        inline attr& operator=(attr&&other) noexcept(std::is_nothrow_move_assignable_v<value_type> &&
                                                     std::is_nothrow_move_constructible_v<value_type>) {
            setter(this->val, std::move(other.val));
            return *this;
        }

        template<class U>
            requires std::same_as<std::decay_t<U>, T>
        inline attr& operator=(U&&u) {
            setter(this->val, std::move(std::forward<U>(u)));
            return *this;
        }

        // inline void swap(attr&other)
        //     noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_swappable_v<T>) {
        //     using std::swap;
        //     swap(_get(), other._get());
        // }

        operator T() const { return _get(); }

        constexpr std::strong_ordering operator<=>(const attr&rhs) const {
            return _get() <=> rhs._get();
        }

        constexpr auto operator<=>(const T&value) const {
            return _get() <=> value;
        }

        constexpr bool operator==(const attr&rhs) const {
            return _get() == rhs._get();
        }

        constexpr bool operator==(const T&value) const {
            return _get() == value;
        }

        class ValueGetter {
            friend class attr;

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
            friend class attr;

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

        ValueGetter getter;
        ValueSetter setter;

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

        constexpr value_type _get() const noexcept { return getter(this->val); }
    };

    template<class T>
    inline constexpr bool operator==(const attr<T>&lhs, const attr<T>&rhs) {
        return lhs == rhs;
    }

    template<class T>
    inline constexpr auto operator<=>(const attr<T>&lhs, const attr<T>&rhs) {
        return lhs <=> rhs;
    }

    template<class T>
    constexpr auto operator<=>(const T&value, const attr<T>&opt) {
        return value <=> opt;
    }

    template<class T>
    constexpr bool operator==(const T&value, const attr<T>&opt) {
        return value == opt;
    }

    template<class Tp>
    inline constexpr std::enable_if_t<std::is_move_constructible_v<Tp> && std::is_swappable_v<Tp>, void>
    swap(attr<Tp>&lhs, attr<Tp>&rhs) noexcept(noexcept(lhs.swap(rhs))) {
        lhs.swap(rhs);
    }

}

namespace std {
    template<class T>
    struct hash<std::__enable_hash_helper<touka::attr<T>, remove_const_t<T>>> {
        size_t operator()(const touka::attr<T>&attr) const noexcept {
            return hash<std::remove_const_t<T>>()(attr._get());
        }
    };
}

#endif //ATTR_HPP
