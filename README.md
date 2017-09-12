# JavaScript/C++ Rosetta Stone

I'm going to reproduce the behavior of the JavaScript language in C++. Dynamic duck typing, objects out of nothing, prototypal inheritance, lexical closures -- all of it -- in C++. As our languages get further from the machine, the more dynamic languages such as JavaScript can seem almost magical. I'm going to peek below the syntactic sugar of the JavaScript language and reproduce its behavior closer to the machine. This exercise can help JavaScript developers to understand what their language is doing under the hood and let us talk about its behavior in concrete terms rather than in metaphors. And for developers coming from other languages and new to JavaScript, this exercise can help to understand the concrete operations that lie beneath such a dynamic language.

<!-- MarkdownTOC autolink=true bracket=round style=ordered -->

1. [Dynamic types](#dynamic-types)
    1. [Variant](#variant)
    1. [Any](#any)
1. [Objects](#objects)
1. [Arrays](#arrays)
1. [Prototypal inheritance](#prototypal-inheritance)
1. [Variadic functions](#variadic-functions)
    1. [Mixed-type arguments](#mixed-type-arguments)
1. ["This"](#this)
1. [Closures](#closures)
1. [Function objects](#function-objects)
1. [Lexical environment and scope chains](#lexical-environment-and-scope-chains)
1. [Capture by reference vs by value](#capture-by-reference-vs-by-value)
    1. [`let` and capturing by value](#let-and-capturing-by-value)
1. [Garbage collection](#garbage-collection)
1. [Classes](#classes)
    1. [Factory functions](#factory-functions)
    1. [Prototype chains](#prototype-chains)
    1. [Constructor functions](#constructor-functions)
1. [That's all folks! \(...for now\)](#thats-all-folks-for-now)
1. [Copyright](#copyright)

<!-- /MarkdownTOC -->

## Dynamic types

First up, dynamic types. JavaScript, of course, is dynamically typed. A variable can be assigned and reassigned any type of value. C++, on the other hand, is statically typed. If you declare a variable to be an `int`, then C++ will allocate a fixed-size block of memory, maybe 4 bytes, which means that later trying to reassign an 8-byte double or a 32-byte string into that variable wouldn't work because there simply isn't enough space.

### Variant

One way around that problem is to allocate as much space as the largest of several types. C++ supports this with a feature called unions. The union of a 4-byte int, an 8-byte double, and a 32-byte string would yield a 32-byte data type -- as large as the largest member. This means a variable of that union type could be reassigned at any time an int or a double or a string.

It's dangerously easy, however, to assign an int, for example, and then try to read and interpret that binary data as if it were a string, so for safety, we wrap and control the usage of a union with a well behaved abstraction, a user-defined type that we traditionally name `variant`.

###### JavaScript
```javascript
let x;

x = true;
x = 42;
x = "Hello";
```

###### C++
```c++
variant<bool, int, string> x;

x = true;
x = 42;
x = "Hello"s;
```

### Any

But because the size of a union is at least as large as its largest member, space is wasted. A variable that needs only 1 byte for a bool might nonetheless allocate 32 bytes just in case we later wanted to assign a string into it. But also as bad, we have to know about and list every type that might be assigned to the variant. Ideally we'd like to be able to assign any arbitrary type. That brings us to another user-defined type named, unsurprisingly, `any`. It works by allocating the value on the free store and using it through a pointer. If all we need is a 1-byte bool, then it dynamically allocates and points to just 1 byte. Later if we reassign a string into that variable, then it frees the 1-byte bool and dynamically allocates and points to 32 bytes for the string. Now we don't need to know about every type ahead of time, and it can store (nearly) any arbitrary type we want.

###### JavaScript
```javascript
class SomeArbitraryType {}

let x;

x = true;
x = 42;
x = "Hello";
x = new SomeArbitraryType();
```

###### C++
```c++
class Some_arbitrary_type {};

any x;

x = true;
x = 42;
x = "Hello"s;
x = Some_arbitrary_type{};
```

## Objects

In JavaScript, an object is a collection of key-value pairs. In other languages, a collection of key-value pairs is instead known as an associative array, or a map, or a dictionary, and it's often implemented as a hash table. Which means we can reproduce JavaScript's objects simply by instantiating a hash table. In C++, the standard hash table type is named `unordered_map`.


###### JavaScript
```javascript
let myCar = {
    make: "Ford",
    model: "Mustang",
    year: 1969
};
```

###### C++
```c++
unordered_map<string, any> my_car {
    {"make", "Ford"s},
    {"model", "Mustang"s},
    {"year", 1969}
};
```

I'll admit this revelation in particular ruined some of JavaScript's mystique for me. We in the JavaScript community often tout that JavaScript can [create objects ex nihilo ("out of nothing")](https://en.wikipedia.org/wiki/Prototype-based_programming#Object_construction), something we're told few languages can do. But I've realized this isn't a *technical* achievement; it's a *branding* achievement. Every language I'm aware of can create hash tables ex nihilo, and it seems JavaScript simply re-branded hash tables as the generic object.

## Arrays

In JavaScript, an array is itself an object -- in the JavaScript sense of the word. Which is to say, a JavaScript array is a hash table where the string keys we insert just happen to look like integer indexes.

###### JavaScript
```javascript
let fruits = ["Mango", "Apple", "Orange"];
```

###### C++
```c++
unordered_map<string, any> fruits {
    {"0", "Mango"s},
    {"1", "Apple"s},
    {"2", "Orange"s}
};
```

And, of course, since arrays are objects, we can still assign string keys that *don't* look like integer indexes into our array.

###### JavaScript
```javascript
let fruits = ["Mango", "Apple", "Orange"];
fruits.model = "Mustang";
```

###### C++
```c++
unordered_map<string, any> fruits {
    {"0", "Mango"s},
    {"1", "Apple"s},
    {"2", "Orange"s}
};
fruits["model"] = "Mustang"s;
```

## Prototypal inheritance

Next up, the now famous prototypal inheritance. I'm assuming my audience here already knows that [JavaScript objects delegate to other objects](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Guide/Details_of_the_Object_Model). Since JavaScript's objects are ultimately hash tables, we can likewise describe JavaScript's prototypal inheritance as hash tables delegating element access to other hash tables.

This time, unfortunately, C++ does not have a ready-made type for this task. We'll have to make it ourselves. We in the JavaScript community sometimes say it's easy for prototypal to emulate classical but hard for classical to emulate prototypal, but implementing prototypal inheritance turned out to be far simplier than even I originally expected. Up to this point, we've been using `unordered_map` as our equivalent to JavaScript's objects. Now we're going to extend that type to add the so-called `[[Prototype]]` link, and we're going to override the element access operator so it will delegate access misses to that link.

###### C++
```c++
class Delegating_unordered_map : private unordered_map<string, any> {
    public:
        Delegating_unordered_map* __proto__ {};

        auto find_in_chain(const string& key) {
            // Check own property
            auto found_value = find(key);
            if (found_value != end()) return found_value;

            // Else, delegate to prototype
            if (__proto__) {
                auto found_value = __proto__->find_in_chain(key);
                if (found_value != __proto__->end()) return found_value;
            }

            return end();
        }

        any& operator[](const string& key) {
            auto found_value = find_in_chain(key);
            if (found_value != end()) return found_value->second;

            // Else, super call, which will create and return an empty `any`
            return unordered_map<string, any>::operator[](key);
        }

        // Borrow constructor
        using unordered_map<string, any>::unordered_map;
};
```

With that done, let's take a look at prototypal inheritance side-by-side in JavaScript and C++.

###### JavaScript
```javascript
let o = {a: 1, b: 2};
let oProto = {b: 3, c: 4};
Object.setPrototypeOf(o, oProto);

// Is there an "a" own property on o? Yes, and its value is 1.
o.a; // 1

// Is there a "b" own property on o? Yes, and its value is 2.
// The prototype also has a "b" property, but it's not visited.
o.b; // 2

// Is there a "c" own property on o? No, check its prototype.
// Is there a "c" own property on o.[[Prototype]]? Yes, its value is 4.
o.c; // 4

// Is there a "d" own property on o? No, check its prototype.
// Is there a "d" own property on o.[[Prototype]]? No, check its prototype.
// o.[[Prototype]].[[Prototype]] is null, stop searching.
// No property found, return undefined.
o.d; // undefined
```

###### C++
```c++
Delegating_unordered_map o {{"a", 1}, {"b", 2}};
Delegating_unordered_map o_proto {{"b", 3}, {"c", 4}};
o.__proto__ = &o_proto;

// Is there an "a" own property on o? Yes, and its value is 1.
o["a"]; // 1

// Is there a "b" own property on o? Yes, and its value is 2.
// The prototype also has a "b" property, but it's not visited.
o["b"]; // 2

// Is there a "c" own property on o? No, check its prototype.
// Is there a "c" own property on o.__proto__? Yes, its value is 4.
o["c"]; // 4

// Is there a "d" own property on o? No, check its prototype.
// Is there a "d" own property on o.__proto__? No, check its prototype.
// o.__proto__.__proto__ is null, stop searching.
// No property found, return undefined.
o["d"]; // undefined (an empty `any`)
```

We've finally settled on the finished object structure to reproduce JavaScript's behavior (or as close as we'll get here, anyway), so for convenience in the rest of the samples, I'm going to alias that object type to a shorter, friendlier name.

###### C++
```c++
using js_object = Delegating_unordered_map;
```

## Variadic functions

Variadic functions are functions that take a variable number of arguments. In JavaScript, *every* function is variadic. You can call any function with any number of arguments, and they will all be available in a special array-like variable called `arguments`. C++, on the other hand, needs to know the exact number and types of arguments to be able to call a function correctly. But as it turns out, it's simple to reproduce JavaScript's behavior. Our C++ functions could accept just a single parameter, an array of `any`s, and we can name that parameter `arguments`. In C++, the standard dynamically-sized array type is named `vector`.

###### JavaScript
```javascript
function plusAll() {
    let sum = 0;
    for (let i = 0; i < arguments.length; i++) {
        sum += arguments[i];
    }
    return sum;
}

plusAll(4, 8); // 12
plusAll(4, 8, 15, 16, 23, 42); // 108
```

###### C++
```c++
any plus_all(vector<any> arguments) {
    auto sum = 0;
    for (auto i = 0; i < arguments.size(); ++i) {
        sum += any_cast<int>(arguments[i]);
    }
    return sum;
}

plus_all({4, 8}); // 12
plus_all({4, 8, 15, 16, 23, 42}); // 108
```

Admittedly both of these code samples were written to highlight the similarities, but neither adheres to what each language would consider good style. The JavaScript sample should use rest parameters and reduce, and the C++ sample should use iterators and standard algorithms.


###### JavaScript
```javascript
function plusAll(...args) {
    return args.reduce((accumulator, currentValue) => {
        return accumulator + currentValue;
    }, 0);
}
```

###### C++
```c++
any plus_all(vector<any> arguments) {
    return accumulate(
        arguments.begin(), arguments.end(), 0,
        [] (auto accumulator, auto current_value) {
            return accumulator + any_cast<int>(current_value);
        }
    );
}
```

### Mixed-type arguments

You may have noticed that the C++ version assumes every item in the argument list is an `int`. In JavaScript, the arguments could have been a mix of strings and numbers, but the C++ version, as currently written, will only add numbers. How does JavaScript pull this off? Turns out [the addition operator does some type checking](https://www.ecma-international.org/ecma-262/7.0/index.html#sec-addition-operator-plus). Every time we use the `+` operator, JavaScript will check if either operand is a string, and if so, it will do concatention. Otherwise, it will do numeric addition.

###### JavaScript
```javascript
function plusAll(...args) {
    return args.reduce((accumulator, currentValue) => {
        return accumulator + currentValue;
    }, 0);
}

plusAll(4, 8, "!", 15, 16, 23, 42); // "12!15162342"
```

###### C++
```c++
any js_plus(const any& lval, const any& rval) {
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

any plus_all(vector<any> arguments) {
    return accumulate(
        arguments.begin(), arguments.end(), any{0},
        [] (auto accumulator, auto current_value) {
            return js_plus(accumulator, current_value);
        }
    );
}

plus_all({4, 8, "!"s, 15, 16, 23, 42}); // "12!15162342"
```

## "This"

Although `this` is handled separately from other parameters, nonetheless in a lot of ways, JavaScript's `this` behaves just like a parameter, albeit one that is often passed and received implicitly. We can reproduce JavaScript's behavior by adding one extra parameter before our arguments list, and we can name that parameter `this_`. The trailing underscore is there because `this` is a reserved name in C++.

###### JavaScript
```javascript
function add(c, d) {
    return this.a + this.b + c + d;
}

let o = {a: 1, b: 3};

// The first parameter is the object to use as
// "this"; subsequent parameters are passed as
// arguments in the function call
add.call(o, 5, 7); // 1 + 3 + 5 + 7 = 16

// The first parameter is the object to use as
// "this"; the second is an array whose
// elements are used as the arguments in the function call
add.apply(o, [10, 20]); // 1 + 3 + 10 + 20 = 34
```

###### C++
```c++
any add(any this_, vector<any> arguments) {
    return (
        any_cast<int>(any_cast<js_object>(this_)["a"]) +
        any_cast<int>(any_cast<js_object>(this_)["b"]) +
        any_cast<int>(arguments[0]) +
        any_cast<int>(arguments[1])
    );
}

js_object o {{"a", 1}, {"b", 3}};

// The first parameter is the object to use as
// "this"; the second is an array whose
// elements are used as the arguments in the function call
add(o, {5, 7}); // 1 + 3 + 5 + 7 = 16
add(o, {10, 20}); // 1 + 3 + 10 + 20 = 34
```

The way we now call our C++ functions exactly matches JavaScript's `apply`, where the first argument is the `this` value and the second argument is an array that lists all other arguments. We're sometimes told that using `this` is inherently stateful, but at the end of the day, `this` is just a parameter. If we want our object methods to be pure, for example, then we can choose to not mutate `this` just like we would choose to not mutate any other parameter.

## Closures

Conceptually, a closure is a function with non-local ([free](https://en.wikipedia.org/wiki/Free_variables_and_bound_variables)) variables. The concrete technique we use to implement that concept is a structure containing a function and an environment record. Or, put another way, a closure is an object with a single member function and private data. When we execute that function, it can access the "captured" variables from the environment stored in the closure object's private data.

Depending on the language, we can make the closure object itself callable. You may have already seen callable objects in other languages. Python, for example, lets you define a [`__call__`](https://docs.python.org/3/reference/datamodel.html#object.__call__) method, and PHP has a special [`__invoke`](http://php.net/manual/en/language.oop5.magic.php#object.invoke) method. In C++, we define an [`operator()`](http://en.cppreference.com/w/cpp/language/operators#Function_call_operator) member function. And it's these functions that are executed when an object is called as if it were a function.

As you read these descriptions, keep in mind that C++'s notion of a function is not the same as JavaScript's notion of a function. In C++, a function can be distinct from a closure and not associated with any environment. But in JavaScript, *every* function has an associated environment. What JavaScript calls a function isn't the function *part* of a closure. Rather, what JavaScript calls a function is the closure itself -- a callable object that contains an environment.

###### JavaScript
```javascript
function outside(x) {
    function inside(y) {
        return x + y;
    }

    return inside;
}

let fnInside = outside(3);
fnInside(5); // 8

outside(3)(5); // 8
```

###### C++
```c++
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

    // This is our closure, an instance of the above class, a callable object
    // that is constructed with and stores a value from its environment
    Inside inside {x};

    return inside;
}

auto fn_inside = outside(3);
fn_inside(5); // 8

outside(3)(5); // 8
```

This C++ sample demonstrates what's ultimately happening under the hood, but admittedly it's verbose. In 2011, C++ introduced a lambda expression syntax, which produces a closure object and is syntactic sugar for defining and instantiating a callable class, same as we did above.

###### C++
```c++
auto outside(int x) {
    // This is our closure, a callable object that
    // stores a value from its environment
    auto inside = [x] (int y) {
        return x + y;
    };

    return inside;
}

auto fn_inside = outside(3);
fn_inside(5); // 8

outside(3)(5); // 8
```

In these C++ samples, I defined only `inside` as a callable object and I left `outside` as a plain function because that's all that was necessary to reproduce JavaScript's behavior for this particular code sample. But in truth, *every* function in JavaScript is a callable object. *Every* function is a closure.

## Function objects

In fact, JavaScript's functions aren't just objects in the C++ sense of the word, they're also objects in the JavaScript sense of the word. Which means JavaScript's functions are callable hash tables. We can both invoke them as a function *and* assign to them key-value pairs. To reproduce this behavior in C++, we'll extend the `Delegating_unordered_map` we made earlier and specialize it to also be callable.

###### C++
```c++
class Callable_delegating_unordered_map : public Delegating_unordered_map {
    function<any(any, vector<any>)> function_body_;

    public:
        Callable_delegating_unordered_map(function<any(any, vector<any>)> function_body) :
            function_body_ {function_body}
        {}

        auto operator()(any this_ = {}, vector<any> arguments = {}) {
            return function_body_(this_, arguments);
        }
};
```

With that done, let's take a look at callable hash tables.

###### JavaScript
```javascript
function square(number) {
    return number * number;
}

square.make = "Ford";
square.model = "Mustang";
square.year = 1969;

square(4); // 16
```

###### C++
```c++
Callable_delegating_unordered_map square {[] (any this_, vector<any> arguments) {
    return any_cast<int>(arguments[0]) * any_cast<int>(arguments[0]);
}};

square["make"] = "Ford"s;
square["model"] = "Mustang"s;
square["year"] = 1969;

square(nullptr, {4}); // 16
```

We've finally settled on the finished function object structure to reproduce JavaScript's behavior (or as close as we'll get here, anyway), so for convenience in the rest of the samples, I'm going to alias that object type to a shorter, friendlier name.

###### C++
```c++
using js_function = Callable_delegating_unordered_map;
```

## Lexical environment and scope chains

The C++ closure examples from earlier do all we need to capture *individual* variables from any scope level, but that's not quite how JavaScript works. In JavaScript, what we see as local variables are actually entries in an object called an environment record, which pairs variable names with values. And each environment delegates accesses to the environment record of the next outer scope.

That sounds familiar. We've seen this pattern before. As it turns out, the `Delegating_unordered_map` we made to reproduce prototypal inheritance is exactly what we need to also reproduce JavaScript's scope chains.

###### JavaScript
```javascript
let globalVariable = "xyz";

function f() {
    let localVariable = true;

    function g() {
        let anotherLocalVariable = 123;

        // All variables of surrounding scopes are accessible
        localVariable = false;
        globalVariable = "abc";
    }
}
```

###### C++
```c++
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

        // All variables of surrounding scopes are accessible
        g_environment["localVariable"] = false;
        g_environment["globalVariable"] = "abc"s;

        return any{};
    }};

    return any{};
}};
```

## Capture by reference vs by value

In C++, a closure's environment can store either a copy of values in scope or a reference to values in scope. But JavaScript's closures will only ever capture by reference. The difference is especially apparent in the famous problem with creating closures inside a loop.

###### JavaScript
```javascript
let functions = [];

for (var i = 0; i < 3; i++) {
    // Every closure we push captures a reference to the same "i"
    functions.push(() => {
        console.log(i);
    });
}

// 3, 3, 3
functions.forEach(fn => {
    fn();
});
```

###### C++
```c++
vector<js_function> functions_by_value;
vector<js_function> functions_by_ref;

for (auto& i = *(new int{0}); i < 3; ++i) {
    // This closure will capture the value of "i"
    // at the moment the closure is created
    functions_by_value.push_back(js_function{[i] (any this_, vector<any> arguments) {
        cout << i;
        return any{};
    }});

    // This closure will capture a reference to the same "i"
    functions_by_ref.push_back(js_function{[&i] (any this_, vector<any> arguments) {
        cout << i;
        return any{};
    }});
}

// 0, 1, 2
for_each(functions_by_value.begin(), functions_by_value.end(), [] (auto fn) {
    fn();
});

// 3, 3, 3
for_each(functions_by_ref.begin(), functions_by_ref.end(), [] (auto fn) {
    fn();
});
```

### `let` and capturing by value

Recently JavaScript added the [`let` statement](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Statements/let), which brings block scoping to the language. But when it comes to for-loops, then `let` does more than mere block scoping. At every iteration of the loop, [JavaScript creates a brand new *copy* of the loop variable](http://www.ecma-international.org/ecma-262/7.0/index.html#sec-createperiterationenvironment). That's not normal block scoping behavior. Rather, this seems to emulate capture by value in a language that otherwise only supports capture by reference. A JavaScript closure within the loop will still capture by reference, but it won't capture the loop variable itself; it will capture a per-iteration *copy* of the loop variable.

###### JavaScript
```javascript
let functions = [];

for (let i = 0; i < 3; i++) {
    // Every closure we push captures a reference to a per-iteration copy of "i"
    functions.push(() => {
        console.log(i);
    });
}

// 0, 1, 2
functions.forEach((fn) => {
    fn();
});
```

###### C++
```c++
vector<js_function> functions;

for (auto i = 0; i < 3; ++i) {
    // Create a per-iteration copy of "i"
    auto& i_copy = *(new int{i});

    // Every closure we push captures a reference to a per-iteration copy of "i"
    functions.push_back(js_function{[&i_copy] (any this_, vector<any> arguments) {
        cout << i_copy;
        return any{};
    }});
}

// 0, 1, 2
for_each(functions.begin(), functions.end(), [] (auto fn) {
    fn();
});
```

## Garbage collection

One problem with capturing by reference is we now have to be conscientious of memory allocation lifetimes. Normally the lifetime of a value in memory is tied to a particular block, such as a for-loop or a function, but since the closures in the last sample will outlive their block, then any values they capture by reference will also need to outlive their block. In the last C++ sample, we avoided that issue by allowing the per-iteration copies to stay alive forever. But in JavaScript, memory is garbage collected. The lifetime of *any* value isn't necessarily tied to any particular block. If something somewhere retains a reference to a value, then that value will be kept alive.

In C++, a guiding principle for the language is you don't pay for what you don't use, so there are several garbage collection strategies available -- such as [`unique_ptr`](http://en.cppreference.com/w/cpp/memory/unique_ptr), [`shared_ptr`](http://en.cppreference.com/w/cpp/memory/shared_ptr), and [`deferred_ptr`](https://github.com/hsutter/gcpp) -- each with incrementally more features but also with more overhead. Since we're reproducing the behavior of JavaScript, we'll go straight to `deferred_ptr` for garbage collection.

Here's the last C++ sample again but this time with a garbage collector to alleviate the object lifetime issue.

###### C++
```c++
deferred_heap my_heap;

vector<js_function> functions;

for (auto i = 0; i < 3; ++i) {
    // Create a per-iteration copy of "i"
    auto i_copy = my_heap.make<int>(i);

    // Every closure we push captures a reference to a per-iteration copy of "i"
    functions.push_back(js_function{[i_copy] (any this_, vector<any> arguments) {
        cout << *i_copy;
        return any{};
    }});
}

// 0, 1, 2
for_each(functions.begin(), functions.end(), [] (auto fn) {
    fn();
});

// Destroy and deallocate any unreachable objects
my_heap.collect();
```

In the sample above, only the per-iteration copy is garbage collected. If we want *all* of our objects, arrays, and function objects to also be garbage collected, then we'll have to make a few changes. First, we need to update the `Delegating_unordered_map` and change the raw `__proto__` pointer to the garbage collector `deferred_ptr`.

###### C++
```c++
class Delegating_unordered_map : private unordered_map<string, any> {
    public:
        deferred_ptr<Delegating_unordered_map> __proto__ {};

        // ...
};
```

And second, when we want a new `js_object`, we'll have to ask the garbage collector to make it for us by calling, for example, `my_heap.make<js_object>( js_object{{"a", 1}, {"b", 2}} )`. And we'll have to refer to those garbage collected object types as `deferred_ptr<js_object>`. That can be just a little tedious and verbose, so let's define a couple helper functions and a couple type aliases to be a touch cleaner.

###### C++
```c++
auto make_js_object(const js_object& obj = {}) {
    return my_heap.make<js_object>(obj);
}

auto make_js_function(const js_function& func) {
    return my_heap.make<js_function>(func);
}

using js_object_ref = deferred_ptr<js_object>;
using js_function_ref = deferred_ptr<js_function>;
```

You may notice I aliased a type name with the suffix "ptr" to a name with the suffix "ref". Admittedly the terminology here gets a bit awkward. What JavaScript calls a reference behaves like what C++ calls a pointer. C++ also has a feature it calls a reference, but it's different than what JavaScript calls a reference.

## Classes

JavaScript has a multitude of styles for creating objects, but despite the variety and how different the syntax for each may look, they build on each other in incremental steps.

### Factory functions

The first and simplest way to create a large set of objects that share a common interface and implementation is a factory function. We just create a `js_object` "ex nilo" inside a function and return it. This lets us create the same type of object multiple times and in multiple places just by invoking a function.

###### JavaScript
```javascript
function thing() {
    return {
        x: 42,
        y: 3.14,
        f: function() {},
        g: function() {}
    };
}

let o = thing();
```

###### C++
```c++
auto thing = make_js_function({[] (any this_, vector<any> arguments) {
    return make_js_object({
        {"x", 42},
        {"y", 3.14},
        {"f", make_js_function({[] (any this_, vector<any> arguments) { return any{}; }})},
        {"g", make_js_function({[] (any this_, vector<any> arguments) { return any{}; }})}
    });
}});

auto o = (*thing)();
```

But there's a drawback. This approach can cause memory bloat because each object contains its own unique copy of each function. Ideally we want every object to share just one copy of its functions.

### Prototype chains

We can take advantage of JavaScript's prototypal inheritance and change our factory function so that each object it creates contains only the data unique to that particular object, and delegate all other property requests to a single, shared object.

###### JavaScript
```javascript
let thingPrototype = {
    f: function() {},
    g: function() {}
};

function thing() {
    let o = {
        x: 42,
        y: 3.14
    };

    Object.setPrototypeOf(o, thingPrototype);

    return o;
}

let o = thing();
```

###### C++
```c++
auto thing_prototype = make_js_object({
    {"f", make_js_function({[] (any this_, vector<any> arguments) { return any{}; }})},
    {"g", make_js_function({[] (any this_, vector<any> arguments) { return any{}; }})}
});

auto thing = make_js_function({[=] (any this_, vector<any> arguments) {
    auto o = make_js_object({
        {"x", 42},
        {"y", 3.14}
    });

    o->__proto__ = thing_prototype;

    return o;
}});

auto o = (*thing)();
```

In fact, this is such a common pattern in JavaScript that the language has built-in support for it. We don't need to create our own shared object (the prototype object). Instead, a prototype object is created for us automatically, attached to every function under the property name `prototype`, and we can put our shared data there.

###### JavaScript
```javascript
function thing() {
    let o = {
        x: 42,
        y: 3.14
    };

    Object.setPrototypeOf(o, thing.prototype);

    return o;
}

thing.prototype.f = function() {};
thing.prototype.g = function() {};

let o = thing();
```

###### C++
```c++
js_function_ref thing = make_js_function({[=] (any this_, vector<any> arguments) {
    auto o = make_js_object({
        {"x", 42},
        {"y", 3.14}
    });

    o->__proto__ = any_cast<js_object_ref>((*thing)["prototype"]);

    return o;
}});

(*thing)["prototype"] = make_js_object({
    {"f", make_js_function({[] (any this_, vector<any> arguments) { return any{}; }})},
    {"g", make_js_function({[] (any this_, vector<any> arguments) { return any{}; }})}
});

auto o = (*thing)();
```

But there's a drawback. This is going to result in some repetition during object creation in every such delegating-to-prototype-factory-function.

### Constructor functions

JavaScript has some built-in support to isolate the repetition. The `new` keyword will create an object that delegates to some other arbitrary function's prototype. Then it will invoke that function to perform initialization with the newly created object as the `this` argument. And finally it will return the object.

###### JavaScript
```javascript
function Thing() {
    this.x = 42;
    this.y = 3.14;
}

Thing.prototype.f = function() {};
Thing.prototype.g = function() {};

let o = new Thing();
```

###### C++
```c++
auto js_new(js_function_ref constructor, vector<any> arguments = {}) {
    auto o = make_js_object();
    o->__proto__ = any_cast<js_object_ref>((*constructor)["prototype"]);

    (*constructor)(o, arguments);

    return o;
}

auto Thing = make_js_function({[] (any this_, vector<any> arguments) {
    (*any_cast<js_object_ref>(this_))["x"] = 42;
    (*any_cast<js_object_ref>(this_))["y"] = 3.14;

    return any{};
}});

(*Thing)["prototype"] = make_js_object({
    {"f", make_js_function({[] (any this_, vector<any> arguments) { return any{}; }})},
    {"g", make_js_function({[] (any this_, vector<any> arguments) { return any{}; }})}
});

auto o = js_new(Thing);
```

## That's all folks! (...for now)

I hope you found this look under the hood just as interesting and enlightening as I did.

I'd like this to become a living and growing document. That's why I'm hosting it here on GitHub rather than on some blog site. I'm always looking to make the explanations better and the code clearer, as well as to lift the hood of even more dark corners of the language. I welcome and appreciate any contributions to make it better.

## Copyright

Copyright 2017 Jeff Mott. Creative Commons [Attribution-NonCommercial 4.0 International](https://creativecommons.org/licenses/by-nc/4.0/) license.
