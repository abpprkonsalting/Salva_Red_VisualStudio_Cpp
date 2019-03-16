#define STRICT

#include <crtdbg.h>
#include <errno.h>

#include "file_error.h"
#include "Common.h"
#include "conf_file.h"

using namespace std;

#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1

struct parameters_t parameters[] = {

	{ L"Carpeta_Salva\0", NULL},
	{ L"Carpeta_Origen\0", NULL},
	{ L"Tiempo_entre_salvas\0", NULL},	// Seconds.
	{ L"Tiempo_entre_intentos_acceso\0", NULL },	// Seconds.
	{ NULL, NULL}
};