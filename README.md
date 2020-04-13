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

### Writing external files
It's possible to load external files written in Lispy by calling the load function inside the interpreter interface or by executing it with the files to be evaluated as extra arguments.
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

## Lispy basics
There is a lot to say about Lispy, people familiar with Lisp or any of its dialect won't have any problem understanding the language, but to anyone else it may look a bit foreign, but I will try to make it as clear as possible for anyone.
### Data types
There are only 5 data type you will generally interact with, **numbers**, **strings**, **functions**, **S-expressions** and **Q-expression**. <br>
**Numbers:** Lispy supports integers and decimal numbers, they behave similarly  
**Strings**
**Functions**
**S-expressions**
**Q-expression**.
<br> 
## Installation
This software supports all platforms that have a C compiler, the only dependecy is the editline library, on Mac it comes with Command Line Tools, on Linux you can install it `with sudo apt-get install libedit-dev` while on  Fedora you can use the command `su -c "yum install libedit-dev*"`.<br>
Taken care of that you can compile lispy.c with this command `cc -std=c99 -Wall lispy.c mpc.c -ledit -lm -o lispy`.
#### Using cs50 ide, step by step 
Clone the repository in a folder in your ide using the command `git clone https://github.com/Federico-abss/Lispy.git`, <br> after this you need to install one dependency by writing the command `sudo apt-get install libedit-dev` and then compile using `cc -std=c99 -Wall lispy.c mpc.c -ledit -lm -o lispy`. You can now execute the compiled file!
#### Enjoy using Lispy!

## Credits
Cannot express my gratitude enough to Mr Daniel Holden for his incredible work on the "Build Your Own Lisp" book and MPC library, 
he is the person that made this project possible.  

* Build Your Own Lisp [Book](https://www.amazon.com/Build-Your-Own-Lisp-Programming/dp/1501006622) and [Website](http://www.buildyourownlisp.com/)
* MPC library [Github Page](https://github.com/orangeduck/mpc)
* Wikipedia for reference on [Lisp](https://en.wikipedia.org/wiki/Lisp_(programming_language)) and [Clojure](https://en.wikipedia.org/wiki/Clojure)
