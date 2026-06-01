/* expr.h - Avaliador de expressoes matematicas (parser recursivo descendente)
 *
 * Suporta:
 *   - operadores: + - * / % ^  (^ associativo a direita), - unario, ! (fatorial pos-fixo)
 *   - multiplicacao implicita: 2x, 2(3+1), (x+1)(x-1), 2sin(x)
 *   - parenteses ( ) e modulo/valor absoluto |x|
 *   - funcoes: sin cos tan asin acos atan sinh cosh tanh asinh acosh atanh
 *              ln log log2 exp sqrt cbrt abs sign floor ceil round trunc frac
 *              gamma fact deg rad min max mod pow atan2 hypot gcd lcm
 *              comb perm root nthroot
 *   - constantes: pi e phi tau
 *   - variaveis definidas pelo usuario + variavel especial "x" (para graficos) e "ans"
 */
#ifndef EXPR_H
#define EXPR_H

#define EXPR_MAX_VARS 64
#define EXPR_NAME_LEN 32

typedef enum { ANGLE_RAD = 0, ANGLE_DEG = 1, ANGLE_GRAD = 2 } AngleMode;

typedef struct {
    char  name[EXPR_NAME_LEN];
    double value;
} ExprVar;

typedef struct {
    ExprVar   vars[EXPR_MAX_VARS];
    int       var_count;
    AngleMode angle;     /* modo de angulo usado por sin/cos/tan e inversas */
    /* estado interno do parser (preenchido durante eval) */
    const char *src;
    const char *p;
    int         ok;       /* 1 = sucesso, 0 = erro */
    char        error[128];
} ExprContext;

/* Inicializa o contexto (limpa variaveis, modo radianos). */
void expr_init(ExprContext *ctx);

/* Define/atualiza uma variavel. Retorna 1 se ok. */
int  expr_set_var(ExprContext *ctx, const char *name, double value);

/* Le uma variavel; *found indica se existe (pode ser NULL). */
double expr_get_var(ExprContext *ctx, const char *name, int *found);

/* Avalia a expressao. Em erro, ctx->ok=0 e ctx->error contem a mensagem. */
double expr_eval(ExprContext *ctx, const char *expression);

#endif /* EXPR_H */
