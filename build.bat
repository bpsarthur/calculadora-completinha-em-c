@echo off
REM ============================================================
REM  Build da Calculadora Cientifica (TDM-GCC / MinGW)
REM ============================================================
setlocal
set RL=libs\raylib-5.5_win64_mingw-w64

gcc -O2 -std=c11 -Wall ^
    main.c expr.c ^
    -o calculadora.exe ^
    -I"%RL%\include" ^
    -L"%RL%\lib" ^
    -lraylib -lopengl32 -lgdi32 -lwinmm

if %errorlevel%==0 (
    echo.
    echo [OK] Compilado: calculadora.exe
) else (
    echo.
    echo [ERRO] Falha na compilacao.
)
endlocal
