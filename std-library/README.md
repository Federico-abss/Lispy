## Lispy Standard Library
I already discussed in detail about [functions](https://github.com/Federico-abss/Lispy#functions) and [builtins](https://github.com/Federico-abss/Lispy/tree/master/Builtins-Functions) in Lispy, and the ways builtins are superior to custom functions, but the functions in the stdlibrary have many advantages themselves, that make them a great way to expand the functionalities of the language and create powerful programs. <br>
To begin with, custom functions are quite simple, writing a builtin instead requires a lot of work as [this example](https://github.com/Federico-abss/Lispy/blob/master/examples/builtinfun_and_stdfunction.md) showcases, furthermore they are self documenting since when you call them without arguments the interpreter will show their definition.
```
lispy> fst
(\ {l} {eval (head l)})
;;; fst takes a single argument and applies head and eval on it
```
I will provide examples of some other functions here, but in the [stdlibrary file](https://github.com/Federico-abss/Lispy/blob/master/std-library/std_library.lspy) you can find every function prefaced with a short explanation of their use.

### Recursion in Lispy
Let me address the elephant in the room, I didn't show any for or while loops yet, how do you write an iterative function in Lispy? <br>
The short answer is that you don't. The bad news is that Lispy doesn't support iterative functions, only recursive ones. 
The good one is that the [Church-Turing thesis](https://en.wikipedia.org/wiki/Church%E2%80%93Turing_thesis) proved already that you can write every iterative program in a recursive fashion and viceversa. <br>
Let's take a very common iteration example, printing the same message multiple times, and write it recursively.
```
;;; this function takes a number n and another lvalue s, and prints s n times to the terminal
(fun {multiplePrint n s} {
  select
    { (== n 1) (print s)}
    { otherwise (print s (multiplePrint (- n 1) s))}
}) ;;; you see that the function calls itself in its definition
```
```
lispy> multiplePrint 5 "string"
"string" 
"string" () 
"string" () 
"string" () 
"string" () 
()
```
Here is other two examples of popular algorithms, Fibonacci and factorial of a number, written recursively inside the stdlibrary.
```
lispy> fib 10
55
lispy> fib
(\ {n} {select {(== n 0) 0} {(== n 1) 1} {otherwise (+ (fib (- n 1)) (fib (- n 2)))}})

lispy> fact 5
120
lispy> fact
(\ {n} {if (== n 0) {1} {* n (fact (- n 1))}})
```
### Map, Filter and Reduce
In every language, being able to manipulate lists is a powerful tool, and this is especially true in Lispy since it relies so much on qexprs! These functions also help overcome the lack of iterative loops since they are able to substitute typical `for` operations. <br>
**`map`** takes two arguments, a function and a list, and applies the function to every value inside the list and then returns it containing the modified values.
```
lispy> map - {5 6 7 8 2 22 44}
{-5 -6 -7 -8 -2 -22 -44}

lispy> map (\ {x} {+ x 10}) {5 2 11}
{15 12 21}

lispy> print {"hello" "world"}
{"hello" "world"}
()
lispy> map print {"hello" "world"}
"hello"
"world"
{() ()}
```
**`filter`** is a function which, takes in some functional condition and a list, and only includes items of the list which match that condition.
```
lispy> filter (\ {x} {> x 2}) {5 2 11 -7 8 1}
{5 11 8}
```
There is no real `reduce` in the stdlibrary, but you can use other instruments to obtain a similar result, the **`unpack`** function for example takes in a function `f` and a list `l`, and extracts every value from `l` and evaluates them using `f`.
```
lispy> unpack + {10 5 3 2}
20
lispy> unpack join {"Thanks " "Reddit " "for " "the support!"}
"Thanks Reddit for the support!"
```
