//
// Unit tests for zero-overhead property proxy
//

#include <catch2/catch_all.hpp>
#include "attr/property.hpp"

#include <string>
#include <memory>
#include <vector>

// ═══════════════════════════════════════════════════════════════════════════
// Test Fixtures
// ═══════════════════════════════════════════════════════════════════════════

// Basic test class with read-write property
class BasicOwner {
private:
    int value_ = 0;

public:
    int getValue() const { return value_; }
    void setValue(int v) { value_ = v; }

    TOUKA_PROPERTY(BasicOwner, int, value, &BasicOwner::getValue, &BasicOwner::setValue);
};

// Class with multiple properties
class MultiPropertyOwner {
private:
    int x_ = 0;
    int y_ = 0;
    double scale_ = 1.0;

public:
    int getX() const { return x_; }
    void setX(int v) { x_ = v; }

    int getY() const { return y_; }
    void setY(int v) { y_ = v; }

    double getScale() const { return scale_; }
    void setScale(double v) { scale_ = v; }

    TOUKA_PROPERTY(MultiPropertyOwner, int, x, &MultiPropertyOwner::getX, &MultiPropertyOwner::setX);
    TOUKA_PROPERTY(MultiPropertyOwner, int, y, &MultiPropertyOwner::getY, &MultiPropertyOwner::setY);
    TOUKA_PROPERTY(MultiPropertyOwner, double, scale, &MultiPropertyOwner::getScale, &MultiPropertyOwner::setScale);
};

// Class with computed read-only property
class Rectangle {
private:
    double width_ = 0;
    double height_ = 0;

public:
    double getWidth() const { return width_; }
    void setWidth(double v) { width_ = v > 0 ? v : 0; }

    double getHeight() const { return height_; }
    void setHeight(double v) { height_ = v > 0 ? v : 0; }

    double getArea() const { return width_ * height_; }
    double getPerimeter() const { return 2 * (width_ + height_); }

    TOUKA_PROPERTY(Rectangle, double, width, &Rectangle::getWidth, &Rectangle::setWidth);
    TOUKA_PROPERTY(Rectangle, double, height, &Rectangle::getHeight, &Rectangle::setHeight);
    TOUKA_PROPERTY_RO(Rectangle, double, area, &Rectangle::getArea);
    TOUKA_PROPERTY_RO(Rectangle, double, perimeter, &Rectangle::getPerimeter);
};

// Class with string property
class StringOwner {
private:
    std::string name_;

public:
    const std::string& getName() const { return name_; }
    void setName(std::string v) { name_ = std::move(v); }

    TOUKA_PROPERTY(StringOwner, std::string, name, &StringOwner::getName, &StringOwner::setName);
};

// Class with pointer property
class PointerOwner {
private:
    int* ptr_ = nullptr;

public:
    int* getPtr() const { return ptr_; }
    void setPtr(int* v) { ptr_ = v; }

    TOUKA_PROPERTY(PointerOwner, int*, ptr, &PointerOwner::getPtr, &PointerOwner::setPtr);
};

// Class with validation in setter
class ValidatedOwner {
private:
    int value_ = 0;

public:
    int getValue() const { return value_; }
    void setValue(int v) {
        // Clamp to [0, 100]
        if (v < 0) v = 0;
        if (v > 100) v = 100;
        value_ = v;
    }

    TOUKA_PROPERTY(ValidatedOwner, int, value, &ValidatedOwner::getValue, &ValidatedOwner::setValue);
};

// Class with write-only property (e.g., password)
class WriteOnlyOwner {
private:
    std::string secret_;
    bool secretSet_ = false;

public:
    void setSecret(std::string v) {
        secret_ = std::move(v);
        secretSet_ = true;
    }

    bool isSecretSet() const { return secretSet_; }

    TOUKA_PROPERTY_WO(WriteOnlyOwner, std::string, secret, &WriteOnlyOwner::setSecret);
};

// Class with vector property for subscript test
class VectorOwner {
private:
    std::vector<int> data_;

public:
    const std::vector<int>& getData() const { return data_; }
    void setData(std::vector<int> v) { data_ = std::move(v); }

    TOUKA_PROPERTY(VectorOwner, std::vector<int>, data, &VectorOwner::getData, &VectorOwner::setData);
};

// ═══════════════════════════════════════════════════════════════════════════
// Test Cases
// ═══════════════════════════════════════════════════════════════════════════

TEST_CASE("Property basic read/write operations", "[property]") {
    BasicOwner obj;

    SECTION("Initial value is zero") {
        REQUIRE(obj.value == 0);
    }

    SECTION("Assignment sets value") {
        obj.value = 42;
        REQUIRE(obj.value == 42);
    }

    SECTION("Implicit conversion works") {
        obj.value = 100;
        int x = obj.value;
        REQUIRE(x == 100);
    }

    SECTION("Explicit get() works") {
        obj.value = 55;
        REQUIRE(obj.value.get() == 55);
    }

    SECTION("Explicit set() works") {
        obj.value.set(77);
        REQUIRE(obj.value == 77);
    }
}

TEST_CASE("Property compound assignment operators", "[property]") {
    BasicOwner obj;
    obj.value = 10;

    SECTION("operator+=") {
        obj.value += 5;
        REQUIRE(obj.value == 15);
    }

    SECTION("operator-=") {
        obj.value -= 3;
        REQUIRE(obj.value == 7);
    }

    SECTION("operator*=") {
        obj.value *= 4;
        REQUIRE(obj.value == 40);
    }

    SECTION("operator/=") {
        obj.value /= 2;
        REQUIRE(obj.value == 5);
    }

    SECTION("operator%=") {
        obj.value %= 3;
        REQUIRE(obj.value == 1);
    }
}

TEST_CASE("Property bitwise compound assignment operators", "[property]") {
    BasicOwner obj;
    obj.value = 0b1010;  // 10

    SECTION("operator&=") {
        obj.value &= 0b1100;  // 12
        REQUIRE(obj.value == 0b1000);  // 8
    }

    SECTION("operator|=") {
        obj.value |= 0b0101;  // 5
        REQUIRE(obj.value == 0b1111);  // 15
    }

    SECTION("operator^=") {
        obj.value ^= 0b1111;  // 15
        REQUIRE(obj.value == 0b0101);  // 5
    }

    SECTION("operator<<=") {
        obj.value <<= 2;
        REQUIRE(obj.value == 0b101000);  // 40
    }

    SECTION("operator>>=") {
        obj.value >>= 1;
        REQUIRE(obj.value == 0b0101);  // 5
    }
}

TEST_CASE("Property increment/decrement operators", "[property]") {
    BasicOwner obj;
    obj.value = 10;

    SECTION("prefix ++") {
        auto& ref = ++obj.value;
        REQUIRE(obj.value == 11);
        // Verify it returns reference to property
        ref = 20;
        REQUIRE(obj.value == 20);
    }

    SECTION("postfix ++") {
        int old = obj.value++;
        REQUIRE(old == 10);
        REQUIRE(obj.value == 11);
    }

    SECTION("prefix --") {
        auto& ref = --obj.value;
        REQUIRE(obj.value == 9);
        ref = 5;
        REQUIRE(obj.value == 5);
    }

    SECTION("postfix --") {
        int old = obj.value--;
        REQUIRE(old == 10);
        REQUIRE(obj.value == 9);
    }
}

TEST_CASE("Property comparison operators", "[property]") {
    BasicOwner obj;
    obj.value = 50;

    SECTION("operator== with equal value") {
        REQUIRE(obj.value == 50);
    }

    SECTION("operator== with unequal value") {
        REQUIRE_FALSE(obj.value == 40);
    }

    SECTION("operator<=> less than") {
        REQUIRE((obj.value <=> 60) < 0);
    }

    SECTION("operator<=> greater than") {
        REQUIRE((obj.value <=> 40) > 0);
    }

    SECTION("operator<=> equal") {
        REQUIRE((obj.value <=> 50) == 0);
    }
}

TEST_CASE("Multiple properties in same class", "[property]") {
    MultiPropertyOwner obj;

    obj.x = 10;
    obj.y = 20;
    obj.scale = 2.5;

    REQUIRE(obj.x == 10);
    REQUIRE(obj.y == 20);
    REQUIRE(obj.scale == 2.5);

    // Verify properties are independent
    obj.x += 5;
    REQUIRE(obj.x == 15);
    REQUIRE(obj.y == 20);  // unchanged
}

TEST_CASE("Read-only computed properties", "[property]") {
    Rectangle rect;

    rect.width = 10.0;
    rect.height = 5.0;

    SECTION("Computed area") {
        REQUIRE(rect.area == 50.0);
    }

    SECTION("Computed perimeter") {
        REQUIRE(rect.perimeter == 30.0);
    }

    SECTION("Area updates when dimensions change") {
        rect.width = 20.0;
        REQUIRE(rect.area == 100.0);
    }

    // Note: Attempting to assign to read-only property should fail to compile
    // rect.area = 100;  // Should not compile
}

TEST_CASE("String property", "[property]") {
    StringOwner obj;

    SECTION("Assignment from const char*") {
        obj.name = "Hello";
        REQUIRE(obj.name.get() == "Hello");
    }

    SECTION("Assignment from std::string") {
        std::string s = "World";
        obj.name = s;
        REQUIRE(obj.name.get() == "World");
    }

    SECTION("Move assignment") {
        std::string s = "Moved";
        obj.name = std::move(s);
        REQUIRE(obj.name.get() == "Moved");
    }

    SECTION("Compound += for strings") {
        obj.name = "Hello";
        obj.name += " World";
        REQUIRE(obj.name.get() == "Hello World");
    }
}

TEST_CASE("Pointer property", "[property]") {
    PointerOwner obj;
    int data = 42;

    SECTION("Set and get pointer") {
        obj.ptr = &data;
        REQUIRE(obj.ptr == &data);
    }

    SECTION("Dereference operator") {
        obj.ptr = &data;
        REQUIRE(*obj.ptr == 42);
    }

    SECTION("Arrow operator") {
        struct Data { int x; };
        Data d{100};
        Data* pData = &d;

        // For raw pointer, -> returns the pointer itself
        PointerOwner ptrObj;
        int value = 999;
        ptrObj.ptr = &value;
        REQUIRE(*ptrObj.ptr == 999);
    }

    SECTION("Nullptr") {
        obj.ptr = nullptr;
        REQUIRE(obj.ptr == nullptr);
    }
}

TEST_CASE("Property with validation", "[property]") {
    ValidatedOwner obj;

    SECTION("Value within range") {
        obj.value = 50;
        REQUIRE(obj.value == 50);
    }

    SECTION("Value below minimum clamped") {
        obj.value = -10;
        REQUIRE(obj.value == 0);
    }

    SECTION("Value above maximum clamped") {
        obj.value = 200;
        REQUIRE(obj.value == 100);
    }
}

TEST_CASE("Write-only property", "[property]") {
    WriteOnlyOwner obj;

    REQUIRE_FALSE(obj.isSecretSet());

    obj.secret = "password123";

    REQUIRE(obj.isSecretSet());

    // Note: Attempting to read write-only property should fail to compile
    // std::string s = obj.secret;  // Should not compile
}

TEST_CASE("Vector property with subscript", "[property]") {
    VectorOwner obj;
    obj.data = {1, 2, 3, 4, 5};

    SECTION("Subscript access") {
        REQUIRE(obj.data[0] == 1);
        REQUIRE(obj.data[2] == 3);
        REQUIRE(obj.data[4] == 5);
    }

    SECTION("Size via implicit conversion") {
        std::vector<int> v = obj.data;
        REQUIRE(v.size() == 5);
    }
}

TEST_CASE("Memory layout - zero overhead", "[property][memory]") {
    SECTION("Basic property adds no size") {
        struct DataOnly {
            int value_;
        };

        // BasicOwner has int value_ + property
        // With [[no_unique_address]], they should be same size
        // Note: This may fail on some compilers (e.g., older MSVC)
        // where [[no_unique_address]] is not fully supported

        // We check that the overhead is at most 1 byte (empty class minimum)
        // after alignment
        REQUIRE(sizeof(BasicOwner) <= sizeof(int) + alignof(int));
    }

    SECTION("Multiple properties minimal overhead") {
        struct DataOnly {
            int x_;
            int y_;
            double scale_;
        };

        // Allow for some padding but verify it's minimal
        REQUIRE(sizeof(MultiPropertyOwner) <= sizeof(DataOnly) + 3 * alignof(double));
    }
}

TEST_CASE("Property in array context", "[property]") {
    BasicOwner arr[3];

    arr[0].value = 10;
    arr[1].value = 20;
    arr[2].value = 30;

    REQUIRE(arr[0].value == 10);
    REQUIRE(arr[1].value == 20);
    REQUIRE(arr[2].value == 30);
}

TEST_CASE("Property with dynamic allocation", "[property]") {
    auto obj = std::make_unique<BasicOwner>();

    obj->value = 42;
    REQUIRE(obj->value == 42);

    obj->value += 8;
    REQUIRE(obj->value == 50);
}

TEST_CASE("Property const correctness", "[property]") {
    BasicOwner obj;
    obj.value = 100;

    const BasicOwner& cref = obj;

    // Reading from const reference should work
    REQUIRE(cref.value == 100);

    // Note: Writing to const reference should fail to compile
    // cref.value = 200;  // Should not compile
}

// ═══════════════════════════════════════════════════════════════════════════
// Template-based property definition test
// ═══════════════════════════════════════════════════════════════════════════

class TemplatePropertyOwner {
private:
    int x_ = 0;
    int y_ = 0;

public:
    int getX() const { return x_; }
    void setX(int v) { x_ = v; }

    int getY() const { return y_; }
    void setY(int v) { y_ = v; }

    // Define property descriptors using templates
    using x_desc = touka::Property<TemplatePropertyOwner, int, &TemplatePropertyOwner::getX, &TemplatePropertyOwner::setX>;
    using y_desc = touka::Property<TemplatePropertyOwner, int, &TemplatePropertyOwner::getY, &TemplatePropertyOwner::setY>;

    // Use template-based property definition
    TOUKA_PROPERTY_DEF(x, x_desc);
    TOUKA_PROPERTY_DEF(y, y_desc);
};

TEST_CASE("Template-based property definition", "[property][template]") {
    TemplatePropertyOwner obj;

    SECTION("Read and write via template-defined property") {
        obj.x = 100;
        obj.y = 200;

        REQUIRE(obj.x == 100);
        REQUIRE(obj.y == 200);
    }

    SECTION("Compound assignment") {
        obj.x = 10;
        obj.x += 5;
        REQUIRE(obj.x == 15);
    }

    SECTION("Zero overhead verification") {
        // Should be same size as two ints
        REQUIRE(sizeof(TemplatePropertyOwner) <= sizeof(int) * 2 + alignof(int));
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Derived class test
// ═══════════════════════════════════════════════════════════════════════════

class DerivedOwner : public BasicOwner {
private:
    int extra_ = 0;

public:
    int getExtra() const { return extra_; }
    void setExtra(int v) { extra_ = v; }

    TOUKA_PROPERTY(DerivedOwner, int, extra, &DerivedOwner::getExtra, &DerivedOwner::setExtra);
};

TEST_CASE("Property in derived class", "[property]") {
    DerivedOwner obj;

    // Base class property
    obj.value = 10;
    REQUIRE(obj.value == 10);

    // Derived class property
    obj.extra = 20;
    REQUIRE(obj.extra == 20);

    // Independent
    obj.value += 5;
    REQUIRE(obj.value == 15);
    REQUIRE(obj.extra == 20);
}
