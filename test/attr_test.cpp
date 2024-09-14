//
// Created by Touka on 2024/9/7.
//

// #include "attr/attr.hpp"
// #include "attr/optional.hpp"

template <typename T>
class Prop {
public:
    // 构造函数，可以接受一个初始值
    Prop(T value = T{}) : value_(value) {}

    // 隐式转换为 T 类型
    operator T() const { // NOLINT(*-explicit-constructor)
        return value_;
    }

    // 重载赋值运算符
    Prop& operator=(const T& newValue) {
        value_ = newValue;
        return *this;
    }

private:
    T value_;
};

int main() {
    // Prop<int> 类型可以隐式转换为 int
    Prop myProp{42};
    int i = myProp;  // 隐式转换
    myProp = 100;    // 赋值操作

    return 0;
}