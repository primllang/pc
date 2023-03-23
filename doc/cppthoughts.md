# C++ and bad language design

> *"C++ is a horrible language”* -- Linus Torvalds

> *"[C++] certainly has its good points. But by and large I think it's a bad language. It does a lot of things half well and it’s just a garbage heap of ideas that are mutually exclusive. Everybody I know, whether it’s personal or corporate, selects a subset and these subsets are different. So it’s not a good language to transport an algorithm—to say, "I wrote it; here, take it." It’s way too big, way too complex. And it’s obviously built by a committee. Stroustrup campaigned for years and years and years, way beyond any sort of technical contributions he made to the language, to get it adopted and used. And he sort of ran all the standards committees with a whip and a chair. And he said "no" to no one. He put every feature in that language that ever existed. It wasn't cleanly designed—it was just the union of everything that came along. And I think it suffered drastically from that."* -- Ken Thompson

> *"C++ is a badly designed and ugly language"* -- Richard Stallman

> *"C++ is steaming pile of crap. It's like lot of the features in C++ were designed to make your code more error prone, and less understandable.  Consider for example, there are 18 different forms of variable initialization in the standard, with very close syntax.  At the same time, lot of the features that could have helped your code were never added, instead programmers need to resort to add-ons like compiler warnings, SAL, GSL, safe str, static code analysis, address sanitizer, and build tools like CMake ..."* -- Yasser Asmi

> "Top 10 dumb mistakes to avoid with C++ 11 smart pointers" https://www.acodersjourney.com/top-10-dumb-mistakes-avoid-c-11-smart-pointers/ -- acodersjourney.com

But, are these dumb mistakes or bad language design?

And just for fun... checkout this "C++ roasting" by Tim Straubinger. Guaranteed to make you laugh... [2020-tim-c++_roast.pdf](https://www.cs.ubc.ca/~udls/slides/2020-tim-c++_roast.pdf)

# C++ love and hate
## // Why we love C++
C++ remains one of the most widely used languages.  Lot of us grew up with C/C++ and love it.  Many new developers also gravitate toward C++ even in the presence of many new and powerful languages.  Why is this?

C++ is simply the best way on the planet for writing **most efficient CPU code** (performance and size).  Because modern C++ compilers understand the complex CPU instruction scheduling, they can generate code that will be more efficient than handwritten assembly code (there are a few small exceptions).   This is why lot of system level code (OSes, webservers, etc) is written in C++.  Performance is also why most AAA games and their engines are also written in this language.

C++ lets you visualize and control the generated code, the memory usage, synchronization, and the list goes on.  There are hundreds of compilers, optimizers, analyzers, debuggers that have been finetuned over decades.

C++ code, by far, is the most portable.  After rebuilding, it can run on all sorts of platforms with ease and with efficiency.

It is beautiful!

## // What goes wrong
Over the decades the language has become overly complex and has a lot of baggage.  Many of the features have clearly lost sight of why C++ is great.  To make matters worse, developers and "patterns" have abused the features and misled folks.  Millions of lines code and libraries have been piled up that which have embraced the worse parts of the language.  There are layers upon layers, abstractions upon abstractions--with lot of C++ code that does very little in terms of maintenance and clarity and hurts performance.  Due to the vast feature set of the language, there are multitudes of coding styles.  If you see yourself using, Boost, you might as well become a Java programmer.

**C++ is not religion**.  Everything Stroustrup, Sutter, and C++ standards bodies recommend is not gospel. And every C++ feature is not a gem.  In fact, very little positive progress has happened since original C++.  There were some dark periods where of early STL, operator overloading, templates and exceptions.   With out-of-control memory management, tons of extra object copies being made, unbearable syntax, or inability to debug, the new additions were a mess.  Why couldn't (and we still can't) do a printf of a std::string? Why must we be subjected to using << operator and std::cout?  Why haven't we evolved fundamentals like RAII and pointers?

C++1/C++14/C++17/C++20 have a few shining examples of good, well thought out features such as copy and move semantics, auto keyword, etc.  However, 30+ years later, and there is still no solution for header file management, or modern components (npm, go get, etc)., and it still lags behind others in webservices support.

"Modern C++" has been taking a very different route than other modern languages.  Incredibly it all somehow works but it isn't optimal.

# C++ Coding Guidlines
Lot of great systems developers see the above problems, and revert to straight C and write beautiful code in C.  Examples include NT kernel, Linux kernel, and Nginx.  I don't recommend this for most people because 1) it is way harder and marginal benefits, 2) You are depriving yourself of great C++ features such as RAII and classes, 3) Come on, it is the 21st century!

Don't revert to C, you can use a subset of C++ that will get you the exact same level of performance and control.

I recommend using a subset of C++ language features and patterns that accomplish our desired goals, prioritize simplicity, prioritize clarity and visibility, prioritize correctness, and better align with other modern languages.  Let's call it C+- (C plus minus).

1. Don't prepare for a future that may never come.  People create unnecessary abstractions/interfaces/adaptors/plugins etc thinking they will be able to replace some parts of the code in the future.  Don't do this unless it is a known requirement.
2. Don't go overboard on "information hiding". Clarity comes from easily reading the code.  The actual code.  You may be thinking since my interface and my class has a descriptive name, it should be clear what it does.  If the code is buried beneath multiple abstractions, files, and patterns, you are doing a disservice to yourself and other maintaining the code.
3. Don't feel the need to create classes for everything.  Classes are great way to organize your code and to provide a good easy to call API.
4. Don't feel the need to provide "accessors" for everything.  Sometimes it is just fine to allow access to public member variables of a class.   It adds clarity.
5. Don't feel the need to create tons of small header and CPP files for each little class.  Think of logically organizing your code.  It is ok to have multiple classes in a header file.  It is OK to declare your class in the CPP file if only used by that file.
6. Don't use inheritance unless it really is a clear-cut case like: Shape, Box, Circle, Triangle.  Say no to multiple inheritance.
7. Abstract at a higher level.  It may be tempting to create interfaces for everything and then create objects that implement them.  But stop and think is it really required.
8. Avoid exceptions.  Exceptions only work well if the entire program consistently uses them.  Otherwise, you have unhandled exceptions.  Don't expect everyone else to adhere to using exceptions.
9. Don't use virtual functions unless there is actually a need.  If you want override functionality of an object, consider exposing stl::function member variables that the caller can point to their own function.
10. Never use "using namespace" in a header file.  Always specify namespace explicitly.  OK to use using namespace in a CPP file.
11. Do a lot of logging--you will need it!  Keep in mind that logging is a great way debugging and verifying your program.  In some environments, you may only get a log file instead of a debugger.  But logging can bloat your code.  Develop a strategy about how you minimize the impact.  Print error information of why something failed.  Be explicit about what will be in DEBUG builds or not etc.  Remember C++ is about CONTROL not just raw performance.  Debuggability of your system is critical.
12. Use C++17 or later.  Most compilers are already supporting this.
13. Use std::string for your strings.  It is decent but understand it is an object and not POD.  Use "const std::string&" to receive a string as a parameter.  In some cases, you may need to use "const char*" If you are manipulating char* buffers, do it carefully and handle null termination and lack of length information.
14. Struct can be used as a class.  But if it is a class with methods, use the "class" keyword.  "struct" is best use for C style structs representing POD or memory layout.
15. Use size_t not ints for sizes and lengths and indexes
16. Use const but don't go overboard with it. Simplicity is the key.  Every class you write is not going to be used by some stranger somewhere.
17. Feel free to use stl containers.  They are now tolerable with C++11 syntax of auto and range.  But make sure you understand the semantics of each container fully.  Performance wise, you could still right more performant container in some cases, but it is not worth the effort.  STL containers are decent and very convenient.
18. Don't use boost.  Sort of a personal opinion but also the library is huge. Stick to standard C++ and use appropriate libraries for specific needs.
19. Don't declare global objects.  It is hard to control the order of constructors and destructors for such objects as they are called before main.
20. Use new and delete for objects.  Use malloc/free for allocating buffers of bytes.  You may also use new/delete[] for arrays but I'd minimize this but may be better to use stl::vector.
21. It is also OK to carefully create a stl::list or stl::vector or stl::map of pointers.  As long as you understand and control how the objects will be freed.
22. Depend on **RAII**.  This is a greatest C++ feature.
23. You can use smart pointers, but don't expect everyone else to adhere to this.
24. Check parameters and conditions early in the function, return from function on failure.  If you have non RAII, make sure you clean up.
25. Use section code comments.  In the comment, explain what and why, don't explain how.  How should come from reading the code.
26. Braces.  Use the style used by the team generally.  Or when modifying a file, be consistent with existing file.  Currently we are using the Allman style.  It is recommended that you ALWAYS use braces even when they are not required (ie if statement).
27. Naming conventions
28. Use **camelCase** for most identifiers such as functions, methods, or member variables (example: getEnv())
29. Use **ProperCase** for class names (example: ProcessTable)
30. Use all lower case for locals and parameters.  Keep these names short and simple (example: len, rotation)
31. Don't use underscores.
32. Use "m" prefix for class member variables.  And "s" for static variables. **No underscores**. ( Example: mHeight).  Prefixes can be left out from "structs".
33. Objects with static/global storage
34. use trivially destructible objects
35. constexpr guarantees trivial destructor
36. For init, need to ensure class constructors execute, and initializer is evaluated
37. Do not define implicit conversions. Use the explicit keyword for conversion operators and single-argument constructors explicit keyword applied to a constructor or a conversion operator ensures that it can only be used when the destination type is explicit at the point of use, e.g., with a cast.
38. constexpr definitions enable a more robust specification of the constant parts of an interface.  Avoid complexifying function definitions to enable their use with constexpr. Do not use constexpr to force inlining
39. Use <cstdint> which defines types like int16_t, uint32_t, int64_t
40. use standard types like size_t and ptrdiff_t
41. std::Initializer_list<> is decent and can be used. It's not STL it’s a core part of the language now.

## Additional Guides
Google guide is pretty good:
https://google.github.io/styleguide/cppguide.html#C++_Version

Art of Unix Programming:
https://www.linuxtopia.org/online_books/programming_books/art_of_unix_programming/ch01s06.html

