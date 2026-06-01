# Calculadora Científica / Gráfica em C

Calculadora completa escrita em **C (C11)** com interface gráfica usando
**raylib** (renderização) + **raygui** (widgets). **Tema claro e escuro**
(botão no canto superior direito), layout centralizado, 8 abas.

> Observação: os rótulos da interface são em português **sem acentos** de
> propósito — a fonte embutida do raygui só tem caracteres ASCII. A lógica e
> os cálculos não são afetados.

## Como compilar e rodar

Você já tem o **TDM-GCC (MinGW)**. Na pasta do projeto:

```bat
build.bat
calculadora.exe
```

Ou com make:

```bat
mingw32-make
mingw32-make run
```

O `build.bat`/`Makefile` já apontam para a raylib que baixei em
`libs/raylib-5.5_win64_mingw-w64`. Bibliotecas de sistema usadas no link:
`-lraylib -lopengl32 -lgdi32 -lwinmm`.

## Arquivos

| Arquivo            | Conteúdo                                                        |
|--------------------|----------------------------------------------------------------|
| `expr.h` / `expr.c`| Avaliador de expressões (parser recursivo descendente)         |
| `main.c`           | Interface gráfica e todas as abas                              |
| `build.bat`        | Script de compilação (Windows)                                 |
| `Makefile`         | Alternativa via `mingw32-make`                                 |
| `libs/`            | raylib + raygui (cabeçalhos e `libraylib.a`)                   |

## Abas e funcionalidades

### 1. Científica
- Teclado numérico + funções: `sin cos tan asin acos atan sinh cosh tanh`,
  `ln log log2 exp sqrt cbrt abs`, potência `^`, fatorial `!`, módulo `%`.
- Constantes: `pi e phi tau`.
- Modos de ângulo **RAD / DEG / GRAD**.
- Multiplicação implícita: `2x`, `2(3+1)`, `2sin(pi/2)`.
- Valor absoluto com `|x|`.
- **Memória**: MC, MR, M+, M-.
- **Histórico** clicável (reaproveita a expressão).
- **Variáveis do usuário**: digite `a = 3*4` e use `a` ou `ans` depois.
- Aceita **teclado físico** (digitação, Backspace, Enter avalia, Del limpa).

### 2. Gráfico
- Até **4 funções** f(x) simultâneas, com cor e liga/desliga.
- Janela ajustável (Xmin/Xmax/Ymin/Ymax), **ajuste automático de Y**.
- **Pan** (arrastar com o mouse), **zoom** (roda do mouse ou botões).
- Eixos, grade com escala automática e **trace** (coordenada sob o cursor).

### 3. Cálculo
- `f(a)`, **derivada** `f'(a)` e segunda derivada `f''(a)` (diferenças finitas).
- **Integral** definida em `[a,b]` (regra de Simpson, `n` subdivisões).
- **Raiz** em `[a,b]` por bisseção.
- **Somatório** e **produtório** de `f(i)` para `i = a..b`.

### 4. Matrizes (até 6×6)
- A+B, A−B, A×B, transposta, **determinante** e **inversa** (Gauss-Jordan).

### 5. Estatística
- n, soma, média, mín, máx, amplitude, mediana, variância e desvio padrão
  (populacional e amostral).
- **Regressão linear** (a, b, coeficiente r e r²).

### 6. Bases
- Conversão **DEC ↔ HEX ↔ OCT ↔ BIN**.
- Operações bit a bit: AND, OR, XOR, NOT, deslocamentos `<<` e `>>`.

### 7. Complexos
- A±B, A×B, A/B, módulo, argumento, conjugado, A², forma polar.

### 8. Conversão de unidades
- Comprimento, massa, área, volume, tempo, velocidade, dados, ângulo,
  pressão, energia e **temperatura** (C/F/K).
- **Moeda com cotação ao vivo**: categoria "Moeda (online)" busca as taxas de
  câmbio reais na hora do cálculo (USD, BRL, EUR, GBP, JPY, CNY, CAD, AUD,
  CHF, ARS, MXN, INR). A busca roda em **segundo plano** (não trava a tela) e
  mostra o horário da cotação. Requer internet.
  - Fonte: [open.er-api.com](https://open.er-api.com) (gratuita, sem chave).
  - Implementado em C com **WinINet** (`net.c`), isolado da raylib para evitar
    conflitos da `windows.h`. Link: `-lwininet`.

## Sintaxe aceita pelo avaliador

```
+  -  *  /  %  ^        (^ associa à direita)
-x  +x                  (unário)
n!                      (fatorial pós-fixo)
2x   2(3)   (x+1)(x-1)  (multiplicação implícita)
|x|                     (valor absoluto)
funções: sin cos tan asin acos atan sec csc cot
         sinh cosh tanh asinh acosh atanh
         ln log(x) log(base,x) log2 log10 exp
         sqrt cbrt abs sign floor ceil round trunc frac
         gamma fact deg rad min max mod pow atan2 hypot
         gcd lcm comb perm root/nthroot
constantes: pi e phi tau inf nan
variáveis: x, ans e as que você definir (ex.: a = 5)
```
