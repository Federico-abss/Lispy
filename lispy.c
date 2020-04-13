#include "mpc.h"
#include <stdio.h>
#include <stdlib.h>

// sudo apt-get install libedit-dev
// cc -std=c99 -Wall lispy.c mpc.c -ledit -lm -o lispy

/* If we are compiling on Windows compile these functions */
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

/* Fake readline function */
char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

/* Fake add_history function */
void add_history(char* unused) {}

/* Otherwise include the editline headers */
#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

/* function to execute power operations */
long power(long x, long y) {
    int total;
    if (y == 0) { return 1; }
    else if (y == 1) { return x; }
    else if (y == -1) { return 1 / x; }
    else if (y > 0) {
        total = x;
        total *= power(x, y - 1);
    }
    else {
        total = 1 / x;
        total *= power(x, y + 1);
    }

    return total;
}

/* Builtins macros for error checking reporting */

#define LASSERT(args, cond, fmt, ...) \
  if (!(cond)) { lval* err = lval_err(fmt, ##__VA_ARGS__); lval_del(args); return err; }

#define LASSERT_TYPE(func, args, index, expect) \
  LASSERT(args, args->cell[index]->type == expect, \
    "Function '%s' passed incorrect type for argument %i. Got %s, Expected %s.", \
    func, index, ltype_name(args->cell[index]->type), ltype_name(expect))

#define LASSERT_NUM(func, args, num) \
  LASSERT(args, args->count == num, \
    "Function '%s' passed incorrect number of arguments. Got %i, Expected %i.", \
    func, args->count, num)

#define LASSERT_NOT_EMPTY(func, args, index) \
  LASSERT(args, args->cell[index]->count != 0, \
    "Function '%s' passed {} for argument %i.", func, index);


/* forward declaration for the compiler */
struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

/* Parser Declariations */
lenv* lenv_new(void);
mpc_parser_t* Number;
mpc_parser_t* Symbol;
mpc_parser_t* String;
mpc_parser_t* Comment;
mpc_parser_t* Sexpr;
mpc_parser_t* Qexpr;
mpc_parser_t* Expr;
mpc_parser_t* Lispy;

/* Create Enumeration of Possible lval Types */
enum { LVAL_ERR, LVAL_LONG, LVAL_DOUBLE, LVAL_STR, LVAL_SYM, LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR };

/* create a function pointer type */
typedef lval*(*lbuiltin)(lenv*, lval*);

/* our basic data structures, used for numbers, errors and expressions */
typedef struct lval {
    int type;
    long num;
    double dec;
    /* Error and Symbol types have some string data */
    char* err;
    char* str;
    char* sym;
    /* function type */
    lbuiltin builtin;
    /* container for lambda functions */
    lenv* env;
    lval* formals;
    lval* body;
    /* Count and Pointer to a list of "lval*" */
    int count;
    struct lval** cell;
} lval;

/* Construct a pointer to a new integer type lval */
lval* lval_long(long x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_LONG;
    v->num = x;
    v->dec = x;
    return v;
}

/* Create  a pointer to a new decimal type lval */
lval* lval_double(double x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_DOUBLE;
    v->dec = x;
    return v;
}

/* Construct a pointer to a new Error lval */
lval* lval_err(char* fmt, ...) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;

    /* Create a va list and initialize it */
    va_list va;
    va_start(va, fmt);

    /* Allocate 512 bytes of space */
    v->err = malloc(512);

    /* printf the error string with a maximum of 511 characters */
    vsnprintf(v->err, 511, fmt, va);

    /* Reallocate to number of bytes actually used */
    v->err = realloc(v->err, strlen(v->err)+1);

    /* Cleanup our va list */
    va_end(va);

    return v;
}

/* Construct a pointer to a new String lval */
lval* lval_str(char* s) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_STR;
    v->str = malloc(strlen(s) + 1);
    strcpy(v->str, s);
    return v;
}

/* Construct a pointer to a new Symbol lval */
lval* lval_sym(char* s) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}

/* Construct a pointer to a new function lval */
lval* lval_fun(lbuiltin func) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->builtin = func;
    return v;
}

/* A pointer to a new empty Sexpr lval */
lval* lval_sexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

/* A pointer to a new empty Qexpr lval */
lval* lval_qexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

/* forward declaration for the compiler */
void lenv_del(lenv* e);
lenv* lenv_copy(lenv* e);
void lenv_put(lenv* e, lval* k, lval* v);

/* delete a lval and all its content */
void lval_del(lval* v) {

    switch (v->type) {
        case LVAL_LONG: break;
        case LVAL_DOUBLE: break;

        /* Clear lambda functions */
        case LVAL_FUN:
        if (!v->builtin) {
            lenv_del(v->env);
            lval_del(v->formals);
            lval_del(v->body);
        }
        break;

        /* For Err, Str or Sym free the string data */
        case LVAL_ERR: free(v->err); break;
        case LVAL_SYM: free(v->sym); break;
        case LVAL_STR: free(v->str); break;

        /* If Qexpr or Sexpr then delete all elements inside */
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            for (int i = 0; i < v->count; i++) {
                lval_del(v->cell[i]);
            }
            /* Also free the memory allocated to contain the pointers */
            free(v->cell);
        break;
    }

    /* Free the memory allocated for the "lval" struct itself */
    free(v);
}

/* create a copy of a lval */
lval* lval_copy(lval* v) {

    lval* x = malloc(sizeof(lval));
    x->type = v->type;

    switch (v->type) {
        case LVAL_LONG: x->num = v->num; x->dec = v->dec; break;
        case LVAL_DOUBLE: x->dec = v->dec; break;

        /* Copy for builtin and lambda functions */
        case LVAL_FUN:
            if (v->builtin) {
                x->builtin = v->builtin;
            } else {
                x->builtin = NULL;
                x->env = lenv_copy(v->env);
                x->formals = lval_copy(v->formals);
                x->body = lval_copy(v->body);
            }
        break;

        /* Copy Strings using malloc and strcpy */
        case LVAL_ERR:
            x->err = malloc(strlen(v->err) + 1);
            strcpy(x->err, v->err); break;
        case LVAL_SYM:
            x->sym = malloc(strlen(v->sym) + 1);
            strcpy(x->sym, v->sym); break;
        case LVAL_STR:
            x->str = malloc(strlen(v->str) + 1);
            strcpy(x->str, v->str); break;

        /* Copy Lists by copying each sub-expression */
        case LVAL_SEXPR:
        case LVAL_QEXPR:
            x->count = v->count;
            x->cell = malloc(sizeof(lval*) * x->count);
            for (int i = 0; i < x->count; i++) {
                x->cell[i] = lval_copy(v->cell[i]);
            }
        break;
  }

  return x;
}

/* adds elements to a sexpr but also manages the number of cells and the memory */
lval* lval_add(lval* v, lval* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count-1] = x;
    return v;
}

/* move the content of a qexpr into another qexpr */
lval* lval_join(lval* x, lval* y) {
    /* For strings */
    if ((x->type == LVAL_STR) & (y->type == LVAL_STR)) {
        char str[1024];
        strcpy(str, x->str);
        strcat(str, y->str);

        lval_del(x); lval_del(y);
        return lval_str(str);
    }

    /* For each cell in 'y' add it to 'x' */
    for (int i = 0; i < y->count; i++) {
        x = lval_add(x, y->cell[i]);
    }

    /* Delete the empty 'y' and return 'x' */
    free(y->cell); free(y);
    return x;
}

/* takes element away from sexpr and shifts the others in memory before returning extracted value */
lval* lval_pop(lval* v, int i) {
    /* Find the item at "i" */
    lval* x = v->cell[i];

    /* Shift memory after the item at "i" over the top */
    memmove(&v->cell[i], &v->cell[i+1],
        sizeof(lval*) * (v->count-i-1));

    /* Decrease the count of items in the list */
    v->count--;

    /* Reallocate the memory used */
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    return x;
}

/* takes element from sexpr then deletes the rest of it */
lval* lval_take(lval* v, int i) {
    lval* x = lval_pop(v, i);
    lval_del(v);
    return x;
}


/* Lisp Environment */


/* define the environment struct */
struct lenv {
    lenv* par;
    int count;
    char** syms;
    lval** vals;
};

/* create a new system environment */
lenv* lenv_new(void) {
    lenv* e = malloc(sizeof(lenv));
    e->par = NULL;
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}

/* Construct a pointer to a new lambda lval */
lval* lval_lambda(lval* formals, lval* body) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->builtin = NULL;
    v->env = lenv_new();
    v->formals = formals;
    v->body = body;
    return v;
}

/* delete an environment */
void lenv_del(lenv* e) {
    /* Iterate over all items in environment deleting them */
    for (int i = 0; i < e->count; i++) {
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }
    /* Free allocated memory for lists */
    free(e->syms);
    free(e->vals);
    free(e);
}

/* lookup for a value in the environment */
lval* lenv_get(lenv* e, lval* k) {

    /* Iterate over all items in environment */
    for (int i = 0; i < e->count; i++) {
        /* Check if the stored string matches the symbol string */
        /* If it does, return a copy of the value */
        if (strcmp(e->syms[i], k->sym) == 0) {
            return lval_copy(e->vals[i]);
        }
    }
    /* If no symbol check in parent otherwise error */
    if (e->par) {
        return lenv_get(e->par, k);
    } else {
        return lval_err("Unbound Symbol '%s'", k->sym);
    }
}

void lenv_def(lenv* e, lval* k, lval* v) {
    /* Iterate till e has no parent */
    while (e->par) { e = e->par; }
    /* Put value in e */
    lenv_put(e, k, v);
}

/* Insert a new value in the environment */
void lenv_put(lenv* e, lval* k, lval* v) {

    /* Iterate over all items in environment to see if variable exists */
    for (int i = 0; i < e->count; i++) {

        /* If variable is found delete it and replace it with new var */
        if (strcmp(e->syms[i], k->sym) == 0) {
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v);
            return;
        }
    }

    /* If no existing entry found allocate space for new entry */
    e->count++;
    e->vals = realloc(e->vals, sizeof(lval*) * e->count);
    e->syms = realloc(e->syms, sizeof(char*) * e->count);

    /* Copy contents of lval and symbol string into new location */
    e->vals[e->count-1] = lval_copy(v);
    e->syms[e->count-1] = malloc(strlen(k->sym)+1);
    strcpy(e->syms[e->count-1], k->sym);
}

/* copy an environment */
lenv* lenv_copy(lenv* e) {
    lenv* n = malloc(sizeof(lenv));
    n->par = e->par;
    n->count = e->count;
    n->syms = malloc(sizeof(char*) * n->count);
    n->vals = malloc(sizeof(lval*) * n->count);

    /* copy all the variables inside */
    for (int i = 0; i < e->count; i++) {
        n->syms[i] = malloc(strlen(e->syms[i]) + 1);
        strcpy(n->syms[i], e->syms[i]);
        n->vals[i] = lval_copy(e->vals[i]);
    }
    return n;
}


/* Read and print lvals */


/* list of types to use in error reporting */
char* ltype_name(int t) {
    switch(t) {
        case LVAL_FUN: return "Function";
        case LVAL_LONG: return "Integer";
        case LVAL_DOUBLE: return "Decimal";
        case LVAL_STR: return "String";
        case LVAL_ERR: return "Error";
        case LVAL_SYM: return "Symbol";
        case LVAL_SEXPR: return "S-Expression";
        case LVAL_QEXPR: return "Q-Expression";
        default: return "Unknown";
    }
}

/* if lval is of numeric type it reads it*/
lval* lval_read_num(mpc_ast_t* t) {
    errno = 0;

    /* see if a double is given by checking if contents have a decimal point */
    if (strchr(t->contents, '.') != NULL) {
        double x_double = strtof(t->contents, NULL);
        return errno != ERANGE ? lval_double(x_double) : lval_err("invalid number");
    /* otherwise treat it as a long */
    } else {
        long x_long = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_long(x_long) : lval_err("invalid number");
    }
}

/* if lval is of string type it reads it*/
lval* lval_read_str(mpc_ast_t* t) {

    /* Cut off the final quote character */
    t->contents[strlen(t->contents)-1] = '\0';
    /* Copy the string missing out the first quote character */
    char* unescaped = malloc(strlen(t->contents+1)+1);
    strcpy(unescaped, t->contents+1);

    /* Pass through the unescape function */
    unescaped = mpcf_unescape(unescaped);
    /* Construct a new lval using the string */
    lval* str = lval_str(unescaped);
    /* Free the string and return */
    free(unescaped); return str;
}

/* read other types of lval */
lval* lval_read(mpc_ast_t* t) {

    /* If Symbol or Number return conversion to that type */
    if (strstr(t->tag, "number")) { return lval_read_num(t); }
    if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }
    if (strstr(t->tag, "string")) { return lval_read_str(t); }

    /* If root (>) or sexpr or quexpr then create empty list */
    lval* x = NULL;
    if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
    if (strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }
    if (strstr(t->tag, "qexpr"))  { x = lval_qexpr(); }

    /* Fill this list with any valid expression contained within */
    for (int i = 0; i < t->children_num; i++) {
        if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
        if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
        if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
        if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
        if (strstr(t->children[i]->tag, "comment")) { continue; }
        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;
}


/* forward declarations for the compiler */
lval* lval_eval_sexpr(lenv* e, lval* v);
void lval_print(lval* v);
lval* builtin_eval(lenv* e, lval* a);
lval* builtin_list(lenv* e, lval* a);


/* execute costum functions */
lval* lval_call(lenv* e, lval* f, lval* a) {
    /* If Builtin then simply apply that */
    if (f->builtin) { return f->builtin(e, a); }

    /* Record Argument Counts */
    int given = a->count;
    int total = f->formals->count;

    while (a->count) {
        /* If we've ran out of formal arguments to bind */
        if (f->formals->count == 0) {
            lval_del(a); return lval_err(
                "Function passed too many arguments. "
                "Got %i, Expected %i.", given, total);
        }

        /* Pop the first symbol and the value next to it */
        lval* sym = lval_pop(f->formals, 0);

        /* Special Case to deal with '&' */
        if (strcmp(sym->sym, "&") == 0) {

            /* Ensure '&' is followed by another symbol */
            if (f->formals->count != 1) {
                lval_del(a);
                return lval_err("Function format invalid. "
                    "Symbol '&' not followed by single symbol.");
            }

            /* Next formal should be bound to remaining arguments */
            lval* nsym = lval_pop(f->formals, 0);
            lenv_put(f->env, nsym, builtin_list(e, a));
            lval_del(sym); lval_del(nsym);
            break;
        }

        /* Pop the next argument from the list */
        lval* val = lval_pop(a, 0);

        /* Bind a copy into of them the function's environment */
        lenv_put(f->env, sym, val);

        /* Delete symbol and value */
        lval_del(sym); lval_del(val);
    }

    lval_del(a);

    /* If '&' remains in formal list bind to empty list */
    if (f->formals->count > 0 &&
        strcmp(f->formals->cell[0]->sym, "&") == 0) {

        /* Check to ensure that & is not passed invalidly. */
        if (f->formals->count != 2) {
            return lval_err("Function format invalid. "
                "Symbol '&' not followed by single symbol.");
        }

        /* Pop and delete '&' symbol */
        lval_del(lval_pop(f->formals, 0));

        /* Pop next symbol and create empty list */
        lval* sym = lval_pop(f->formals, 0);
        lval* val = lval_qexpr();

        /* Bind to environment and delete */
        lenv_put(f->env, sym, val);
        lval_del(sym); lval_del(val);
    }

    /* If all formals have been bound evaluate */
    if (f->formals->count == 0) {

        /* Set environment parent to evaluation environment */
        f->env->par = e;

        /* Evaluate and return */
        return builtin_eval(
            f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
    } else {
        /* Otherwise return partially evaluated function */
        return lval_copy(f);
    }
}

/* identifies S-expressions and call functions to evaluate them */
lval* lval_eval(lenv* e, lval* v) {
    if (v->type == LVAL_SYM) {
        lval* x = lenv_get(e, v);
        lval_del(v); return x;
    }
    if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(e, v); }
    return v;
}

lval* builtin_exit(lenv* e, lval* a);
lval* builtin_env(lenv* e, lval* a);
/* Evaluate S-expressions */
lval* lval_eval_sexpr(lenv* e, lval* v) {

    /* Evaluate Children */
    for (int i = 0; i < v->count; i++) {
        v->cell[i] = lval_eval(e, v->cell[i]);
    }

    /* Error Checking */
    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
    }

    /* Empty Expression */
    if (v->count == 0) { return v; }

    /* special case for exit and env functions */
    if ((v->cell[0]->builtin == builtin_exit) | (v->cell[0]->builtin == builtin_env)) {
        lval* f = lval_pop(v, 0);
        lval* result = f->builtin(e, v);
        lval_del(f); return result;
    }

    /* Single Expression */
    if (v->count == 1) { return lval_take(v, 0); }

    /* Ensure first element is a function after evaluation */
    lval* f = lval_pop(v, 0);
    if (f->type != LVAL_FUN) {
        lval* err = lval_err(
            "S-Expression starts with incorrect type. "
            "Got %s, Expected %s.",
            ltype_name(f->type), ltype_name(LVAL_FUN));
        lval_del(f); lval_del(v);
        return err;
    }

    /* If so call function to get result */
    lval* result = lval_call(e, f, v);
    lval_del(f);
    return result;
}

/* equality comparison */
int lval_eq(lval* x, lval* y) {

    /* Different Types are always unequal, except integers and decimals */
    if (!(((x->type == LVAL_LONG) & (y->type == LVAL_DOUBLE)) |
        ((y->type == LVAL_LONG) & (x->type == LVAL_DOUBLE)))) {
        if (x->type != y->type) { return 0; }
    }

    /* Compare Based upon type */
    switch (x->type) {
        /* Compare Number Value */
        case LVAL_LONG:
        case LVAL_DOUBLE:
            return (x->dec == y->dec);

        /* Compare String Values */
        case LVAL_ERR: return (strcmp(x->err, y->err) == 0);
        case LVAL_SYM: return (strcmp(x->sym, y->sym) == 0);
        case LVAL_STR: return (strcmp(x->str, y->str) == 0);

        /* If builtin compare, otherwise compare formals and body */
        case LVAL_FUN:
            if (x->builtin || y->builtin) {
                return (x->builtin == y->builtin);
            } else {
                return (lval_eq(x->formals, y->formals)
                && lval_eq(x->body, y->body));
            }

        /* If list compare every individual element */
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            if (x->count != y->count) { return 0; }
            for (int i = 0; i < x->count; i++) {
                /* If any element not equal then whole list not equal */
                if (!lval_eq(x->cell[i], y->cell[i])) { return 0; }
            }
            /* Otherwise lists must be equal */
            return 1;
        break;
    }
    return 0;
}

/* Print every element of an expression */
void lval_print_expr(lval* v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {

        /* Print Value contained within */
        lval_print(v->cell[i]);

        /* Don't print trailing space if last element */
        if (i != (v->count-1)) {
            putchar(' ');
        }
    }
    putchar(close);
}

/* print strings */
void lval_print_str(lval* v) {
    /* Make a Copy of the string */
    char* escaped = malloc(strlen(v->str)+1);
    strcpy(escaped, v->str);
    /* Pass it through the escape function */
    escaped = mpcf_escape(escaped);
    /* Print it between " characters */
    printf("\"%s\"", escaped);
    /* free the copied string */
    free(escaped);
}

/* Identify and format the different types for print */
void lval_print(lval* v) {
    switch (v->type) {
        case LVAL_LONG:
            if (abs(v->num - v->dec) > 0.000001) {
                printf("%f", v->dec); break;
            }
            printf("%li", v->num); break;
        case LVAL_DOUBLE: printf("%f", v->dec); break;
        case LVAL_ERR:    printf("Error: %s", v->err); break;
        case LVAL_SYM:    printf("%s", v->sym); break;
        case LVAL_STR:    lval_print_str(v); break;
        case LVAL_FUN:
            if (v->builtin) {
                printf("<builtin>");
            } else {
                printf("(\\ "); lval_print(v->formals);
                putchar(' '); lval_print(v->body); putchar(')');
            }
        break;
        case LVAL_SEXPR:  lval_print_expr(v, '(', ')'); break;
        case LVAL_QEXPR:  lval_print_expr(v, '{', '}'); break;
    }
}

/* add a newline at the end of the expression */
void lval_println(lval* v) { lval_print(v); putchar('\n'); }


/* Builtin functions */


/* read external files */
lval* builtin_load(lenv* e, lval* a) {
    LASSERT_NUM("load", a, 1);
    LASSERT_TYPE("load", a, 0, LVAL_STR);

    /* Parse File given by string name */
    mpc_result_t r;
    if (mpc_parse_contents(a->cell[0]->str, Lispy, &r)) {

        /* Read contents */
        lval* expr = lval_read(r.output);
        mpc_ast_delete(r.output);

        /* Evaluate each Expression */
        while (expr->count) {
             lval* x = lval_eval(e, lval_pop(expr, 0));
            /* If Evaluation leads to error print it */
            if (x->type == LVAL_ERR) { lval_println(x); }
            lval_del(x);
        }

        /* Delete expressions and arguments */
        lval_del(expr);
        lval_del(a);

        /* Return empty list */
        return lval_sexpr();

    } else {
        /* Get Parse Error as String */
        char* err_msg = mpc_err_string(r.error);
        mpc_err_delete(r.error);

        /* Create new error message using it */
        lval* err = lval_err("Could not load Library %s", err_msg);
        free(err_msg);
        lval_del(a);

        /* Cleanup and return error */
        return err;
    }
}

/* print message from user input */
lval* builtin_print(lenv* e, lval* a) {

    /* Print each argument followed by a space */
    for (int i = 0; i < a->count; i++) {
        lval_print(a->cell[i]); putchar(' ');
    }

    /* Print a newline and delete arguments */
    putchar('\n');
    lval_del(a);

    return lval_sexpr();
}

/* create error message from user input */
lval* builtin_error(lenv* e, lval* a) {
    LASSERT_NUM("error", a, 1);
    LASSERT_TYPE("error", a, 0, LVAL_STR);

    /* Construct Error from first argument */
    lval* err = lval_err(a->cell[0]->str);

    /* Delete arguments and return */
    lval_del(a);
    return err;
}

/* return first element of a qexpr and deletes the rest */
lval* builtin_head(lenv* e, lval* a) {
    LASSERT_NUM("head", a, 1);
    LASSERT_NOT_EMPTY("head", a, 0);

    if (a->cell[0]->type == LVAL_QEXPR) {
        lval* v = lval_take(a, 0);
        while (v->count > 1) { lval_del(lval_pop(v, 1)); }
        return v;
    }
    /* return only first character for strings */
    else if (a->cell[0]->type == LVAL_STR) {
        char num = (a->cell[0]->str)[0];
        char letter[12];
        letter[0] = (char)num;
        lval_del(a); return lval_str(letter);
    }

    lval_del(a);
    return lval_err("Function 'head' expected a String or a Q-expression");
}

/* remove first element of a qexpr and return the rest */
lval* builtin_tail(lenv* e, lval* a) {
    LASSERT_NUM("tail", a, 1);
    LASSERT_NOT_EMPTY("tail", a, 0);

    if (a->cell[0]->type == LVAL_QEXPR) {
        lval* v = lval_take(a, 0);
        lval_del(lval_pop(v, 0));
        return v;
    }
    /* remove only first character for strings */
    else if (a->cell[0]->type == LVAL_STR) {
        char* s = (a->cell[0]->str);
        char text[strlen(s)];
        int i;

        for (i = 0; s[i+1] != '\0'; ++i) { text[i] = s[i+1]; }
        text[i] = '\0';

        lval_del(a); return lval_str(text);
    }

    lval_del(a);
    return lval_err("Function 'tail' expected a String or a Q-expression");
}

/* convert sexpr into qexpr */
lval* builtin_list(lenv* e, lval* a) {
    a->type = LVAL_QEXPR;
    return a;
}

/* convert qexpr into sexpr and evaluates it */
lval* builtin_eval(lenv* e, lval* a) {
    LASSERT_NUM("eval", a, 1);
    LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);

    lval* x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(e, x);
}

/* join multiple qexprs or strings */
lval* builtin_join(lenv* e, lval* a) {

    if (a->cell[0]->type == LVAL_QEXPR) {
        for (int i = 0; i < a->count; i++) {
            LASSERT_TYPE("join", a, i, LVAL_QEXPR);
        }
    } else {
        for (int i = 0; i < a->count; i++) {
            LASSERT_TYPE("join", a, i, LVAL_STR);
        }
    }

    lval* x = lval_pop(a, 0);
    while (a->count) {
        x = lval_join(x, lval_pop(a, 0));
    }

    lval_del(a); return x;
}

/* takes a value and a Q-Expression and appends it to the front */
lval* builtin_cons(lenv* e, lval* a) {
    LASSERT_NUM("cons", a, 2);
    LASSERT_TYPE("cons", a, 1, LVAL_QEXPR);

    lval* x = lval_qexpr();
    /* converts first argument to a qexpr if it is not already */
    if (a->cell[0]->type != LVAL_QEXPR) {
        x = lval_add(x, lval_pop(a, 0));
    }
    else { x = lval_pop(a, 0); }

    /* perform a simple union of 2 quexpr */
    x = lval_join(x, lval_take(a, 0));

    return x;
}

/* returns the number of elements in a Q-Expression */
lval* builtin_len(lenv* e, lval* a) {
    LASSERT_NUM("len", a, 1);
    LASSERT_TYPE("len", a, 0, LVAL_QEXPR);

    lval* n = lval_long(a->cell[0]->count);

    lval_del(a); return n;
}

/* returns all of a Q-Expression except the final element */
lval* builtin_init(lenv* e, lval* a) {
    LASSERT_NUM("init", a, 1);
    LASSERT_NOT_EMPTY("init", a, 0);

    if (a->cell[0]->type == LVAL_QEXPR) {
        lval* v = lval_take(a, 0);
        lval_del(lval_pop(v, v->count-1));
        return v;
    }
    else if (a->cell[0]->type == LVAL_STR) {
        if (strcmp(a->cell[0]->str, "") == 0) {
            lval_del(a);
            return lval_err("Function 'init' cannot process \"\"");
        }
        char* s = a->cell[0]->str;
        char text[strlen(s)];
        int i;

        for (i = 0; s[i+1] != '\0'; ++i) { text[i] = s[i]; }
        text[i] = '\0';

        lval_del(a); return lval_str(text);
    }

    lval_del(a);
    return lval_err("Function 'init' expected a String or a Q-expression");
}

/* returns the element from a specific index in the list */
lval* builtin_index(lenv* e, lval* a) {
    LASSERT_NUM("index", a, 2);
    LASSERT_TYPE("index", a, 0, LVAL_LONG);
    LASSERT_TYPE("index", a, 1, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("index", a, 1);

    int index = a->cell[0]->num;
    lval* list = lval_take(a, 1);
    /* error checking, controls if index is a valid positive number */
    if ((list->count <= index) | (index < 0)) {
        return lval_err("index out of range, the list has length %i", list->count);
    }

    lval* n = lval_take(list, index);
    lval* v = lval_qexpr();
    lval_add(v, n);

    return v;
}

/* curry function, insert values in a qexpr */
lval* builtin_pack(lenv* e, lval* a) {
    LASSERT_TYPE("pack", a, 0, LVAL_FUN);

    lval* eval = lval_sexpr();
    lval_add(eval, lval_pop(a, 0));
    lval* packed = lval_qexpr();

    /* extract every value after the function into the qexpr */
    while (a->count) {
        lval_add(packed, lval_pop(a, 0));
    }
    lval_add(eval, packed);

    lval_del(a); return lval_eval_sexpr(e, eval);
}

/* uncurry function, extract values from a qexpr */
lval* builtin_unpack(lenv* e, lval* a) {
    LASSERT_NUM("unpack", a, 2);
    LASSERT_TYPE("unpack", a, 0, LVAL_FUN);
    LASSERT_TYPE("unpack", a, 1, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("unpack", a, 1);

    lval* eval = lval_sexpr();
    lval_add(eval, lval_pop(a, 0));
    lval* x = lval_take(a, 0);

    /* extract every value into the sexpr then evaluate it */
    while (x->count) {
        lval_add(eval, lval_pop(x, 0));
    }

    lval_del(x); return lval_eval_sexpr(e, eval);
}

/* print out all the named values in the environment */
lval* builtin_env(lenv* e, lval* a) {
    lval* x = lval_qexpr();

    for (int i = 0; i < e->count; i++) {
        /* copy each var from the env */
        lval* y = lval_sym(e->syms[i]);
        x = lval_add(x, y);
    }

    lval_del(a); return x;
}

/* definition of lambda function */
lval* builtin_lambda(lenv* e, lval* a) {
    /* Check Two arguments, each of which are Q-Expressions */
    LASSERT_NUM("\\", a, 2);
    LASSERT_TYPE("\\", a, 0, LVAL_QEXPR);
    LASSERT_TYPE("\\", a, 1, LVAL_QEXPR);

    /* Check first Q-Expression contains only Symbols */
    for (int i = 0; i < a->cell[0]->count; i++) {
        LASSERT(a, (a->cell[0]->cell[i]->type == LVAL_SYM),
            "Cannot define non-symbol. Got %s, Expected %s.",
            ltype_name(a->cell[0]->cell[i]->type),ltype_name(LVAL_SYM));
    }

    /* Pop first two arguments and pass them to lval_lambda */
    lval* formals = lval_pop(a, 0);
    lval* body = lval_pop(a, 0);
    lval_del(a);

    return lval_lambda(formals, body);
}

/* create custom named functions using lambda functions
   chapter 12 - interesting functions - function definition */
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

    lval_del(lambda); lval_del(name); return lval_sexpr();
}

/* exit the program */
lval* builtin_exit(lenv* e, lval* a) {
    exit(0);
}

/* Use operator strings to see which operation to perform */
lval* builtin_op(lenv* e, lval* a, char* op) {

    /* Ensure all arguments are numbers */
    for (int i = 0; i < a->count; i++) {
        if ((a->cell[i]->type != LVAL_LONG) & (a->cell[i]->type != LVAL_DOUBLE)) {
            lval_del(a);
            return lval_err("Cannot operate on non-number!");
        }
    }

    /* Pop the first element */
    lval* x = lval_pop(a, 0);

    /* If no arguments and sub then perform unary negation */
    if ((strcmp(op, "-") == 0) && a->count == 0) {
        x->num = -x->num;
        x->dec = -x->dec;
    }

    /* While there are still elements remaining */
    while (a->count > 0) {

        /* Pop the next element */
        lval* y = lval_pop(a, 0);
        /* operations for integer numbers */
        if ((x->type == LVAL_LONG) & (y->type == LVAL_LONG)) {

            if (strcmp(op, "+") == 0) { x->num += y->num; }
            if (strcmp(op, "-") == 0) { x->num -= y->num; }
            if (strcmp(op, "*") == 0) { x->num *= y->num; }
            if (strcmp(op, "/") == 0) {
                if (y->num == 0) {
                    lval_del(x); lval_del(y);
                    x = lval_err("Division By Zero!"); break;
                }
                x->num /= y->num;
            }
            if (strcmp(op, "%") == 0) { x->num %= y->num; }
            if (strcmp(op, "max") == 0) { if (x->num <= y->num) { x->num = y->num; }}
            if (strcmp(op, "min") == 0) { if (x->num >= y->num) { x->num = y->num; }}
            if (strcmp(op, "^" ) == 0) { x->num = power(x->num, y->num); }

            x->dec = x->num;
        }
        else {
            /* operations for decimal numbers */
            if (strcmp(op, "+") == 0) { x->dec += y->dec; }
            if (strcmp(op, "-") == 0) { x->dec -= y->dec; }
            if (strcmp(op, "*") == 0) { x->dec *= y->dec; }
            if (strcmp(op, "/") == 0) {
                if (y->num == 0) {
                    lval_del(x); lval_del(y);
                    x = lval_err("Division By Zero!"); break;
                }
                x->dec /= y->dec;
            }
            if (strcmp(op, "max") == 0) { if (x->dec <= y->dec) { x->dec = y->dec; }}
            if (strcmp(op, "min") == 0) { if (x->dec >= y->dec) { x->dec = y->dec; }}
        }

        lval_del(y);
    }

    lval_del(a); return x;
}

/* conditional functions for number */
lval* builtin_ord(lenv* e, lval* a, char* op) {
    LASSERT_NUM(op, a, 2);

    /* ensure every argument is a number */
    if (((a->cell[0]->type != LVAL_LONG) & (a->cell[0]->type != LVAL_DOUBLE)) &
        ((a->cell[1]->type != LVAL_LONG) & (a->cell[1]->type != LVAL_DOUBLE))) {
        lval_del(a); return lval_err("Error, %s can only compare numbers", op);
    }

    int r;
    if (strcmp(op, ">")  == 0) {
        r = (a->cell[0]->dec >  a->cell[1]->dec);
    }
    if (strcmp(op, "<")  == 0) {
        r = (a->cell[0]->dec <  a->cell[1]->dec);
    }
    if (strcmp(op, ">=") == 0) {
        r = (a->cell[0]->dec >= a->cell[1]->dec);
    }
    if (strcmp(op, "<=") == 0) {
        r = (a->cell[0]->dec <= a->cell[1]->dec);
    }

    lval_del(a); return lval_long(r);
}

/* conditional 'and' 'or' */
lval* builtin_con(lenv* e, lval* a, char* op) {
    LASSERT_NUM(op, a, 2);
    LASSERT_TYPE(op, a, 1, LVAL_LONG);
    LASSERT_TYPE(op, a, 2, LVAL_LONG);

    int r = 0;
    if (strcmp(op, "and") == 0) {
        if ((a->cell[0]->num == 1) & (a->cell[1]->num == 1)) {
            r = 1;
        }
    }
    if (strcmp(op, "or") == 0) {
        if ((a->cell[0]->num == 1) | (a->cell[1]->num == 1)) {
            r = 1;
        }
    }

    lval_del(a); return lval_long(r);
}

/* conditional 'not */
lval* builtin_not(lenv* e, lval* a) {
    LASSERT_NUM("not", a, 1);
    LASSERT_TYPE("not", a, 0, LVAL_LONG);

    lval* r = lval_take(a, 0);
    r->num = !r->num;
    r->dec = r->num;

    return r;
}

/* equality functions */
lval* builtin_cmp(lenv* e, lval* a, char* op) {
    LASSERT_NUM(op, a, 2);

    int r;
    if (strcmp(op, "==") == 0) {
        r = lval_eq(a->cell[0], a->cell[1]);
    }
    else if (strcmp(op, "!=") == 0) {
        r = !(lval_eq(a->cell[0], a->cell[1]));
    }

    lval_del(a); return lval_long(r);
}

/* builtin if */
lval* builtin_if(lenv* e, lval* a) {
    LASSERT_NUM("if", a, 3);
    LASSERT_TYPE("if", a, 0, LVAL_LONG);
    LASSERT_TYPE("if", a, 1, LVAL_QEXPR);
    LASSERT_TYPE("if", a, 2, LVAL_QEXPR);

    /* Mark Both Expressions as evaluable */
    lval* x;
    a->cell[1]->type = LVAL_SEXPR;
    a->cell[2]->type = LVAL_SEXPR;

    if (a->cell[0]->num) {
        /* If condition is true evaluate first expression */
        x = lval_eval(e, lval_pop(a, 1));
    } else {
        /* Otherwise evaluate second expression */
        x = lval_eval(e, lval_pop(a, 2));
    }

    lval_del(a); return x;
}

/* define builtins to insert in the environment */
lval* builtin_add(lenv* e, lval* a) { return builtin_op(e, a, "+"); }
lval* builtin_sub(lenv* e, lval* a) { return builtin_op(e, a, "-"); }
lval* builtin_mul(lenv* e, lval* a) { return builtin_op(e, a, "*"); }
lval* builtin_div(lenv* e, lval* a) { return builtin_op(e, a, "/"); }
lval* builtin_mod(lenv* e, lval* a) { return builtin_op(e, a, "%"); }
lval* builtin_max(lenv* e, lval* a) { return builtin_op(e, a, "max"); }
lval* builtin_min(lenv* e, lval* a) { return builtin_op(e, a, "min"); }
lval* builtin_pow(lenv* e, lval* a) { return builtin_op(e, a, "^"); }
lval* builtin_gt(lenv* e, lval* a) { return builtin_ord(e, a, ">"); }
lval* builtin_lt(lenv* e, lval* a) { return builtin_ord(e, a, "<"); }
lval* builtin_ge(lenv* e, lval* a) { return builtin_ord(e, a, ">="); }
lval* builtin_le(lenv* e, lval* a) { return builtin_ord(e, a, "<="); }
lval* builtin_eq(lenv* e, lval* a) { return builtin_cmp(e, a, "=="); }
lval* builtin_ne(lenv* e, lval* a) { return builtin_cmp(e, a, "!="); }
lval* builtin_and(lenv* e, lval* a) { return builtin_con(e, a, "and"); }
lval* builtin_or(lenv* e, lval* a)  { return builtin_con(e, a, "or");  }


/* insert functionality for user to create variables */
lval* builtin_var(lenv* e, lval* a, char* func) {
    LASSERT_TYPE("def", a, 0, LVAL_QEXPR);
    lval* syms = a->cell[0];

    /* Ensure all elements of first list are symbols */
    for (int i = 0; i < syms->count; i++) {
        LASSERT(a, (syms->cell[i]->type == LVAL_SYM),
            "Function '%s' cannot define non-symbol. "
            "Got %s, Expected %s.", func,
            ltype_name(syms->cell[i]->type), ltype_name(LVAL_SYM));
    }

    /* Check correct number of symbols and values */
    LASSERT(a, (syms->count == a->count-1),
        "Function '%s' passed too many arguments for symbols. "
        "Got %i, Expected %i.", func, syms->count, a->count-1);

    /* Assign copies of values to symbols */
    for (int i = 0; i < syms->count; i++) {
        /* If 'def' define in globally. If 'put' define in locally */
        if (strcmp(func, "def") == 0) {
            lenv_def(e, syms->cell[i], a->cell[i+1]);
        }

        if (strcmp(func, "=")   == 0) {
            lenv_put(e, syms->cell[i], a->cell[i+1]);
        }
    }

    lval_del(a); return lval_sexpr();
}

/* functions for global and local assignment */
lval* builtin_def(lenv* e, lval* a) { return builtin_var(e, a, "def"); }
lval* builtin_put(lenv* e, lval* a) { return builtin_var(e, a, "="); }

/* insert functions in the environment */
void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
    lval* k = lval_sym(name);
    lval* v = lval_fun(func);
    lenv_put(e, k, v);
    lval_del(k); lval_del(v);
}

/* list of builtin functionalities */
void lenv_add_builtins(lenv* e) {
    /* environment functions */
    lenv_add_builtin(e, "def",  builtin_def);
    lenv_add_builtin(e, "=",   builtin_put);
    lenv_add_builtin(e, "env", builtin_env);
    lenv_add_builtin(e, "\\",  builtin_lambda);
    lenv_add_builtin(e, "fun",  builtin_fun);
    lenv_add_builtin(e, "exit", builtin_exit);

    /* String Functions */
    lenv_add_builtin(e, "load",  builtin_load);
    lenv_add_builtin(e, "error", builtin_error);
    lenv_add_builtin(e, "print", builtin_print);

    /* List Functions */
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "join", builtin_join);
    lenv_add_builtin(e, "cons", builtin_cons);
    lenv_add_builtin(e, "len",   builtin_len);
    lenv_add_builtin(e, "init", builtin_init);
    lenv_add_builtin(e, "index", builtin_index);
    lenv_add_builtin(e, "pack", builtin_pack);
    lenv_add_builtin(e, "unpack", builtin_unpack);

    /* Conditionals Functions */
    lenv_add_builtin(e, ">", builtin_gt);
    lenv_add_builtin(e, "<", builtin_lt);
    lenv_add_builtin(e, ">=", builtin_ge);
    lenv_add_builtin(e, "<=", builtin_le);
    lenv_add_builtin(e, "==", builtin_eq);
    lenv_add_builtin(e, "!=", builtin_ne);
    lenv_add_builtin(e, "if", builtin_if);
    lenv_add_builtin(e, "and", builtin_and);
    lenv_add_builtin(e,  "or",  builtin_or);
    lenv_add_builtin(e, "not", builtin_not);

    /* Mathematical Functions */
    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);
    lenv_add_builtin(e, "%", builtin_mod);
    lenv_add_builtin(e, "max", builtin_max);
    lenv_add_builtin(e, "min", builtin_min);
    lenv_add_builtin(e, "^", builtin_pow);
}


/* Main program where our Lisp prompt runs */
int main(int argc, char** argv){
    /* Create Some Parsers */
    Number  = mpc_new("number");
    Symbol  = mpc_new("symbol");
    String  = mpc_new("string");
    Comment = mpc_new("comment");
    Sexpr   = mpc_new("sexpr");
    Qexpr   = mpc_new("qexpr");
    Expr    = mpc_new("expr");
    Lispy   = mpc_new("lispy");


    /* Define them with the following Language */
    mpca_lang(MPCA_LANG_DEFAULT,
    "                                                    \
        number  : /[+-]?([0-9]*[.])?[0-9]+/ ;            \
        symbol  : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!^%&]+/ ;   \
        string  : /\"(\\\\.|[^\"])*\"/ ;                 \
        comment : /;[^\\r\\n]*/ ;                        \
        sexpr   : '(' <expr>* ')' ;                      \
        qexpr   : '{' <expr>* '}' ;                      \
        expr    : <number> | <symbol>  | <sexpr> |       \
                  <string> | <comment> | <qexpr> ;       \
        lispy   : /^/ <expr>* /$/ ;                      \
    ",
    Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Lispy);

    /* Print Version and Exit Information */
    puts("Lispy Version 1.0.0.1");
    puts("Hi, I am Federico and this is my version of Lisp, made especially for you with love");
    puts("Press Ctrl+c or write exit to Exit\n");

    /* initialize the environment and load std */
    lenv* e = lenv_new();
    lenv_add_builtins(e);
    lval* standard = lval_add(lval_sexpr(), lval_str("std_library.lspy"));
    lval* std = builtin_load(e, standard);

    /* if only one argument is provided start a never ending loop */
    if (argc == 1) {
        while (1) {

            /* Output our prompt and get input*/
            char* input = readline("lispy> ");

            /* Add input to history */
            add_history(input);

            /* Attempt to Parse the user Input */
            mpc_result_t r;
            if (mpc_parse("<stdin>", input, Lispy, &r)) {

                /* Evaluate the operation and then do it in C */
                lval* x = lval_eval(e, lval_read(r.output));
                lval_println(x);
                lval_del(x);
                mpc_ast_delete(r.output);
            }
            else {
                /* Otherwise Print the Error */
                mpc_err_print(r.error);
                mpc_err_delete(r.error);
            }

            free(input);
        }
    }

    /* instead if supplied with list of files */
    if (argc >= 2) {

        /* loop over each supplied filename (starting from 1) */
        for (int i = 1; i < argc; i++) {

            /* Argument list with a single argument, the filename */
            lval* args = lval_add(lval_sexpr(), lval_str(argv[i]));

            /* Pass to builtin load and get the result */
            lval* x = builtin_load(e, args);

            /* If the result is an error be sure to print it */
            if (x->type == LVAL_ERR) { lval_println(x); }
            lval_del(x);
        }
    }

    /* Undefine and Delete our Parsers and env before exiting the code */
    mpc_cleanup(8, Number, Symbol, String, Comment, Sexpr,  Qexpr,  Expr, Lispy);

    lval_del(std); lenv_del(e); return 0;
}