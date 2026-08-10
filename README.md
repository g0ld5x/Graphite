"Programs must be written for people to read, and only incidentally for machines to execute."  — Harold Abelson


--GRAPHITE--


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


::Variable Decleration::

var x = 32; // semicolons are optional. and this is a comment!
const x = 32; // constant values cannot be changed.

global var x = 32; //global variables are accesible no matter their scope.

strict var x = 32; // the strict keyword makes it so that type changes arent allowed in runtime.
// if you try to do x = "test"; you will get an error.

strict global var x = 32; 
global strict var x = 32;


// these are both accepted. There is no spesific order when using the global and strict keywords.
::The Standart Library::

use "stdlib"

//or

use{
    "stdlib"
}

// note that when you are importing directly a folder that folder must have a .grh file that lists all the exportable files.

::Function Declaration::

//functions are declared as shown:

fn test(int a,int b){ 
    return a + b;
}

// you can specify parameter types using int,string,double,bool, or value[] for array values. Or you can leave them unspecified like fn test (a,b);

::Basic Hello World::

-Example.gr-

-1 use "stdlib";
-2 std.print("Hello World!");

// note that semicolons are optional, I use semicolons in examples because i am familiar with using them. Newlines work just as fine.

::Arrays::

// you can define arrays as shown:

var x = [1,2,3,4,5]; // note that arrays dont force the type inside to be the same.

var y = [1,2,3,"hello",true,3.14,["G","r"]];

::Loops::

1-While
2-For (In construction, yet to be implemented.)

// An example while loop that gets user input and prints it.

use "stdlib";

while(true){
    var userInput= std.input(); // or you can use the std.prompt to show a message inside.
    std.print(userInput);
}

// There are more interesting examples in the Examples folder. Note that anything related to arrays is still being fixed and optimized so its almost guaranteed to have some undefined behaviour on some rarer use cases. Other than arrays the language is mostly ok

::Spaces::

// you can create namespaces using the keyword space

use "stdlib"

space Math{
    fn add(a,b){return a+b;}
}

std.print(Math.add(1,3));

::If Else::

use "stdlib"

if(foo() isType std.type.string()){
    std.print("foo() is a string");
}

::Operators::

// Binary Operators

+,-,*,/,%,&&,||,^,<,>,==,!=,<=,>=,===

// Unary Operators

+,-,!

// Operators with keyword equivalents:

== is the same as 'is'

&& is the same as 'and'

|| is the same as 'or'

=== is the same as 'isType'

// the keyword equivalents behave exactly the same way.

:: A basic Delta V calculator::

use "stdlib"

fn deltaV(isp, wetMass, dryMass) {
    const g0 = 9.806
    return isp * g0 * std.ln(wetMass / dryMass)
}

std.print(deltaV(180, 3000, 2600))