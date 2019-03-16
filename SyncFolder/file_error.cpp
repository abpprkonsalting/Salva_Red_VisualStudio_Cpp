#include "stdafx.h"
#include "file_error.h"

struct string_maps dominios[] = {

	{ FILE_ERRORS, L"FILE_ERRORS\0" },
	{ FOLDER_ERRORS, L"FOLDER_ERRORS\0" },
	{ UNDETERMINED_ERROR_DOMAIN, L"UNDETERMINED_ERROR_DOMAIN\0" },
	{ MEMORY_ERRORS, L"MEMORY_ERRORS\0" },
	{ CHARACTER_STRING_ERRORS, L"CHARACTER_STRING_ERRORS\0" },
	{ PROGRAM_PARAMS_ERRORS, L"PROGRAM_PARAMS_ERRORS\0" },
	{ MESSAGE_LOG, L"MESSAGE_LOG\0" },
	{0, NULL }

};

struct string_maps errores[] = {

	{0x20000001,L"FILENAME_TOO_LONG\0"},
	{0x20000002,L"INVALID_FUNCTION_PARAM\0"},
	{0x20000003 ,L"MEMORY_ALLOCATION_ERROR\0" },
	{0x20000004 ,L"MEMORY_DEALLOCATION_ERROR\0" },
	{0x20000005 ,L"EMPTY_FILE \0" },
	{0x20000006 ,L"MISSING_PARAMETER_IN_CONFIG_FILE\0" },
	{0x20000007 ,L"CONVERSION_ERROR\0" },
	{0x20000008 ,L"UNSUPPORTED_TEXT_FILE_FORMAT\0" },
	{0x20000009 ,L"INVALID_VALUE_IN_CONFIG_FILE\0" },
	{0x2000000A ,L"INVALID_STRING_PARAM\0" },
	{0x2000000B ,L"INVALID_INT_PARAM\0" },
	{0x2000000C ,L"MISSING_PARAMETER\0" },
	{0x2000000D ,L"MESSAGE\0" },
	{0,NULL}
};

HANDLE	initialize_file_error_library(LPTSTR log_file) {

	hLogFile = CreateFile(log_file, (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_WRITE | FILE_SHARE_READ), NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	return hLogFile;
}

void uninitialize_file_error_library() {

	if (hLogFile != INVALID_HANDLE_VALUE) CloseHandle(hLogFile);
}

wchar_t* get_text(struct string_maps* _dominios, INT code) {

	unsigned int i = 0;

	while (_dominios[i].code != 0) {

		if (_dominios[i].code == code) {

			if (_dominios[i].string != NULL) return _dominios[i].string;
			else return NULL;
		}
		i++;
	}
	return NULL;
}

m_error::m_error(DWORD _domain, DWORD _error_code, LPTSTR _message) {	//0x00AA[NULL]

	//_CrtDbgReportW(_CRT_WARN, TEXT("file_error.cpp"), 6, TEXT("SyncFolder.exe"), TEXT("creating error with code: \"%d\"\r\n"), _error_code);
	
	domain = _domain;
	error_code = _error_code;
	message = _wcsdup(_message);
	time(&error_time);
	next_error = NULL;
}

m_error::m_error(DWORD _domain, DWORD _error_code, UINT size_messages_array, LPTSTR messages[]) {
	
	domain = _domain;
	error_code = _error_code;
	time(&error_time);
	next_error = NULL;

	size_t total_size = 0;
	for (UINT i = 0; i < size_messages_array; i++) {

		total_size = total_size + wcslen(messages[i]);
	}
	total_size++;
	total_size++;

	message = (LPTSTR)calloc(total_size, sizeof(wchar_t));
	for (UINT i = 0; i < size_messages_array; i++) {

		wcscat_s(message, total_size,messages[i]);
	}
}

m_error::~m_error(){

	//LocalFree(message);
	free(message);
	if (next_error != NULL) delete next_error;
}

class m_error** m_error::get_next_error_address(){

	return &(next_error);
}

errno_t	m_error::get_message(LPTSTR Receiver){

	size_t total_size = wcslen(message) + 1;
	Receiver = (wchar_t*)calloc(total_size, sizeof(wchar_t));
	return (wcscpy_s(Receiver, total_size, message));
}

DWORD m_error::get_error_code(){
	return error_code;
}

DWORD m_error::get_error_domain() {
	return domain;
}

INT m_error::print_all_errors() {
	
	/*
		return_value = 1 -> success.
		return_value = 0 -> internal error.
		return_value = -1 -> fatal, hLogFile invalid.
	*/
	
	errno_t err;
	wchar_t* wdomain = NULL;
	wchar_t* wcode = NULL;
	wchar_t* printed_message = NULL;
	size_t total_size;
	DWORD written;
	INT return_value = 0;

//#ifdef _DEBUG
//	_CrtDbgReportW(_CRT_WARN, TEXT("File_error.cpp"), 154, TEXT("SyncFolder.exe"), TEXT("%S\r\n"), L"Entering");
//#endif
	
	if (hLogFile != INVALID_HANDLE_VALUE) {

		wchar_t* w_error_time = (wchar_t*)calloc(26, sizeof(wchar_t));
		
		err = _wctime_s(w_error_time,26,&error_time);

		if (err == 0) {

			wmemset(w_error_time + 24, '\0', 1);

			if ((error_code & 0x20000000) == 0x20000000) {

				// This is a application defined error

				total_size = wcslen(get_text(errores, error_code)) + 1;
				wcode = (wchar_t*)calloc(total_size, sizeof(wchar_t));
				_get_errno(&err);
				if (err != ENOMEM) {

					//err = _ultow_s(error_code, wcode, 12, 10);
					//if (err == 0) return_value = TRUE;
					wcscpy_s(wcode, total_size, get_text(errores, error_code));
					return_value = 1;
				}
//#ifdef _DEBUG
//				_CrtDbgReportW(_CRT_WARN, TEXT("File_error.cpp"), 154, TEXT("SyncFolder.exe"), TEXT("wcode: %S\r\n"), wcode);
//#endif
			}
			else {

				// This is a windows error.

				LPVOID lpMsgBuf = NULL;

				DWORD message_size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL, error_code, 0, (LPTSTR)&lpMsgBuf, 0, NULL);

				if (message_size > 0) {

					wcode = (wchar_t*)calloc(message_size + 1, sizeof(wchar_t));
					_get_errno(&err);
					if (err != ENOMEM) {

						// Any other situation different that succeed in coping the lpMsgBuf to wcode do not set the return_value flag so when
						// we check it further on the function does not continue it's natural course.
						if (SUCCEEDED(StringCchCopy(wcode, message_size + 1, (LPTSTR)lpMsgBuf))) {

							wchar_t caracter = '\0';
							wmemcpy_s(wcode + message_size - 1, 1, &caracter, 1);
							//memcpy((void*)(wcode + message_size),&caracter, 1);
							return_value = TRUE;
						}

					}	// If the wcode buffer couldn't be set there is no StringCchCopy and the next step is to LocalFree the lpMsgBuf 
				}
				else {
					// The text of the windows message could not be retrieved so we stick to the original code.

					wcode = (wchar_t*)calloc(12, sizeof(wchar_t));
					_get_errno(&err);
					if (err != ENOMEM) {

						err = _ultow_s(error_code, wcode, 12, 10);
						if (err == 0) return_value = TRUE;
					}
				}
				if (lpMsgBuf != NULL) LocalFree(lpMsgBuf);
			}

			if (return_value) {

				return_value = FALSE;

				total_size = 10 + wcslen(w_error_time) + wcslen(get_text(dominios,domain)) + wcslen(wcode) + wcslen(message);
//#ifdef _DEBUG
//					_CrtDbgReportW(_CRT_WARN, TEXT("File_error.cpp"), 154, TEXT("SyncFolder.exe"), TEXT("%d\r\n"), total_size);
//#endif
				printed_message = (wchar_t*)calloc(total_size, sizeof(wchar_t));
				if (err != ENOMEM) {

					if (wcscat_s(printed_message, total_size, w_error_time) == 0) {

						if (wcscat_s(printed_message, total_size, L": \0") == 0) {

							if (wcscat_s(printed_message, total_size, get_text(dominios,domain)) == 0) {

								if (wcscat_s(printed_message, total_size, L" :\0") == 0) {

									if (wcscat_s(printed_message, total_size, wcode) == 0) {

										if (wcscat_s(printed_message, total_size, L" = \0") == 0) {

											if (wcscat_s(printed_message, total_size, message) == 0) {

												if (wcscat_s(printed_message, total_size, L"\r\n\0") == 0) {

													//_CrtDbgReportW(_CRT_WARN, TEXT("file_error.cpp"), 101, TEXT("SyncFolder.exe"), TEXT("%S\r\n"), printed_message);

													size_t ReturnValue;
													if (wcstombs_s(&ReturnValue, NULL, 0, printed_message, _TRUNCATE) == 0) {

														char* printed_message_c = (char*)calloc(ReturnValue, 1);
														_get_errno(&err);
														if (err != ENOMEM) {

															if (wcstombs_s(&ReturnValue, printed_message_c, ReturnValue, printed_message, _TRUNCATE) == 0) {

																if (WriteFile(hLogFile, printed_message_c, ReturnValue - 1, &written, NULL) == TRUE) {

																	if (next_error != NULL) {

																		if (next_error->print_all_errors() == TRUE) return_value = TRUE;
																	}
																}
															}
														}
														if (printed_message_c != NULL) free(printed_message_c);
													}
												}
											}
										}
									}
								}
							}
						}

					}
				}
				if (printed_message != NULL) free(printed_message);
			}
			if (wcode != NULL) free(wcode);
		}			
		free(w_error_time);
		return return_value;
	}
	else return -1;
}

BOOL m_error::contain_errors() {

	class m_error* merror_c = this;

	while (merror_c != NULL) {

		if (merror_c->get_error_domain() != MESSAGE_LOG) return TRUE;
		merror_c = merror_c->next_error;
	}
	return FALSE;
}

DWORD m_error::get_last_error_domain() {

	if (next_error != NULL) return next_error->get_last_error_domain();
	else return domain;
}

DWORD m_error::get_last_error_code() {

	if (next_error != NULL) return next_error->get_last_error_code();
	else return error_code;
}

errno_t m_error::get_last_error_message(LPTSTR* Receiver) {

	class m_error* merror_c = this;

	while (merror_c->next_error != NULL) merror_c = merror_c->next_error;
	
	size_t total_size = wcslen(merror_c->message) + 1;
	*Receiver = (wchar_t*)calloc(total_size, sizeof(wchar_t));
	return (wcscpy_s(*Receiver, total_size, merror_c->message));
	
}

class m_error** m_error::get_last_next_error_address() {

	if (next_error != NULL) return next_error->get_last_next_error_address();
	else return &next_error;

}