#define BOOST_TEST_MODULE JS_CPP_rosetta_stone_samples_test

// (MSVC) Suppress warnings from dependencies; they're not ours to fix
#pragma warning(push, 0)

    #include <algorithm>
    #include <functional>
    #include <numeric>
    #include <sstream>
    #include <string>
    #include <unordered_map>
    #include <vector>
    #include <boost/any.hpp>
    #include <boost/test/included/unit_test.hpp>
    #include <boost/variant.hpp>
    #include <deferred_heap.h>
    #include <gsl/gsl>

#pragma warning(pop)

using std::accumulate;
using std::for_each;
using std::function;
using std::string;
using namespace std::string_literals;
using std::stringstream;
using std::to_string;
using std::unordered_map;
using std::vector;
using boost::any;
using boost::any_cast;
using boost::get;
using boost::variant;
using gcpp::deferred_heap;

BOOST_AUTO_TEST_CASE(variant_test) {
    variant<bool, int, string> x;

    x = true;

    BOOST_TEST(get<bool>(x) == true);

    x = 42;

    BOOST_TEST(get<int>(x) == 42);

    x = "Hello"s;

    BOOST_TEST(get<string>(x) == "Hello"s);
}

BOOST_AUTO_TEST_CASE(any_test) {
    class Some_arbitrary_type {};

    any x;

    x = true;

    BOOST_TEST(any_cast<bool>(x) == true);

    x = 42;

    BOOST_TEST(any_cast<int>(x) == 42);

    x = "Hello"s;

    BOOST_TEST(any_cast<string>(x) == "Hello"s);

    x = Some_arbitrary_type{};

    any_cast<Some_arbitrary_type>(x); // will throw if fails
}

BOOST_AUTO_TEST_CASE(objects_test) {
    unordered_map<string, any> myCar {
        {"make", "Ford"s},
        {"model", "Mustang"s},
        {"year", 1969}
    };

    BOOST_TEST(any_cast<string>(myCar["make"]) == "Ford"s);
    BOOST_TEST(any_cast<string>(myCar["model"]) == "Mustang"s);
    BOOST_TEST(any_cast<int>(myCar["year"]) == 1969);
}

BOOST_AUTO_TEST_CASE(arrays_test) {
    unordered_map<string, any> fruits {
        {"0", "Mango"s},
        {"1", "Apple"s},
        {"2", "Orange"s}
    };

    BOOST_TEST(any_cast<string>(fruits["0"]) == "Mango"s);
    BOOST_TEST(any_cast<string>(fruits["1"]) == "Apple"s);
    BOOST_TEST(any_cast<string>(fruits["2"]) == "Orange"s);

    fruits["model"] = "Mustang"s;

    BOOST_TEST(any_cast<string>(fruits["model"]) == "Mustang"s);
}

class Delegating_unordered_map : private unordered_map<string, any> {
    public:
        Delegating_unordered_map* __proto__ {};

        any& operator[](const string& key) {
            // Check own property
            const auto found_value = find(key);
            if (found_value != end()) {
                return found_value->second;
            }

            // Else, delegate to prototype
            if (__proto__) {
                return (*__proto__)[key];
            }

            // Else, super call, which will create and return an empty `any`
            return unordered_map<string, any>::operator[](key);
        }

        // Borrow constructor
        using unordered_map<string, any>::unordered_map;
};

BOOST_AUTO_TEST_CASE(prototypal_inheritance_test) {
    Delegating_unordered_map o {{"a", 1}, {"b", 2}};
    Delegating_unordered_map o_proto {{"b", 3}, {"c", 4}};
    o.__proto__ = &o_proto;

    // Is there an "a" own property on o? Yes, and its value is 1.
    BOOST_TEST(any_cast<int>(o["a"]) == 1);

    // Is there a "b" own property on o? Yes, and its value is 2.
    // The prototype also has a "b" property, but it's not visited.
    BOOST_TEST(any_cast<int>(o["b"]) == 2);

    // Is there a "c" own property on o? No, check its prototype.
    // Is there a "c" own property on o.__proto__? Yes, its value is 4.
    BOOST_TEST(any_cast<int>(o["c"]) == 4);

    // Is there a "d" own property on o? No, check its prototype.
    // Is there a "d" own property on o.__proto__? No, check its prototype.
    // o.__proto__.__proto__ is null, stop searching.
    // No property found, return undefined.
    BOOST_TEST(o["d"].empty());
}

using js_object = Delegating_unordered_map;

namespace variadic {
    any plus_all(vector<any> arguments) {
        auto sum = 0;
        for (auto i = 0; i < arguments.size(); ++i) {
            sum += any_cast<int>(arguments[i]);
        }
        return sum;
    }

    BOOST_AUTO_TEST_CASE(variadic_test) {
        BOOST_TEST(any_cast<int>(plus_all({4, 8})) == 12);
        BOOST_TEST(any_cast<int>(plus_all({4, 8, 15, 16, 23, 42})) == 108);
    }
}

namespace variadic_stl {
    any plus_all(vector<any> arguments) {
        return accumulate(
            arguments.begin(), arguments.end(), 0,
            [] (auto accumulator, auto current_value) {
                return accumulator + any_cast<int>(current_value);
            }
        );
    }

    BOOST_AUTO_TEST_CASE(variadic_stl_test) {
        BOOST_TEST(any_cast<int>(plus_all({4, 8})) == 12);
        BOOST_TEST(any_cast<int>(plus_all({4, 8, 15, 16, 23, 42})) == 108);
    }
}

any js_plus(any lval, any rval) {
    // If either operand is a string...
    if (
        lval.type() == typeid(string) ||
        rval.type() == typeid(string)
    ) {
        // Convert both operands to a string and do concatenation
        auto lval_str = (
            lval.type() != typeid(string) ?
                to_string(any_cast<int>(lval)) :
                any_cast<string>(lval)
        );
        auto rval_str = (
            rval.type() != typeid(string) ?
                to_string(any_cast<int>(rval)) :
                any_cast<string>(rval)
        );

        return lval_str + rval_str;
    }

    // Else, numeric addition
    return any_cast<int>(lval) + any_cast<int>(rval);
}

namespace variadic_mixedtype {
    any plus_all(vector<any> arguments) {
        return accumulate(
            arguments.begin(), arguments.end(), any{0},
            [] (auto accumulator, auto current_value) {
                return js_plus(accumulator, current_value);
            }
        );
    }

    BOOST_AUTO_TEST_CASE(variadic_mixedtype_test) {
        BOOST_TEST(any_cast<string>(plus_all({4, 8, "!"s, 15, 16, 23, 42})) == "12!15162342"s);
    }
}

namespace this_ {
    any add(any this_, vector<any> arguments) {
        return (
            any_cast<int>(any_cast<js_object>(this_)["a"]) +
            any_cast<int>(any_cast<js_object>(this_)["b"]) +
            any_cast<int>(arguments[0]) +
            any_cast<int>(arguments[1])
        );
    }

    BOOST_AUTO_TEST_CASE(this_test) {
        js_object o {{"a", 1}, {"b", 3}};

        // The first parameter is the object to use as
        // "this"; the second is an array whose
        // elements are used as the arguments in the function call
        BOOST_TEST(any_cast<int>(add(o, {5, 7})) == 16);
        BOOST_TEST(any_cast<int>(add(o, {10, 20})) == 34);
    }
}

namespace closures {
    auto outside(int x) {
        // A class that privately stores "x" and
        // can be called as if it were a function
        class Inside {
            int x_;

            public:
                Inside(int x) : x_ {x} {}

                auto operator()(int y) {
                    return x_ + y;
                }
        };

        // This is our closure, an instance of the above class,
        // a callable object that remembers a value from its environment
        Inside inside {x};

        return inside;
    }

    BOOST_AUTO_TEST_CASE(closures_test) {
        auto fn_inside = outside(3);
        BOOST_TEST(fn_inside(5) == 8);

        BOOST_TEST(outside(3)(5) == 8);
    }
}

namespace closures_lambda {
    auto outside(int x) {
        // This is our closure, a callable object that
        // remembers a value from its environment
        auto inside = [x] (int y) {
            return x + y;
        };

        return inside;
    }

    BOOST_AUTO_TEST_CASE(closures_lambda_test) {
        auto fn_inside = outside(3);
        BOOST_TEST(fn_inside(5) == 8);

        BOOST_TEST(outside(3)(5) == 8);
    }
}

class Callable_delegating_unordered_map : public Delegating_unordered_map {
    function<any(any, vector<any>)> function_body_;

    public:
        Callable_delegating_unordered_map(function<any(any, vector<any>)> function_body) :
            function_body_ {function_body}
        {}

        any operator()(any this_ = {}, vector<any> arguments = {}) {
            return function_body_(this_, arguments);
        }
};

BOOST_AUTO_TEST_CASE(function_object) {
    Callable_delegating_unordered_map square {[] (any this_, vector<any> arguments) {
        return any_cast<int>(arguments[0]) * any_cast<int>(arguments[0]);
    }};

    square["make"] = "Ford"s;
    square["model"] = "Mustang"s;
    square["year"] = 1969;

    BOOST_TEST(any_cast<int>(square(nullptr, {4})) == 16);
}

using js_function = Callable_delegating_unordered_map;

BOOST_AUTO_TEST_CASE(scope_chains_test) {
    Delegating_unordered_map global_environment;
    global_environment["globalVariable"] = "xyz"s;

    global_environment["f"] = js_function{[&] (any this_, vector<any> arguments) {
        Delegating_unordered_map f_environment;
        f_environment.__proto__ = &global_environment;

        f_environment["localVariable"] = true;

        f_environment["g"] = js_function{[&] (any this_, vector<any> arguments) {
            Delegating_unordered_map g_environment;
            g_environment.__proto__ = &f_environment;

            g_environment["anotherLocalVariable"] = 123;

            BOOST_TEST(any_cast<string>(g_environment["globalVariable"]) == "xyz"s);
            BOOST_TEST(any_cast<bool>(g_environment["localVariable"]) == true);
            BOOST_TEST(any_cast<int>(g_environment["anotherLocalVariable"]) == 123);

            // All variables of surrounding scopes are accessible
            g_environment["localVariable"] = false;
            g_environment["globalVariable"] = "abc"s;

            BOOST_TEST(any_cast<bool>(g_environment["localVariable"]) == false);
            BOOST_TEST(any_cast<string>(g_environment["globalVariable"]) == "abc"s);

            return any{};
        }};

        any_cast<js_function>(f_environment["g"])();
        BOOST_TEST(any_cast<bool>(f_environment["localVariable"]) == false);

        return any{};
    }};

    any_cast<js_function>(global_environment["f"])();
    BOOST_TEST(any_cast<string>(global_environment["globalVariable"]) == "abc"s);
}

namespace closures_in_loop {
    stringstream cout; // mock cout

    BOOST_AUTO_TEST_CASE(closures_in_loop_test) {
        vector<js_function> functions;

        for (auto i = 0; i < 5; ++i) {
            // Every closure we push captures the value of "i"
            // at the moment the closure is created
            functions.emplace_back([i] (any this_, vector<any> arguments) {
                cout << i;
                return any{};
            });
        }

        // 0, 1, 2, 3, 4
        for_each(functions.begin(), functions.end(), [] (auto fn) {
            fn();
        });

        BOOST_TEST(cout.str() == "01234"s);
    }
}

namespace closures_byref_in_loop {
    stringstream cout; // mock cout

    BOOST_AUTO_TEST_CASE(closures_byref_in_loop_test) {
        vector<js_function> functions;

        for (auto& i = *(new int{0}); i < 5; ++i) {
            // Every closure we push captures a reference to the same "i"
            functions.emplace_back([&i] (any this_, vector<any> arguments) {
                cout << i;
                return any{};
            });
        }

        // 5, 5, 5, 5, 5
        for_each(functions.begin(), functions.end(), [] (auto fn) {
            fn();
        });

        BOOST_TEST(cout.str() == "55555"s);
    }
}

namespace closures_peritercopy_in_loop {
    stringstream cout; // mock cout

    BOOST_AUTO_TEST_CASE(closures_peritercopy_in_loop) {
        vector<js_function> functions;

        for (auto i = 0; i < 5; ++i) {
            // Create a per-iteration copy of "i"
            auto& i_copy = *(new int{i});

            // Every closure we push captures a reference to a per-iteration copy of "i"
            functions.emplace_back([&i_copy] (any this_, vector<any> arguments) {
                cout << i_copy;
                return any{};
            });
        }

        // 0, 1, 2, 3, 4
        for_each(functions.begin(), functions.end(), [] (auto fn) {
            fn();
        });

        BOOST_TEST(cout.str() == "01234"s);
    }
}

namespace garbage_collection {
    stringstream cout; // mock cout

    BOOST_AUTO_TEST_CASE(garbage_collection_test) {
        deferred_heap my_heap;

        vector<js_function> functions;

        for (auto i = 0; i < 5; ++i) {
            // Create a per-iteration copy of "i"
            auto i_copy = my_heap.make<int>(i);

            // Every closure we push captures a reference to a per-iteration copy of "i"
            functions.emplace_back([i_copy] (any this_, vector<any> arguments) {
                cout << *i_copy;
                return any{};
            });
        }

        // 0, 1, 2, 3, 4
        for_each(functions.begin(), functions.end(), [] (auto fn) {
            fn();
        });

        // Destroy and deallocate any unreachable objects
        my_heap.collect();

        BOOST_TEST(cout.str() == "01234"s);
    }
}
