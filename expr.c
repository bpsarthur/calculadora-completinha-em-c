/* expr.c - implementacao do avaliador de expressoes */
#include "expr.h"
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_E
#define M_E 2.71828182845904523536
#endif
#define PHI 1.61803398874989484820
#define TAU (2.0*M_PI)

static jmp_buf g_jmp;   /* usado para abortar em caso de erro de sintaxe */

static void fail(ExprContext *ctx, const char *msg) {
    ctx->ok = 0;
    snprintf(ctx->error, sizeof(ctx->error), "%s", msg);
    longjmp(g_jmp, 1);
}

/* ---- utilitarios de angulo ---- */
static double to_rad(ExprContext *ctx, double a) {
    switch (ctx->angle) {
        case ANGLE_DEG:  return a * M_PI / 180.0;
        case ANGLE_GRAD: return a * M_PI / 200.0;
        default:         return a;
    }
}
static double from_rad(ExprContext *ctx, double a) {
    switch (ctx->angle) {
        case ANGLE_DEG:  return a * 180.0 / M_PI;
        case ANGLE_GRAD: return a * 200.0 / M_PI;
        default:         return a;
    }
}

/* fatorial generalizado via funcao gamma para reais */
static double fac(double x) {
    if (x < 0 && floor(x) == x) return NAN;       /* polo nos inteiros negativos */
    if (floor(x) == x && x <= 170) {              /* inteiro: produto exato-ish */
        double r = 1.0; for (int i = 2; i <= (int)x; i++) r *= i; return r;
    }
    return tgamma(x + 1.0);
}
static double ncomb(double n, double k) { return fac(n) / (fac(k) * fac(n - k)); }
static double nperm(double n, double k) { return fac(n) / fac(n - k); }
static double dgcd(double a, double b) {
    long long x = (long long)llabs((long long)a), y = (long long)llabs((long long)b);
    while (y) { long long t = x % y; x = y; y = t; } return (double)x;
}

/* ---- gerenciamento de variaveis ---- */
void expr_init(ExprContext *ctx) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->angle = ANGLE_RAD;
    ctx->ok = 1;
}

int expr_set_var(ExprContext *ctx, const char *name, double value) {
    for (int i = 0; i < ctx->var_count; i++)
        if (strcmp(ctx->vars[i].name, name) == 0) { ctx->vars[i].value = value; return 1; }
    if (ctx->var_count >= EXPR_MAX_VARS) return 0;
    snprintf(ctx->vars[ctx->var_count].name, EXPR_NAME_LEN, "%s", name);
    ctx->vars[ctx->var_count].value = value;
    ctx->var_count++;
    return 1;
}

double expr_get_var(ExprContext *ctx, const char *name, int *found) {
    for (int i = 0; i < ctx->var_count; i++)
        if (strcmp(ctx->vars[i].name, name) == 0) { if (found) *found = 1; return ctx->vars[i].value; }
    if (found) *found = 0;
    return 0.0;
}

/* ---- tokenizacao basica ---- */
static void skip_ws(ExprContext *ctx) { while (*ctx->p == ' ' || *ctx->p == '\t') ctx->p++; }

/* protótipos */
static double parse_expr(ExprContext *ctx);
static double parse_add(ExprContext *ctx);
static double parse_mul(ExprContext *ctx);
static double parse_pow(ExprContext *ctx);
static double parse_unary(ExprContext *ctx);
static double parse_postfix(ExprContext *ctx);
static double parse_atom(ExprContext *ctx);

static int starts_atom(char c) {
    return isdigit((unsigned char)c) || c == '.' || c == '(' || c == '|' ||
           isalpha((unsigned char)c) || c == '_';
}

/* aplica uma funcao por nome a uma lista de argumentos */
static double apply_func(ExprContext *ctx, const char *name, double *a, int n) {
    #define F1(fn) do{ if(n!=1) fail(ctx,"numero de argumentos invalido"); return fn(a[0]); }while(0)
    #define F2(fn) do{ if(n!=2) fail(ctx,"numero de argumentos invalido"); return fn(a[0],a[1]); }while(0)
    if (!strcmp(name,"sin"))  { if(n!=1) fail(ctx,"sin(x)"); return sin(to_rad(ctx,a[0])); }
    if (!strcmp(name,"cos"))  { if(n!=1) fail(ctx,"cos(x)"); return cos(to_rad(ctx,a[0])); }
    if (!strcmp(name,"tan"))  { if(n!=1) fail(ctx,"tan(x)"); return tan(to_rad(ctx,a[0])); }
    if (!strcmp(name,"asin")) { if(n!=1) fail(ctx,"asin(x)"); return from_rad(ctx,asin(a[0])); }
    if (!strcmp(name,"acos")) { if(n!=1) fail(ctx,"acos(x)"); return from_rad(ctx,acos(a[0])); }
    if (!strcmp(name,"atan")) { if(n!=1) fail(ctx,"atan(x)"); return from_rad(ctx,atan(a[0])); }
    if (!strcmp(name,"sec"))  { if(n!=1) fail(ctx,"sec(x)"); return 1.0/cos(to_rad(ctx,a[0])); }
    if (!strcmp(name,"csc"))  { if(n!=1) fail(ctx,"csc(x)"); return 1.0/sin(to_rad(ctx,a[0])); }
    if (!strcmp(name,"cot"))  { if(n!=1) fail(ctx,"cot(x)"); return 1.0/tan(to_rad(ctx,a[0])); }
    if (!strcmp(name,"sinh")) F1(sinh);
    if (!strcmp(name,"cosh")) F1(cosh);
    if (!strcmp(name,"tanh")) F1(tanh);
    if (!strcmp(name,"asinh")) F1(asinh);
    if (!strcmp(name,"acosh")) F1(acosh);
    if (!strcmp(name,"atanh")) F1(atanh);
    if (!strcmp(name,"ln"))   F1(log);
    if (!strcmp(name,"exp"))  F1(exp);
    if (!strcmp(name,"sqrt")) F1(sqrt);
    if (!strcmp(name,"cbrt")) F1(cbrt);
    if (!strcmp(name,"abs"))  F1(fabs);
    if (!strcmp(name,"floor")) F1(floor);
    if (!strcmp(name,"ceil")) F1(ceil);
    if (!strcmp(name,"round")) F1(round);
    if (!strcmp(name,"trunc")) F1(trunc);
    if (!strcmp(name,"gamma")) F1(tgamma);
    if (!strcmp(name,"sign"))  { if(n!=1) fail(ctx,"sign(x)"); return (a[0]>0)-(a[0]<0); }
    if (!strcmp(name,"frac"))  { if(n!=1) fail(ctx,"frac(x)"); return a[0]-trunc(a[0]); }
    if (!strcmp(name,"fact"))  { if(n!=1) fail(ctx,"fact(x)"); return fac(a[0]); }
    if (!strcmp(name,"deg"))   { if(n!=1) fail(ctx,"deg(x)"); return a[0]*180.0/M_PI; }
    if (!strcmp(name,"rad"))   { if(n!=1) fail(ctx,"rad(x)"); return a[0]*M_PI/180.0; }
    if (!strcmp(name,"log"))   { /* log(x)=base10 ; log(b,x)=base b */
        if (n==1) return log10(a[0]);
        if (n==2) return log(a[1])/log(a[0]);
        fail(ctx,"log(x) ou log(base,x)");
    }
    if (!strcmp(name,"log2")) F1(log2);
    if (!strcmp(name,"log10")) F1(log10);
    if (!strcmp(name,"min"))  { if(n<1) fail(ctx,"min(...)"); double m=a[0]; for(int i=1;i<n;i++) if(a[i]<m)m=a[i]; return m; }
    if (!strcmp(name,"max"))  { if(n<1) fail(ctx,"max(...)"); double m=a[0]; for(int i=1;i<n;i++) if(a[i]>m)m=a[i]; return m; }
    if (!strcmp(name,"mod"))  F2(fmod);
    if (!strcmp(name,"pow"))  F2(pow);
    if (!strcmp(name,"atan2")) { if(n!=2) fail(ctx,"atan2(y,x)"); return from_rad(ctx,atan2(a[0],a[1])); }
    if (!strcmp(name,"hypot")) F2(hypot);
    if (!strcmp(name,"gcd"))  F2(dgcd);
    if (!strcmp(name,"lcm"))  { if(n!=2) fail(ctx,"lcm(a,b)"); double g=dgcd(a[0],a[1]); return g==0?0:fabs(a[0]*a[1])/g; }
    if (!strcmp(name,"comb")) F2(ncomb);
    if (!strcmp(name,"perm")) F2(nperm);
    if (!strcmp(name,"root")) { if(n!=2) fail(ctx,"root(n,x)"); return pow(a[1],1.0/a[0]); }
    if (!strcmp(name,"nthroot")) { if(n!=2) fail(ctx,"nthroot(n,x)"); return pow(a[1],1.0/a[0]); }
    fail(ctx, "funcao desconhecida");
    return 0.0;
    #undef F1
    #undef F2
}

static double parse_atom(ExprContext *ctx) {
    skip_ws(ctx);
    char c = *ctx->p;

    if (c == '(') {
        ctx->p++;
        double v = parse_expr(ctx);
        skip_ws(ctx);
        if (*ctx->p != ')') fail(ctx, "parentese ')' esperado");
        ctx->p++;
        return v;
    }
    if (c == '|') {  /* valor absoluto |x| */
        ctx->p++;
        double v = parse_expr(ctx);
        skip_ws(ctx);
        if (*ctx->p != '|') fail(ctx, "barra '|' de fechamento esperada");
        ctx->p++;
        return fabs(v);
    }
    if (isdigit((unsigned char)c) || c == '.') {
        char *end;
        double v = strtod(ctx->p, &end);
        if (end == ctx->p) fail(ctx, "numero invalido");
        ctx->p = end;
        return v;
    }
    if (isalpha((unsigned char)c) || c == '_') {
        char name[EXPR_NAME_LEN]; int i = 0;
        while ((isalnum((unsigned char)*ctx->p) || *ctx->p == '_') && i < EXPR_NAME_LEN - 1)
            name[i++] = *ctx->p++;
        name[i] = '\0';
        skip_ws(ctx);
        if (*ctx->p == '(') {  /* chamada de funcao */
            ctx->p++;
            double args[16]; int n = 0;
            skip_ws(ctx);
            if (*ctx->p != ')') {
                for (;;) {
                    if (n >= 16) fail(ctx, "argumentos demais");
                    args[n++] = parse_expr(ctx);
                    skip_ws(ctx);
                    if (*ctx->p == ',') { ctx->p++; continue; }
                    break;
                }
            }
            skip_ws(ctx);
            if (*ctx->p != ')') fail(ctx, "')' esperado na chamada de funcao");
            ctx->p++;
            return apply_func(ctx, name, args, n);
        }
        /* constantes */
        if (!strcmp(name, "pi"))  return M_PI;
        if (!strcmp(name, "e"))   return M_E;
        if (!strcmp(name, "phi")) return PHI;
        if (!strcmp(name, "tau")) return TAU;
        if (!strcmp(name, "inf")) return INFINITY;
        if (!strcmp(name, "nan")) return NAN;
        /* variavel */
        int found = 0;
        double v = expr_get_var(ctx, name, &found);
        if (!found) fail(ctx, "variavel/identificador desconhecido");
        return v;
    }
    fail(ctx, "token inesperado");
    return 0.0;
}

static double parse_postfix(ExprContext *ctx) {
    double v = parse_atom(ctx);
    for (;;) {
        skip_ws(ctx);
        if (*ctx->p == '!') { ctx->p++; v = fac(v); }
        else break;
    }
    return v;
}

static double parse_unary(ExprContext *ctx) {
    skip_ws(ctx);
    if (*ctx->p == '-') { ctx->p++; return -parse_unary(ctx); }
    if (*ctx->p == '+') { ctx->p++; return  parse_unary(ctx); }
    return parse_postfix(ctx);
}

static double parse_pow(ExprContext *ctx) {
    double base = parse_unary(ctx);
    skip_ws(ctx);
    if (*ctx->p == '^') {
        ctx->p++;
        double exp_ = parse_pow(ctx);   /* associativo a direita */
        return pow(base, exp_);
    }
    return base;
}

static double parse_mul(ExprContext *ctx) {
    double v = parse_pow(ctx);
    for (;;) {
        skip_ws(ctx);
        char c = *ctx->p;
        if (c == '*') { ctx->p++; v *= parse_pow(ctx); }
        else if (c == '/') { ctx->p++; double d = parse_pow(ctx); v /= d; }
        else if (c == '%') { ctx->p++; v = fmod(v, parse_pow(ctx)); }
        else if (starts_atom(c) && c != '|') {
            /* multiplicacao implicita: 2x, 2(3), 2sin(x). |x| nao implicito p/ evitar ambiguidade */
            v *= parse_pow(ctx);
        }
        else break;
    }
    return v;
}

static double parse_add(ExprContext *ctx) {
    double v = parse_mul(ctx);
    for (;;) {
        skip_ws(ctx);
        char c = *ctx->p;
        if (c == '+') { ctx->p++; v += parse_mul(ctx); }
        else if (c == '-') { ctx->p++; v -= parse_mul(ctx); }
        else break;
    }
    return v;
}

static double parse_expr(ExprContext *ctx) { return parse_add(ctx); }

double expr_eval(ExprContext *ctx, const char *expression) {
    ctx->src = expression;
    ctx->p   = expression;
    ctx->ok  = 1;
    ctx->error[0] = '\0';

    if (setjmp(g_jmp)) {
        return NAN;  /* erro: ctx->error ja preenchido */
    }
    skip_ws(ctx);
    if (*ctx->p == '\0') { fail(ctx, "expressao vazia"); }
    double v = parse_expr(ctx);
    skip_ws(ctx);
    if (*ctx->p != '\0') fail(ctx, "caractere inesperado no fim");
    return v;
}
