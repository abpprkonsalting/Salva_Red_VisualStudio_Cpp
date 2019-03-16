
#include "stdafx.h"
#include "SyncFolder.h"


#pragma comment(lib, "advapi32.lib")

#define SVCNAME TEXT("SyncFolder")
#define SVCNAME_FULL TEXT("Servicio Salva Automática de Archivos")

SERVICE_STATUS          gSvcStatus;
SERVICE_STATUS_HANDLE   gSvcStatusHandle;
HANDLE                  ghSvcStopEvent = NULL;

//VOID SvcInstall(LPCTSTR _lpServiceStartName, LPCTSTR _lpPassword);
VOID SvcInstall(void);
VOID WINAPI SvcCtrlHandler(DWORD);
VOID WINAPI SvcMain(DWORD, LPTSTR *);

VOID ReportSvcStatus(DWORD, DWORD, DWORD);
VOID SvcInit(DWORD, LPTSTR *);
//VOID SvcReportEvent(LPTSTR);
void DoDeleteSvc(void);
void DisplayUsage();

//
// Purpose: 
//   Entry point for the process
//
// Parameters:
//   None
// 
// Return value:
//   None
//
void __cdecl _tmain(int argc, TCHAR *argv[])
{
	// If command-line parameter is "install", install the service. 
	// Otherwise, the service is probably being started by the SCM.

	if (lstrcmpi(argv[1], TEXT("install")) == 0)
	{
		//if (argc != 4) {

		//	printf("ERROR: Incorrect number of arguments\n\n");
		//	DisplayUsage();
		//	return;
		//}
		//// Chequear aquí que el parámetro pasado como usuario es un usuario existente en algún dominio conocido.
		//// Chequear que el password sea correcto.

		//SvcInstall(argv[2], argv[3]);
		//return;

		if (argc != 2) {

			printf("ERROR: Incorrect use\n\n");
			DisplayUsage();
			return;
		}
		SvcInstall();
		return;
	}
	else if (lstrcmpi(argv[1], TEXT("uninstall")) == 0) {

		if (argc != 2) {

			printf("ERROR: Incorrect use\n\n");
			DisplayUsage();
			return;
		}
		DoDeleteSvc();
		return;
	}
	else DisplayUsage();

	// Aquí tengo que buscar una manera de asegurarme de que el proceso que está llamando a este es el Service Control Manager, si no es así imprimir un error y salir sin
	// hacer nada.

	// TO_DO: Add any additional services for the process to this table.
	SERVICE_TABLE_ENTRY DispatchTable[] =
	{
		{ SVCNAME, (LPSERVICE_MAIN_FUNCTION)SvcMain },
		{ NULL, NULL }
	};

	// This call returns when the service has stopped. 
	// The process should simply terminate when the call returns.

	if (!StartServiceCtrlDispatcher(DispatchTable))
	{
		//SvcReportEvent(TEXT("StartServiceCtrlDispatcher"));
	}
}

//
// Purpose: 
//   Installs a service in the SCM database
//
// Parameters:
//   None
// 
// Return value:
//   None
//
//VOID SvcInstall(LPCTSTR _lpServiceStartName, LPCTSTR _lpPassword)
VOID SvcInstall(void)
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	TCHAR szPath[MAX_PATH];

	if (!GetModuleFileName(NULL, szPath, MAX_PATH))
	{
		printf("Cannot install service (%d)\n", GetLastError());
		return;
	}

	// Get a handle to the SCM database. 

	schSCManager = OpenSCManager(
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == schSCManager)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	// Create the service

	//schService = CreateService(
	//	schSCManager,              // SCM database 
	//	SVCNAME,                   // name of service 
	//	SVCNAME_FULL,             // service name to display 
	//	SERVICE_ALL_ACCESS,        // desired access 
	//	SERVICE_WIN32_OWN_PROCESS, // service type 
	//	SERVICE_DEMAND_START,      // start type						// Esto debo cambiarlo a SERVICE_AUTO_START 
	//	SERVICE_ERROR_NORMAL,      // error control type 
	//	szPath,                    // path to service's binary 
	//	NULL,                      // no load ordering group			// Esto debo investigarlo para que el servicio se cargue después que la red esté lista.
	//	NULL,                      // no tag identifier					// Lo mismo que arriba.
	//	NULL,                      // no dependencies					// Lo mismo que arriba.
	//	_lpServiceStartName,
	//	_lpPassword);

	schService = CreateService(
		schSCManager,              // SCM database 
		SVCNAME,                   // name of service 
		SVCNAME_FULL,             // service name to display 
		SERVICE_ALL_ACCESS,        // desired access 
		SERVICE_WIN32_OWN_PROCESS, // service type 
		SERVICE_AUTO_START,			// start type						// Esto debo cambiarlo a SERVICE_AUTO_START 
		SERVICE_ERROR_NORMAL,      // error control type 
		szPath,                    // path to service's binary 
		NULL,                      // no load ordering group			// Esto debo investigarlo para que el servicio se cargue después que la red esté lista.
		NULL,                      // no tag identifier					// Lo mismo que arriba.
		NULL,                      // no dependencies					// Lo mismo que arriba.
		NULL,
		NULL);

	if (schService == NULL)
	{
		printf("CreateService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}

	else {

		SERVICE_DESCRIPTION _description = { TEXT("Salva automáticamente y de manera periódica los archivos contenidos en una carpeta hacia otra") };
		ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, &_description);
		printf("Service installed successfully\n");
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

//
// Purpose: 
//   Entry point for the service
//
// Parameters:
//   dwArgc - Number of arguments in the lpszArgv array
//   lpszArgv - Array of strings. The first string is the name of
//     the service and subsequent strings are passed by the process
//     that called the StartService function to start the service.
// 
// Return value:
//   None.
//
VOID WINAPI SvcMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
	// Register the handler function for the service

	gSvcStatusHandle = RegisterServiceCtrlHandler(
		SVCNAME,
		SvcCtrlHandler);

	if (!gSvcStatusHandle)
	{
		//SvcReportEvent(TEXT("RegisterServiceCtrlHandler"));
		return;
	}

	// These SERVICE_STATUS members remain as set here

	gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	gSvcStatus.dwServiceSpecificExitCode = 0;

	// Report initial status to the SCM

	ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

	// Perform service-specific initialization and work.

	SvcInit(dwArgc, lpszArgv);
}

Folder* instanciar_Folder_class(LPTSTR Folder_name) {

	Folder* return_value = NULL;
	wchar_t* temp = NULL;
	class m_error* merror = NULL;

	//#ifdef _DEBUG
	//	_CrtDbgReportW(_CRT_WARN, TEXT("SyncFolder.cpp"),14, TEXT("SyncFolder.exe"), TEXT("%d\r\n"), 1);
	//#endif

	BOOL try_again;

	do {

		// Chequear el tamaño del archivo log, si es más grande de lo establecido en la variable del archivo de configuración para eso (por hacer)
		// eliminarlo y crear uno nuevo. En este caso para eso es necesario que el parámetro que se le pase a esta función sea un puntero a hLogFile,
		// para poder modificarlo desde aquí adentro y seguir usándolo en main.

		try_again = FALSE;

		return_value = new Folder(Folder_name, NULL, FALSE, 0, &merror);

		if (merror != NULL) {

			if (merror->print_all_errors() == -1) exit(-1);

			LPTSTR	error_message = NULL;
			//try_again = FALSE;
			merror->get_last_error_message(&error_message);

			if (wcscmp(error_message, Folder_name) == 0) {	// The error is related with the Folder class been instantiated.

				DWORD domain = merror->get_last_error_domain();
				if (domain != MESSAGE_LOG) {							// This is actually an error, not a message

					DWORD code = merror->get_last_error_code();

					delete merror;
					merror = NULL;

					if ((code & 0x20000000) == 0x20000000) {	// This is an application defined error code, so it's fatal.

						LPTSTR messages[3] = { L"Error grave instanciando la clase Folder para la carpeta: \"\0",Folder_name, L"\".\0" };
						merror = new m_error(MESSAGE_LOG, MESSAGE, 3, messages);
						if (merror->print_all_errors() == -1) exit(-1);

						delete merror;
						merror = NULL;
						delete return_value;
						return NULL;
					}
					else {	// This is an error defined by windows, so it's possible to recover from it. Delete the Folder class and start over again in 10s

						LPTSTR messages[3] = { L"Error no grave instanciando la clase Folder para la carpeta: \"\0",Folder_name, L"\".\0" };
						merror = new m_error(MESSAGE_LOG, MESSAGE, 3, messages);
						if (merror->print_all_errors() == -1) exit(-1);

						delete merror;
						merror = NULL;

						delete return_value;
						try_again = TRUE;
						//Sleep(CONFd(L"Tiempo_entre_intentos_acceso\0") * 1000);
						if (WaitForSingleObject(ghSvcStopEvent, (CONFd(L"Tiempo_entre_intentos_acceso\0") * 1000)) != WAIT_TIMEOUT) return NULL;
					}
				}
			}
			if (merror != NULL) delete merror;
			merror = NULL;
		}
	} while (try_again);
	return return_value;
}

//
// Purpose: 
//   The service code
//
// Parameters:
//   dwArgc - Number of arguments in the lpszArgv array
//   lpszArgv - Array of strings. The first string is the name of
//     the service and subsequent strings are passed by the process
//     that called the StartService function to start the service.
// 
// Return value:
//   None
//
VOID SvcInit(DWORD dwArgc, LPTSTR *lpszArgv)
{
	// TO_DO: Declare and set any required variables.
	//   Be sure to periodically call ReportSvcStatus() with 
	//   SERVICE_START_PENDING. If initialization fails, call
	//   ReportSvcStatus with SERVICE_STOPPED.

	/********************** Global Variables ****************************/

	class m_error* merror = NULL;
	Folder* Remote_Folder = NULL;
	Folder* Local_Folder = NULL;
	Folder* Compare_results = NULL;

	wchar_t* temp = NULL;

	TCHAR SyncFolderServicePath[MAX_PATH];

	if (!GetModuleFileName(NULL, SyncFolderServicePath, MAX_PATH))
	{
		ReportSvcStatus(SERVICE_STOPPED, GetLastError(), 0);
		return;
	}

	LPTSTR p_tmp = wcsrchr(SyncFolderServicePath, '\\');	// Search for the last \ in the string
	p_tmp++;
	wmemset(p_tmp, '\0', 1);
	p_tmp++;
	wmemset(p_tmp, '\0', 1);

	size_t TmpFilesize = wcslen(SyncFolderServicePath) + 8;
	LPTSTR SyncFolderTmpFile = (LPTSTR)calloc(TmpFilesize, sizeof(wchar_t));
	wcscpy_s(SyncFolderTmpFile, TmpFilesize, SyncFolderServicePath);
	wcscat_s(SyncFolderTmpFile, TmpFilesize, TEXT("log.txt"));

	HANDLE	file_error_library_hLogFile = initialize_file_error_library(SyncFolderTmpFile);

	free(SyncFolderTmpFile);

	if (file_error_library_hLogFile == INVALID_HANDLE_VALUE) {
		
		ReportSvcStatus(SERVICE_STOPPED, ERROR_FILE_NOT_FOUND, 0);
		return;
	}

	TmpFilesize = wcslen(SyncFolderServicePath) + 14;
	SyncFolderTmpFile = (LPTSTR)calloc(TmpFilesize, sizeof(wchar_t));
	wcscpy_s(SyncFolderTmpFile, TmpFilesize, SyncFolderServicePath);
	wcscat_s(SyncFolderTmpFile, TmpFilesize, TEXT("settings.txt\0"));

	if (!read_conf_file(SyncFolderTmpFile, parameters, &merror)) {

		if (merror != NULL) {

			// Esto aquí se puede optimizar cambiando el orden de ejecución: se agregar el error nuevo a la cadena de errores ya existente, se imprimen todos los errores y finalmente
			// se imprimen todos de una vez.

			merror->print_all_errors();
			delete merror;
			merror = NULL;
			merror = new m_error(MESSAGE_LOG, MESSAGE, L"Error leyendo el archivo de configuración.\0");
			merror->print_all_errors();
			delete merror;
			merror = NULL;
		}
		free(SyncFolderTmpFile);
		uninitialize_file_error_library();
		ReportSvcStatus(SERVICE_STOPPED, ERROR_FILE_NOT_FOUND, 0);
		return;
	}
	else {

		if (merror != NULL) {

			if (merror->print_all_errors() == -1) {

				uninitialize_file_error_library();
				ReportSvcStatus(SERVICE_STOPPED, ERROR_FILE_NOT_FOUND, 0);
				delete merror;
				free(SyncFolderTmpFile);
				return;
			}
			delete merror;
			merror = NULL;
		}
		free(SyncFolderTmpFile);
	}

	// Create an event. The control handler function, SvcCtrlHandler,
	// signals this event when it receives the stop control code.

	ghSvcStopEvent = CreateEvent(
		NULL,    // default security attributes
		TRUE,    // manual reset event
		FALSE,   // not signaled
		NULL);   // no name

	if (ghSvcStopEvent == NULL)
	{
		ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
		return;
	}

	// Report running status when initialization is complete.

	ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

	Remote_Folder = instanciar_Folder_class(CONF(L"Carpeta_Salva\0"));

	if (Remote_Folder == NULL) {
	//if (1) {

		uninitialize_file_error_library();
		ReportSvcStatus(SERVICE_STOPPED, ERROR_PATH_NOT_FOUND, 0);
		return;
	}

	while (1)
	{
		// Chequear el tamaño del archivo log, si es más grande de lo establecido en la variable del archivo de configuración para eso (por hacer)
		// eliminarlo y crear uno nuevo.

		Local_Folder = instanciar_Folder_class(CONF(L"Carpeta_Origen\0"));

		if (Local_Folder != NULL) {

			/*LPTSTR messages[3] = { L"Instanciada la clase Folder para la carpeta: \"\0",CONF(L"Carpeta_Origen"),L"\".\0" };
			merror = new m_error(MESSAGE_LOG, MESSAGE,3, messages);
			merror->print_all_errors(hLogFile);

			delete merror;
			merror = NULL;*/

			// Antes de empezar una fase de procesamiento determinada se chequea que el servicio no se haya señalizado como detenido.
			DWORD ServiceStoped = WaitForSingleObject(ghSvcStopEvent, 0);
			if ((ServiceStoped == WAIT_ABANDONED) || (ServiceStoped == WAIT_OBJECT_0) || (ServiceStoped == WAIT_FAILED)) {

				delete Local_Folder;
				Local_Folder = NULL;
				uninitialize_file_error_library();
				ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
				return;
			}

			Compare_results = Remote_Folder->compare_content(Local_Folder, (WTIME | SIZE), &merror);

			if (merror != NULL) {

				if (merror->print_all_errors() == -1) {

					delete merror;
					merror = NULL;
					delete Local_Folder;
					Local_Folder = NULL;
					if (Compare_results != NULL) delete Compare_results;
					Compare_results = NULL;
					uninitialize_file_error_library();
					ReportSvcStatus(SERVICE_STOPPED, ERROR_FILE_NOT_FOUND, 0);
					return;
				}
				delete merror;
				merror = NULL;
			}

			if (Compare_results != NULL) {

				LPTSTR messages[5] = { L"Encontradas diferencias entre la carpeta: \"\0",CONF(L"Carpeta_Origen"),L"\" y la carpeta: \"\0",CONF(L"Carpeta_Salva\0"),
					L"\".\0" };
				merror = new m_error(MESSAGE_LOG, MESSAGE, 5, messages);
				if (merror->print_all_errors() == -1) {

					delete merror;
					merror = NULL;
					delete Local_Folder;
					Local_Folder = NULL;
					delete Compare_results;
					Compare_results = NULL;
					uninitialize_file_error_library();
					ReportSvcStatus(SERVICE_STOPPED, ERROR_FILE_NOT_FOUND, 0);
					return;
				}

				delete merror;
				merror = NULL;

				// Antes de empezar una fase de procesamiento determinada se chequea que el servicio no se haya señalizado como detenido.
				DWORD ServiceStoped = WaitForSingleObject(ghSvcStopEvent, 0);
				if ((ServiceStoped == WAIT_ABANDONED) || (ServiceStoped == WAIT_OBJECT_0) || (ServiceStoped == WAIT_FAILED)) {

					delete Local_Folder;
					Local_Folder = NULL;
					delete Compare_results;
					Compare_results = NULL;
					uninitialize_file_error_library();
					ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
					return;
				}

				Remote_Folder->synchronize(Compare_results, &merror);

				if (merror != NULL) {

					if (merror->print_all_errors() == -1) {

						delete merror;
						merror = NULL;
						delete Local_Folder;
						Local_Folder = NULL;
						delete Compare_results;
						Compare_results = NULL;
						uninitialize_file_error_library();
						ReportSvcStatus(SERVICE_STOPPED, ERROR_FILE_NOT_FOUND, 0);
						return;
					}
					delete merror;
					merror = NULL;
				}
				delete Compare_results;
				Compare_results = NULL;
			}

			delete Local_Folder;
			Local_Folder = NULL;
		}
		else {

			uninitialize_file_error_library();
			ReportSvcStatus(SERVICE_STOPPED, ERROR_PATH_NOT_FOUND, 0);
			return;
		}

		LPTSTR messages[3] = { L"Terminado un ciclo de comparación de la carpeta: \"\0",CONF(L"Carpeta_Origen"), L"\".\0" };
		merror = new m_error(MESSAGE_LOG, MESSAGE, 3, messages);
		if (merror->print_all_errors() == -1) {

			delete merror;
			merror = NULL;
			uninitialize_file_error_library();
			ReportSvcStatus(SERVICE_STOPPED, ERROR_FILE_NOT_FOUND, 0);
			return;
		}

		delete merror;
		merror = NULL;

		// Check whether to stop the service.

		if (WaitForSingleObject(ghSvcStopEvent, (CONFd(L"Tiempo_entre_salvas\0") * 1000)) != WAIT_TIMEOUT) {

			uninitialize_file_error_library();
			ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
			return;
		}
	}
}

//
// Purpose: 
//   Sets the current service status and reports it to the SCM.
//
// Parameters:
//   dwCurrentState - The current state (see SERVICE_STATUS)
//   dwWin32ExitCode - The system error code
//   dwWaitHint - Estimated time for pending operation, 
//     in milliseconds
// 
// Return value:
//   None
//
VOID ReportSvcStatus(DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwWaitHint)
{
	static DWORD dwCheckPoint = 1;

	// Fill in the SERVICE_STATUS structure.

	gSvcStatus.dwCurrentState = dwCurrentState;
	gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
	gSvcStatus.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_START_PENDING)
		gSvcStatus.dwControlsAccepted = 0;
	else gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	if ((dwCurrentState == SERVICE_RUNNING) ||
		(dwCurrentState == SERVICE_STOPPED))
		gSvcStatus.dwCheckPoint = 0;
	else gSvcStatus.dwCheckPoint = dwCheckPoint++;

	// Report the status of the service to the SCM.
	SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}

//
// Purpose: 
//   Called by SCM whenever a control code is sent to the service
//   using the ControlService function.
//
// Parameters:
//   dwCtrl - control code
// 
// Return value:
//   None
//
VOID WINAPI SvcCtrlHandler(DWORD dwCtrl)
{
	// Handle the requested control code. 

	switch (dwCtrl)
	{
	case SERVICE_CONTROL_STOP:
		ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

		// Signal the service to stop.

		SetEvent(ghSvcStopEvent);
		ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);

		return;

	case SERVICE_CONTROL_INTERROGATE:
		break;

	default:
		break;
	}

}

//
// Purpose: 
//   Logs messages to the event log
//
// Parameters:
//   szFunction - name of function that failed
// 
// Return value:
//   None
//
// Remarks:
//   The service must have an entry in the Application event log.
//
//VOID SvcReportEvent(LPTSTR szFunction)
//{
//	HANDLE hEventSource;
//	LPCTSTR lpszStrings[2];
//	TCHAR Buffer[80];
//
//	hEventSource = RegisterEventSource(NULL, SVCNAME);
//
//	if (NULL != hEventSource)
//	{
//		StringCchPrintf(Buffer, 80, TEXT("%s failed with %d"), szFunction, GetLastError());
//
//		lpszStrings[0] = SVCNAME;
//		lpszStrings[1] = Buffer;
//
//		ReportEvent(hEventSource,        // event log handle
//			EVENTLOG_ERROR_TYPE, // event type
//			0,                   // event category
//			SVC_ERROR,           // event identifier
//			NULL,                // no security identifier
//			2,                   // size of lpszStrings array
//			0,                   // no binary data
//			lpszStrings,         // array of strings
//			NULL);               // no binary data
//
//		DeregisterEventSource(hEventSource);
//	}
//}

void DoDeleteSvc()
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	SERVICE_STATUS ssStatus;

	// Get a handle to the SCM database. 

	schSCManager = OpenSCManager(
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == schSCManager)
	{
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	// Get a handle to the service.

	schService = OpenService(
		schSCManager,       // SCM database 
		SVCNAME,			// name of service 
		DELETE);            // need delete access 

	if (schService == NULL)
	{
		printf("OpenService failed (%d)\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}
	
	// Attempt to close the service.

	ControlService(schService, SERVICE_CONTROL_STOP,&ssStatus);

	// Delete the service.

	if (!DeleteService(schService))
	{
		printf("DeleteService failed (%d)\n", GetLastError());
	}
	else printf("Service deleted successfully\n");

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

void DisplayUsage()
{
	/*printf("Description:\n");
	printf("\tCommand-line tool that controls the service \"Servicio sincronizador de archivos.\"\n\n");
	printf("Usage:\n");
	printf("SyncFolder command [command parameters]\n\n");
	printf("\taccepted commands: \n");
	printf("\t  install account password\n");
	printf("\t\t  account: the user account under wich the service will run in the form: \"DomainName\\UserName\"\n");
	printf("\t\t  password: the password for user account under wich the service will run\n");
	printf("\t  unistall\n");*/

	printf("Description:\n");
	printf("\tCommand-line tool that controls the service: \"Servicio sincronizador de archivos.\"\n\n");
	printf("Usage:\n");
	printf("\tSyncFolder install\n");
	printf("\tSyncFolder uninstall\n");
}