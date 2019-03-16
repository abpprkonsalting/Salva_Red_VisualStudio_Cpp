#include <windows.h>
#include <tchar.h> 
#include <stdio.h>
#include <strsafe.h>
#include <crtdbg.h>
#include <time.h>
//#include <Shlwapi.h>

#ifndef _FILE_ERROR_LIBRARY_DEFINED_
#define _FILE_ERROR_LIBRARY_DEFINED_

#define SET_MERROR(PP_MERROR,_domain,_error_code,_message) {*PP_MERROR = new m_error(_domain, _error_code,_message);PP_MERROR = (*PP_MERROR)->get_next_error_address();}
#define SET_MERROR_ARRAY(PP_MERROR,_domain,_error_code,elements,_message) {*PP_MERROR = new m_error(_domain, _error_code,elements,_message);PP_MERROR = (*PP_MERROR)->get_next_error_address();}

#define SET_PRINT_DELETE_SINGLE_MERROR(_domain,_error_code,_message) {class m_error* e = new m_error(_domain, _error_code,_message);e->print_all_errors();delete e;}
#define SET_PRINT_DELETE_SINGLE_MERROR_ARRAY(_domain,_error_code,elements,_message) {class m_error* e = new m_error(_domain, _error_code,elements,_message);e->print_all_errors();delete e;}

#define ADD_MERROR(PP_MERROR,MERROR) {*PP_MERROR = MERROR; class m_error** t = MERROR->get_last_next_error_address();PP_MERROR = t;}


//#define SET_PRINT_DELETE_SINGLE_MERROR(PP_MERROR,_domain,_error_code,_message) {}
//#define SET_PRINT_DELETE_SINGLE_MERROR_ARRAY(PP_MERROR,_domain,_error_code,elements,_message) {}
//#define ADD_MERROR(PP_MERROR,MERROR) {}

typedef enum _ERROR_DOMAINS {
	FILE_ERRORS=1,
	FOLDER_ERRORS,
	UNDETERMINED_ERROR_DOMAIN,
	MEMORY_ERRORS,
	CHARACTER_STRING_ERRORS,
	PROGRAM_PARAMS_ERRORS,
	MESSAGE_LOG
} ERROR_DOMAINS;

struct string_maps {

	INT			code;
	wchar_t*	string;
};

/*************** Application defined error codes ****************************/

/* The Windows error code definitions for the Win32 API functions are defined
 in the header file WinError.h. It is stated there that the error codes have the
 following structure:

 Values are 32 bit values laid out as follows:

   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
  +---+-+-+-----------------------+-------------------------------+
  |Sev|C|R|     Facility          |               Code            |
  +---+-+-+-----------------------+-------------------------------+

  where

      Sev - is the severity code

          00 - Success
          01 - Informational
          10 - Warning
          11 - Error

      C - is the Customer code flag

      R - is a reserved bit

      Facility - is the facility code

      Code - is the facility's status code

********************************************************************************
From there it can be seen that the bit 29 is reserved for custom error codes,
i.e: error codes defined by the applications; So, a the range usable for application
error codes could be: 0x20000000 - 0x3FFFFFFF.

Carefull with this, because the use of this bit does not means that windows
error codes are bellow 0x20000000, the windows error codes are 32 bit values, so there are
an upper part above the customer code flag, and could be codes that have this bit 
cleared and are above 0x20000000, e.g: 0x40000000:

 3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
+---+-+-+-----------------------+-------------------------------+
|0 1|0|0|0 0 0 0 0 0 0 0 0 0 0 0|0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0|
+---+-+-+-----------------------+-------------------------------+

*/

#define USE_APPLICATION_DEFINED_ERROR_CODES 0
#ifdef USE_APPLICATION_DEFINED_ERROR_CODES 

#ifndef FILENAME_TOO_LONG
#define FILENAME_TOO_LONG 0x20000001
#endif // !FILENAME_TOO_LONG

#ifndef INVALID_FUNCTION_PARAM
#define INVALID_FUNCTION_PARAM 0x20000002
#endif // !INVALID_FUNCTION_PARAM

#ifndef MEMORY_ALLOCATION_ERROR
#define MEMORY_ALLOCATION_ERROR 0x20000003
#endif

#ifndef MEMORY_DEALLOCATION_ERROR
#define MEMORY_DEALLOCATION_ERROR 0x20000004
#endif

#ifndef EMPTY_FILE
#define EMPTY_FILE 0x20000005
#endif

#ifndef MISSING_PARAMETER_IN_CONFIG_FILE
#define MISSING_PARAMETER_IN_CONFIG_FILE 0x20000006
#endif

#ifndef CONVERSION_ERROR
#define CONVERSION_ERROR 0x20000007
#endif

#ifndef UNSUPPORTED_TEXT_FILE_FORMAT
#define UNSUPPORTED_TEXT_FILE_FORMAT 0x20000008
#endif

#ifndef INVALID_VALUE_IN_CONFIG_FILE
#define INVALID_VALUE_IN_CONFIG_FILE 0x20000009
#endif

#ifndef INVALID_STRING_PARAM
#define INVALID_STRING_PARAM 0x2000000A
#endif

#ifndef INVALID_INT_PARAM
#define INVALID_INT_PARAM 0x2000000B
#endif

#ifndef MISSING_PARAMETER
#define MISSING_PARAMETER 0x2000000C
#endif

#ifndef MESSAGE
#define MESSAGE 0x2000000D
#endif

/****************************/
/*	Add more error codes here
	before the range reserved
	for ERRNO error codes */



/****************************/
#ifndef ERRNO_CODE_MASK
#define ERRNO_CODE_MASK 0x30000000
#endif

/*************************************************************************/

#endif

static HANDLE hLogFile = INVALID_HANDLE_VALUE;

HANDLE	initialize_file_error_library(LPTSTR log_file);

void uninitialize_file_error_library();

wchar_t* get_string(struct domains* _dominios, INT Nombre);

class m_error {

	// Properties

private:

	DWORD	domain;
	DWORD	error_code;
	LPTSTR	message;
	time_t	error_time;
	class m_error* next_error;
	

public:
	

	// Methods

public:

	m_error(DWORD _domain, DWORD _error_code, LPTSTR _message);
	m_error(DWORD _domain, DWORD _error_code, UINT size_messages_array, LPTSTR messages[]);
	~m_error();
	
	INT print_all_errors();
	class m_error** get_next_error_address();
	errno_t	get_message(LPTSTR Receiver);
	DWORD get_error_code();
	DWORD get_error_domain();

	BOOL contain_errors();
	
	DWORD get_last_error_domain();
	DWORD get_last_error_code();
	errno_t get_last_error_message(LPTSTR* Receiver);
	class m_error** get_last_next_error_address();
	

};

#endif