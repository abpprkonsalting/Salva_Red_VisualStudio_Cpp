#include "stdafx.h"
#include "conf_file.h"
//#include "file_error.h"


int check_text_file_format(char* content){

	return 0;
}

BOOL read_conf_file(wchar_t* Origen, struct parameters_t* parametros, class m_error** p_error){

	class m_error** p_error_t = p_error;
	
	HANDLE hConfigFile;
	LARGE_INTEGER ConfigFileSize;
	char* file_content = NULL;
	wchar_t* file_content_converted = NULL;
	errno_t err;
	DWORD NumberOfBytesRead;
	unsigned int i;
	int file_format;
	BOOL resultado = TRUE;
	DWORD last_error;
	//size_t total_size;
	wchar_t* temp = NULL;

	//SET_PRINT_DELETE_SINGLE_MERROR( MESSAGE_LOG, MESSAGE, TEXT("Leyendo parámetros desde el archivo de configuración."))
		SET_PRINT_DELETE_SINGLE_MERROR(MESSAGE_LOG, MESSAGE, TEXT("Leyendo parámetros desde el archivo de configuración."))


	hConfigFile = CreateFile(Origen, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hConfigFile == INVALID_HANDLE_VALUE){

		last_error = GetLastError();
		//SET_PRINT_DELETE_SINGLE_MERROR( FILE_ERRORS, last_error, Origen)
			SET_PRINT_DELETE_SINGLE_MERROR(FILE_ERRORS, last_error, Origen)
		
		return FALSE;
	}

	//_CrtDbgReportW(_CRT_WARN, TEXT("SyncFolder.cpp"),34, TEXT("SyncFolder.exe"), TEXT("config file: \"%S\" opened\r\n"), Origen);
	if (!GetFileSizeEx(hConfigFile, &ConfigFileSize)){

		last_error = GetLastError();
		//SET_PRINT_DELETE_SINGLE_MERROR( FILE_ERRORS, last_error, Origen)
			SET_PRINT_DELETE_SINGLE_MERROR(FILE_ERRORS, last_error, Origen)
		resultado = FALSE;
	}
	else {

		//_CrtDbgReportW(_CRT_WARN, TEXT("SyncFolder.cpp"), 55, TEXT("SyncFolder.exe"), TEXT("config file size: %d\r\n"), ConfigFileSize.QuadPart);

		file_content = (char*)calloc((size_t)(ConfigFileSize.QuadPart + 1), 1);
		_get_errno(&err);
		if (err == ENOMEM) {

			//SET_PRINT_DELETE_SINGLE_MERROR( MEMORY_ERRORS, MEMORY_ALLOCATION_ERROR, Origen)
				SET_PRINT_DELETE_SINGLE_MERROR(MEMORY_ERRORS, MEMORY_ALLOCATION_ERROR, Origen)
			resultado = FALSE;
		}
		else {

			// ReadFile needs to be used in a way that I can check the correct arrival to EOF.

			if (!ReadFile(hConfigFile, file_content, (DWORD)ConfigFileSize.QuadPart, &NumberOfBytesRead, NULL)){

				last_error = GetLastError();
				SET_PRINT_DELETE_SINGLE_MERROR( FILE_ERRORS, last_error, Origen)
				resultado = FALSE;
			}
			else {

				//_CrtDbgReportW(_CRT_WARN, TEXT("SyncFolder.cpp"), 99, TEXT("SyncFolder.exe"), TEXT("Bytes read from the config file: %d\r\n"),NumberOfBytesRead);
				//_CrtDbgReport(_CRT_WARN, "SyncFolder.cpp", 100, "SyncFolder.exe", "Config file content: \r\n%S\r\n", (char*)file_content);

				file_format = check_text_file_format(file_content);

				switch (file_format) {

				case 0:	// The file format is ANSI single byte.

					size_t cantidad_necesaria;
					size_t cantidad_convertida;

					mbstowcs_s(&cantidad_necesaria, NULL, 0, file_content, _TRUNCATE);
					_get_errno(&err);
					if (err != 0) {

						SET_PRINT_DELETE_SINGLE_MERROR( CHARACTER_STRING_ERRORS, CONVERSION_ERROR, Origen)
						resultado = FALSE;
					}
					else {

						file_content_converted = (wchar_t*)calloc(cantidad_necesaria, sizeof(wchar_t));
						_get_errno(&err);
						if (err == ENOMEM) {

							SET_PRINT_DELETE_SINGLE_MERROR( MEMORY_ERRORS, MEMORY_ALLOCATION_ERROR, Origen)
							resultado = FALSE;
						}
						else {

							mbstowcs_s(&cantidad_convertida, file_content_converted, cantidad_necesaria, file_content, _TRUNCATE);
							_get_errno(&err);
							if (err != 0) {

								SET_PRINT_DELETE_SINGLE_MERROR( CHARACTER_STRING_ERRORS, CONVERSION_ERROR, Origen)
								resultado = FALSE;
							}
						}
						// free file_content, we don't need it any more.
						free(file_content);
						_get_errno(&err);
						if (err != 0){

							SET_PRINT_DELETE_SINGLE_MERROR( MEMORY_ERRORS, MEMORY_DEALLOCATION_ERROR, Origen)
							resultado = FALSE;
						}
					}
					break;
				case 1:
					// Not implemented yet;
					break;
				case 2:

					// Chequear el comienzo del archivo por BOM para eliminarlo

					file_content_converted = (wchar_t*)file_content;
					break;
				default:
					SET_PRINT_DELETE_SINGLE_MERROR( CHARACTER_STRING_ERRORS, UNSUPPORTED_TEXT_FILE_FORMAT, Origen)
					resultado = FALSE;
					break;
				}

				//_CrtDbgReportW(_CRT_WARN, TEXT("SyncFolder.cpp"), 170, TEXT("SyncFolder.exe"), TEXT("Config file content: \r\n%S\r\n"), file_content_converted);

				if (resultado){	// No ocurrió ningún error en la conversión de formato de archivo.

					wchar_t seps[] = L"\r\n";
					wchar_t seps_nombre[] = L" ";
					wchar_t* line = NULL;
					wchar_t* next_line = NULL;
					wchar_t* context;
					wchar_t* Nombre;
					wchar_t* Valor;
					wchar_t* Comilla_inicial;
					wchar_t* Comilla_final;
					wchar_t* Comentario;

					line = wcstok_s(file_content_converted, seps, &next_line);
					//line = strtok_s(file_content, seps, &next_line);

					if (line == NULL){	// This is an error because there is not lines in the file

						SET_PRINT_DELETE_SINGLE_MERROR( FILE_ERRORS, EMPTY_FILE, Origen)
						resultado = FALSE;
					}
					else {

						while (line != NULL){

							//_CrtDbgReportW(_CRT_WARN, L"SyncFolder.cpp", 197, L"SyncFolder.exe", L"line:%S\r\n", line);
							Comentario = NULL;
							Comentario = wcspbrk(line, L"#;");

							if (Comentario != NULL) memset((char*)Comentario, 0, 2);

							Nombre = NULL;
							context = NULL;
							Valor = NULL;
							Comilla_inicial = NULL;
							Comilla_final = NULL;

							Nombre = wcstok_s(line, seps_nombre, &context);	// Search in the line the first token after the initial spaces (if any).

							if (Nombre != NULL){										// If a token (Name of the parameter) was found in this line.

								//_CrtDbgReportW(_CRT_WARN, L"SyncFolder.cpp", 208, L"SyncFolder.exe", L"Nombre a procesar: %S\r\n", Nombre);
									
								i = 0;
								while (parametros[i].Nombre != NULL){				// Search through the array of Names of parameters.

									if (wcscmp(parametros[i].Nombre, Nombre) == 0){	// If this is the name of the parameter

										if (parametros[i].Valor == NULL){			// If the parameter has not been filled yet.

											Comilla_inicial = wcspbrk(context, L"\"");	// Search for an initial quotation mark in the remainder of the line. 

											if (Comilla_inicial != NULL) {	// If there is an initial quotation mark

												Comilla_final = wcspbrk(Comilla_inicial + 1, L"\"");	// search for the clossing quotation mark.

												if (Comilla_final != NULL) {

												//	_CrtDbgReportW(_CRT_WARN, TEXT("SyncFolder.cpp"), 240, TEXT("SyncFolder.exe")
												//		, TEXT("final: %S\r\n"), Comilla_final - 1);

													memset((char*)Comilla_final, 0, 2);
													parametros[i].Valor = _wcsdup(Comilla_inicial + 1);

												}
												else {	// This is an error.

													SET_PRINT_DELETE_SINGLE_MERROR( CHARACTER_STRING_ERRORS, INVALID_VALUE_IN_CONFIG_FILE, parametros[i].Nombre)
													resultado = FALSE;
													break;
												}
											}
											else {
												Valor = wcstok_s(NULL, L" #;", &context);	// Search again in the remains of the line for the next token 
																							// that does not begin with spaces and return a pointer to 
																							// it in the variable Valor

												if (Valor != NULL){						// If a token is found

														parametros[i].Valor = _wcsdup(Valor);	// If there are not # or ; characters in the Value
																								// duplicate the string.
												}
											}
										}
										break;
									}
									i++;
								}
							}
							if (resultado) line = wcstok_s(NULL, seps, &next_line);	// Process next line
							else break;
						}

						// Finished reading all lines
						// Check that all the parameters were read from the configuration file, if not return 

						if (resultado) {

							//_CrtDbgReportW(_CRT_WARN, L"conf_file.cpp", 285, L"SyncFolder.exe", L"Variables leidas desde el archivo settings.txt:\r\n\r\n");

							i = 0;
							while (parametros[i].Nombre != NULL){				// Search through the array of parameters.

								if (parametros[i].Valor == NULL) {			// If a parameter is blank this is an error.

									SET_PRINT_DELETE_SINGLE_MERROR( FILE_ERRORS, MISSING_PARAMETER_IN_CONFIG_FILE, parametros[i].Nombre)
									resultado = FALSE;
									break;
								}
								else {
									//_CrtDbgReportW(_CRT_WARN, L"conf_file.cpp", 300, L"SyncFolder.exe", L"%S : %S\r\n", parametros[i].Nombre, parametros[i].Valor);
									
									LPTSTR messages[3] = { parametros[i].Nombre,L": \0",parametros[i].Valor };
									SET_PRINT_DELETE_SINGLE_MERROR_ARRAY( MESSAGE_LOG, MESSAGE,3,messages)
								}
								i++;
							}
						}
					}
				}
			}

			// free file_content_converted

			if (file_content_converted != NULL) free(file_content_converted);
			_get_errno(&err);
			if (err != 0){

				SET_PRINT_DELETE_SINGLE_MERROR( MEMORY_ERRORS, MEMORY_DEALLOCATION_ERROR, Origen)
				resultado = FALSE;
			}
		}
	}
	// Close the file handle
	if (!CloseHandle(hConfigFile)) {

		last_error = GetLastError();
		SET_PRINT_DELETE_SINGLE_MERROR( FILE_ERRORS, last_error, Origen)
		resultado = FALSE;
	}
	
	return resultado;
}

wchar_t* conf_string(struct parameters_t* parametros, wchar_t* Nombre){

	unsigned int i = 0;

	while (parametros[i].Nombre != NULL){				// Search through the array of parameters names.

		if (wcscmp(parametros[i].Nombre, Nombre) == 0){	// If this is the parameters name

			if (parametros[i].Valor != NULL) return parametros[i].Valor;
			else return NULL;	
		}
		i++;
	}
	return NULL;
}

int conf_int(struct parameters_t* parametros, wchar_t* Nombre){

	errno_t err;
	unsigned int i = 0;
	int return_value;

	while (parametros[i].Nombre != NULL){				// Search through the array of parameters names.

		if (wcscmp(parametros[i].Nombre, Nombre) == 0){	// If this is the parameters name

			if (parametros[i].Valor != NULL){

				return_value = _wtoi(parametros[i].Valor);

				_get_errno(&err);
				if (err != 0) return 0;
				else return return_value;
			}
			else return 0;
		}
		i++;
	}
	return 0;
}