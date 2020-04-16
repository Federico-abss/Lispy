# Functions
I briefly spoke about functions in the [intro section](https://github.com/Federico-abss/Lispy#functions) already, here I will go more in
detail on builtin functions and what happens when you call one in Lispy. <br>
In the introduction I also described Lispy as an **homoiconic** language, homoiconicity (code as data) is when a program can access its 
own functions and procedures while running, and programmatically alter itself on the fly.
This is possible in Lispy because functions are considered data typed like numbers and strings, and can be modified by other functions 
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
when it sees the symbol associated to it. That's how I inserted basic functionalities into the language like arithmetic operations and
list manipulations, but it doesn't stop here, from these very simple builtins you can write complex functions like the ones in the 
stdlibrary and expand the functionalities of the language without writing a single line of C. <br>
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
