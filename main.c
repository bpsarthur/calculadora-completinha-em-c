/* ============================================================================
 *  CALCULADORA CIENTIFICA / GRAFICA  -  C + raylib + raygui
 *  Abas: Cientifica | Grafico | Calculo | Matrizes | Estatistica |
 *        Bases | Complexos | Conversao
 *  Temas: claro e escuro (botao no canto superior direito).
 *  (Rotulos sem acento de proposito: a fonte padrao do raygui e ASCII.)
 * ==========================================================================*/
#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "expr.h"
#include "net.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ----------------------------------------------------------------------------
 *  Estado global
 * --------------------------------------------------------------------------*/
static ExprContext ctx;
static char  display[256] = "";
static double memval = 0.0;
static int   angle_sel = 0;             /* 0=RAD 1=DEG 2=GRAD */

#define HIST_MAX 128
static char  hist[HIST_MAX][128];
static int   hist_count = 0;

static int g_editId   = -1;
static int g_idCounter = 0;

/* ----------------------------------------------------------------------------
 *  Tema
 * --------------------------------------------------------------------------*/
typedef struct {
    Color winBg, header, title, sub;
    Color dispBg, dispBorder, dispText, dispPreview;
    Color bNum, bOp, bFun, bAccent, bDanger, bTxt;     /* botoes */
    Color plotBg, plotGrid, plotAxis, plotLabel;
} Theme;

static Theme TH;
static bool  darkMode = true;

static unsigned int U32(Color c){ return ((unsigned)c.r<<24)|((unsigned)c.g<<16)|((unsigned)c.b<<8)|c.a; }

static void apply_theme(bool dark){
    GuiLoadStyleDefault();
    GuiSetStyle(DEFAULT, TEXT_SIZE, 18);
    GuiSetStyle(DEFAULT, BORDER_WIDTH, 1);

    if (dark){
        TH.winBg=(Color){24,28,38,255}; TH.header=(Color){90,170,255,255};
        TH.title=(Color){223,230,238,255}; TH.sub=(Color){120,130,150,255};
        TH.dispBg=(Color){15,19,27,255}; TH.dispBorder=(Color){58,70,90,255};
        TH.dispText=(Color){240,245,252,255}; TH.dispPreview=(Color){120,180,255,255};
        TH.bNum=(Color){45,53,68,255}; TH.bOp=(Color){54,74,104,255};
        TH.bFun=(Color){40,60,55,255}; TH.bAccent=(Color){45,120,230,255};
        TH.bDanger=(Color){150,60,70,255}; TH.bTxt=(Color){230,236,245,255};
        TH.plotBg=(Color){16,20,28,255}; TH.plotGrid=(Color){38,46,60,255};
        TH.plotAxis=(Color){140,150,170,255}; TH.plotLabel=(Color){120,130,150,255};

        GuiSetStyle(DEFAULT, BACKGROUND_COLOR,    0x1b2230ff);
        GuiSetStyle(DEFAULT, LINE_COLOR,          0x3a4658ff);
        GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, 0x3a4658ff);
        GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL,   0x2d3544ff);
        GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL,   0xd7e0eeff);
        GuiSetStyle(DEFAULT, BORDER_COLOR_FOCUSED,0x5aa0ffff);
        GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED,  0x37506fff);
        GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED,  0xffffffff);
        GuiSetStyle(DEFAULT, BORDER_COLOR_PRESSED,0x5aa0ffff);
        GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED,  0x3d5fa0ff);
        GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED,  0xffffffff);
        GuiSetStyle(DEFAULT, BASE_COLOR_DISABLED, 0x222a38ff);
        GuiSetStyle(DEFAULT, TEXT_COLOR_DISABLED, 0x66718aff);
    } else {
        TH.winBg=(Color){237,241,247,255}; TH.header=(Color){30,110,210,255};
        TH.title=(Color){30,40,55,255}; TH.sub=(Color){120,130,145,255};
        TH.dispBg=(Color){255,255,255,255}; TH.dispBorder=(Color){180,190,205,255};
        TH.dispText=(Color){25,32,45,255}; TH.dispPreview=(Color){60,120,200,255};
        TH.bNum=(Color){255,255,255,255}; TH.bOp=(Color){223,233,248,255};
        TH.bFun=(Color){226,242,232,255}; TH.bAccent=(Color){40,120,235,255};
        TH.bDanger=(Color){235,150,160,255}; TH.bTxt=(Color){35,45,60,255};
        TH.plotBg=(Color){252,253,255,255}; TH.plotGrid=(Color){222,228,238,255};
        TH.plotAxis=(Color){120,130,150,255}; TH.plotLabel=(Color){140,150,165,255};

        GuiSetStyle(DEFAULT, BACKGROUND_COLOR,    0xedf1f7ff);
        GuiSetStyle(DEFAULT, LINE_COLOR,          0xc2ccd8ff);
        GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, 0xb4becbff);
        GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL,   0xffffffff);
        GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL,   0x1e2836ff);
        GuiSetStyle(DEFAULT, BORDER_COLOR_FOCUSED,0x2878ebff);
        GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED,  0xdde9fbff);
        GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED,  0x153a78ff);
        GuiSetStyle(DEFAULT, BORDER_COLOR_PRESSED,0x2878ebff);
        GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED,  0xbcd6ffff);
        GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED,  0x10305fff);
        GuiSetStyle(DEFAULT, BASE_COLOR_DISABLED, 0xe2e7eeff);
        GuiSetStyle(DEFAULT, TEXT_COLOR_DISABLED, 0xa0a8b4ff);
    }
}

/* ----------------------------------------------------------------------------
 *  Helpers gerais
 * --------------------------------------------------------------------------*/
static Rectangle R(float x, float y, float w, float h){ Rectangle r={x,y,w,h}; return r; }

static const char* fmtnum(double v){
    if (isnan(v)) return "erro";
    if (isinf(v)) return v<0 ? "-infinito" : "infinito";
    if (v==0) return "0";
    return TextFormat("%.10g", v);
}

static void hist_push(const char* line){
    if (hist_count >= HIST_MAX){
        memmove(hist[0], hist[1], sizeof(hist[0])*(HIST_MAX-1));
        hist_count = HIST_MAX-1;
    }
    snprintf(hist[hist_count++], 128, "%s", line);
}

static void disp_insert(const char* s){
    size_t l = strlen(display), a = strlen(s);
    if (l + a < sizeof(display)-1){ memcpy(display+l, s, a); display[l+a]='\0'; }
}
static void disp_backspace(void){ size_t l=strlen(display); if(l) display[l-1]='\0'; }

static int is_ident(const char* s){
    if (!*s || !(isalpha((unsigned char)*s)||*s=='_')) return 0;
    for (const char* p=s+1; *p; ++p) if(!(isalnum((unsigned char)*p)||*p=='_')) return 0;
    return 1;
}
static void trim(char* s){
    char* p=s; while(*p==' '||*p=='\t')p++;
    if(p!=s) memmove(s,p,strlen(p)+1);
    size_t l=strlen(s); while(l&&(s[l-1]==' '||s[l-1]=='\t'))s[--l]='\0';
}

static void do_eval(void){
    if (display[0]=='\0') return;
    ctx.angle = (AngleMode)angle_sel;
    char* eq = strchr(display, '=');
    if (eq){
        char name[64]; size_t nl = (size_t)(eq-display);
        if (nl < sizeof(name)){
            memcpy(name, display, nl); name[nl]='\0'; trim(name);
            if (is_ident(name)){
                double v = expr_eval(&ctx, eq+1);
                if (ctx.ok){ expr_set_var(&ctx,name,v); hist_push(TextFormat("%s = %s",name,fmtnum(v)));
                             snprintf(display,sizeof(display),"%s",fmtnum(v)); }
                else hist_push(TextFormat("ERRO: %s", ctx.error));
                return;
            }
        }
    }
    double v = expr_eval(&ctx, display);
    if (ctx.ok){
        hist_push(TextFormat("%s = %s", display, fmtnum(v)));
        expr_set_var(&ctx,"ans",v);
        snprintf(display,sizeof(display),"%s",fmtnum(v));
    } else hist_push(TextFormat("ERRO: %s", ctx.error));
}

static int EditBox(Rectangle r, char* buf, int size){
    int id = g_idCounter++;
    bool em = (g_editId == id);
    if (GuiTextBox(r, buf, size, em)){ g_editId = em ? -1 : id; return 1; }
    return 0;
}
#define EditNum EditBox

/* botao colorido (define cor base, restaura depois) */
static bool CBtn(Rectangle r, const char* label, Color base, Color text){
    int oN=GuiGetStyle(BUTTON,BASE_COLOR_NORMAL),  oF=GuiGetStyle(BUTTON,BASE_COLOR_FOCUSED),
        oP=GuiGetStyle(BUTTON,BASE_COLOR_PRESSED), oTN=GuiGetStyle(BUTTON,TEXT_COLOR_NORMAL),
        oTF=GuiGetStyle(BUTTON,TEXT_COLOR_FOCUSED),oTP=GuiGetStyle(BUTTON,TEXT_COLOR_PRESSED);
    GuiSetStyle(BUTTON,BASE_COLOR_NORMAL,  (int)U32(base));
    GuiSetStyle(BUTTON,BASE_COLOR_FOCUSED, (int)U32(ColorBrightness(base, 0.18f)));
    GuiSetStyle(BUTTON,BASE_COLOR_PRESSED, (int)U32(ColorBrightness(base, 0.35f)));
    GuiSetStyle(BUTTON,TEXT_COLOR_NORMAL,  (int)U32(text));
    GuiSetStyle(BUTTON,TEXT_COLOR_FOCUSED, (int)U32(text));
    GuiSetStyle(BUTTON,TEXT_COLOR_PRESSED, (int)U32(text));
    bool p = GuiButton(r, label);
    GuiSetStyle(BUTTON,BASE_COLOR_NORMAL,oN); GuiSetStyle(BUTTON,BASE_COLOR_FOCUSED,oF);
    GuiSetStyle(BUTTON,BASE_COLOR_PRESSED,oP);GuiSetStyle(BUTTON,TEXT_COLOR_NORMAL,oTN);
    GuiSetStyle(BUTTON,TEXT_COLOR_FOCUSED,oTF);GuiSetStyle(BUTTON,TEXT_COLOR_PRESSED,oTP);
    return p;
}

/* ============================================================================
 *  Display da Cientifica (desenho proprio, sem GuiTextBox)
 * ==========================================================================*/
static void draw_calc_display(Rectangle dr){
    DrawRectangleRounded(dr, 0.10f, 8, TH.dispBg);
    DrawRectangleRoundedLinesEx(dr, 0.10f, 8, 1.5f, TH.dispBorder);

    const char* t = display[0] ? display : "0";
    int fs = 36;
    int tw = MeasureText(t, fs);
    float pad = 16;
    /* rola para a esquerda se nao couber */
    float tx = dr.x + dr.width - pad - tw;
    if (tx < dr.x + pad) tx = dr.x + pad;
    float ty = dr.y + 10;
    BeginScissorMode((int)dr.x+2,(int)dr.y+2,(int)dr.width-4,(int)dr.height-4);
    DrawText(t, (int)tx, (int)ty, fs, TH.dispText);
    /* cursor piscando */
    if (fmod(GetTime(),1.0) < 0.5){
        float cx = tx + tw + 2;
        DrawRectangle((int)cx, (int)ty+2, 2, fs, TH.dispText);
    }
    EndScissorMode();

    /* previa do resultado ao vivo */
    if (display[0]){
        ExprContext pc = ctx; pc.angle=(AngleMode)angle_sel;
        const char* expr = display;
        char* eq = strchr(display,'='); char rhs[256];
        if (eq){ snprintf(rhs,sizeof(rhs),"%s",eq+1); expr=rhs; }
        double v = expr_eval(&pc, expr);
        if (pc.ok){
            const char* pv = TextFormat("= %s", fmtnum(v));
            int pw = MeasureText(pv, 20);
            DrawText(pv, (int)(dr.x+dr.width-pad-pw), (int)(dr.y+dr.height-26), 20, TH.dispPreview);
        }
    }
}

/* ============================================================================
 *  ABA: CIENTIFICA
 * ==========================================================================*/
static void draw_sci(Rectangle area){
    float x = area.x, y = area.y;
    float sideW = 250;
    float mainW = area.width - sideW - 16;

    /* display */
    draw_calc_display(R(x, y, mainW, 74));

    /* modo de angulo + memoria */
    float my = y + 84;
    GuiToggleGroup(R(x, my, 60, 26), "RAD;DEG;GRAD", &angle_sel);
    GuiLabel(R(x+210, my, 240, 26), TextFormat("Memoria: %s", fmtnum(memval)));

    /* teclado */
    float gy = my + 38;
    float gridW = mainW;
    float cols = 8.0f, gap = 6;
    float bw = (gridW - (cols-1)*gap) / cols;
    float bh = 44;
    #define CELL(c,row) R(x + (c)*(bw+gap), gy + (row)*(bh+gap), bw, bh)

    /* linha 0: funcoes trig */
    struct { const char* lab; const char* ins; } frow[] = {
        {"sin","sin("},{"cos","cos("},{"tan","tan("},
        {"asin","asin("},{"acos","acos("},{"atan","atan("},
        {"sinh","sinh("},{"cosh","cosh("}
    };
    for(int c=0;c<8;c++) if(CBtn(CELL(c,0),frow[c].lab,TH.bFun,TH.bTxt)) disp_insert(frow[c].ins);

    struct { const char* lab; const char* ins; } frow2[] = {
        {"tanh","tanh("},{"ln","ln("},{"log","log("},{"log2","log2("},
        {"exp","exp("},{"sqrt","sqrt("},{"cbrt","cbrt("},{"abs","abs("}
    };
    for(int c=0;c<8;c++) if(CBtn(CELL(c,1),frow2[c].lab,TH.bFun,TH.bTxt)) disp_insert(frow2[c].ins);

    struct { const char* lab; const char* ins; } frow3[] = {
        {"pi","pi"},{"e","e"},{"phi","phi"},{"x^2","^2"},
        {"x^3","^3"},{"x^y","^"},{"1/x","1/("},{"n!","!"}
    };
    for(int c=0;c<8;c++) if(CBtn(CELL(c,2),frow3[c].lab,TH.bFun,TH.bTxt)) disp_insert(frow3[c].ins);

    /* bloco numerico: colunas 0..2 = digitos, col 3 = operadores; cols 4..7 extras */
    struct { int c,r; const char* lab; const char* ins; } keys[] = {
        {0,3,"7","7"},{1,3,"8","8"},{2,3,"9","9"},
        {0,4,"4","4"},{1,4,"5","5"},{2,4,"6","6"},
        {0,5,"1","1"},{1,5,"2","2"},{2,5,"3","3"},
        {0,6,"0","0"},{1,6,".","."},{2,6,"ANS","ans"},
    };
    for(int i=0;i<12;i++) if(CBtn(CELL(keys[i].c,keys[i].r),keys[i].lab,TH.bNum,TH.bTxt)) disp_insert(keys[i].ins);

    /* operadores coluna 3 */
    if(CBtn(CELL(3,3),"/",TH.bOp,TH.bTxt)) disp_insert("/");
    if(CBtn(CELL(3,4),"*",TH.bOp,TH.bTxt)) disp_insert("*");
    if(CBtn(CELL(3,5),"-",TH.bOp,TH.bTxt)) disp_insert("-");
    if(CBtn(CELL(3,6),"+",TH.bOp,TH.bTxt)) disp_insert("+");

    /* parenteses / virgula / mod / barra */
    if(CBtn(CELL(4,3),"(",TH.bOp,TH.bTxt)) disp_insert("(");
    if(CBtn(CELL(5,3),")",TH.bOp,TH.bTxt)) disp_insert(")");
    if(CBtn(CELL(6,3),",",TH.bOp,TH.bTxt)) disp_insert(",");
    if(CBtn(CELL(7,3),"%",TH.bOp,TH.bTxt)) disp_insert("%");
    if(CBtn(CELL(4,4),"|x|",TH.bOp,TH.bTxt)) disp_insert("|");
    if(CBtn(CELL(5,4),"mod",TH.bFun,TH.bTxt)) disp_insert("mod(");
    if(CBtn(CELL(6,4),"comb",TH.bFun,TH.bTxt)) disp_insert("comb(");
    if(CBtn(CELL(7,4),"perm",TH.bFun,TH.bTxt)) disp_insert("perm(");

    /* C / DEL (linha 5, col 4-5) */
    if(CBtn(CELL(4,5),"C",TH.bDanger,WHITE)) display[0]='\0';
    if(CBtn(CELL(5,5),"DEL",TH.bDanger,WHITE)) disp_backspace();
    /* "=" grande: col 6..7, linhas 5..6 */
    if(CBtn(R(x+6*(bw+gap), gy+5*(bh+gap), 2*bw+gap, 2*bh+gap), "=", TH.bAccent, WHITE)) do_eval();
    /* AVALIAR largo: col 4..5, linha 6 */
    if(CBtn(R(x+4*(bw+gap), gy+6*(bh+gap), 2*bw+gap, bh), "AVALIAR", TH.bAccent, WHITE)) do_eval();

    /* memoria (linha 7, col 0..3) */
    if(CBtn(CELL(0,7),"MC",TH.bOp,TH.bTxt)) memval=0;
    if(CBtn(CELL(1,7),"MR",TH.bOp,TH.bTxt)) disp_insert(TextFormat("%.12g",memval));
    if(CBtn(CELL(2,7),"M+",TH.bOp,TH.bTxt)){ ExprContext c2=ctx;c2.angle=(AngleMode)angle_sel; double v=expr_eval(&c2,display); if(c2.ok) memval+=v; }
    if(CBtn(CELL(3,7),"M-",TH.bOp,TH.bTxt)){ ExprContext c2=ctx;c2.angle=(AngleMode)angle_sel; double v=expr_eval(&c2,display); if(c2.ok) memval-=v; }

    /* painel lateral: historico + variaveis */
    float hx = x + area.width - sideW, hw = sideW;
    GuiGroupBox(R(hx, y, hw, 300), "Historico");
    for (int i=0;i<hist_count && i<12;i++){
        int idx = hist_count-1-i;
        if (GuiLabelButton(R(hx+10, y+12+i*22, hw-20, 20), hist[idx])){
            char tmp[128]; snprintf(tmp,sizeof(tmp),"%s",hist[idx]);
            char* eq=strstr(tmp," = "); if(eq){*eq='\0'; snprintf(display,sizeof(display),"%s",tmp);}
        }
    }
    if (CBtn(R(hx, y+304, hw, 28), "Limpar historico", TH.bDanger, WHITE)) hist_count=0;

    GuiGroupBox(R(hx, y+342, hw, 220), "Variaveis");
    int shown=0;
    for (int i=0;i<ctx.var_count && shown<9;i++){
        GuiLabel(R(hx+10, y+352+shown*22, hw-20, 20),
                 TextFormat("%s = %s", ctx.vars[i].name, fmtnum(ctx.vars[i].value)));
        shown++;
    }
    GuiLabel(R(hx, y+574, hw, 40), "Dica: a = 3*4  define variavel.\nUse 'ans' p/ o ultimo resultado.");

    /* teclado fisico (somente esta aba e quando nenhum textbox tem foco) */
    if (g_editId < 0){
        int ch;
        while ((ch = GetCharPressed()) > 0)                 /* caracteres imprimiveis */
            if (ch>=32 && ch<127){ char s[2]={(char)ch,0}; disp_insert(s); }
        int kc;
        while ((kc = GetKeyPressed()) > 0){                 /* teclas de controle (fila robusta) */
            if (kc==KEY_ENTER || kc==KEY_KP_ENTER) do_eval();
            else if (kc==KEY_BACKSPACE) disp_backspace();
            else if (kc==KEY_DELETE) display[0]='\0';
            else if (kc==KEY_ESCAPE) display[0]='\0';
        }
        if (IsKeyPressedRepeat(KEY_BACKSPACE)) disp_backspace();  /* segurar apaga */
    }
    #undef CELL
}

/* ============================================================================
 *  ABA: GRAFICO
 * ==========================================================================*/
#define NFUN 4
static char  gfun[NFUN][128] = { "sin(x)", "", "", "" };
static bool  gon[NFUN]       = { true, false, false, false };
static double gxmin=-10, gxmax=10, gymin=-6, gymax=6;
static bool  gdrag=false; static Vector2 gprev;
static Color gcol[NFUN] = {
    {90,170,255,255},{255,110,110,255},{110,220,140,255},{240,190,80,255}
};

static void graph_autoY(Rectangle plot){
    double ymin=1e30, ymax=-1e30; int any=0;
    ExprContext c2 = ctx; c2.angle=(AngleMode)angle_sel;
    for (int f=0; f<NFUN; f++){
        if(!gon[f] || gfun[f][0]==0) continue;
        for (int px=0; px<=(int)plot.width; px+=2){
            double xw = gxmin + (px/plot.width)*(gxmax-gxmin);
            expr_set_var(&c2,"x",xw);
            double yw = expr_eval(&c2, gfun[f]);
            if (c2.ok && isfinite(yw)){ if(yw<ymin)ymin=yw; if(yw>ymax)ymax=yw; any=1; }
        }
    }
    if (any && ymax>ymin){ double pad=(ymax-ymin)*0.1+1e-9; gymin=ymin-pad; gymax=ymax+pad; }
}

static void draw_graph(Rectangle area){
    float panelW = 300;
    GuiGroupBox(R(area.x, area.y, panelW, area.height), "Funcoes f(x)");
    for (int f=0; f<NFUN; f++){
        float yy = area.y + 16 + f*42;
        GuiCheckBox(R(area.x+12, yy+6, 18, 18), "", &gon[f]);
        DrawRectangle((int)area.x+36,(int)yy+8,14,14, gcol[f]);
        EditBox(R(area.x+58, yy, panelW-72, 30), gfun[f], sizeof(gfun[f]));
    }
    float cy = area.y + 16 + NFUN*42 + 8;
    GuiLine(R(area.x+8, cy, panelW-16, 12), "Janela"); cy += 20;
    static char sxmin[24],sxmax[24],symin[24],symax[24];
    snprintf(sxmin,24,"%g",gxmin); snprintf(sxmax,24,"%g",gxmax);
    snprintf(symin,24,"%g",gymin); snprintf(symax,24,"%g",gymax);
    GuiLabel(R(area.x+12, cy, 44,26),"Xmin"); EditNum(R(area.x+58,cy,76,26),sxmin,24);
    GuiLabel(R(area.x+150,cy,44,26),"Xmax"); EditNum(R(area.x+198,cy,80,26),sxmax,24); cy+=32;
    GuiLabel(R(area.x+12, cy, 44,26),"Ymin"); EditNum(R(area.x+58,cy,76,26),symin,24);
    GuiLabel(R(area.x+150,cy,44,26),"Ymax"); EditNum(R(area.x+198,cy,80,26),symax,24); cy+=36;
    if (CBtn(R(area.x+12, cy, 130,30),"Aplicar janela",TH.bAccent,WHITE)){
        gxmin=atof(sxmin); gxmax=atof(sxmax); gymin=atof(symin); gymax=atof(symax);
        if(gxmax<=gxmin)gxmax=gxmin+1;
        if(gymax<=gymin)gymax=gymin+1;
    }
    if (GuiButton(R(area.x+150, cy, 138,30),"Resetar")){ gxmin=-10;gxmax=10;gymin=-6;gymax=6; }
    cy+=36;
    Rectangle plot = R(area.x+panelW+12, area.y, area.width-panelW-12, area.height);
    if (GuiButton(R(area.x+12, cy, 276,30),"Ajustar Y (auto)")) graph_autoY(plot);
    cy+=36;
    if (GuiButton(R(area.x+12, cy, 132,30),"Zoom +")){
        double cxw=(gxmin+gxmax)/2, cyw=(gymin+gymax)/2;
        gxmin=cxw+(gxmin-cxw)*0.8; gxmax=cxw+(gxmax-cxw)*0.8;
        gymin=cyw+(gymin-cyw)*0.8; gymax=cyw+(gymax-cyw)*0.8;
    }
    if (GuiButton(R(area.x+150, cy, 138,30),"Zoom -")){
        double cxw=(gxmin+gxmax)/2, cyw=(gymin+gymax)/2;
        gxmin=cxw+(gxmin-cxw)*1.25; gxmax=cxw+(gxmax-cxw)*1.25;
        gymin=cyw+(gymin-cyw)*1.25; gymax=cyw+(gymax-cyw)*1.25;
    }
    cy+=40;
    GuiLabel(R(area.x+12, cy, panelW-20, 30), "Arraste = mover | roda = zoom");

    DrawRectangleRec(plot, TH.plotBg);
    DrawRectangleLinesEx(plot, 1, TH.dispBorder);
    BeginScissorMode((int)plot.x,(int)plot.y,(int)plot.width,(int)plot.height);

    double sx = plot.width /(gxmax-gxmin);
    double sy = plot.height/(gymax-gymin);
    #define X2P(xw) (plot.x + (float)(((xw)-gxmin)*sx))
    #define Y2P(yw) (plot.y + (float)((gymax-(yw))*sy))
    #define P2X(px) (gxmin + ((px)-plot.x)/sx)
    #define P2Y(py) (gymax - ((py)-plot.y)/sy)

    double stepx = pow(10, floor(log10((gxmax-gxmin)/8))); if((gxmax-gxmin)/stepx>16)stepx*=2; if((gxmax-gxmin)/stepx>16)stepx*=2.5;
    double stepy = pow(10, floor(log10((gymax-gymin)/6))); if((gymax-gymin)/stepy>16)stepy*=2; if((gymax-gymin)/stepy>16)stepy*=2.5;
    for (double gx=ceil(gxmin/stepx)*stepx; gx<=gxmax; gx+=stepx){
        float px=X2P(gx); DrawLine((int)px,(int)plot.y,(int)px,(int)(plot.y+plot.height),TH.plotGrid);
        DrawText(TextFormat("%g",gx),(int)px+2,(int)(plot.y+plot.height)-16,10,TH.plotLabel);
    }
    for (double gy2=ceil(gymin/stepy)*stepy; gy2<=gymax; gy2+=stepy){
        float py=Y2P(gy2); DrawLine((int)plot.x,(int)py,(int)(plot.x+plot.width),(int)py,TH.plotGrid);
        DrawText(TextFormat("%g",gy2),(int)plot.x+2,(int)py+1,10,TH.plotLabel);
    }
    if (gymin<0&&gymax>0){ float py=Y2P(0); DrawLine((int)plot.x,(int)py,(int)(plot.x+plot.width),(int)py,TH.plotAxis); }
    if (gxmin<0&&gxmax>0){ float px=X2P(0); DrawLine((int)px,(int)plot.y,(int)px,(int)(plot.y+plot.height),TH.plotAxis); }

    ExprContext c2 = ctx; c2.angle=(AngleMode)angle_sel;
    for (int f=0; f<NFUN; f++){
        if(!gon[f] || gfun[f][0]==0) continue;
        float prevpx=0, prevpy=0; int have=0;
        for (int px=0; px<=(int)plot.width; px++){
            double xw = P2X(plot.x+px);
            expr_set_var(&c2,"x",xw);
            double yw = expr_eval(&c2, gfun[f]);
            if (c2.ok && isfinite(yw)){
                float ppx = plot.x+px, ppy = Y2P(yw);
                if (have && fabs(ppy-prevpy) < plot.height*1.5)
                    DrawLineEx((Vector2){prevpx,prevpy},(Vector2){ppx,ppy},2.0f,gcol[f]);
                prevpx=ppx; prevpy=ppy; have=1;
            } else have=0;
        }
    }

    Vector2 m = GetMousePosition();
    if (CheckCollisionPointRec(m, plot)){
        double xw=P2X(m.x), yw=P2Y(m.y);
        DrawCircleV(m,3,TH.plotAxis);
        DrawText(TextFormat("x=%.4g  y=%.4g", xw, yw),(int)plot.x+8,(int)plot.y+8,16,TH.dispText);
    }
    EndScissorMode();

    if (CheckCollisionPointRec(m, plot)){
        float wheel = GetMouseWheelMove();
        if (wheel!=0){
            double fx=P2X(m.x), fy=P2Y(m.y);
            double k = wheel>0?0.85:1.176;
            gxmin=fx+(gxmin-fx)*k; gxmax=fx+(gxmax-fx)*k;
            gymin=fy+(gymin-fy)*k; gymax=fy+(gymax-fy)*k;
        }
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){ gdrag=true; gprev=m; }
    }
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) gdrag=false;
    if (gdrag){
        Vector2 d={m.x-gprev.x, m.y-gprev.y}; gprev=m;
        double dx=d.x/sx, dy=d.y/sy;
        gxmin-=dx; gxmax-=dx; gymin+=dy; gymax+=dy;
    }
    #undef X2P
    #undef Y2P
    #undef P2X
    #undef P2Y
}

/* ============================================================================
 *  ABA: CALCULO
 * ==========================================================================*/
static char cf[128]="x^2";
static char ca[32]="0", cb[32]="1", cn[32]="1000";
static char calc_out[256]="";
static double eval_at(ExprContext* c, const char* f, double x){ expr_set_var(c,"x",x); return expr_eval(c,f); }

static void draw_calc(Rectangle area){
    float x=area.x, y=area.y;
    GuiLabel(R(x,y,80,28),"f(x) ="); EditBox(R(x+80,y,area.width-92,30), cf, sizeof(cf)); y+=42;
    GuiLabel(R(x,y,30,28),"a");  EditNum(R(x+34,y,90,30),ca,sizeof(ca));
    GuiLabel(R(x+140,y,30,28),"b"); EditNum(R(x+174,y,90,30),cb,sizeof(cb));
    GuiLabel(R(x+280,y,30,28),"n"); EditNum(R(x+314,y,90,30),cn,sizeof(cn)); y+=46;

    ExprContext c2 = ctx; c2.angle=(AngleMode)angle_sel;
    double a=atof(ca), b=atof(cb); int n=atoi(cn); if(n<2)n=2;

    if (CBtn(R(x,y,140,32),"f(a)",TH.bFun,TH.bTxt)){
        double v=eval_at(&c2,cf,a);
        if (c2.ok) snprintf(calc_out,256,"f(%g) = %s", a, fmtnum(v));
        else       snprintf(calc_out,256,"erro: %s", c2.error);
    }
    if (CBtn(R(x+150,y,150,32),"f'(a) derivada",TH.bFun,TH.bTxt)){
        double h=1e-5*(fabs(a)+1);
        double d=(eval_at(&c2,cf,a+h)-eval_at(&c2,cf,a-h))/(2*h);
        snprintf(calc_out,256,"f'(%g) ~= %s", a, fmtnum(d));
    }
    if (CBtn(R(x+310,y,140,32),"f''(a)",TH.bFun,TH.bTxt)){
        double h=1e-4*(fabs(a)+1);
        double d2=(eval_at(&c2,cf,a+h)-2*eval_at(&c2,cf,a)+eval_at(&c2,cf,a-h))/(h*h);
        snprintf(calc_out,256,"f''(%g) ~= %s", a, fmtnum(d2));
    }
    y+=40;
    if (CBtn(R(x,y,210,32),"Integral [a,b] (Simpson)",TH.bAccent,WHITE)){
        if(n%2)n++;
        double h=(b-a)/n, s=eval_at(&c2,cf,a)+eval_at(&c2,cf,b);
        for(int i=1;i<n;i++) s+=(i%2?4:2)*eval_at(&c2,cf,a+i*h);
        snprintf(calc_out,256,"Integral de %g a %g = %s", a,b,fmtnum(s*h/3.0));
    }
    if (CBtn(R(x+220,y,210,32),"Raiz em [a,b] (bissecao)",TH.bAccent,WHITE)){
        double fa=eval_at(&c2,cf,a), fb=eval_at(&c2,cf,b);
        if (fa*fb>0) snprintf(calc_out,256,"f(a) e f(b) tem o mesmo sinal: sem raiz garantida");
        else {
            double lo=a,hi=b,mid=0;
            for(int i=0;i<200;i++){ mid=(lo+hi)/2; double fm=eval_at(&c2,cf,mid);
                if (fa*fm<=0){hi=mid;} else {lo=mid; fa=fm;} }
            snprintf(calc_out,256,"raiz ~= %s   (f=%.2e)", fmtnum(mid), eval_at(&c2,cf,mid));
        }
    }
    y+=40;
    if (CBtn(R(x,y,210,32),"Somatorio i=a..b",TH.bFun,TH.bTxt)){
        double s=0; for(long i=(long)a;i<=(long)b;i++){ expr_set_var(&c2,"x",(double)i); s+=expr_eval(&c2,cf); }
        snprintf(calc_out,256,"Somatorio = %s", fmtnum(s));
    }
    if (CBtn(R(x+220,y,210,32),"Produtorio i=a..b",TH.bFun,TH.bTxt)){
        double s=1; for(long i=(long)a;i<=(long)b;i++){ expr_set_var(&c2,"x",(double)i); s*=expr_eval(&c2,cf); }
        snprintf(calc_out,256,"Produtorio = %s", fmtnum(s));
    }
    y+=50;
    GuiGroupBox(R(x,y,area.width, 70),"Resultado");
    DrawText(calc_out, (int)x+14, (int)y+24, 22, TH.dispText);
    y+=86;
    GuiLabel(R(x,y,area.width,24),"Use 'x' como variavel. Derivada por diferencas finitas; integral por Simpson.");
}

/* ============================================================================
 *  ABA: MATRIZES
 * ==========================================================================*/
#define MAXN 6
static int mrA=2,mcA=2,mrB=2,mcB=2;
static char Acell[MAXN][MAXN][12];
static char Bcell[MAXN][MAXN][12];
static double Rmat[MAXN*2][MAXN*2]; static int Rr=0,Rc=0; static char mres[128]="";
static int matrix_inited=0;

static void matrix_init(void){
    for(int i=0;i<MAXN;i++)for(int j=0;j<MAXN;j++){
        snprintf(Acell[i][j],12, (i==j)?"1":"0");
        snprintf(Bcell[i][j],12, (i==j)?"1":"0");
    }
    matrix_inited=1;
}
static void getmat(char cell[MAXN][MAXN][12], int r,int c, double out[MAXN][MAXN]){
    for(int i=0;i<r;i++)for(int j=0;j<c;j++) out[i][j]=atof(cell[i][j]);
}
static void setR(double m[MAXN][MAXN],int r,int c){ Rr=r;Rc=c; for(int i=0;i<r;i++)for(int j=0;j<c;j++)Rmat[i][j]=m[i][j]; }
static double determinant(double m[MAXN][MAXN], int n){
    double a[MAXN][MAXN]; memcpy(a,m,sizeof(double)*MAXN*MAXN); double det=1;
    for(int i=0;i<n;i++){
        int piv=i; for(int k=i+1;k<n;k++) if(fabs(a[k][i])>fabs(a[piv][i]))piv=k;
        if(fabs(a[piv][i])<1e-15) return 0;
        if(piv!=i){ for(int j=0;j<n;j++){double t=a[i][j];a[i][j]=a[piv][j];a[piv][j]=t;} det=-det; }
        det*=a[i][i];
        for(int k=i+1;k<n;k++){ double f=a[k][i]/a[i][i]; for(int j=i;j<n;j++) a[k][j]-=f*a[i][j]; }
    }
    return det;
}
static int inverse(double m[MAXN][MAXN], int n, double inv[MAXN][MAXN]){
    double a[MAXN][2*MAXN];
    for(int i=0;i<n;i++){ for(int j=0;j<n;j++)a[i][j]=m[i][j]; for(int j=0;j<n;j++)a[i][n+j]=(i==j); }
    for(int i=0;i<n;i++){
        int piv=i; for(int k=i+1;k<n;k++) if(fabs(a[k][i])>fabs(a[piv][i]))piv=k;
        if(fabs(a[piv][i])<1e-12) return 0;
        for(int j=0;j<2*n;j++){double t=a[i][j];a[i][j]=a[piv][j];a[piv][j]=t;}
        double d=a[i][i]; for(int j=0;j<2*n;j++)a[i][j]/=d;
        for(int k=0;k<n;k++) if(k!=i){ double f=a[k][i]; for(int j=0;j<2*n;j++) a[k][j]-=f*a[i][j]; }
    }
    for(int i=0;i<n;i++)for(int j=0;j<n;j++) inv[i][j]=a[i][n+j];
    return 1;
}
static void draw_matrix_grid(float x,float y,char cell[MAXN][MAXN][12],int r,int c){
    float cw=56,ch=30,g=4;
    for(int i=0;i<r;i++)for(int j=0;j<c;j++)
        EditBox(R(x+j*(cw+g), y+i*(ch+g), cw, ch), cell[i][j], 12);
}
static void draw_matrix(Rectangle area){
    if(!matrix_inited) matrix_init();
    float x=area.x,y=area.y;
    GuiLabel(R(x,y,120,24),"Matriz A");
    GuiSpinner(R(x+110,y,90,24),"lin",&mrA,1,MAXN, false);
    GuiSpinner(R(x+210,y,90,24),"col",&mcA,1,MAXN, false);
    draw_matrix_grid(x, y+30, Acell, mrA, mcA);

    float bx = x + 6*(56+4) + 40;
    GuiLabel(R(bx,y,120,24),"Matriz B");
    GuiSpinner(R(bx+110,y,90,24),"lin",&mrB,1,MAXN, false);
    GuiSpinner(R(bx+210,y,90,24),"col",&mcB,1,MAXN, false);
    draw_matrix_grid(bx, y+30, Bcell, mrB, mcB);

    float oy = y + 30 + MAXN*34 + 8;
    double A[MAXN][MAXN],B[MAXN][MAXN],Rm[MAXN][MAXN];
    getmat(Acell,mrA,mcA,A); getmat(Bcell,mrB,mcB,B);

    if (CBtn(R(x,oy,90,30),"A + B",TH.bOp,TH.bTxt)){
        if(mrA==mrB&&mcA==mcB){ for(int i=0;i<mrA;i++)for(int j=0;j<mcA;j++)Rm[i][j]=A[i][j]+B[i][j]; setR(Rm,mrA,mcA); snprintf(mres,128,"A+B ok"); }
        else snprintf(mres,128,"dimensoes incompativeis p/ soma");
    }
    if (CBtn(R(x+100,oy,90,30),"A - B",TH.bOp,TH.bTxt)){
        if(mrA==mrB&&mcA==mcB){ for(int i=0;i<mrA;i++)for(int j=0;j<mcA;j++)Rm[i][j]=A[i][j]-B[i][j]; setR(Rm,mrA,mcA); snprintf(mres,128,"A-B ok"); }
        else snprintf(mres,128,"dimensoes incompativeis p/ subtracao");
    }
    if (CBtn(R(x+200,oy,90,30),"A x B",TH.bOp,TH.bTxt)){
        if(mcA==mrB){ for(int i=0;i<mrA;i++)for(int j=0;j<mcB;j++){double s=0;for(int k=0;k<mcA;k++)s+=A[i][k]*B[k][j];Rm[i][j]=s;} setR(Rm,mrA,mcB); snprintf(mres,128,"A*B ok"); }
        else snprintf(mres,128,"colunas de A != linhas de B");
    }
    if (CBtn(R(x+300,oy,110,30),"Transposta A",TH.bFun,TH.bTxt)){
        for(int i=0;i<mcA;i++)for(int j=0;j<mrA;j++)Rm[i][j]=A[j][i];
        setR(Rm,mcA,mrA); snprintf(mres,128,"A^T ok");
    }
    if (CBtn(R(x+420,oy,90,30),"det(A)",TH.bAccent,WHITE)){
        if(mrA==mcA){ snprintf(mres,128,"det(A) = %s", fmtnum(determinant(A,mrA))); Rr=0; }
        else snprintf(mres,128,"A nao e quadrada");
    }
    if (CBtn(R(x+520,oy,110,30),"Inversa A",TH.bAccent,WHITE)){
        if(mrA==mcA){ if(inverse(A,mrA,Rm)){setR(Rm,mrA,mrA);snprintf(mres,128,"A^-1 ok");} else snprintf(mres,128,"A nao inversivel (det=0)"); }
        else snprintf(mres,128,"A nao e quadrada");
    }
    oy+=38; GuiLabel(R(x,oy,area.width,24), mres); oy+=28;
    if (Rr>0){
        GuiGroupBox(R(x,oy,area.width, MAXN*34+20),"Resultado");
        float cw=70,ch=28,g=4;
        for(int i=0;i<Rr;i++)for(int j=0;j<Rc;j++)
            GuiLabel(R(x+12+j*(cw+g), oy+14+i*(ch+g), cw, ch), fmtnum(Rmat[i][j]));
    }
}

/* ============================================================================
 *  ABA: ESTATISTICA
 * ==========================================================================*/
static char statbuf[2048]="1 2 3 4 5 6 7 8 9 10";
static char statx[1024]="1 2 3 4 5";
static char staty[1024]="2 4 5 4 5";
static char stat_out[512]="";
static int parse_nums(const char* s, double* out, int max){
    int n=0; const char* p=s;
    while(*p && n<max){
        while(*p && !(isdigit((unsigned char)*p)||*p=='-'||*p=='+'||*p=='.')) p++;
        if(!*p) break;
        char* e; double v=strtod(p,&e); if(e==p){p++;continue;} out[n++]=v; p=e;
    }
    return n;
}
static int cmpd(const void* a,const void* b){ double d=*(const double*)a-*(const double*)b; return (d>0)-(d<0); }
static void draw_stats(Rectangle area){
    float x=area.x,y=area.y;
    GuiLabel(R(x,y,area.width,22),"Dados (separados por espaco, virgula ou linha):"); y+=26;
    EditBox(R(x,y,area.width,70), statbuf, sizeof(statbuf)); y+=80;
    if (CBtn(R(x,y,210,32),"Calcular estatisticas",TH.bAccent,WHITE)){
        double d[1024]; int n=parse_nums(statbuf,d,1024);
        if(n==0) snprintf(stat_out,512,"nenhum dado");
        else{
            double sum=0,mn=d[0],mx=d[0]; for(int i=0;i<n;i++){sum+=d[i];if(d[i]<mn)mn=d[i];if(d[i]>mx)mx=d[i];}
            double mean=sum/n, ss=0; for(int i=0;i<n;i++)ss+=(d[i]-mean)*(d[i]-mean);
            double varp=ss/n, vars=(n>1)?ss/(n-1):0;
            qsort(d,n,sizeof(double),cmpd);
            double med=(n%2)? d[n/2] : (d[n/2-1]+d[n/2])/2.0;
            snprintf(stat_out,512,
              "n=%d   soma=%s   media=%s\nmin=%s   max=%s   amplitude=%s\nmediana=%s\nvar(pop)=%s  desv(pop)=%s\nvar(amostra)=%s  desv(amostra)=%s",
              n,fmtnum(sum),fmtnum(mean),fmtnum(mn),fmtnum(mx),fmtnum(mx-mn),
              fmtnum(med),fmtnum(varp),fmtnum(sqrt(varp)),fmtnum(vars),fmtnum(sqrt(vars)));
        }
    }
    y+=42; GuiGroupBox(R(x,y,area.width,140),"Resultado");
    DrawText(stat_out, (int)x+14, (int)y+12, 18, TH.dispText); y+=152;
    GuiLine(R(x,y,area.width,12),"Regressao linear (Y = a + b X)"); y+=18;
    GuiLabel(R(x,y,40,26),"X"); EditBox(R(x+44,y,area.width-44,30),statx,sizeof(statx)); y+=36;
    GuiLabel(R(x,y,40,26),"Y"); EditBox(R(x+44,y,area.width-44,30),staty,sizeof(staty)); y+=36;
    static char reg_out[256]="";
    if (CBtn(R(x,y,210,32),"Calcular regressao",TH.bAccent,WHITE)){
        double X[512],Y[512]; int nx=parse_nums(statx,X,512), ny=parse_nums(staty,Y,512);
        int n=nx<ny?nx:ny;
        if(n<2) snprintf(reg_out,256,"precisa de ao menos 2 pares");
        else{
            double sx=0,sy=0,sxx=0,sxy=0,syy=0;
            for(int i=0;i<n;i++){sx+=X[i];sy+=Y[i];sxx+=X[i]*X[i];sxy+=X[i]*Y[i];syy+=Y[i]*Y[i];}
            double b=(n*sxy-sx*sy)/(n*sxx-sx*sx), a=(sy-b*sx)/n;
            double r=(n*sxy-sx*sy)/sqrt((n*sxx-sx*sx)*(n*syy-sy*sy));
            snprintf(reg_out,256,"a(intercepto)=%s   b(inclinacao)=%s\nr=%s   r^2=%s",
                     fmtnum(a),fmtnum(b),fmtnum(r),fmtnum(r*r));
        }
    }
    y+=40; DrawText(reg_out, (int)x, (int)y, 18, TH.dispText);
}

/* ============================================================================
 *  ABA: BASES
 * ==========================================================================*/
static char baseInp[40]="255";
static char bx_[40]="12", by_[40]="10";
static char base_out[400]="";
static char bit_out[200]="";
static void to_bin(long long v, char* out, int sz){
    char tmp[80]; int i=0; unsigned long long u=(unsigned long long)v;
    if(u==0){ snprintf(out,sz,"0"); return; }
    while(u){ tmp[i++]= (u&1)?'1':'0'; u>>=1; }
    int o=0; while(i>0 && o<sz-1) out[o++]=tmp[--i]; out[o]=0;
}
static void draw_bases(Rectangle area){
    float x=area.x,y=area.y;
    GuiLabel(R(x,y,area.width,24),"Valor decimal inteiro:"); y+=28;
    EditBox(R(x,y,200,30), baseInp, sizeof(baseInp));
    if (CBtn(R(x+212,y,170,30),"Converter bases",TH.bAccent,WHITE)){
        long long v=(long long)strtoll(baseInp,NULL,10); char bin[80]; to_bin(v,bin,80);
        snprintf(base_out,400,"DEC: %lld\nHEX: %llX\nOCT: %llo\nBIN: %s",
                 v,(unsigned long long)v,(unsigned long long)v,bin);
    }
    y+=40; GuiGroupBox(R(x,y,380,120),"Resultado");
    DrawText(base_out,(int)x+14,(int)y+12,18,TH.dispText); y+=132;
    GuiLine(R(x,y,area.width,12),"Operacoes bit a bit (inteiros)"); y+=18;
    GuiLabel(R(x,y,30,28),"X"); EditBox(R(x+34,y,120,30),bx_,sizeof(bx_));
    GuiLabel(R(x+170,y,30,28),"Y"); EditBox(R(x+204,y,120,30),by_,sizeof(by_)); y+=40;
    long long X=strtoll(bx_,NULL,10), Y=strtoll(by_,NULL,10), res=0;
    const char* ops[6]={"X AND Y","X OR Y","X XOR Y","NOT X","X << Y","X >> Y"};
    for(int i=0;i<6;i++){
        if (CBtn(R(x+i*104, y, 98, 32), ops[i], TH.bOp, TH.bTxt)){
            switch(i){case 0:res=X&Y;break;case 1:res=X|Y;break;case 2:res=X^Y;break;
                      case 3:res=~X;break;case 4:res=X<<Y;break;case 5:res=X>>Y;break;}
            char bin[80]; to_bin(res,bin,80);
            snprintf(bit_out,200,"%s = %lld   (HEX %llX, BIN %s)", ops[i], res,(unsigned long long)res,bin);
        }
    }
    y+=42; DrawText(bit_out,(int)x,(int)y,18,TH.dispText);
}

/* ============================================================================
 *  ABA: COMPLEXOS
 * ==========================================================================*/
static char are_[24]="3", aim_[24]="4", bre_[24]="1", bim_[24]="2";
static char cx_out[300]="";
static void cset(double re,double im){
    double mag=hypot(re,im), ph=atan2(im,re);
    snprintf(cx_out,300,"Resultado: %.6g %s %.6g i\nMagnitude = %.6g\nFase = %.6g rad (%.6g graus)\nForma polar: %.6g angulo %.6g graus",
        re,(im<0?"-":"+"),fabs(im),mag,ph,ph*180.0/M_PI,mag,ph*180.0/M_PI);
}
static void draw_complex(Rectangle area){
    float x=area.x,y=area.y;
    GuiLabel(R(x,y,160,28),"A = re + im i");
    GuiLabel(R(x,y+34,30,28),"re"); EditBox(R(x+34,y+34,90,30),are_,sizeof(are_));
    GuiLabel(R(x+140,y+34,30,28),"im"); EditBox(R(x+174,y+34,90,30),aim_,sizeof(aim_));
    GuiLabel(R(x+320,y,160,28),"B = re + im i");
    GuiLabel(R(x+320,y+34,30,28),"re"); EditBox(R(x+354,y+34,90,30),bre_,sizeof(bre_));
    GuiLabel(R(x+460,y+34,30,28),"im"); EditBox(R(x+494,y+34,90,30),bim_,sizeof(bim_));
    y+=84;
    double ar=atof(are_),ai=atof(aim_),br=atof(bre_),bi=atof(bim_);
    if (CBtn(R(x,y,90,32),"A+B",TH.bOp,TH.bTxt)) cset(ar+br, ai+bi);
    if (CBtn(R(x+100,y,90,32),"A-B",TH.bOp,TH.bTxt)) cset(ar-br, ai-bi);
    if (CBtn(R(x+200,y,90,32),"A*B",TH.bOp,TH.bTxt)) cset(ar*br-ai*bi, ar*bi+ai*br);
    if (CBtn(R(x+300,y,90,32),"A/B",TH.bOp,TH.bTxt)){ double d=br*br+bi*bi; if(d!=0) cset((ar*br+ai*bi)/d,(ai*br-ar*bi)/d); else snprintf(cx_out,300,"divisao por zero"); }
    y+=40;
    if (CBtn(R(x,y,90,32),"|A|",TH.bFun,TH.bTxt)) snprintf(cx_out,300,"|A| = %.8g", hypot(ar,ai));
    if (CBtn(R(x+100,y,90,32),"arg(A)",TH.bFun,TH.bTxt)) snprintf(cx_out,300,"arg(A) = %.8g rad (%.6g graus)", atan2(ai,ar), atan2(ai,ar)*180/M_PI);
    if (CBtn(R(x+200,y,90,32),"conj(A)",TH.bFun,TH.bTxt)) cset(ar,-ai);
    if (CBtn(R(x+300,y,90,32),"A^2",TH.bFun,TH.bTxt)) cset(ar*ar-ai*ai, 2*ar*ai);
    y+=48; GuiGroupBox(R(x,y,area.width,120),"Resultado");
    DrawText(cx_out,(int)x+14,(int)y+12,18,TH.dispText);
}

/* ============================================================================
 *  ABA: CONVERSAO
 * ==========================================================================*/
typedef struct { const char* name; double factor; } Unit;
/* kind: 0 = fator fixo | 1 = temperatura | 2 = moeda (cotacao online) */
typedef struct { const char* cat; const char* list; const Unit* units; int n; int kind; } Category;
static const Unit U_len[] = {{"m",1},{"km",1000},{"cm",0.01},{"mm",0.001},{"mi",1609.344},{"yd",0.9144},{"ft",0.3048},{"in",0.0254},{"milha naut",1852}};
static const Unit U_mass[]= {{"kg",1},{"g",0.001},{"mg",1e-6},{"t",1000},{"lb",0.45359237},{"oz",0.028349523}};
static const Unit U_area[]= {{"m2",1},{"km2",1e6},{"cm2",1e-4},{"ha",10000},{"acre",4046.8564},{"ft2",0.09290304}};
static const Unit U_vol[] = {{"L",1},{"mL",0.001},{"m3",1000},{"gal(US)",3.785411784},{"gal(UK)",4.54609},{"ft3",28.316846}};
static const Unit U_time[]= {{"s",1},{"min",60},{"h",3600},{"dia",86400},{"semana",604800},{"ano",31557600}};
static const Unit U_spd[] = {{"m/s",1},{"km/h",0.277778},{"mph",0.44704},{"no",0.514444}};
static const Unit U_data[]= {{"byte",1},{"KB",1024},{"MB",1048576},{"GB",1073741824.0},{"TB",1099511627776.0},{"bit",0.125}};
static const Unit U_ang[] = {{"rad",1},{"grau",M_PI/180.0},{"grad",M_PI/200.0}};
static const Unit U_pres[]= {{"Pa",1},{"kPa",1000},{"bar",100000},{"atm",101325},{"mmHg",133.322},{"psi",6894.757}};
static const Unit U_en[]  = {{"J",1},{"kJ",1000},{"cal",4.184},{"kcal",4184},{"Wh",3600},{"kWh",3.6e6},{"eV",1.602176634e-19}};

/* ---- Moeda: codigos (mesma ordem da lista do dropdown) e cotacoes em CODE por 1 USD ---- */
#define CURLIST "USD;BRL;EUR;GBP;JPY;CNY;CAD;AUD;CHF;ARS;MXN;INR"
static const char* CUR[] = {"USD","BRL","EUR","GBP","JPY","CNY","CAD","AUD","CHF","ARS","MXN","INR"};
#define NCUR ((int)(sizeof(CUR)/sizeof(CUR[0])))
static double curRate[NCUR];          /* quantas unidades da moeda por 1 USD */
static int    curLoaded = 0;
static int    curFetching = 0;        /* 1 = busca em andamento (thread) */
static char   curStatus[160] = "Clique em 'Atualizar cotacao' para buscar online.";

static const Category cats[] = {
    {"Comprimento","m;km;cm;mm;mi;yd;ft;in;milha naut", U_len, 9, 0},
    {"Massa","kg;g;mg;t;lb;oz", U_mass,6, 0},
    {"Area","m2;km2;cm2;ha;acre;ft2", U_area,6, 0},
    {"Volume","L;mL;m3;gal(US);gal(UK);ft3", U_vol,6, 0},
    {"Tempo","s;min;h;dia;semana;ano", U_time,6, 0},
    {"Velocidade","m/s;km/h;mph;no", U_spd,4, 0},
    {"Dados","byte;KB;MB;GB;TB;bit", U_data,6, 0},
    {"Angulo","rad;grau;grad", U_ang,3, 0},
    {"Pressao","Pa;kPa;bar;atm;mmHg;psi", U_pres,6, 0},
    {"Energia","J;kJ;cal;kcal;Wh;kWh;eV", U_en,7, 0},
    {"Temperatura","C;F;K", NULL,3, 1},
    {"Moeda (online)", CURLIST, NULL, NCUR, 2},
};
#define NCATS ((int)(sizeof(cats)/sizeof(cats[0])))
static int convCat=0, convFrom=0, convTo=1;
static char convInp[32]="1", conv_out[64]="";
static bool ddCatOpen=false, ddFromOpen=false, ddToOpen=false;
static double temp_to_base(int u,double v){ if(u==0)return v; if(u==1)return (v-32)*5.0/9.0; return v-273.15; }
static double temp_from_base(int u,double c){ if(u==0)return c; if(u==1)return c*9.0/5.0+32; return c+273.15; }

/* extrai "CODE":numero de um JSON simples */
static int json_rate(const char* json, const char* code, double* out){
    char key[16]; snprintf(key,16,"\"%s\":",code);
    const char* p = strstr(json, key);
    if(!p) return 0;
    *out = atof(p + strlen(key));
    return 1;
}
static void do_convert(void);   /* definida adiante */

/* inicia a busca em segundo plano (nao trava a UI) */
static void start_fetch(void){
    if (curFetching) return;
    curFetching = 1;
    snprintf(curStatus,160,"Buscando cotacao online...");
    http_get_async("https://open.er-api.com/v6/latest/USD");
}
/* aplica o JSON recebido as cotacoes */
static void apply_rates(const char* buf){
    if (!strstr(buf,"\"rates\"") && !strstr(buf,"\"success\"")){ curLoaded=0; snprintf(curStatus,160,"Resposta inesperada da API."); return; }
    int ok=1;
    for (int i=0;i<NCUR;i++){ double r; if(json_rate(buf,CUR[i],&r) && r>0) curRate[i]=r; else ok=0; }
    if (ok){
        curLoaded=1;
        time_t t=time(NULL); struct tm* lt=localtime(&t);
        char ts[64]; strftime(ts,64,"%d/%m/%Y %H:%M:%S", lt);
        snprintf(curStatus,160,"Cotacao ao vivo - obtida em %s (base USD)", ts);
    } else { curLoaded=0; snprintf(curStatus,160,"Erro ao interpretar a cotacao."); }
}
/* chamada a cada frame no main: verifica se a busca terminou */
static void poll_fetch(void){
    if (!curFetching) return;
    static char rbuf[1<<15];
    int r = http_get_poll(rbuf, sizeof(rbuf));
    if (r==1){ curFetching=0; apply_rates(rbuf); do_convert(); }
    else if (r==-1){ curFetching=0; curLoaded=0; snprintf(curStatus,160,"Falha de conexao - verifique a internet."); }
}
static void do_convert(void){
    double v=atof(convInp); const Category* C=&cats[convCat];
    if (C->kind==2){              /* moeda */
        if(convFrom>=NCUR)convFrom=0;
        if(convTo>=NCUR)convTo=0;
        if(!curLoaded || curRate[convFrom]<=0){ snprintf(conv_out,64,"---"); return; }
        double usd = v / curRate[convFrom];
        snprintf(conv_out,64,"%.4f", usd * curRate[convTo]);
    } else if (C->kind==1){       /* temperatura */
        double base=temp_to_base(convFrom,v); snprintf(conv_out,64,"%.8g", temp_from_base(convTo,base));
    } else {                      /* fator fixo */
        if(convFrom>=C->n)convFrom=0;
        if(convTo>=C->n)convTo=0;
        double base=v*C->units[convFrom].factor; snprintf(conv_out,64,"%.10g", base/C->units[convTo].factor);
    }
}
static const char* unit_name(int cat, int idx){
    const Category* C=&cats[cat];
    if (C->kind==2) return CUR[(idx<NCUR)?idx:0];
    if (C->kind==1) return idx==0?"C":idx==1?"F":"K";
    return C->units[(idx<C->n)?idx:0].name;
}
static void draw_convert(Rectangle area){
    float x=area.x,y=area.y;
    const Category* C=&cats[convCat];
    int isCur = (C->kind==2);

    GuiLabel(R(x,y,120,28),"Categoria:");
    GuiLabel(R(x,y+44,120,28),"De:");
    GuiLabel(R(x+260,y+44,40,28),"Para:");
    GuiLabel(R(x,y+96,120,28),"Valor:");
    if (EditBox(R(x+90,y+96,160,32), convInp, sizeof(convInp))) do_convert();

    /* Para moeda: o botao Converter dispara a busca online no momento do calculo */
    if (CBtn(R(x+260,y+96,160,32), isCur?"Converter (online)":"Converter", TH.bAccent, WHITE)){
        if (isCur) start_fetch();
        do_convert();
    }

    DrawText(TextFormat("= %s %s", conv_out, unit_name(convCat,convTo)), (int)x, (int)y+150, 28, TH.header);

    if (isCur){
        if (CBtn(R(x+430,y+96,170,32), "Atualizar cotacao", TH.bOp, TH.bTxt)) start_fetch();
        DrawText(curStatus, (int)x, (int)y+196, 16, TH.sub);
        DrawText("Fonte: open.er-api.com (gratis, sem chave). Requer internet.",
                 (int)x, (int)y+218, 14, TH.sub);
        if (curLoaded){
            /* tabela rapida: 1 USD = ... nas principais */
            DrawText(TextFormat("1 USD = %.4f BRL   |   1 EUR = %.4f BRL   |   1 BRL = %.4f USD",
                     curRate[1], curRate[1]/curRate[2], 1.0/curRate[1]),
                     (int)x, (int)y+248, 16, TH.dispText);
        }
    }

    /* dropdowns por ultimo (z-order) */
    if (GuiDropdownBox(R(x+260,y+44,200,32), C->list, &convTo, ddToOpen)){ ddToOpen=!ddToOpen; do_convert(); }
    if (GuiDropdownBox(R(x+90,y+44,150,32), C->list, &convFrom, ddFromOpen)){ ddFromOpen=!ddFromOpen; do_convert(); }
    static char catlist[256]; if(catlist[0]==0){ int o=0; for(int i=0;i<NCATS;i++) o+=snprintf(catlist+o,256-o,"%s%s", i?";":"", cats[i].cat); }
    int prevCat=convCat;
    if (GuiDropdownBox(R(x+110,y,200,32), catlist, &convCat, ddCatOpen)){ ddCatOpen=!ddCatOpen; }
    if (convCat!=prevCat){
        convFrom=0; convTo=1;
        if (cats[convCat].kind==2 && !curLoaded) start_fetch();  /* busca ao entrar em Moeda */
        do_convert();
    }
}

/* ============================================================================
 *  MAIN
 * ==========================================================================*/
/* informacoes do monitor onde a janela esta (preenchidas no main) */
static int g_monIndex = 0, g_monW = 0, g_monH = 0, g_refresh = 60;

int main(void){
    /* FLAG_VSYNC_HINT sincroniza a renderizacao com a taxa do monitor (sem tearing) */
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(1200, 800, "Calculadora Cientifica - C / raylib");

    /* --- adapta janela e FPS ao monitor onde o programa esta situado --- */
    g_monIndex = GetCurrentMonitor();
    g_monW     = GetMonitorWidth(g_monIndex);
    g_monH     = GetMonitorHeight(g_monIndex);
    g_refresh  = GetMonitorRefreshRate(g_monIndex);
    if (g_refresh <= 0) g_refresh = 60;          /* fallback se o driver nao informar */
    if (g_monW   <= 0)  g_monW = 1920;
    if (g_monH   <= 0)  g_monH = 1080;

    /* tamanho proporcional ao monitor (70% larg x 80% alt), com limites sensatos */
    int winW = (int)(g_monW * 0.70f);
    int winH = (int)(g_monH * 0.80f);
    if (winW < 960)   winW = 960;
    if (winH < 700)   winH = 700;
    if (winW > g_monW) winW = g_monW;
    if (winH > g_monH) winH = g_monH;
    SetWindowSize(winW, winH);
    SetWindowMinSize(960, 700);

    /* centraliza a janela no monitor atual (considera a posicao dele no desktop) */
    Vector2 mp = GetMonitorPosition(g_monIndex);
    SetWindowPosition((int)mp.x + (g_monW - winW)/2, (int)mp.y + (g_monH - winH)/2);

    /* FPS alvo = taxa de atualizacao do monitor (o vsync acima ja sincroniza nesse ritmo) */
    SetTargetFPS(g_refresh);

    apply_theme(darkMode);

    expr_init(&ctx);
    expr_set_var(&ctx,"ans",0);
    do_convert();

    const char* tabs[] = {"Cientifica","Grafico","Calculo","Matrizes","Estatistica","Bases","Complexos","Conversao"};
    int activeTab = 0, prevTab = 0;

    while (!WindowShouldClose()){
        int sw=GetScreenWidth(), sh=GetScreenHeight();
        g_idCounter = 0;
        if (activeTab != prevTab){ g_editId=-1; prevTab=activeTab; }
        poll_fetch();   /* verifica se a cotacao em segundo plano chegou */

        /* se a janela foi movida para outro monitor, re-sincroniza o FPS com a nova taxa */
        int curMon = GetCurrentMonitor();
        if (curMon != g_monIndex){
            g_monIndex = curMon;
            g_monW = GetMonitorWidth(curMon); g_monH = GetMonitorHeight(curMon);
            g_refresh = GetMonitorRefreshRate(curMon); if (g_refresh<=0) g_refresh=60;
            SetTargetFPS(g_refresh);
        }

        /* largura de conteudo centralizada (limita o estiramento em telas largas) */
        float CW = (float)sw - 32; if (CW > 1180) CW = 1180;
        float ox = (sw - CW)/2.0f;

        BeginDrawing();
        ClearBackground(TH.winBg);

        DrawText("CALCULADORA CIENTIFICA", (int)ox, 14, 24, TH.header);
        DrawText("C + raylib / raygui", (int)ox+340, 22, 14, TH.sub);
        /* leitura ao vivo: FPS atual | taxa do monitor | resolucao */
        DrawText(TextFormat("%d FPS | %d Hz (vsync) | %dx%d", GetFPS(), g_refresh, g_monW, g_monH),
                 (int)ox+500, 22, 14, TH.sub);

        /* botao de tema */
        if (GuiButton(R(ox+CW-150, 12, 150, 28), darkMode?"Tema: ESCURO":"Tema: CLARO")){
            darkMode = !darkMode; apply_theme(darkMode);
        }

        GuiTabBar(R(ox, 50, CW, 32), tabs, 8, &activeTab);

        Rectangle area = R(ox, 96, CW, sh-112);
        switch (activeTab){
            case 0: draw_sci(area); break;
            case 1: draw_graph(area); break;
            case 2: draw_calc(area); break;
            case 3: draw_matrix(area); break;
            case 4: draw_stats(area); break;
            case 5: draw_bases(area); break;
            case 6: draw_complex(area); break;
            case 7: draw_convert(area); break;
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
