# Graphite

> **"Programs must be written for people to read, and only incidentally for machines to execute."**
> — Harold Abelson

## Graphite

A small demo code:

```graphite
use "stdlib"

fn graphite() {
    var i = 0

    std.println("")
    std.println("    ╔══════════════════════════╗")
    std.println("    ║        <GRAPHITE>        ║")
    std.println("    ╠══════════════════════════╣")

    while (i < 3) {
        if (i == 0) {
            std.println("    ║   -Simple syntax         ║")
        } else {
            if (i == 1) {
                std.println("    ║   -User control          ║")
            } else {
                std.println("    ║   -Built from scratch    ║")
            }
        }

        i = i + 1
    }

    std.println("    ╚══════════════════════════╝")
    std.println("")
    std.println("    Simple by design.")
    std.println("    Powerful by choice.")
}

graphite()
```

---

## Variable Declaration

```graphite
var x = 32
const x = 32
```

Semicolons are optional, and `//` is used for comments.

```graphite
var x = 32; // This is also valid.
```

### Global Variables

```graphite
global var x = 32
```

Global variables are accessible regardless of their scope.

### Strict Variables

The `strict` keyword prevents a variable's type from changing at runtime.

```graphite
strict var x = 32

x = "test" // Error
```

`global` and `strict` can be used in either order:

```graphite
strict global var x = 32
global strict var x = 32
```

---

## Standard Library

The standard library can be imported using:

```graphite
use "stdlib"
```

or:

```graphite
use {
    "stdlib"
}
```

When importing a folder directly, the folder must contain a `.grh` file specifying which files are exportable.

---

## Function Declaration

Functions are declared using `fn`:

```graphite
fn test(int a, int b) {
    return a + b
}
```

Parameter types can be specified using:

* `int`
* `string`
* `double`
* `bool`
* `value[]` for arrays

Parameters can also be left untyped:

```graphite
fn test(a, b) {
    return a + b
}
```

---

## Basic Hello World

**Example.gr**

```graphite
use "stdlib"

std.print("Hello World!")
```

Semicolons are optional. They are used in some examples simply because they are familiar to programmers coming from languages such as C, C++, and Java.

Newlines work just as well.

---

## Arrays

Arrays can contain values of different types:

```graphite
var x = [1, 2, 3, 4, 5]

var y = [
    1,
    2,
    3,
    "hello",
    true,
    3.14,
    ["G", "r"]
]
```

> **Note:** Array functionality is still being worked on and optimized. Some less common use cases may currently result in undefined behavior.

---

## Loops

Currently supported:

1. `while`
2. `for` — *under construction*

### While

```graphite
use "stdlib"

while (true) {
    var userInput = std.input()
    std.print(userInput)
}
```

'std.prompt(text)' can also be used to prompt text to the screen before the input.

---

## Spaces

Graphite uses the `space` keyword to create namespaces:

```graphite
use "stdlib"

space Math {
    fn add(a, b) {
        return a + b
    }
}

std.print(Math.add(1, 3))
```

---

## If / Else

```graphite
use "stdlib"

if (foo() isType std.type.string()) {
    std.print("foo() is a string")
}
```

---

## Operators

### Binary Operators

```text
+  -  *  /  %  &&  ||  ^
<  >  ==  !=  <=  >=  ===
```

### Unary Operators

```text
+  -  !
```

### Keyword Equivalents

Some operators have keyword equivalents:

| Operator | Keyword  | 
| -------- | -------- | 
| `==`     | `is`     | 
| `&&`     | `and`    |  
| `\|\|`     |  `or`    | 
| `===`    | `isType` |  

The keyword equivalents behave exactly the same as their operator counterparts.

---

## A Basic Delta-V Calculator

Graphite can also be used for math projects:

```graphite
use "stdlib"

fn deltaV(isp, wetMass, dryMass) {
    const g0 = 9.806

    return isp * g0 * std.ln(wetMass / dryMass)
}

std.print(deltaV(180, 3000, 2600))
```

---

## Status

Graphite is still under development.

The core language is functional, but some features—particularly arrays—are still being fixed and optimized.

More examples can be found in the `Examples` folder.

Also make sure to check out the documentation for more info!







bonsoir elliot