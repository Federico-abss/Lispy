An example of the simplicity of using custom functions compared to writing your own builtins in C. <br>
Builtin `fun` and stdlibrary `function` do exactly the same thing behind the scenes, allowing you to write custom functions using Lispy, but I wrote the functionality twice to highlight the difference in approach between them. <br>

This snippet of code shows how builtin `fun` is written using C:

```
/* create custom named functions using lambda functions */
lval* builtin_fun(lenv* e, lval* a) {
    LASSERT_NUM("fun", a, 2);
    LASSERT_TYPE("fun", a, 0, LVAL_QEXPR);
    LASSERT_TYPE("fun", a, 1, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("fun", a, 0);
    LASSERT_NOT_EMPTY("fun", a, 1);

    /* create lambda function that gets called when we call costum function */
    lval* body = lval_pop(a, 1);
    lval* args = builtin_tail(e, (lval_copy(a)));
    lval* lambda = lval_lambda(args, body);

    /* bind custom name to the lambda function in the environment */
    lval* name = lval_take(builtin_head(e, a), 0);
    lenv_def(e, name, lambda);

    lval_del(lambda); 
    lval_del(name); 
    return lval_sexpr();
}
```

It amounts to 20 lines of code, 9 functions calls and a ton of pointers. Took me about 10 hours of work.

Here instead is `function` as written in the stdlibrary:

```
; Function Definitions
(def {function} (\ {f b} {
  def (head f) (\ (tail f) b)
}))
```
