#include <catch2/catch_test_macros.hpp>
#include "attr/attr.hpp"
#include <string>
#include <memory>
#include <catch2/matchers/catch_matchers.hpp>

class TestClass {
    PROPERTY(int, basic_prop)
    READONLY_PROPERTY(std::string, readonly_prop)
    private:
        std::string& get_readonly_prop() { return m_readonly_prop; }
    public:
    REQUIRED_PROPERTY(double, required_prop)
    INIT_ONLY_PROPERTY(int, init_only_prop)
    EXPRESSION_PROPERTY(int, computed_prop, m_basic_prop * 2)
    PROPERTY_EX(int, validated_prop, 
        { return obj->m_validated_prop; },
        if (value < 0) throw std::invalid_argument("Value must be non-negative");)
    PROPERTY_IMPL(float, impl_prop, , ,);
    private:
        float get_impl_prop() const { return m_impl_prop; }
    public:
};

TEST_CASE("基本属性操作测试", "[property][basic]") {
    TestClass obj;
    
    SECTION("基本属性读写") {
        obj.basic_prop(42);
        REQUIRE(obj.basic_prop() == 42);
        
        obj.basic_prop() = 100;
        REQUIRE(obj.basic_prop() == 100);
    }
    
    SECTION("只读属性") {
        // 编译时错误：readonly_prop 没有setter
        // obj.readonly_prop("test");
        REQUIRE(obj.readonly_prop().empty());
        
        const TestClass& const_obj = obj;
        REQUIRE(const_obj.readonly_prop().empty());
    }
    
    SECTION("必需属性") {
        REQUIRE_THROWS_WITH(obj.required_prop(), "Required property 'required_prop' not initialized");
        
        obj.required_prop(3.14);
        REQUIRE(obj.required_prop() == 3.14);
        
        obj.required_prop() = 2.718;
        REQUIRE(obj.required_prop() == 2.718);
    }
    
    SECTION("初始化一次属性") {
        obj.init_only_prop(42);
        REQUIRE(obj.init_only_prop() == 42);
        
        REQUIRE_THROWS_WITH(obj.init_only_prop(100), "Cannot modify init-only property 'init_only_prop'");
        REQUIRE(obj.init_only_prop() == 42);
    }
    
    SECTION("计算属性") {
        obj.basic_prop(21);
        REQUIRE(obj.computed_prop() == 42);
        
        obj.basic_prop(50);
        REQUIRE(obj.computed_prop() == 100);
    }

    SECTION("带验证的属性") {
        obj.validated_prop(10);
        REQUIRE(obj.validated_prop() == 10);

        REQUIRE_THROWS_WITH(obj.validated_prop(-5), "Value must be non-negative");
        REQUIRE(obj.validated_prop() == 10);
    }

    SECTION("带实现的属性") {
        REQUIRE(obj.impl_prop() == 3.14f);
        obj.impl_prop(2.718f);
        REQUIRE(obj.impl_prop() == 2.718f);
    }
}

TEST_CASE("属性类型安全性测试", "[property][type_safety]") {
    TestClass obj;
    
    SECTION("类型转换") {
        obj.basic_prop(42.5);  // double到int的隐式转换
        REQUIRE(obj.basic_prop() == 42);
        
        obj.required_prop(5);   // int到double的转换
        REQUIRE(obj.required_prop() == 5.0);

        // 不允许的转换会在编译时失败
        // obj.basic_prop("42");
    }

    SECTION("引用类型测试") {
        obj.required_prop(3.14159);
        const double& ref = obj.required_prop();
        REQUIRE(ref == 3.14159);

        // 只读属性返回const引用
        const std::string& str_ref = obj.readonly_prop();
        REQUIRE(str_ref.empty());
    }
}

TEST_CASE("属性初始化状态测试", "[property][init_state]") {
    TestClass obj;

    SECTION("必需属性初始化状态") {
        REQUIRE_THROWS_WITH(obj.required_prop(), "Required property 'required_prop' not initialized");
        
        obj.required_prop(1.0);
        REQUIRE_NOTHROW(obj.required_prop());

        // 可以多次修改
        obj.required_prop(2.0);
        REQUIRE(obj.required_prop() == 2.0);
    }

    SECTION("初始化一次属性状态") {
        REQUIRE_NOTHROW(obj.init_only_prop(1));
        REQUIRE_THROWS_WITH(obj.init_only_prop(2), "Cannot modify init-only property 'init_only_prop'");

        // 通过const引用访问不会抛出异常
        const TestClass& const_obj = obj;
        REQUIRE_NOTHROW(const_obj.init_only_prop());
    }
}

TEST_CASE("属性自然语法测试", "[property][syntax]") {
    TestClass obj;

    SECTION("链式赋值") {
        obj.basic_prop(1).basic_prop(2).basic_prop(3);
        REQUIRE(obj.basic_prop() == 3);
    }

    SECTION("运算符重载") {
        obj.basic_prop() = 42;
        REQUIRE(obj.basic_prop() == 42);

        int value = obj.basic_prop();
        REQUIRE(value == 42);
    }

    SECTION("const正确性") {
        const TestClass const_obj;
        REQUIRE_NOTHROW(const_obj.basic_prop());
        REQUIRE_NOTHROW(const_obj.readonly_prop());
        REQUIRE_NOTHROW(const_obj.computed_prop());
    }
}
