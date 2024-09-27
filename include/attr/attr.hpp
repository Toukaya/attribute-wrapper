//
// Created by Touka on 2024/9/7.
//

#ifndef ATTR_HPP
#define ATTR_HPP
#include <initializer_list>
#include <type_traits>
#include <__bit_reference>
#include <__concepts/constructible.h>
#include <__utility/in_place.h>


namespace attr {
    template<typename T>
    class attr;

    constexpr bool EXCEPTIONS_ENABLED = false;
    constexpr bool ASSERT_ENABLED = false;

#pragma region bad_attr_access
    template<bool>
    struct bad_attr_access_impl;

    template<>
    struct bad_attr_access_impl<true> : public std::logic_error {
        bad_attr_access_impl() : std::logic_error("attr::bad_attr_access exception") {}
        ~bad_attr_access_impl() noexcept override = default;
    };

    template<>
    struct bad_attr_access_impl<false> {};

    using bad_attr_access = bad_attr_access_impl<EXCEPTIONS_ENABLED>;
#pragma endregion

    namespace Internal {
        template<typename T>
        concept TriviallyDestructible = std::is_trivially_destructible_v<T>;

        template<typename T>
        struct attr_storage {
            using value_type = std::remove_const_t<T>;

            constexpr attr_storage() noexcept = default;

            constexpr explicit attr_storage(const value_type& v) : engaged(true) {
                ::new (std::addressof(val)) value_type(v);
            }

            constexpr explicit attr_storage(value_type&& v) : engaged(true) {
                ::new (std::addressof(val)) value_type(std::move(v));
            }

            constexpr ~attr_storage() {
                if (engaged) destruct_value();
            }

            template<typename... Args>
            constexpr explicit attr_storage(std::in_place_t, Args&&... args) : engaged(true) {
                ::new (std::addressof(val)) value_type(std::forward<Args>(args)...);
            }

            template<typename U, typename... Args>
                requires std::constructible_from<T, std::initializer_list<U>&, Args...>
            constexpr explicit attr_storage(std::in_place_t, std::initializer_list<U> ilist, Args&&... args) : engaged(true) {
                ::new (std::addressof(val)) value_type(ilist, std::forward<Args>(args)...);
            }

            constexpr void destruct_value() { reinterpret_cast<value_type*>(std::addressof(val))->~value_type(); }

            std::aligned_storage_t<sizeof(value_type), std::alignment_of_v<value_type>> val;
            bool engaged = false;
        };

        template<TriviallyDestructible T>
        struct attr_storage<T> {
            using value_type = std::remove_const_t<T>;

            constexpr attr_storage() noexcept = default;
            constexpr explicit attr_storage(const value_type& v) : engaged(true) { ::new (std::addressof(val)) value_type(v); }
            constexpr explicit attr_storage(value_type&& v) : engaged(true) { ::new (std::addressof(val)) value_type(std::move(v)); }

            template<typename... Args>
            constexpr explicit attr_storage(std::in_place_t, Args&&... args)
                : engaged(true) { ::new (std::addressof(val)) value_type(std::forward<Args>(args)...); }

            template<typename U, typename... Args>
                requires std::constructible_from<T, std::initializer_list<U>&, Args...>
            constexpr explicit attr_storage(std::in_place_t, std::initializer_list<U> ilist, Args&&... args)
                : engaged(true) { ::new (std::addressof(val)) value_type(ilist, std::forward<Args>(args)...); }

            constexpr ~attr_storage() noexcept = default;
            constexpr void destruct_value() {}

            std::aligned_storage_t<sizeof(value_type), std::alignment_of_v<value_type>> val;
            bool engaged = false;
        };
    } // namespace Internal

    template <typename T, typename U>
    concept AttrConstructible =
        std::constructible_from<T, U&&> &&
        !std::same_as<std::remove_cvref_t<U>, std::in_place_t> &&
        !std::same_as<std::remove_cvref_t<U>, attr<T>>;

    template<typename T>
    class attr : private Internal::attr_storage<std::remove_cv_t<T>> {
        using base_type = Internal::attr_storage<std::remove_cv_t<T>>;

        using base_type::destruct_value;
        using base_type::engaged;
        using base_type::val;

    public:
        using value_type = T;
        using value_result_type = std::remove_volatile_t<value_type>;

        static_assert(!std::is_reference_v<value_type>, "attr of a reference type is ill-formed");
        static_assert(!std::is_same_v<value_type, std::in_place_t>, "attr of a in_place_t type is ill-formed");

        constexpr attr() noexcept = default;
        explicit constexpr attr(const value_type& value) : base_type(value) {}
        explicit constexpr attr(value_type&& value) noexcept(std::is_nothrow_move_constructible_v<T>)
            : base_type(std::move(value)) {}
        attr(const attr& other) : base_type()
        {
            engaged = other.engaged;

            if (engaged)
            {
                auto* pOtherValue = reinterpret_cast<const T*>(std::addressof(other.val));
                ::new (std::addressof(val)) value_type(*pOtherValue);
            }
        }
        attr(attr&& other) noexcept : base_type()
        {
            engaged = other.engaged;

            if (engaged)
            {
                auto* pOtherValue = reinterpret_cast<T*>(std::addressof(other.val));
                ::new (std::addressof(val)) value_type(std::move(*pOtherValue));
            }
        }

        template <typename... Args>
        constexpr explicit attr(std::in_place_t, Args&&... args) : base_type(std::in_place, std::forward<Args>(args)...) {}

        template <typename U = value_type>
        requires AttrConstructible<T, U>
        inline explicit constexpr attr(U&& value)
            : base_type(std::in_place, std::forward<U>(value)) {}

        inline attr& operator=(const attr& other) {
            auto* pOtherValue = reinterpret_cast<const T*>(std::addressof(other.val));
            if (engaged == other.engaged)
            {
                if (engaged)
                    *get_value_address() = *pOtherValue;
            }
            else
            {
                if (engaged)
                {
                    destruct_value();
                    engaged = false;
                }
                else
                {
                    construct_value(*pOtherValue);
                    engaged = true;
                }
            }
            return *this;
        }

        inline attr& operator=(attr&& other) noexcept(noexcept(std::is_nothrow_move_assignable_v<value_type> &&
                                           std::is_nothrow_move_constructible_v<value_type>)) {
            auto* pOtherValue = reinterpret_cast<T*>(std::addressof(other.val));
            if (engaged == other.engaged)
            {
                if (engaged)
                    *get_value_address() = std::move(*pOtherValue);
            }
            else
            {
                if (engaged)
                {
                    destruct_value();
                    engaged = false;
                }
                else
                {
                    construct_value(std::move(*pOtherValue));
                    engaged = true;
                }
            }
            return *this;
        }

        template <class U>
        requires std::same_as<std::decay_t<U>, T>
        inline attr& operator=(U&& u)
        {
            if(engaged)
            {
                *get_value_address() = std::forward<U>(u);
            }
            else
            {
                engaged = true;
                construct_value(std::forward<U>(u));
            }

            return *this;
        }

        inline void swap(attr& other)
        noexcept(std::is_nothrow_move_constructible_v<T>&& std::is_nothrow_swappable_v<T>)
        {
            using std::swap;
            if (engaged == other.engaged)
            {
                if (engaged)
                    swap(**this, *other);
            }
            else
            {
                if (engaged)
                {
                    other.construct_value(std::move(*(value_type*)std::addressof(val)));
                    destruct_value();
                }
                else
                {
                    construct_value(std::move(*((value_type*)std::addressof(other.val))));
                    other.destruct_value();
                }

                swap(engaged, other.engaged);
            }
        }

        inline void reset()
        {
            if (engaged)
            {
                destruct_value();
                engaged = false;
            }
        }

    private:
        template <class... Args>
        inline void construct_value(Args&&... args)
        { ::new (std::addressof(val)) value_type(std::forward<Args>(args)...); }

        inline T* get_value_address() noexcept(!EXCEPTIONS_ENABLED)
        {
            if constexpr (EXCEPTIONS_ENABLED) {
                if(!engaged)
                    throw bad_attr_access();
            } else if constexpr (ASSERT_ENABLED) {
                assert(engaged, "no value to retrieve");
            }
            return reinterpret_cast<T*>(std::addressof(val));
        }

        inline const T* get_value_address() const noexcept(!EXCEPTIONS_ENABLED)
        {
            if constexpr (EXCEPTIONS_ENABLED) {
                if(!engaged)
                    throw bad_attr_access();
            } else if constexpr (ASSERT_ENABLED) {
                assert(engaged, "no value to retrieve");
            }
            return reinterpret_cast<T*>(std::addressof(val));
        }

        inline value_type& get_value_ref() noexcept(!EXCEPTIONS_ENABLED)
        {
            if constexpr (EXCEPTIONS_ENABLED) {
                if(!engaged)
                    throw bad_attr_access();
            } else if constexpr (ASSERT_ENABLED) {
                assert(engaged, "no value to retrieve");
            }
            return *static_cast<value_type *>(std::addressof(val));
        }

        inline const value_type& get_value_ref() const noexcept(!EXCEPTIONS_ENABLED)
        {
            if constexpr (EXCEPTIONS_ENABLED) {
                if(!engaged)
                    throw bad_attr_access();
            } else if constexpr (ASSERT_ENABLED) {
                assert(engaged, "no value to retrieve");
            }
            return *static_cast<value_type *>(std::addressof(val));
        }

        inline value_type&& get_rvalue_ref() noexcept(!EXCEPTIONS_ENABLED)
        {
            if constexpr (EXCEPTIONS_ENABLED) {
                if(!engaged)
                    throw bad_attr_access();
            } else if constexpr (ASSERT_ENABLED) {
                assert(engaged, "no value to retrieve");
            }
            return std::move(*static_cast<value_type *>(std::addressof(val)));
        }
    };

}

#endif //ATTR_HPP
