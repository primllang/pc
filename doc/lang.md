# Priml Language

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
In Priml, code is organized in **units** (homage to Turbo Pascal). Units are a collection of Priml source files (.pc) and are separately compiled modules of your program.  Units can take depenencies on other units (like libraries). There are two types of units: EXE and LIB.

All entities defined inside a unit are visible everywhere in that unit. This also means that there is no requirement for declaring entities separetly in a header file (like C++). In fact, there are no header files.  You just define the entity (func, object etc) in one place and it is instantly visible to all the other code in the unit.

However, the entities from the unit are not visible outside the unit. To make them visible outside the unit, they have to be defined with the **pub** keyword to make them public.

## Source Line
Unlike other languages, Priml doesn't use semicolon to separate lines of source code. This means that there are rules on what can be specified on a single line.  For example, the entire function specification including params and return type must be on a single source line.

Line continuation character '\\' can be used to split the single source line onto multiple lines.

## Type System
Value types directly store the data. Examples include signed and unsigned integer values, floating point values, as well as dynamic strings, which are first class citizens in Priml.

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
```
type celcius ::= float32
type temperature ::= celcius
```

### Reference Types
Priml memory management depends on references.  Reference types are where references to objects are stored.  All objects in priml are accessed via references, and are reference counted. More specifically Priml uses Automatic Reference Counting or ARC behind the scence.

Your object, which is ref counted, is not moved by the language. Even the data structures like Vector don't move the objects when they grow or shrink in size.  Moreover, objects in data structures also honor reference counting.  This is different than Rust lang which says "every type must be ready for it to be blindly memcopied to somewhere else in memory".

```
object MyObj
{
    int32 age
    float32 weight
}
```

## Comments
Priml comments are specified the same syntax as C++ comments.  There are two types.

```C++
// This is a line comment

/* This
   is a block
   comment
*/
```

## Variable Definition
In Priml variables are definition using the keyword **var**. The variable is visible only at the scope it was declared. If the type cannot be automatically deduced, the definition must specify the type.

```
var a = -20_000
var b = 50_000u64
var c = 0xABCDU32
var g = 3.0E20

var int32 x = 10

var MyObj obj = new MyObj
var obj = new MyObj
```

## Function Definition
Functions are defined using the **func** keyword.

```
func add(float32 x, float32 y) -> float32
{
    return x + y
}
```

The func is visible to all source files inside the unit. But not outside the unit. The pub keyword can be used to make it public and visible outside the unit.

```
pub func add(float32 x, float32 y) -> float32
{
    return x + y
}
```

## Flow Control
This section goes over the various ways code or statements flow is controlled.  Also pay attention to which parts of statements need to be on a line by themselves.

### Statement block
In Priml, the block of statment is defined by braces which must go on their own lines.
```
{
    // scopes variables ...
    // statements ...
}
```

### The **if** statement
Similar to C++, the if statement tests for conditions, and can have else and else-if segments.  Note that unlike C++, there are no parantheses for the condition and no semicolons.
```
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
- Each case is a statement block must be sarrounded by braces
- There is no fall through,
- Multiple values can be specified on a single line with commas
- The default case is handled with elipses ... syntax
- The ... can be by itself in a case or combined with other values but be the last value on the line.
    - Ex: case 2, 3, ...
```
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