/* net.c - http_get (WinINet) sincrono e assincrono.
 * Nao inclui raylib.h, evitando conflitos de simbolos entre windows.h e raylib. */
#include <windows.h>
#include <wininet.h>
#include <string.h>
#include "net.h"

int http_get(const char* url, char* out, int outsz){
    if (!out || outsz < 1) return 0;
    out[0] = '\0';

    HINTERNET hi = InternetOpenA("CalculadoraC/1.0",
                                 INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hi) return 0;

    DWORD to = 6000;   /* timeouts para nao pendurar a busca */
    InternetSetOptionA(hi, INTERNET_OPTION_CONNECT_TIMEOUT, &to, sizeof(to));
    InternetSetOptionA(hi, INTERNET_OPTION_RECEIVE_TIMEOUT, &to, sizeof(to));
    InternetSetOptionA(hi, INTERNET_OPTION_SEND_TIMEOUT,    &to, sizeof(to));

    DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE |
                  INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_UI;
    HINTERNET hu = InternetOpenUrlA(hi, url, NULL, 0, flags, 0);
    if (!hu){ InternetCloseHandle(hi); return 0; }

    DWORD total = 0, read = 0;
    while (total < (DWORD)(outsz - 1)){
        if (!InternetReadFile(hu, out + total, (DWORD)(outsz - 1 - total), &read)) break;
        if (read == 0) break;
        total += read;
    }
    out[total] = '\0';

    InternetCloseHandle(hu);
    InternetCloseHandle(hi);
    return (int)total;
}

/* ---------------- versao assincrona ---------------- */
/* estado: 0 ocioso, 1 buscando, 2 pronto, 3 falhou */
static volatile LONG g_state = 0;
static char  g_url[1024];
static char  g_buf[1 << 15];
static volatile LONG g_len = 0;

static DWORD WINAPI worker(LPVOID param){
    (void)param;
    int n = http_get(g_url, g_buf, (int)sizeof(g_buf));
    g_len = n;
    InterlockedExchange(&g_state, (n > 0) ? 2 : 3);
    return 0;
}

void http_get_async(const char* url){
    if (g_state == 1) return;                 /* ja em andamento */
    strncpy(g_url, url, sizeof(g_url) - 1);
    g_url[sizeof(g_url) - 1] = '\0';
    InterlockedExchange(&g_state, 1);
    HANDLE h = CreateThread(NULL, 0, worker, NULL, 0, NULL);
    if (h) CloseHandle(h);
    else InterlockedExchange(&g_state, 3);
}

int http_get_poll(char* out, int outsz){
    LONG s = g_state;
    if (s == 1 || s == 0) return 0;           /* buscando ou ocioso */
    if (s == 2){
        int n = g_len; if (n > outsz - 1) n = outsz - 1;
        memcpy(out, g_buf, n); out[n] = '\0';
        InterlockedExchange(&g_state, 0);
        return 1;
    }
    InterlockedExchange(&g_state, 0);         /* s == 3 */
    return -1;
}
