/* net.h - cliente HTTP minimo (isolado da raylib para evitar conflito com windows.h) */
#ifndef NET_H
#define NET_H

/* GET sincrono: grava a resposta (texto) em out (terminada em \0).
 * Retorna bytes lidos, ou 0 em falha. Usa WinINet; linkar com -lwininet. */
int http_get(const char* url, char* out, int outsz);

/* --- versao assincrona (nao trava a UI) --- */
/* Inicia um GET em segundo plano (ignora se ja houver um em andamento). */
void http_get_async(const char* url);
/* Consulta o resultado: 0 = ainda buscando, 1 = concluido (copiou em out),
 * -1 = falhou. */
int  http_get_poll(char* out, int outsz);

#endif
