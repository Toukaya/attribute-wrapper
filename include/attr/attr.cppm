//
// Created by Touka on 2024/9/8.
//

export module attr;
import <algorithm>;
import <initializer_list>;
import <type_traits>;
import <__concepts/constructible.h>;
import <__utility/in_place.h>;

namespace attr {
namespace detail {

template<typename T>
concept TriviallyDestructible = std::is_trivially_destructible_v<T>;

template<typename T>
struct attr_storage {
    using value_type = std::remove_const_t<T>;

    constexpr attr_storage() noexcept = default;
    constexpr attr_storage(const value_type& v) : engaged(true) { new (&val) value_type(v); }
    constexpr attr_storage(value_type&& v) : engaged(true) { new (&val) value_type(std::move(v)); }

    template<typename... Args>
    constexpr explicit attr_storage(std::in_place_t, Args&&... args)
        : engaged(true) { new (&val) value_type(std::forward<Args>(args)...); }

    template<typename U, typename... Args>
        requires std::constructible_from<T, std::initializer_list<U>&, Args...>
    constexpr explicit attr_storage(std::in_place_t, std::initializer_list<U> ilist, Args&&... args)
        : engaged(true) { new (&val) value_type(ilist, std::forward<Args>(args)...); }

    constexpr ~attr_storage() { if (engaged) destruct_value(); }

    constexpr void destruct_value() { reinterpret_cast<value_type*>(&val)->~value_type(); }

    alignas(value_type) unsigned char val[sizeof(value_type)];
    bool engaged = false;
};

template<TriviallyDestructible T>
struct attr_storage<T> {
    using value_type = std::remove_const_t<T>;

    // Same constructors as above...

    constexpr ~attr_storage() = default;
    constexpr void destruct_value() {}

    alignas(value_type) unsigned char val[sizeof(value_type)];
    bool engaged = false;
};

} // namespace detail
}
