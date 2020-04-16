# Functions
I briefly spoke about functions in the [intro section](https://github.com/Federico-abss/Lispy#functions) already, here I will go more in
detail on builtin functions and what happens when you call one in Lispy. <br>
In the introduction I also described Lispy as an **homoiconic** language, homoiconicity (code as data) is when a program can access its 
own functions and procedures while running, and programmatically alter itself on the fly.
This is possible in Lispy because functions are normal data types just like numbers and strings, and can be modified by other functions 
or stored in lists.
```
;;; call a function (code)
lispy> + 1 1
2

;;; turn the function call into a qexpr
;;; (turning code into data, which is a list of symbols)
lispy> list + 1 1                                                                                     
{<builtin> 1 1}

;;; get the first element on the list
;;; (operating on code as data)
lispy> fst (list + 1 1)                                         
<builtin>

;;; use first element to write another function
;;; (turning data into new code)
lispy> (fst (list + 1 1)) 2 2                                        
4
```
This is the way in which the [standard library](https://github.com/Federico-abss/Lispy/tree/master/std-library) is written, using 
builtins to create new functionalities for the language, and we'll se more example of homoiconicity when working with builtin `fun`.
### What is a builtin and errors
How do bultins actually work under the hood? In reality they are nothing more than a function written in C, that the interpreter calls
when evaluating the symbol associated to it. That's how I inserted basic functionalities into the language like arithmetic operations and list manipulation, but it doesn't stop here, from very simple builtin operations you can write complex functions like the ones in the stdlibrary and expand the functionalities of the language without writing a single line of C. <br>
You can recognise a builtin by calling it without arguments in the interpreter.
```
lispy> head
<builtin>
;;; custom and stdlibrary functions return their definition instead
lispy> pack
(\ {f & xs} {f xs})
```
Builtins have the advantage of having a rigid error checking and reporting structure that is not possible to easily implement in Lispy,
that's how builtin functions have customized error messages, and most common ones are **incorrect number of arguments**, 
**incorrect type of arguments** and **expected non empty list**.
```
lispy> tail 1 2
Error: Function 'tail' passed incorrect number of arguments. Got 2, Expected 1.

lispy> head (+ 1 3)
Error: Function 'head' expected a String or a Q-expression

lispy> fst {}                                                                                                  
Error: Function 'head' passed {} for argument 0.
;;; expected non empty list but received empty qexpr

lispy> fst
(\ {l} {eval (head l)})
;;; error message refers to head because fst is from stdlibrary and head is used in its definition
```
## Builtin functions
### General use functions
Let's start by visualizing every builtin in the interpreter interface by using **`env`**, a function that displays every symbol bound to the current environment, keep in mind that an env is comparable to a scope in other languages.
```
;;; let's use env in the global environment
lispy> env
{def = env \ fun exit load error print list head tail eval join cons len init index pack unpack 
> < >= <= == != if and or not + - * / % max min ^}
;;; if you use env in in your interface is going to show the stdlibrary functions as well
```
I will keep addressing the main functions in order, starting with **`def`** and **`=`**, which are both used to create new variables,
the first one always operates in the global env while the other in the local env.
```
;;; remember that in the interface we are working in the global env by default
;;; def and = accept 2 arguments, first the symbol inside a list and then the lval you want to associate to it
lispy> def {x} 10
()
lispy> = {y} 5
()
lispy> + x y
15
```
**`\`** is used to create lambda functions, custom single use expression to execute operations in your code. `\` automatically creates a new scope every time it is used so the main env doesn't get polluted with the lambda function arguments.
```
lispy> * 2 (+ 3 4)
14
;;; to create a lambda write \ followed by 2 lists, one containing the arguments and one the body of the function
lispy> (\ {x y} {* 2 (+ x y)}) 3 4
14
```
You can also bind the lambda function to a symbol and make it reusable, using `def`.
```
lispy> def {doublesum} (\ {x g} {* 2 (+ x g)})
()
lispy> doublesum 3 4
14
```
You can probably agree that the syntax for creating even simple reusable lambdas is very complex and hard to remember, that's where
we use builtin **`fun`** to create custom functions with a more user friendly appearance. Just call `fun` followed by a qexpr containing
the name of the function and its arguments, then another qexpr containing the body of the function.
```
lispy> fun {doublesum x y} {* 2 (+ x y)}
()
lispy> doublesum 3 4
14
lispy> doublesum
(\ {x y} {* 2 (+ x y)})
```
Functions that can define functions. That is certainly something we could never do in C, and a showcase of the versatility of homoiconic
code. <br>
**`exit`** interrupts the execution of the program by closing the interpreter.
**`load`** allows us to execute external lispy files and visualize their outputs in the interpreter.
```
;;; call load followed by the relative address of the lspy file wrapped in quotation marks
lispy> load "programs/hello_world.lspy"
"Hello World!"
()
;;; in this example the folder programs is in the same folder as the interpreter
```
### Quexpr and Sexpr functions
Most of these functions work also on strings, builtins benefits from the possibility of "overloading" a function to deal with different types of data, while custom functions are generally limited to one. <br>
**`list`** transforms a sexpr into a quexpr, blocking its evaluation, **`eval`** does the opposite operation by evaluating the data 
inside a list.
```
;;; list accepts a single sexpr as argument while eval a single qexpr
lispy> list + 5 5
{<builtin> 5 5}
lispy> eval {+ 5 5}
10
```
**`head`** and **`tail`** both work on a list and do opposite operations, head takes the first element and tail every element after the first. 
```
;;; list accepts a single sexpr as argument while eval a single qexpr
lispy> list + 5 5
{<builtin> 5 5}
lispy> eval {+ 5 5}
10

;;; they also work on strings!
lispy> head "string"
"s"
```
**`join`** fuses 2 or more qexprs into a single list, works also on string.
```
lispy> join {1 2 3} {4 5}
{1 2 3 4 5}
lispy> join "hello " "to " "the " "world" 
"hello to the world"
```
**`len`** returns the number of lvals in a list or the characters in a string. <br>
**`index`** returns a new list containing the element at index n of a list l, n starts from zero and goes to (- 1 (len l)).
```
lispy> index 0 {1 2 3}
{1}
lispy> index 3 {1 2 3}
Error: index out of range, the list has length 3
```
### Conditional functions
In Lisp there are no boolean values built in the language, conditional functions return `1` when they evaluate an expression to be true and `0` otherwise. You can use true and false in your expressions, but it's just syntactic sugar!
```
lispy> true
1
lispy> false
0
```
**`>`**, **`>=`**, **`<`** and **`<=`** all accept two numerical values as arguments and evaluate one against the other.
```
;;; evaluate if 10 is bigger than 5
lispy> > 10 5
1  ;;; true

;;; evaluate if 1 is bigger or equal to 5
lispy> >= 1 5
0  ;;; false

;;; evaluate if 1 is smaller than -5
lispy> < 1 -5
0

;;; evaluate if -5 is smaller or equal to -5
lispy> <= -5 -5
1
```
**`==`** and **`!=`** evaluate if two lvals are equal or different, work on every lval type.
```
;;; evaluate two strings
lispy> == "some" "thing"
0

;;; two lvalues of different types are always different
lispy> != {12} 12
1

lispy> == 100 (* 10 10)
1
```
**`if`** takes 3 arguments, the first a 0 or a 1, then two qexprs, if the first argument is 1 then the first qexpr gets evaluated, otherwise the latter.
```
lispy> if 1 {* 5 5} {- 5 5}
25
lispy> if (== 10 10) {print "true"} {print "false"}
true
```
### Arithmetic functions
**`+`**, **`-`**, **`*`** and **`/`** are addition, subtraction multiplication and division, they all accept one or more arguments and 
they evaluate them one by one.
```
lispy> + 10 10 10 10
40

;;; first argument minus following arguments
lispy> - 10 5 5 5
-5
;;; if only one argument it negates it
lispy> - 10
-10

lispy> * 2 2 2 2
16

lispy> / 20 2 5 
2
```
**`%`** executes the modulus operation, **`max`** and **`min`** return respectively the biggest and smallest arguments provided.
```
lispy> % 5 3
2
lispy> max 10 15 19
19
lispy> min 10 1 -8
-8
```
