# Lispy
Lispy is an interpreted, homoiconic, functional programming language inspired from the 1958 programming language Lisp. <br>
The interpreter is written in C using the MPC library for parsing operations, and it currently can be used as 
an Interactive Prompt to evaluate expressions in real time or to evaluate external lisp files. <br>

## Working with Lispy
By executing the interpreter file you will be presented with this interface in the command line:
```
Lispy Version 1.0.0.1
Hi, I am Federico and this is my version of Lisp, made especially for you with love
Press Ctrl+c or write exit to Exit

lispy> 
```
You can evaluate expressions by writing them after `lispy>`:
```
lispy> + 1 2 3
6
lispy> print "hello world"
"hello world" 
()
```
You probably noticed the weird way in which the + operator is only present before the numbers and not in between them, this is called <b>Polish Notation</b> and allows some of Lispy's more powerful and versatile features. <br>
More examples of polish notation, `1 + 2 + 6`	is	`+ 1 2 6`, again `6 + (2 * 9)`	is	`+ 6 (* 2 9)` <br> and `(10 * 2) / (4 + 2)`	is	`/ (* 10 2) (+ 4 2)`.

### Evaluating external files
It's possible to load external files written in Lispy by calling the load function inside the interpreter interface or by executing the interpreter followed by the relative path of the files that you want to be evaluated.
```
lispy>  load "hello_world.lspy"
"Hello World!"
```
```
./lispy hello_world.lspy
"Hello World!"
```
Note that to be evaluated, every expression in an external files must be written inside parenthesis, besides the terminal will show only what is being printed using the "print" function and error messages.
```
print "Hello World" ;;; this line is not going to be evaluated
(print "Hello World") ;;; this one is gonna print "Hello World" in the terminal
```

# Lispy basics
There are many unusual functionalities in Lispy, people familiar with Lisp or any of its dialects won't have any problem understanding the language, to anyone else it will look a bit foreign, but I will try to make it as clear as possible for anyone.
That being said this section is barely scraping the surface of what Lispy cand do, I strongly suggest you to read the documentation about [bultin functions](https://github.com/Federico-abss/Lispy/tree/master/Builtins-Functions) after completing this section.
### Lisp Values
There are 7 value types or lvalues you will generally interact with, [**numbers**](https://github.com/Federico-abss/Lispy#numbers), [**strings**](https://github.com/Federico-abss/Lispy#strings), [**errors**](https://github.com/Federico-abss/Lispy#errors), [**functions**](https://github.com/Federico-abss/Lispy#errors), [**S-expressions**](https://github.com/Federico-abss/Lispy#S-expressions), [**Q-expressions**](https://github.com/Federico-abss/Lispy#Q-expressions) and [**environments**](https://github.com/Federico-abss/Lispy#environments).
#### Numbers
Lispy supports integers and decimal numbers, refer to C "long" and "double" for more informations on upper and lower valid ranges. Note that anything following one or more `;` on the same line is interpreted as a comment and not evaluated.
```
lispy> / 10 3
3
lispy> / 10 3.0 
3.333333  
lispy> 1.0   ;;; decimals are always displayed with 6 characters after the decimal point
1.000000
``` 
#### Strings
A string is any combination of zero or more characters contained inside apostrophes.
```
lispy> "Hello World!"
"Hello World!"
lispy> join "Hi " "Federico"
"Hi Federico"
lispy> print "jet fuel cannot melt steel beams"
"jet fuel cannot melt steel beams" 
()
``` 
#### Errors
Errors are generated automatically when the users tries to to evaluate invalid code, but it can also be generated using the "error" function followed by a string. Errors are used to prevent invalid memory accesses in the interpreter and help the user by providing insightful commentary.
```
lispy> + 1 "2"
Error: Cannot operate on non-number!
lispy> + 1 2
3
lispy> error "costum error"
Error: costum error
``` 
#### Functions 
Here is where we start working with some of Lispy's more interesting and powerful features, firstly a function is a type of value bound to a symbol, that when called with the right arguments is gonna perform some actions or otherwise return an error. <br>
To call a function just write its symbol followed by the intended arguments, just note that every function support different values as arguments and while some can accepts any number of arguments other can only accept a defined amount. <br>
There is no distinction between what other languages consider operators like `+` and functions like `print`, lispy interprets them both as symbols, and if they are bound to any function value the function gets called.
```
lispy> - 1 2  ;;; "-" is the symbol for the subtraction function
-1
lispy> head "string"  ;;; "head" is associated to a function that return the first element of a string
"s"
lispy> head "string1" "string2"  ;;; "head" supports only one argument
Error: Function 'head' passed incorrect number of arguments. Got 2, Expected 1.
``` 
I said that functions are bound to a symbol, but you can actually create custom one use functions called lambdas using the `\` symbol. The syntax of the example ahead will probably be confusing but it will hopefully make sense after reading the next sections.
```
lispy> * 2 (+ 3 4)
14
lispy> (\ {x y} {* 2 (+ x y)}) 3 4
14
``` 
There are two different types of functions in Lispy, builtin and custom functions, builtins are functions written in C that perform an operation in Lispy, while customs are lambda functions bound to a symbol.
Refer [here](https://github.com/Federico-abss/Lispy/tree/master/Builtins-Functions) for documentation on builtin functions.
```
lispy> + 3 4
7
lispy> addition 3 4
7
lispy> +
<builtin>
lispy> addition
(\ {x y} {+ x y})
``` 
As you can see above writing the symbol associated to a function will show if the function is builtin, or if it's a lambda, its formulation. 
The standard library is a collection of custom functions written in Lispy that gets automatically loaded when you run the Lispy interface, you can find its documentation [here](https://github.com/Federico-abss/Lispy/tree/master/std-library).
#### S-expressions
S-expressions are lines of code marked as "to be evaluated" from the interpreter, in the interface every piece of code is an sexpr by default, but if you are working with an external file you will need to wrap every expression in parenthesis to make them sexprs.
Nesting sexprs is also importanto to comunicate the proper order of evalution for your code, remember that the interpreter starts always from the most internal sexprs.
```
lispy> print + 5 4
<builtin> 5 4  ;;; unintended behaviour
()
lispy> print (+ 5 4)
9 
()
lispy> + 5 * 3 3 
Error: Cannot operate on non-number!   ;;; unintended behaviour
lispy> + 5 (* 3 3)
14 
``` 
#### Q-expressions
Q-expressions are a collection of lvalues that are not meant to be evaluated by the interpreter, you can creat one by wrapping an expression inside curly braces and they can be used as an equivalent of Python lists or C arrays to store data, but also be manipulated with various functions that accept them as their argument, `eval` for example converts a qexpr into a sexpr.
Note that qexprs can contain anything, functions included, this will allow us to write [code that modifies itself](link) further on.
```
lispy> {+ 1 2 3}
{+ 1 2 3}
lispy> eval {+ 1 2 3}
6
lispy> head + 1 2 3  ;;; example of a function that works on qexprs
Error: Function 'head' passed incorrect number of arguments. Got 4, Expected 1.
lispy> head {+ 1 2 3}
{+}
``` 
#### Environments
Until now I have referred several times to symbols being bound to a function or other lvalues, but what does it mean in concrete? You see there is an environment, which is basically a global scope, that contains every symbol bound to a lvalue and stores them for use. <br>
The first thing the Lispy interpreter does when executed is creating a global environment and loading every builtin function and its symbols in it, you can see them by using the function `env`.
```
lispy> env
{def = env \ fun exit load error print list head tail eval join cons len init index pack unpack 
> < >= <= == != if and or not + - * / % max min ^}
``` 
Environments don't only contain functions, you can associate any lvalue to a symbol and save them for future use like a variable, just remember the environment is deleted every time you interrupt the execution of the interpreter. To create a variable in the global scope use the `get` builtin followed by a symbol in curly brackets and the lvalue you want to bind. 
```
lispy> = {x} 10
()
lispy> print x
10 
()
lispy> + x 5
15
;;; if you call env again you will find your variable at the end of the list
``` 
You can create a local environment for your variables using the `let` function, but you also automatically create a local scope every time you use a custom function, this allows the interpreter to not pollute the global scope every time it evaluates a function.
### Symbols 
We already spoke at length about symbols in lispy, but I have never given a formal definition, the parser recognizes every letter on an international keyboard and the most popular characters, you can use them in any combination to create a symbol.
```
lispy> x
Error: Unbound Symbol 'x'
lispy> = {x} 10
()
lispy> x
10
```
Before assigning a lvalue to it, any symbol is in an "unbound" state, after it gets assigned it will act as a variable by holding the lvalue saved in the global environment. Look at the [environments](https://github.com/Federico-abss/Lispy#environments) section for reference on how to create a variable.
```
lispy> x
Error: Unbound Symbol 'x'
lispy> = {x} 10
()
lispy> x
10
``` 
When you create a variable this is way it is saved inside the global environment, you can find it using `env` at the end of the list of functions. Pay attention when creating a variable, if you choose a symbol already in use you are going to override whatever lvalue was previously stored in it. <br>
Note that in Lispy functions, especially some builtin ones, are used in the definition of the custom standard library function, overriding something like `head` is gonna break the majority of the standard library.
```
lispy> head
<builtin>
lispy> = {head} 5
()
lispy> head
5
``` 
Expanding on this, variables in lispy are immutable, there is no way of modifying them, and even when you assign a new lvalue to the same symbol to update a variable, in the background the interpreter is just destroying the variable and creating a new one associated to the same symbol.
The same happens with symbols bound to qexprs, even though quexprs are often used like lists they are more similar to sets when bound to a symbol, as they cannot be modified after being stored in a variable.
```
lispy> = {x} {1 2 3}
()
lispy> head x
{1}
lispy> x  
{1 2 3}   ;;; "head" didn't actually remove the first value as expected
lispy> = {x} (tail x)
()   ;;; we need to rebind the symbol and use a different function to perform the intended operation
lispy> x
{2 3}
``` 

# Installation
This software supports all platforms but requires a C compiler, the only other dependecy you need is the editline library, on Mac it comes with Command Line Tools, on Linux you can install it `with sudo apt-get install libedit-dev` while on Fedora you can use the command `su -c "yum install libedit-dev*"`.<br>
Taken care of that you can compile the interpreter file with this command: `cc -std=c99 -Wall lispy.c mpc.c -ledit -lm -o lispy`.
#### Using cs50 ide, step by step 
Clone the repository in a folder in your ide using the command `git clone https://github.com/Federico-abss/Lispy.git` on your terminal, then write the command `sudo apt-get install libedit-dev`  and confirm the operation when asked, finally just move inside the folder and compile using `cc -std=c99 -Wall lispy.c mpc.c -ledit -lm -o lispy`. <br>
You can now execute the compiled file to start the interpreter interface!<br>
**Enjoy using Lispy!** <br>

# Credits
Cannot express my gratitude enough to Mr Daniel Holden for his incredible work on the "Build Your Own Lisp" book and MPC library I used for parsing, he is the person that made this project possible.  

* Build Your Own Lisp [Book](https://www.amazon.com/Build-Your-Own-Lisp-Programming/dp/1501006622) and [Website](http://www.buildyourownlisp.com/);
* MPC library [Github Page](https://github.com/orangeduck/mpc);
* Wikipedia for reference on [Lisp](https://en.wikipedia.org/wiki/Lisp_(programming_language)) and [Clojure](https://en.wikipedia.org/wiki/Clojure).
