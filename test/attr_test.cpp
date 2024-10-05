//
// Created by Touka on 2024/9/7.
//

#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include "attr.hpp"

#include <utility>

struct IntStruct
{
    explicit IntStruct(int in) : data(in) {}
    int data;
};

auto operator<=>(const IntStruct& lhs, const IntStruct& rhs) { return lhs.data <=> rhs.data; }

/////////////////////////////////////////////////////////////////////////////
struct destructor_test
{
	~destructor_test() { destructor_ran = true; }
	static bool destructor_ran;
	static void reset() { destructor_ran = false; }
};
bool destructor_test::destructor_ran = false;

/////////////////////////////////////////////////////////////////////////////
struct copy_test
{
	copy_test() = default;

	copy_test(const copy_test& ct)
	{
		was_copied = true;
		value = ct.value;
	}

	copy_test& operator=(const copy_test& ct)
	{
		was_copied = true;
		value = ct.value;

		return *this;
	}

	// issue a compiler error if container tries to move
	copy_test(copy_test const&&) = delete;
	copy_test& operator=(const copy_test&&) = delete;

	static bool was_copied;

	int value;
};

bool copy_test::was_copied = false;

/////////////////////////////////////////////////////////////////////////////
struct move_test
{
	move_test() = default;

	move_test(move_test&& mt)
	{
		was_moved = true;
		value = mt.value;
	}

	move_test& operator=(move_test&& mt)
	{
		was_moved = true;
		value = mt.value;

		return *this;
	}

	// issue a compiler error if container tries to copy
	move_test(move_test const&) = delete;
	move_test& operator=(const move_test&) = delete;

	static bool was_moved;

	int value;
};

bool move_test::was_moved = false;

/////////////////////////////////////////////////////////////////////////////
template <typename T>
class forwarding_test
{
	attr::attr<T> _attr;

public:
	forwarding_test() : _attr() {}
	forwarding_test(T&& t) : _attr(t) {}
	~forwarding_test() { _attr.reset(); }

	template <typename U>
	T GetValueOrDefault(U&& def) const
	{
		return _attr.value_or(std::forward<U>(def));
	}
};

/////////////////////////////////////////////////////////////////////////////
struct assignment_test
{
	assignment_test()                                  { ++num_objects_inited; }
	assignment_test(assignment_test&&)                 { ++num_objects_inited; }
	assignment_test(const assignment_test&)            { ++num_objects_inited; }
	assignment_test& operator=(assignment_test&&)      { return *this; }
	assignment_test& operator=(const assignment_test&) { return *this; }
	~assignment_test()                                 { --num_objects_inited; }

	static int num_objects_inited;
};

int assignment_test::num_objects_inited = 0;


/////////////////////////////////////////////////////////////////////////////
using namespace std;

TEST_CASE("Optional Traits and Functionality", "[optional]") {
    SECTION("Type traits for optional") {
        REQUIRE((std::is_same_v<attr::attr<int>::value_type, int>));
        REQUIRE((std::is_same_v<attr::attr<short>::value_type, short>));
        REQUIRE(!(std::is_same_v<attr::attr<short>::value_type, long>));
        REQUIRE((std::is_same_v<attr::attr<const short>::value_type, const short>));
        REQUIRE((std::is_same_v<attr::attr<volatile short>::value_type, volatile short>));
        REQUIRE((std::is_same_v<attr::attr<const volatile short>::value_type, const volatile short>));
        #if EASTL_TYPE_TRAIT_is_literal_type_CONFORMANCE
            EASTL_INTERNAL_DISABLE_DEPRECATED() // 'is_literal_type<nullopt_t>': was declared deprecated
            REQUIRE(std::is_literal_type_v<nullopt_t>);
            EASTL_INTERNAL_RESTORE_DEPRECATED()
        #endif

        #if EASTL_TYPE_TRAIT_is_trivially_destructible_CONFORMANCE
            REQUIRE(std::is_trivially_destructible_v<int>);
            REQUIRE(std::is_trivially_destructible_v<Internal::optional_storage<int>>);
            REQUIRE(std::is_trivially_destructible_v<optional<int>>);
            REQUIRE(std::is_trivially_destructible_v<optional<int>> == std::is_trivially_destructible_v<int>);
        #endif

        SECTION("Not trivially destructible test") {
            struct NotTrivialDestructible { ~NotTrivialDestructible() = default; };
            REQUIRE(std::is_trivially_destructible_v<NotTrivialDestructible>);
            REQUIRE(std::is_trivially_destructible_v<attr::attr<NotTrivialDestructible>>);
            REQUIRE(std::is_trivially_destructible_v<attr::Internal::attr_storage<NotTrivialDestructible>>);
            REQUIRE(std::is_trivially_destructible_v<attr::attr<NotTrivialDestructible>> == std::is_trivially_destructible_v<NotTrivialDestructible>);
        }
    }
    //
    // SECTION("Basic optional operations") {
    //     optional<int> o;
    //     REQUIRE(!o.has_value());
    //     REQUIRE(o.value_or(0x8BADF00D) == static_cast<int>(0x8BADF00D));
    //
    //     o = 1024;
    //     REQUIRE(o.has_value());
    //     REQUIRE(o.value_or(0x8BADF00D) == 1024);
    //     REQUIRE(o.value() == 1024);
    //
    //     // Test reset
    //     o.reset();
    //     REQUIRE(!o.has_value());
    //     REQUIRE(o.value_or(0x8BADF00D) == static_cast<int>(0x8BADF00D));
    // }
    //
    // SECTION("Optional initialized with nullopt") {
    //     optional<int> o(nullopt);
    //     REQUIRE(!o.has_value());
    //     REQUIRE(o.value_or(0x8BADF00D) == static_cast<int>(0x8BADF00D));
    // }
    //
    // SECTION("Optional initialized with empty brace initializer") {
    //     optional<int> o = {};
    //     REQUIRE(!o.has_value());
    //     REQUIRE(o.value_or(0x8BADF00D) == static_cast<int>(0x8BADF00D));
    // }
    //
    // SECTION("Optional initialized with value") {
    //     optional<int> o(42);
    //     REQUIRE(o.has_value());
    //     REQUIRE(o.value_or(0x8BADF00D) == 42);
    //
    //     o = nullopt;
    //     REQUIRE(!o.has_value());
    //     REQUIRE(o.value_or(0x8BADF00D) == static_cast<int>(0x8BADF00D));
    // }
    //
    // SECTION("Optional holding value") {
    //     optional<int> o(42);
    //     REQUIRE(o.has_value());
    //     REQUIRE(o.value_or(0x8BADF00D) == 42);
    //     REQUIRE(o.value() == 42);
    // }
    //
    // SECTION("Using make_optional") {
    //     auto o = make_optional(42);
    //     REQUIRE((std::is_same_v<decltype(o), optional<int>>));
    //     REQUIRE(o.has_value());
    //     REQUIRE(o.value_or(0x8BADF00D) == 42);
    //     REQUIRE(o.value() == 42);
    // }

    SECTION("Move semantics with r-value ref and engaged attr") {
        attr::attr uniPtrInt(make_unique<int>(42));
    	unique_ptr<int> result = std::move(uniPtrInt);

        REQUIRE(result != nullptr);
        REQUIRE(*result == 42);
        REQUIRE(uniPtrInt == nullptr); // uniPtrInt has been moved-from
    }
}

int main(int argc, char* argv[]) {
	Catch::Session session;

	int result = session.run(argc, argv);

	return result;
}