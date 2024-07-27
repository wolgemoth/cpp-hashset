# C++ Hashset (1.0.0)

## Table of Contents

- [About](#About)
- [Instructions](#Instructions)
- [Dependencies](#Dependencies)
- [References](#References)

### About

This is a hashset written in C++.

It has a similar API to C#'s [HashSet](https://learn.microsoft.com/en-us/dotnet/api/system.collections.generic.hashset-1?view=net-8.0)  and self-initializes like an [std::vector](https://en.cppreference.com/w/cpp/container/vector). Currently, it uses [sequential chaining](https://en.wikipedia.org/wiki/Hash_table#Separate_chaining) for collision resolution. More collision resolution techniques may be added in the future.

This structure provides robust exception safety, and is suitable for use in a concurrent environment. Furthermore, it supports move semantics and initialiser lists.

Explicit finalization of the hashset is not necessary. However, if you are storing manually-managed memory, then remember to free any elements before removal.

If you find a bug or have a feature-request, please raise an issue.

### Instructions

The implementation is header-only and written in templated C++17. You should not need to make any adjustments to your project settings or compiler flags. 

Simply include it in your project and you are ready to start!

#### Example:
    
    #include <string>
    
    #include "Hashset.hpp"
    
    LouiEriksson::Hashset<std::string> hashset {
        "key1",
        "key2",
        "key3",
    }

    int main() {

        if (const auto item = hashset.Get("key3")) {
            std::cout << "Value: " << item.value() << '\n';
        }
        else {
            std::cout << "Key not in Hashset!\n";
        }

        return 0;
    }

### Dependencies

The hashset was written in C++17 and utilises the following standard headers:

#### &lt;algorithm&gt;
#### &lt;cstddef&gt;
#### &lt;functional&gt;
#### &lt;initializer_list&gt;
#### &lt;iostream&gt;
#### &lt;mutex&gt;
#### &lt;optional&gt;
#### &lt;stdexcept&gt;
#### &lt;vector&gt;

### Why not use &lt;unordered_set&gt;?

I find unordered_set to be way too verbose for most situations.

In this implementation, key existence and retrieval are merged into a single conditional expression. This allows for simpler, cleaner code that affords better exception handling.

### Note

Please note that while the hashset is capable of being used in a concurrent environment, it does not provide a mechanism for synchronising changes to the hashset which are made in between operations.

Therefore, if you need to perform a synchronous series of operations on the hashset while it is being used in a concurrent context, you should lock access to the hashset to one thread while doing so, otherwise you may encounter race conditions.

### References

- Wang, Q. (Harry) (2020). Implementing Your Own HashMap (Explanation + Code). YouTube. Available at: https://www.youtube.com/watch?v=_Q-eNqTOxlE [Accessed 2021].