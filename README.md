# Lispy
Lispy is in interpreted, homoiconic, functional programming language based on Lisp. <br>
The interpreter for the language is written in C using the MPC library for parsing operations, and it currently can be used as 
an Interactive Prompt if executed without arguments from the command line `./lispy`, <br>
or evaluate any number of external files written in Lispy `./lispy example.lspy`

## Installation

### Using cs50 ide 
Clone the repository in a folder in your ide using the command `git clone https://github.com/Federico-abss/Lispy.git`, after this you need to install 
one dependency by writing the command `sudo apt-get install libedit-dev` and then compile using `cc -std=c99 -Wall lispy.c mpc.c -ledit -lm -o lispy`.
#### Enjoy using Lispy!

## Credits
Cannot express my gratitude enough to Mr Daniel Holden for his incredible work on the "Build Your Own Lisp" book and MPC library, 
he is the person that made this project possible.  

* Build Your Own Lisp [Book](https://www.amazon.com/Build-Your-Own-Lisp-Programming/dp/1501006622) and [Website](http://www.buildyourownlisp.com/)
* MPC library [Github Page](https://github.com/orangeduck/mpc)
* Wikipedia for reference on [Lisp](https://en.wikipedia.org/wiki/Lisp_(programming_language)) and [Clojure](https://en.wikipedia.org/wiki/Clojure)
