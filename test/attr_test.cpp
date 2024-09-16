//
// Created by Touka on 2024/9/7.
//

#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include "attr.hpp"

#include <utility>

struct IntStruct
{
    IntStruct(int in) : data(in) {}
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

TEST_CASE("Trivially Destructible Test", "[trivial_destructible]") {
	REQUIRE(is_trivially_destructible_v<int>);
	REQUIRE(is_trivially_destructible_v<attr::Internal::attr_storage<int>>);
	REQUIRE(is_trivially_destructible_v<attr::attr<int>>);
	REQUIRE(is_trivially_destructible_v<attr::attr<int>> == is_trivially_destructible_v<int>);

	struct NotTrivialDestructible { ~NotTrivialDestructible() = default; };

	REQUIRE(!is_trivially_destructible_v<NotTrivialDestructible>);
	REQUIRE(!is_trivially_destructible_v<attr::attr<NotTrivialDestructible>>);
	REQUIRE(!is_trivially_destructible_v<::attr::Internal::attr_storage<NotTrivialDestructible>>);
	REQUIRE(is_trivially_destructible_v<attr::attr<NotTrivialDestructible>> == is_trivially_destructible_v<NotTrivialDestructible>);
}
// TestOptional
//

// int TestOptional()
// {
// 	int nErrorCount = 0;
// 	{
//
// 		{
//
// 		{
// 			attr<int> o(42);
// 			VERIFY(o.value_or(0x8BADF00D) == 42);
// 		}
//
// 		{
// 			attr<int> o(42);
// 			VERIFY(static_cast<bool>(o));
// 			VERIFY(o.value_or(0x8BADF00D) == 42);
// 			VERIFY(o.value() == 42);
// 		}
//
// 		{
// 			auto o = make_attr(42);
// 			VERIFY((is_same<decltype(o), attr<int>>::value));
// 			VERIFY(static_cast<bool>(o));
// 			VERIFY(o.value_or(0x8BADF00D) == 42);
// 			VERIFY(o.value() == 42);
// 		}
//
// 		{
// 			// value_or with this a r-value ref and engaged.
// 			attr<unique_ptr<int>> o = eastl::make_unique<int>(42);
// 			auto result = eastl::move(o).value_or(eastl::make_unique<int>(1337));
// 			VERIFY(result != nullptr);
// 			VERIFY(*result == 42);
// 			VERIFY(o.has_value());
// 			VERIFY(o.value() == nullptr); // o has been moved-from.
// 		}
//
// 		{
// 			// value_or with this a r-value ref and not engaged.
// 			attr<unique_ptr<int>> o;
// 			auto result = eastl::move(o).value_or(eastl::make_unique<int>(1337));
// 			VERIFY(result != nullptr);
// 			VERIFY(*result == 1337);
// 			VERIFY(!o.has_value());
// 		}
//
// 		{
// 			int a = 42;
// 			auto o = make_attr(a);
// 			VERIFY((is_same<decltype(o)::value_type, int>::value));
// 			VERIFY(o.value() == 42);
// 		}
//
// 		{
// 			// test make_attr stripping refs/cv-qualifers
// 			int a = 42;
// 			const volatile int& intRef = a;
// 			auto o = make_attr(intRef);
// 			VERIFY((is_same<decltype(o)::value_type, int>::value));
// 			VERIFY(o.value() == 42);
// 		}
//
// 		{
// 			int a = 10;
// 			const volatile int& aRef = a;
// 			auto o = eastl::make_attr(aRef);
// 			VERIFY(o.value() == 10);
// 		}
//
// 		{
// 			{
// 				struct local { int payload1; };
// 				auto o = eastl::make_attr<local>(42);
// 				VERIFY(o.value().payload1 == 42);
// 			}
// 			{
// 				struct local { int payload1; int payload2; };
// 				auto o = eastl::make_attr<local>(42, 43);
// 				VERIFY(o.value().payload1 == 42);
// 				VERIFY(o.value().payload2 == 43);
// 			}
//
// 			{
// 				struct local
// 				{
// 					local(std::initializer_list<int> ilist)
// 					{
// 						payload1 = ilist.begin()[0];
// 						payload2 = ilist.begin()[1];
// 					}
//
// 					int payload1;
// 					int payload2;
// 				};
//
// 				auto o = eastl::make_attr<local>({42, 43});
// 				VERIFY(o.value().payload1 == 42);
// 				VERIFY(o.value().payload2 == 43);
// 			}
// 		}
//
// 		{
// 			attr<int> o1(42), o2(24);
// 			VERIFY(o1.value() == 42);
// 			VERIFY(o2.value() == 24);
// 			VERIFY(*o1 == 42);
// 			VERIFY(*o2 == 24);
// 			o1 = eastl::move(o2);
// 			VERIFY(*o2 == 24);
// 			VERIFY(*o1 == 24);
// 			VERIFY(o2.value() == 24);
// 			VERIFY(o1.value() == 24);
// 			VERIFY(bool(o1));
// 			VERIFY(bool(o2));
// 		}
//
// 		{
// 			struct local { int payload; };
// 			attr<local> o = local{ 42 };
// 			VERIFY(o->payload == 42);
// 		}
//
// 		{
// 			struct local
// 			{
// 				int test() const { return 42; }
// 			};
//
// 			{
// 				const attr<local> o = local{};
// 				VERIFY(o->test() == 42);
// 				VERIFY((*o).test() == 42);
// 				VERIFY(o.value().test() == 42);
// 				VERIFY(bool(o));
// 			}
//
// 			{
// 				attr<local> o = local{};
// 				VERIFY(bool(o));
// 				o = nullopt;
// 				VERIFY(!bool(o));
//
// 				VERIFY(o.value_or(local{}).test() == 42);
// 				VERIFY(!bool(o));
// 			}
// 		}
// 	}
//
// 	{
// 		copy_test c;
// 		c.value = 42;
//
// 		attr<copy_test> o1(c);
// 		VERIFY(copy_test::was_copied);
//
// 		copy_test::was_copied = false;
//
// 		attr<copy_test> o2(o1);
// 		VERIFY(copy_test::was_copied);
// 		VERIFY(o2->value == 42);
// 	}
//
// 	{
// 		move_test t;
// 		t.value = 42;
//
// 		attr<move_test> o1(eastl::move(t));
// 		VERIFY(move_test::was_moved);
//
// 		move_test::was_moved = false;
//
// 		attr<move_test> o2(eastl::move(o1));
// 		VERIFY(move_test::was_moved);
// 		VERIFY(o2->value == 42);
// 	}
//
// 	{
// 		forwarding_test<float>ft(1.f);
// 		float val = ft.GetValueOrDefault(0.f);
// 		VERIFY(val == 1.f);
// 	}
//
// 	{
// 		assignment_test::num_objects_inited = 0;
// 		{
// 			attr<assignment_test> o1;
// 			attr<assignment_test> o2 = assignment_test();
// 			attr<assignment_test> o3(o2);
// 			VERIFY(assignment_test::num_objects_inited == 2);
// 			o1 = nullopt;
// 			VERIFY(assignment_test::num_objects_inited == 2);
// 			o1 = o2;
// 			VERIFY(assignment_test::num_objects_inited == 3);
// 			o1 = o2;
// 			VERIFY(assignment_test::num_objects_inited == 3);
// 			o1 = nullopt;
// 			VERIFY(assignment_test::num_objects_inited == 2);
// 			o2 = o1;
// 			VERIFY(assignment_test::num_objects_inited == 1);
// 			o1 = o2;
// 			VERIFY(assignment_test::num_objects_inited == 1);
// 		}
// 		VERIFY(assignment_test::num_objects_inited == 0);
//
// 		{
// 			attr<assignment_test> o1;
// 			VERIFY(assignment_test::num_objects_inited == 0);
// 			o1 = nullopt;
// 			VERIFY(assignment_test::num_objects_inited == 0);
// 			o1 = attr<assignment_test>(assignment_test());
// 			VERIFY(assignment_test::num_objects_inited == 1);
// 			o1 = attr<assignment_test>(assignment_test());
// 			VERIFY(assignment_test::num_objects_inited == 1);
// 			attr<assignment_test> o2(eastl::move(o1));
// 			VERIFY(assignment_test::num_objects_inited == 2);
// 			o1 = nullopt;
// 			VERIFY(assignment_test::num_objects_inited == 1);
// 		}
// 		VERIFY(assignment_test::num_objects_inited == 0);
// 	}
//
// 	#if EASTL_VARIADIC_TEMPLATES_ENABLED
// 	{
// 		struct vec3
// 		{
// 			vec3(std::initializer_list<float> ilist) { auto* p = ilist.begin();  x = *p++; y = *p++; z = *p++; }
// 			vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}  // testing variadic template constructor overload
// 			float x = 0, y = 0, z = 0;
// 		};
//
// 		{
// 			attr<vec3> o{ in_place, 4.f, 5.f, 6.f };
// 			VERIFY(o->x == 4 && o->y == 5 && o->z == 6);
// 		}
//
// 		{
// 			attr<vec3> o{ in_place, {4.f, 5.f, 6.f} };
// 			VERIFY(o->x == 4 && o->y == 5 && o->z == 6);
// 		}
//
// 		{
// 			attr<string> o(in_place, {'a', 'b', 'c'});
// 			VERIFY(o == string("abc"));
// 		}
//
// 		// http://en.cppreference.com/w/cpp/utility/attr/emplace
// 		{
// 			attr<vec3> o;
// 			vec3& v = o.emplace(42.f, 42.f, 42.f);
// 			VERIFY(o->x == 42.f && o->y == 42.f && o->z == 42.f);
// 			VERIFY(v.x == 42.f && v.y == 42.f && v.z == 42.f);
// 			v.x = 10.f;
// 			VERIFY(o->x == 10.f && o->y == 42.f && o->z == 42.f);
// 		}
//
// 		{
// 			attr<vec3> o;
// 			vec3& v = o.emplace({42.f, 42.f, 42.f});
// 			VERIFY(o->x == 42.f && o->y == 42.f && o->z == 42.f);
// 			VERIFY(v.x == 42.f && v.y == 42.f && v.z == 42.f);
// 			v.x = 10.f;
// 			VERIFY(o->x == 10.f && o->y == 42.f && o->z == 42.f);
// 		}
//
// 		{
// 			attr<int> o;
// 			int& i = o.emplace(42);
// 			VERIFY(*o == 42);
// 			VERIFY(i == 42);
// 			i = 10;
// 			VERIFY(*o == 10);
// 		}
//
// 		struct nonCopyableNonMovable
// 		{
// 			nonCopyableNonMovable(int v) : val(v) {}
//
// 			nonCopyableNonMovable(const nonCopyableNonMovable&) = delete;
// 			nonCopyableNonMovable(nonCopyableNonMovable&&) = delete;
// 			nonCopyableNonMovable& operator=(const nonCopyableNonMovable&) = delete;
//
// 			int val = 0;
// 		};
//
// 		{
// 			attr<nonCopyableNonMovable> o;
// 			o.emplace(42);
// 			VERIFY(o->val == 42);
// 		}
//
// 		{
// 			// Verify emplace will destruct object if it has been engaged.
// 			destructor_test::reset();
// 			attr<destructor_test> o;
// 			o.emplace();
// 			VERIFY(!destructor_test::destructor_ran);
//
// 			destructor_test::reset();
// 			o.emplace();
// 			VERIFY(destructor_test::destructor_ran);
// 		}
// 	}
// 	#endif
//
//
// 	// swap
// 	{
// 		{
// 			attr<int> o1 = 42, o2 = 24;
// 			VERIFY(*o1 == 42);
// 			VERIFY(*o2 == 24);
// 			o1.swap(o2);
// 			VERIFY(*o1 == 24);
// 			VERIFY(*o2 == 42);
// 		}
//
// 		{
// 			attr<int> o1 = 42, o2 = 24;
// 			VERIFY(*o1 == 42);
// 			VERIFY(*o2 == 24);
// 			swap(o1, o2);
// 			VERIFY(*o1 == 24);
// 			VERIFY(*o2 == 42);
// 		}
//
// 		{
// 			attr<int> o1 = 42, o2;
// 			VERIFY(*o1 == 42);
// 			VERIFY(o2.has_value() == false);
// 			swap(o1, o2);
// 			VERIFY(o1.has_value() == false);
// 			VERIFY(*o2 == 42);
// 		}
//
// 		{
// 			attr<int> o1 = nullopt, o2 = 42;
// 			VERIFY(o1.has_value() == false);
// 			VERIFY(*o2 == 42);
// 			swap(o1, o2);
// 			VERIFY(*o1 == 42);
// 			VERIFY(o2.has_value() == false);
// 		}
// 	}
//
// 	{
// 		attr<IntStruct> o(in_place, 10);
// 		attr<IntStruct> e;
//
// 		VERIFY(o < IntStruct(42));
// 		VERIFY(!(o < IntStruct(2)));
// 		VERIFY(!(o < IntStruct(10)));
// 		VERIFY(e < o);
// 		VERIFY(e < IntStruct(10));
//
// 		VERIFY(o > IntStruct(4));
// 		VERIFY(!(o > IntStruct(42)));
//
// 		VERIFY(o >= IntStruct(4));
// 		VERIFY(o >= IntStruct(10));
// 		VERIFY(IntStruct(4) <= o);
// 		VERIFY(IntStruct(10) <= o);
//
// 		VERIFY(o == IntStruct(10));
// 		VERIFY(o->data == IntStruct(10).data);
//
// 		VERIFY(o != IntStruct(11));
// 		VERIFY(o->data != IntStruct(11).data);
//
// 		VERIFY(e == nullopt);
// 		VERIFY(nullopt == e);
//
// 		VERIFY(o != nullopt);
// 		VERIFY(nullopt != o);
// 		VERIFY(nullopt < o);
// 		VERIFY(o > nullopt);
// 		VERIFY(!(nullopt > o));
// 		VERIFY(!(o < nullopt));
// 		VERIFY(nullopt <= o);
// 		VERIFY(o >= nullopt);
// 	}
//
// 	#if defined(EA_COMPILER_HAS_THREE_WAY_COMPARISON)
// 	{
// 		attr<IntStruct> o(in_place, 10);
// 		attr<IntStruct> e;
//
// 		VERIFY((o <=> IntStruct(42)) < 0);
// 		VERIFY((o <=> IntStruct(2)) >= 0);
// 		VERIFY((o <=> IntStruct(10)) >= 0);
// 		VERIFY((e <=> o) < 0);
// 		VERIFY((e <=> IntStruct(10)) < 0);
//
// 		VERIFY((o <=> IntStruct(4)) > 0);
// 		VERIFY(o <=> IntStruct(42) <= 0);
//
// 		VERIFY((o <=> IntStruct(4)) >= 0);
// 		VERIFY((o <=> IntStruct(10)) >= 0);
// 		VERIFY((IntStruct(4) <=> o) <= 0);
// 		VERIFY((IntStruct(10) <=> o) <= 0);
//
// 		VERIFY((o <=> IntStruct(10)) == 0);
// 		VERIFY((o->data <=> IntStruct(10).data) == 0);
//
// 		VERIFY((o <=> IntStruct(11)) != 0);
// 		VERIFY((o->data <=> IntStruct(11).data) != 0);
//
// 		VERIFY((e <=> nullopt) == 0);
// 		VERIFY((nullopt <=> e) == 0);
//
// 		VERIFY((o <=> nullopt) != 0);
// 		VERIFY((nullopt <=> o) != 0);
// 		VERIFY((nullopt <=> o) < 0);
// 		VERIFY((o <=> nullopt) > 0);
// 		VERIFY((nullopt <=> o) <= 0);
// 		VERIFY((o <=> nullopt) >= 0);
// 	}
// 	#endif
//
// 	// hash
// 	{
// 		{
// 			// verify that the hash an empty eastl::attr object is zero.
// 			typedef hash<attr<int>> hash_attr_t;
// 			attr<int> e;
// 			VERIFY(hash_attr_t{}(e) == 0);
// 		}
//
// 		{
// 			// verify that the hash is the same as the hash of the underlying type
// 			const char* const pMessage = "Electronic Arts Canada";
// 			typedef hash<attr<string>> hash_attr_t;
// 			attr<string> o = string(pMessage);
// 			VERIFY(hash_attr_t{}(o) == hash<string>{}(pMessage));
// 		}
// 	}
//
// 	// sorting
// 	{
// 		vector<attr<int>> v = {{122}, {115}, nullopt, {223}};
// 		sort(begin(v), end(v));
// 		vector<attr<int>> sorted = {nullopt, 115, 122, 223};
//
// 		VERIFY(v == sorted);
// 	}
//
// 	// test destructors being called.
// 	{
// 		destructor_test::reset();
// 		{
// 			attr<destructor_test> o = destructor_test{};
// 		}
// 		VERIFY(destructor_test::destructor_ran);
//
// 		destructor_test::reset();
// 		{
// 			attr<destructor_test> o;
// 		}
// 		// destructor shouldn't be called as object wasn't constructed.
// 		VERIFY(!destructor_test::destructor_ran);
//
//
// 		destructor_test::reset();
// 		{
// 			attr<destructor_test> o = {};
// 		}
// 		// destructor shouldn't be called as object wasn't constructed.
// 		VERIFY(!destructor_test::destructor_ran);
//
// 		destructor_test::reset();
// 		{
// 			attr<destructor_test> o = nullopt;
// 		}
// 		// destructor shouldn't be called as object wasn't constructed.
// 		VERIFY(!destructor_test::destructor_ran);
// 	}
//
// 	// attr rvalue tests
// 	{
// 		VERIFY(*attr<uint32_t>(1u)						== 1u);
// 		VERIFY(attr<uint32_t>(1u).value()				== 1u);
// 		VERIFY(attr<uint32_t>(1u).value_or(0xdeadf00d)	== 1u);
// 		VERIFY(attr<uint32_t>().value_or(0xdeadf00d)	== 0xdeadf00d);
// 		VERIFY(attr<uint32_t>(1u).has_value() == true);
// 		VERIFY(attr<uint32_t>().has_value() == false);
// 		VERIFY( attr<IntStruct>(in_place, 10)->data		== 10);
//
// 	}
//
// 	// alignment type tests
// 	{
// 		static_assert(alignof(attr<Align16>) == alignof(Align16), "attr alignment failure");
// 		static_assert(alignof(attr<Align32>) == alignof(Align32), "attr alignment failure");
// 		static_assert(alignof(attr<Align64>) == alignof(Align64), "attr alignment failure");
// 	}
//
// 	{
// 		// user reported regression that failed to compile
// 		struct local_struct
// 		{
// 			local_struct() {}
// 			~local_struct() {}
// 		};
// 		static_assert(!eastl::is_trivially_destructible_v<local_struct>, "");
//
// 		{
// 			local_struct ls;
// 			eastl::attr<local_struct> o{ls};
// 		}
// 		{
// 			const local_struct ls;
// 			eastl::attr<local_struct> o{ls};
// 		}
// 	}
//
// 	{
// 		{
// 			// user regression
// 			eastl::attr<eastl::string> o = eastl::string("Hello World");
// 			eastl::attr<eastl::string> co;
//
// 			co = o; // force copy-assignment
//
// 			VERIFY( o.value().data() != co.value().data());
// 			VERIFY( o.value().data() == eastl::string("Hello World"));
// 			VERIFY(co.value().data() == eastl::string("Hello World"));
// 		}
// 		{
// 			// user regression
// 			EA_DISABLE_VC_WARNING(4625 4626) // copy/assignment operator constructor was implicitly defined as deleted
// 			struct local
// 			{
// 				eastl::unique_ptr<int> ptr;
// 			};
// 			EA_RESTORE_VC_WARNING()
//
// 			eastl::attr<local> o1 = local{eastl::make_unique<int>(42)};
// 			eastl::attr<local> o2;
//
// 			o2 = eastl::move(o1);
//
// 			VERIFY(!!o1 == true);
// 			VERIFY(!!o2 == true);
// 			VERIFY(!!o1->ptr == false);
// 			VERIFY(!!o2->ptr == true);
// 			VERIFY(o2->ptr.get() != nullptr);
// 		}
// 		{
// 			// user regression
// 			static bool copyCtorCalledWithUninitializedValue;
// 			static bool moveCtorCalledWithUninitializedValue;
// 			copyCtorCalledWithUninitializedValue = moveCtorCalledWithUninitializedValue = false;
// 			struct local
// 			{
// 				uint32_t val;
// 				local()
// 					: val(0xabcdabcd)
// 				{}
// 				local(const local& other)
// 					: val(other.val)
// 				{
// 					if (other.val != 0xabcdabcd)
// 						copyCtorCalledWithUninitializedValue = true;
// 				}
// 				local(local&& other)
// 					: val(eastl::move(other.val))
// 				{
// 					if (other.val != 0xabcdabcd)
// 						moveCtorCalledWithUninitializedValue = true;
// 				}
// 				local& operator=(const local&) = delete;
// 			};
// 			eastl::attr<local> n;
// 			eastl::attr<local> o1(n);
// 			VERIFY(!copyCtorCalledWithUninitializedValue);
// 			eastl::attr<local> o2(eastl::move(n));
// 			VERIFY(!moveCtorCalledWithUninitializedValue);
// 		}
// 	}
//
// 	{
// 		auto testFn = []() -> attr<int>
// 		{
// 			return eastl::nullopt;
// 		};
//
// 		auto o = testFn();
// 		VERIFY(!!o == false);
// 	}
//
// 	nErrorCount += TestOptional_MonadicOperations();
//
// 	return nErrorCount;
// }

