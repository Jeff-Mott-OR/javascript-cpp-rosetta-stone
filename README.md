# JavaScript/C++ Rosetta Stone

I'm going to reproduce the behavior of the JavaScript language in C++. Lexical scopes, closures, prototypal inheritance, duck typing -- all of it -- in C++. I'm doing this mostly because I think it's fun and interesting. But for everyone else, this exercise will help to understand what JavaScript is doing under the hood and let us talk about its behavior in concrete terms rather than in metaphors. This might also be your first exposure to C++. It's a useful skill to have since Node itself, and any addons we might write for it, are in C++, and seeing it side-by-side with JavaScript code that does the same thing can help bridge the gap.

## Dynamic types

First up, dynamic types. JavaScript, of course, is dynamically typed. A variable can be assigned and reassigned any type of value. C++, on the other hand, is statically typed. If you declare a variable to be an `int`, then C++ will allocate a fixed-size block of memory, maybe 4 bytes, which means that later trying to reassign an 8-byte double or a 32-byte string into that variable wouldn't work because there simply isn't enough space.

#### Variant

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

#### Any

But because the size of a union is at least as large as its largest member, space is wasted. A variable that needs only 1 byte for a bool might nonetheless allocate 32 bytes just in case we later wanted to assign a string into it. But also as bad, we have to know about and list every type that might be assigned to the variant. Ideally we'd like to be able to assign any arbitrary type. That brings us to another user-defined type named, unsurprisingly, `any`. It works by dynamically allocating the value and using it through a pointer. If all we need is a 1-byte bool, then it dynamically allocates and points to just 1 byte. Later if we reassign a string into that variable, then it frees the 1-byte bool and dynamically allocates and points to 32 bytes for the string. Now we don't need to know about every type ahead of time, and it can store (nearly) any arbitrary type we want.

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
let o = {
    firstName: "Jane",
    lastName: "Doe"
};
```

###### C++
```c++
unordered_map<string, any> o {
    {"firstName", "Jane"s},
    {"lastName", "Doe"s}
};
```

I'll admit this revelation in particular ruined some of JavaScript's mystique for me. We in the JavaScript community often tout that JavaScript can create objects ex nihilo ("out of nothing"), something we're told few languages can do. But I've realized this isn't a *technical* achievement; it's a *branding* achievement. Every language I'm aware of can create hash tables ex nihilo, and it seems JavaScript simply re-branded hash tables as the generic object.

## Arrays

In JavaScript, an array is itself an object -- in the JavaScript sense of the word -- which is to say, a JavaScript array is a hash table where the string keys we insert just happen to look like integer indexes.

###### JavaScript
```javascript
let fruits = ["Apple", "Banana"];
```

###### C++
```c++
unordered_map<string, any> fruits {
    {"0", "Apple"s},
    {"1", "Banana"s}
};
```

And, of course, since arrays are objects, we can still assign string keys the *don't* look like integer indexes into our array.

###### JavaScript
```javascript
let fruits = ["Apple", "Banana"];
fruits.firstName = "Jane";
```

###### C++
```c++
unordered_map<string, any> fruits {
    {"0", "Apple"s},
    {"1", "Banana"s}
};
fruits["firstName"] = "Jane"s;
```

## Prototypal inheritance

Next up, the now famous prototypal inheritance. I'm assuming my audience here already knows that JavaScript objects delegate to other objects. Since JavaScript's objects are ultimately hash tables, we can likewise describe JavaScript's prototypal inheritance as hash tables delegating element access to other hash tables.

This time, unfortunately, C++ does not have a ready-made type for this task. We'll have to make it ourselves. But this too turned out to be far simplier than even I originally expected. We in the JavaScript community sometimes say it's easy for prototypal to emulate classical but hard for classical to emulate prototypal. It turns out that isn't true. Up to this point, we've been using `unordered_map` as our equivalent to JavaScript's objects. Now we're going to extend that type to add the so-called `[[Prototype]]` link, and we're going to override the element access operator so it will delegate access misses to that link.

###### C++
```c++
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

We've finally settled on the finished object structure to reproduce JavaScript's behavior (or as close as we'll get in this article, anyway), so for convenience in the rest of the samples, I'm going to alias that object type to a shorter, friendlier name.

###### C++
```c++
using js_object = Delegating_unordered_map;
```

## Closures

Closures are functions that remember the environment / scope / context in which they were created. But how exactly does a JavaScript function "remember" things? How does a function become stateful? The answer in hindsight is almost boringly obvious. A closure is an *object*, with a constructor and private data, that we can call *as if* it were a function. You may have already seen callable objects in other languages. Python lets you define a [`__call__`](https://docs.python.org/3/reference/datamodel.html#object.__call__) method, and PHP has a special [`__invoke`](http://php.net/manual/en/language.oop5.magic.php#object.invoke) method. In C++, we define an [`operator()`](http://en.cppreference.com/w/cpp/language/operators#Function_call_operator) member function. And it's these functions that are executed when an object is called as if it were a function.

###### JavaScript
```javascript
function makeFunc() {
    let name = "Rosetta";

    function displayName() {
        console.log(name);
    }

    return displayName;
}

let myFunc = makeFunc();
myFunc(); // "Rosetta"
```

###### C++
```c++
auto make_func() {
    auto name = "Rosetta"s;

    // A class that privately stores a name and
    // can be called as if it were a function
    class Display_name {
        string name_;

        public:
            Display_name(const string& name)
                : name_{name}
            {}

            auto operator()() {
                cout << name_;
            }
    };

    // This is our closure, an instance of the above class,
    // a callable object that remembers a value from its environment
    Display_name display_name {name};

    return display_name;
}

auto my_func = make_func();
my_func(); // "Rosetta"
```

This C++ sample demonstrates what's ultimately happening under the hood, but admittedly it's verbose. In 2011, C++ introduced a lambda syntax, which is syntactic sugar for defining and instantiating a callable class, same as we did above.

###### C++
```c++
auto make_func() {
    auto name = "Rosetta"s;

    // This is our closure, a callable object that
    // remembers a value from its environment
    auto display_name = [name] () {
        cout << name;
    };

    return display_name;
}

auto my_func = make_func();
my_func(); // "Rosetta"
```

## Lexical environment and scope chains

The C++ closure examples above does all we need to capture *individual* variables from any scope level, but that's not quite how JavaScript operates. In JavaScript, what we see as local variables are actually entries in an object called an environment record, which pairs variable names with values. And each environment delegates accesses to the environment record of the next outer scope.

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

global_environment["f"] = [&outer = global_environment] () {
    Delegating_unordered_map f_environment;
    f_environment.__proto__ = &outer;

    f_environment["localVariable"] = true;

    f_environment["g"] = [&outer = f_environment] () {
        Delegating_unordered_map g_environment;
        g_environment.__proto__ = &outer;

        g_environment["anotherLocalVariable"] = 123;

        // All variables of surrounding scopes are accessible
        g_environment["localVariable"] = false;
        g_environment["globalVariable"] = "abc"s;
    };
};
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

#### Mixed-type arguments

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

        return any{lval_str + rval_str};
    }

    // Else, numeric addition
    return any{any_cast<int>(lval) + any_cast<int>(rval)};
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

In a lot of ways, JavaScript's `this` behaves just like a parameter, albeit one that is often passed and received implicitly. We can reproduce JavaScript's behavior by adding one extra parameter before our arguments list.

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

The way we now call our C++ functions exactly matches JavaScript's `apply`, where the first argument is the `this` argument, and the second argument is an array that lists all other arguments. We've finally settled on the finished function signature to reproduce JavaScript's behavior, so for convenience in the rest of the samples, I'm going to alias that function signature to a shorter, friendlier name.

###### C++
```c++
using js_function = function<any(any, vector<any>)>;
```

#### "This" binding

Of the variety of ways JavaScript will decide what `this` will be, they can all translate to this simple `apply`-style usage.

###### JavaScript
```javascript
let o = {
    prop: 37,
    f: function() {
        return this.prop;
    }
};

var prop = 42;

// When a function is called as a method of an object,
// its "this" is set to the object the method is called on
o.f(); // 37

// When a function is called as a simple function call,
// its "this" is the global object
let f = o.f;
f(); // 42
```

###### C++
```c++
js_object o {
    {"prop", 37},
    {"f", js_function{[] (any this_, vector<any> arguments) {
        return any_cast<js_object>(this_)["prop"];
    }}}
};

global_environment["prop"] = 42;

// When a function is called as a method of an object,
// its "this" is set to the object the method is called on
any_cast<js_function>(o["f"])(o, {}); // 37

// When a function is called as a simple function call,
// its "this" is the global object
auto f = any_cast<js_function>(o["f"]);
f(global_environment, {}); // 42
```

We're sometimes told that using `this` is inherently statefull, but it isn't. At the end of the day, `this` is just a parameter. If you want your object methods to be pure, then you can choose to not mutate `this` just like you would choose to not mutate any other argument.

## Capture by reference vs by value

JavaScript's function closures capture by reference, but the earlier C++ closure sample actually captures by value. You can see the difference in this infamous trap where we create closures inside a loop.

###### JavaScript
```javascript
let functions = [];

for (var i = 0; i < 5; i++) {
    // Every closure we push captures a reference to the same "i"
    functions.push(() => {
        console.log(i);
    });
}

// 5, 5, 5, 5, 5
functions.forEach((fn) => {
    fn();
});
```

###### C++
```c++
vector<js_function> functions;

for (auto i = 0; i < 5; ++i) {
    // Every closure we push captures the value of "i"
    // at the moment the closure is created
    functions.push_back([i] (any this_, vector<any> arguments) {
        cout << i;
        return any{};
    });
}

// 0, 1, 2, 3, 4
for_each(functions.begin(), functions.end(), [] (auto fn) {
    fn(nullptr, {});
});
```

If the C++ closures instead captured by reference -- which we can do by putting an `&` in the capture list -- then we'd get the same result as the JavaScript closures.

###### C++
```c++
vector<js_function> functions;

for (auto& i = *(new int{0}); i < 5; ++i) {
    // Every closure we push captures a reference to the same "i"
    functions.push_back([&i] (any this_, vector<any> arguments) {
        cout << i;
        return any{};
    });
}

// 5, 5, 5, 5, 5
for_each(functions.begin(), functions.end(), [] (auto fn) {
    fn(nullptr, {});
});
```

#### `let` and capturing by value

Recently, JavaScript added the [`let` statement](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Statements/let), which brings block scoping to the language. But when it comes to for-loops, then `let` does more than mere block scoping. At every iteration of the loop, [JavaScript creates a brand new *copy* of the loop variable](http://www.ecma-international.org/ecma-262/7.0/index.html#sec-createperiterationenvironment). That's not normal block scoping behavior. Rather, this seems to emulate capture by value in a language that otherwise only supports capture by reference. A JavaScript closure within the loop will still capture by reference, but it won't capture the loop variable itself; it will capture a per-iteration *copy* of the loop variable.

###### JavaScript
```javascript
let functions = [];

for (let i = 0; i < 5; i++) {
    // Every closure we push captures a reference to a per-iteration copy of "i"
    functions.push(() => {
        console.log(i);
    });
}

// 0, 1, 2, 3, 4
functions.forEach((fn) => {
    fn();
});
```

###### C++
```c++
vector<js_function> functions;

for (auto i = 0; i < 5; ++i) {
    // Create a per-iteration copy of "i"
    auto& i_copy = *(new int{i});

    // Every closure we push captures a reference to a per-iteration copy of "i"
    functions.push_back([&i_copy] (any this_, vector<any> arguments) {
        cout << i_copy;
        return any{};
    });
}

// 0, 1, 2, 3, 4
for_each(functions.begin(), functions.end(), [] (auto fn) {
    fn(nullptr, {});
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

for (auto i = 0; i < 5; ++i) {
    // Create a per-iteration copy of "i"
    auto i_copy = my_heap.make<int>(i);

    // Every closure we push captures a reference to a per-iteration copy of "i"
    functions.push_back([i_copy] (any this_, vector<any> arguments) {
        cout << *i_copy;
        return any{};
    });
}

// 0, 1, 2, 3, 4
for_each(functions.begin(), functions.end(), [] (auto fn) {
    fn(nullptr, {});
});

// Destroy and deallocate any unreachable objects
my_heap.collect();

```

## That's all folks! (...for now)

I hope you found this look under the hood just as interesting and enlightening as I did.

I'd like this to become a living and growing document. That's why I'm hosting it here on GitHub rather than on some blog site. I'm always looking to make the explanations better and the code clearer, as well as to lift the hood of even more dark corners of the language. I welcome and appreciate any contributions to make it better.

## Copyright

Copyright 2017 Jeff Mott. Creative Commons [Attribution-NonCommercial 4.0 International](https://creativecommons.org/licenses/by-nc/4.0/) license.
