# Priml Language

- [Priml Language](#priml-language)
  - [Key Features](#key-features)
  - [Units](#units)
  - [Source Line](#source-line)
  - [Type System](#type-system)
    - [Value Types](#value-types)
    - [Literals](#literals)
    - [Reference Types](#reference-types)
  - [Comments](#comments)
  - [Variable Definition](#variable-definition)
  - [Function Definition](#function-definition)
  - [Flow Control](#flow-control)
    - [The statement block](#the-statement-block)
    - [The **if** statement](#the-if-statement)
    - [The **switch** statement](#the-switch-statement)
    - [The **break** statement](#the-break-statement)
    - [The **continue** statement](#the-continue-statement)
    - [The **loop** statement](#the-loop-statement)
    - [The **while** statement](#the-while-statement)
    - [The **for** statement](#the-for-statement)
    - [The **defer** statement](#the-defer-statement)
  - [Object and Impl Blocks](#object-and-impl-blocks)
    - [Creating Objects](#creating-objects)
    - [Access Object Fields and Methods](#access-object-fields-and-methods)
    - [Automatic Reference Counting](#automatic-reference-counting)
  - [Interfaces and Impl Blocks](#interfaces-and-impl-blocks)

## Key Features
- Generates highly optimized native code
- No garbage collection (GC)
- Supports both value types (int64, float64, string, etc.) and Reference types (objects)
- The memory management for objects is based on RAII, which utilizes Automatic Reference Counting (ARC)
- Unlike Rust, the objects in Priml never move in memory, even when inside dynamic data structures
- Interfaces enable defining behavior that can be implemented by multiple objects
- No exceptions. The design emphasizes and supports the handling of errors when they occur
- The code is organized using units and a built-in package manager
- The syntax is clean and easy to read, with no semicolons, a single way to use braces, single way to dereference, no weird constructs, no include files, and no preprocessor

## Units
In Priml, code is organized in **units** (homage to Turbo Pascal). Units are a collection of Priml source files (.pc) and are separately compiled modules of your program.  Units can take dependencies on other units (like libraries). There are two types of units: EXE and LIB.

All entities defined inside a unit are visible everywhere in that unit. This also means that there is no requirement for declaring entities separately in a header file (like C++). In fact, there are no header files.  You just define the entity (func, object etc) in one place and it is instantly visible to all the other code in the unit.

However, the entities from the unit are not visible outside the unit. To make them visible outside the unit, they have to be defined with the **pub** keyword to make them public.

## Source Line
Unlike other languages, Priml doesn't use semicolon to separate lines of source code. This means that there are rules on what can be specified on a single line.  For example, the entire function specification including params and return type must be on a single source line.

Line continuation character '\\' can be used to split the single source line onto multiple lines.

## Type System
Value types directly store the data. Examples include signed and unsigned integer values, floating point values, as well as dynamic strings, which are first class citizens in Priml.

Unlike C++, there are no machine dependent integer types such as int or long.  Ints are defined with exact bit sizes such as int32.

However, usize is machine dependent. It has the same number of bits as an address.  This type should be used for sizes and indexes.

Tye string type is a dynamically sized strings but is still considered a "value type" so all the same semantics apply.  However, Priml implements a number of optimizations such as "small string optimization", and "copy on write".

### Value Types
```
- int8
- int16
- int32
- int64
- uint8
- uint16
- uint32
- uint64
- usize
- float32
- float64
- char
- bool
- string
```

You can create other names for value types using the the **type** keyword.
```rust
type celcius ::= float32
type temperature ::= celcius
```

### Literals
Numeric literals can have a postfix that specify the bit size of the literal.
- unsigned postfixes: u8, u16, u32, u64
- signed postfixes: i8, i16m i32, i64
- floating point postfixes: f32, f64

An '_' can be used anywhere in the number except as first character.  These are allowed for readability but do not change the value of the number.  So 10_000_000 is same as 10000000.

Here are some examples:

```rust
var b = 50_000u64
var b = 50000u64
var c = 0xABCDu32
var g = 3.0E20
```

### Reference Types
Priml memory management depends on references.  Reference types are where references to objects are stored.  All objects in priml are accessed via references, and are reference counted. More specifically Priml uses Automatic Reference Counting or ARC behind the scene.

Your object, which is ref counted, is not moved by the language. Even the data structures like Vector don't move the objects when they grow or shrink in size.  Moreover, objects in data structures also honor reference counting.  This is different than Rust lang which says "every type must be ready for it to be blindly memcopied to somewhere else in memory".

```rust
object MyObj
{
    int32 age
    float32 weight
}
```

## Comments
Priml comments are specified the same syntax as C++ comments.  There are two types.

```rust
// This is a line comment

/* This
   is a block
   comment
*/
```

## Variable Definition
In Priml variables are definition using the keyword **var**. The variable is visible only at the scope it was declared. If the type cannot be automatically deduced, the definition must specify the type.

```rust
var a = -20_000
var g = 3.0E20

var int32 x = 10

var MyObj obj = new MyObj
var obj = new MyObj
```

## Function Definition

```rust
func add(float32 x, float32 y) -> float32
{
    return x + y
}
```

In Priml, functions are the primary means of writing code. A function is defined by typing func followed by the function name and parentheses, and using curly brackets to indicate the function's body. Functions can return values, declared after an arrow, by using the return keyword. Function placement doesn't matter to Priml as long as they are in a visible scope to the caller.

Functions are defined using the **func** keyword.

```go

func greeting(string name) -> string
{
    return "Hello " + name
}

func main()
{
    print(greeting("Priml"));
}

```
Functions within a unit are visible throughout that unit, while using the pub keyword makes them visible outside of it.

```go
pub func add(float32 x, float32 y) -> float32
{
    return x + y
}
```

## Flow Control
This section goes over the various ways code or statements flow is controlled.  Also pay attention to which parts of statements need to be on a line by themselves.

### The statement block
In Priml, the block of statment is defined by braces which must go on their own lines.
```rust
{
    // scopes variables ...
    // statements ...
}
```

### The **if** statement
Similar to C++, the if statement tests for conditions, and can have else and else-if segments.  Note that unlike C++, there are no parentheses for the condition and no semicolons.
```go
if a == 10
{
    print("<if> 10")
}
else if a == 9
{
    print("<else if> 9")
}
else
{
    print("<else>", $a)
}
```

### The **switch** statement
Similar to C++ but there are some important differences.
- Each case is a statement block must be surrounded by braces
- There is no fall through,
- Multiple values can be specified on a single line with commas
- The default case is specified as last case in switch with the ellipses '...' syntax
- The ... can be by itself in a case or combined with other values but be the last value on the line.
    - Ex: case 2, 3, ...
```go
switch x
{
    case 1
    {
        print("1")
    }
    case 2
    {
        print("2")
    }
    case 3, 4
    {
        print("3 or 4")
    }
    case ...
    {
        print("Other values")
    }
}
```

### The **break** statement
This statement can only be used inside a while/for/loop statement.  It is used to break out of the loop.  It has two forms: unconditional and conditional.
```go
break
```
Or, it can optionally specify the ** if segment which will break when the condition is true.
```go
break if i == 8
```

### The **continue** statement
This statement can only be used inside a while/for/loop statement.  It is used to 'continue' to the next iteration of the loop.  It has two forms: unconditional and conditional.
```go
continue
```
Or, it can optionally specify the ** if segment which will break when the condition is true.
```go
continue if i == 8
```


### The **loop** statement
The 'loop' statement creates an infinite loop.  The only way to exit out of the loop is by using the break statement or the return statement.
```go
var i = 0
loop
{
    i += 1
    break if i == 8
}
```

### The **while** statement
The while statement creates a conditional loop.
```go
var i = 10
while i < 20
{
    i += 1
    continue if i == 15
    print($i)
}
```
Note: Priml doesn't support the do-while statement found in C++ and other languages.

### The **for** statement
Unlike the C++ for statement, the Priml for is much simpler and stream lined, but allows expressing the important functionality a more clear way.

The for variable is automatically declared, and there is no need to have a separate var statement for it.  It is visible only inside the for loop.

There are two types of Priml for loops: Range For and Iter For.

**1. Range For**
The Range For is used to express loops over numeric data such as ints, usizes, or floats.  It specifies a range, indicates if it should be inclusive (of the ending point), and if it should be forward or reverse.
```rust
    // Range
    for i : range(0, 4)
    {
        // generates 0 1 2 3
    }

    // Inclusive Range
    for i : rangeinc(0, 4)
    {
        // generates 0 1 2 3 4
    }

    // Reverse Range
    for i : rev range(0, 4)
    {
        // generates 3 2 1 0
    }

    // Reverse Inclusive Range
    for i : rev rangeinc(0, 4)
    {
        // generates 4 3 2 1 0
    }
```
**2. Iter For**
Iter for is used to iterate over a data structure such as Vector or List which implements the iterator. It is specified using the **iter** keyword and the name of the data structure. You can either iterate forward or reverse using the **rev** keyword.
```rust
    var vec = new Vector<MyObj>

    // Forward Iterator
    for i : iter(vec)
    {
        print($i)
    }

    // Reverse Iterator
    for i : rev iter(vec)
    {
        print($i)
    }
```

### The **defer** statement
The defer statement can only be used at the function body scope. It specifies a statement block that doesn't execute the place where the code is written.  Instead, it executes right before the function return.  Multiple blocks can be specified and they will execute in order, at the time of function return.
```swift
defer
{
    print("at exit part 1 (defer)")
}
print("middle")
defer
{
    print("at exit part 2 (defer)")
}
print("more stuff")
```

## Object and Impl Blocks
Priml constructs define the data and functions for a 'object' separately but they are associated.  Keyword **object** is used to define the data portion (fields) and the keyword **impl** is used to define functions (methods) that are associated with the object.

Below code shows an object called Square which defines the various fields that go into it.  The object is visible inside the unit, and its fields are visible inside the unit. If the object needs to be visible outside the unit, it must be marked with the **pub** keyword.  The code also defines a **impl block** where the associated methods are defined.

```rust
// In file a.pc
object Square
{
    float64 x
    float64 y
    float64 side
}

impl Square
{
    func render()
    {
    }
}
```
The impl blocks can be in the same file or in a different source file.  You can also have multiple impl blocks in multiple files.  The methods get associated with the object if the impl block is defined with the same identifier as the object name.  The code below is placed in a different file inside the same unit, and its functions get associated with the Square object automatically.

```rust
// In file b.pc
impl Square
{
    func persist()
    {
    }

    func area() -> float64
    {
    }
}
```

Functions defined inside a impl block become *methods*.  The main difference between functions and methods is that there is a implicit **this** pointer to the object in case of methods.

### Creating Objects
Variables of type object, such as Square, are actually *references*.  When you declare a object variable, you are creating a reference to an object. Someone needs to create the object instance which can be done using the **new** keyword or can be done by a data structure.

```rust
var Square obj = new Square
var obj2 = new Suare   // Type is deduced
```

### Access Object Fields and Methods
Object fields are accessed using '.' notation.  Note that the dot notation is also used to access public objects from other units.

```rust
    var sq = new Square
    print("Area: ", $sq.area())

    // Access an object in another unit named accounts
    cost = account.current.costs.total()
```

### Automatic Reference Counting
Priml utilizes Automatic Reference Counting (ARC) to handle memory usage of your app, meaning memory management is mostly automated and doesn't require manual attention. ARC frees up memory used by object when they are no longer required, but sometimes needs more information to manage memory effectively. Reference counting is exclusive to **objects** and not value types such as strings. Each time a object is created, ARC assigns a memory chunk to store instance information and associated properties, and frees up memory when the instance is no longer necessary. ARC monitors the number of active references to an instance to prevent deallocation while still in use. Variables that assign a object instance create a strong reference, preventing deallocation as long as the strong reference exists.

## Interfaces and Impl Blocks
Interfaces in Priml define shared behavior among objects and can be used to group method signatures. Interface bounds are used to specify generic type behavior. Method signatures in an interface determine an object's functionality, and multiple objects can share behavior if they have the same method signatures. Interfaces can be defined in different units and exported using the pub keyword.

In Priml, interfaces are defined uinsg the **interf** keyword.  And interface implementation is *done for an object* using the **impl** accompanied with a **for** segment.

```rust
// Declare an interface, HasArea
interf HasArea
{
    func area() -> float64
}

// Define an object, Circle
object Circle
{
    float64 x
    float64 y
    float64 radius
}

// Define another object, Square
object Square
{
    float64 x
    float64 y
    float64 side
}

// Implement the interface HasArea for the circle object
impl HasArea for Circle
{
    func area() -> float64
    {
        return 3.14 * (radius * radius)
    }
}

// Implement the interface HasArea for the square object
impl HasArea for Square
{
    func area() -> float64
    {
        return side * side
    }
}
```
A interface reference object can be stored and passed to functions as parameters.  This type of reference object holds a pointer to the interface implemented by the object, so it can point to objects of different types which implement this interface.

```rust
// A function that takes an interface reference to an object
// It can print the area of any object that implements the HasArea interface
func printArea(HasArea shape)
{
    print("This shape has an area of: ", $shape.area())
}

func main()
{
    var Circle c = new Circle
    c.radius = 2.2

    var Square s = new Square
    s.side = 4.2

    printArea(c)
    printArea(s)
}
```