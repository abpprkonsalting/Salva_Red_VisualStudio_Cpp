
#include <windows.h>
#include <tchar.h> 
#include <stdio.h>
#include <strsafe.h>
#include <Shlwapi.h>
#include <crtdbg.h>
#include <errno.h>

#include "file_error.h"

#define CONF(x)	conf_string(parameters,(x))
#define CONFd(x) conf_int(parameters,(x))

struct parameters_t {

	wchar_t*		Nombre;
	wchar_t*		Valor;
};

int check_text_file_format(char* content); // this function returns 0 if the text file format is ANSI single byte, 1 if ANSI multibyte, 2 if Unicode

BOOL read_conf_file(wchar_t* Origen, struct parameters_t* parametros, class m_error** p_error);

wchar_t* conf_string(struct parameters_t* parametros, wchar_t* Nombre);

int conf_int(struct parameters_t* parametros, wchar_t* Nombre);