#include "stdafx.h"
#include "Common.h"

/**********************************************************************************************************************/
/************************************************** class File Methods ************************************************/

/************************************************** Constructors/Destructor *******************************************/

File::File(){

	hashing_algorithm = 0;
	HashValue.QuadPart = 0;
	//Parent = NULL;
	Path = NULL;
	Full_name = NULL;
	Full_name_long = NULL;
	return;
}

File::File(File* From_file) {

	Path = NULL;
	Full_name = NULL;
	Full_name_long = NULL;
	FileData = From_file->get_FileData();
	From_file->get_Path(&Path);
	From_file->get_full_name(&Full_name);
	From_file->get_full_name_long(&Full_name_long);
	is_hashed = From_file->get_if_hashed();
	HashValue = From_file->get_HashValue();
	hashing_algorithm = From_file->get_hashing_algorithm();
}

File::File(LPTSTR Origen, Folder* _Parent, BOOLEAN hashit, unsigned int algorithm, class m_error** p_error){	//0x0002  

	/*********************** Variables definitions ************************************/
	
	HANDLE hFind = INVALID_HANDLE_VALUE;
	class m_error** p_error_t = p_error;
	DWORD last_error;

	/*********************** Body of the function *************************************/

	hashing_algorithm = algorithm;
	HashValue.QuadPart = 0;
	Path = NULL;
	Full_name = NULL;
	Full_name_long = NULL;

	if (wcsncmp(Origen, L"\\\\?\\", 4) == 0) { Origen = Origen + 4; }

	set_Path(Origen);
	LPTSTR p_tmp = wcsrchr(Path, '\\');	// Search for the last \ in the string
	if (p_tmp == NULL){	// There were not '\' characters in the string, so the Origen parameter was wrong.

		SET_PRINT_DELETE_SINGLE_MERROR( FILE_ERRORS, INVALID_FUNCTION_PARAM, Origen)	
		return;
	}
	wmemset(p_tmp + 1, '\0', 1);	//	If the "Origen" variable is a NULL terminated string this will not fail because, in the worth case that there is not a valid 
									//	character after the last '\', there will always be the original \0 character which will be overwritten.

	set_Full_name(Origen);

	hFind = FindFirstFile(Full_name_long, &FileData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		last_error = GetLastError();
		SET_PRINT_DELETE_SINGLE_MERROR( FILE_ERRORS, last_error, Full_name)
		
		return;	// If it's not possible to extract the data of the real file it has not sense to continue on.
	}

	FindClose(hFind);

	status = 0;

	if (_Parent != NULL) {

		class m_error* merror = NULL;
		_Parent->insert_file(this,&merror);
		if (merror != NULL) ADD_MERROR(p_error_t, merror)	
	}

	if (hashit) {

		BYTE* File_content = NULL;
		class m_error* merror = NULL;
		File_content = (BYTE*)load_content(Full_name_long,&merror);
		if (merror != NULL) ADD_MERROR(p_error_t, merror)
		
		if (File_content != NULL) {

			class m_error* merror = NULL;
			calculate_hash(File_content, algorithm, &merror);
			if (merror != NULL) ADD_MERROR(p_error_t, merror)
		}
		delete File_content;
	}
	is_hashed = hashit;
	
	/*LPTSTR messages[5] = { L"Instanciada la clase File: \"\0",Full_name,L", con Path: ",Path,L"\".\0" };
	SET_PRINT_DELETE_SINGLE_MERROR_ARRAY( MESSAGE_LOG, MESSAGE, 5, messages)*/

	return;
}

File::File(WIN32_FIND_DATA* Origin_FileData, Folder* _Parent, BOOLEAN hashit, unsigned int algorithm, class m_error** p_error){

	/*
		!!! VERY IMPORTANT: THIS FUNCTION ONLY CAN BE CALLED FROM INSIDE A FOLDER CLASS THAT IS THE PARENT OF THE FILE ¡¡¡¡

		For creating File classes that represent isolated files there must be used the other constructor.

	*/

	/*********************** Variables definitions ************************************/

	class m_error** p_error_t = p_error;

	/*********************** Body of the function *************************************/

	Path = NULL;
	Full_name = NULL;
	Full_name_long = NULL;
	
	wchar_t* tempo = NULL;
	_Parent->get_full_name(&tempo);

	size_t Path_size = wcslen(tempo) + 2;
	Path = (wchar_t*)calloc(Path_size, sizeof(wchar_t));
	wcscpy_s(Path, Path_size, tempo);
	free(tempo);
	tempo = NULL;
	wcscat_s(Path, Path_size, L"\\");

	get_Path(&tempo);
	size_t Full_name_size = wcslen(tempo) + wcslen(Origin_FileData->cFileName) + 1;
	Full_name = (wchar_t*)calloc(Full_name_size, sizeof(wchar_t));
	wcscpy_s(Full_name, Full_name_size, tempo);
	free(tempo);
	tempo = NULL;
	wcscat_s(Full_name, Full_name_size, Origin_FileData->cFileName);
	set_Full_Name_long();

	//LPTSTR messages[3] = { L"Instanciado el archivo: \"\0",Full_name,L"\".\0" };
	//SET_PRINT_DELETE_SINGLE_MERROR_ARRAY(MESSAGE_LOG, MESSAGE, 3, messages)

	memcpy(&FileData, Origin_FileData, sizeof(WIN32_FIND_DATA));

	status = 0;

	if (_Parent != NULL) {

		class m_error* merror = NULL;
		_Parent->insert_file(this, &merror);
		if (merror != NULL) ADD_MERROR(p_error_t, merror)
	}

	if (hashit) {

		BYTE* File_content = NULL;
		class m_error* merror = NULL;
		File_content = (BYTE*)load_content(Full_name_long, &merror);
		if (merror != NULL) ADD_MERROR(p_error_t, merror)

		if (File_content != NULL) {

			class m_error* merror = NULL;
			calculate_hash(File_content, algorithm, &merror);
			if (merror != NULL) ADD_MERROR(p_error_t, merror)
		}
		delete File_content;
	}
	is_hashed = hashit;

	/*LPTSTR messages[5] = { L"Instanciada la clase File: \"\0",Full_name,L", con Path: ",Path,L"\".\0" };
	SET_PRINT_DELETE_SINGLE_MERROR_ARRAY( MESSAGE_LOG, MESSAGE, 5, messages)*/

	return;
}

File::~File() {

	if (Path != NULL) free(Path);
	if (Full_name != NULL) free(Full_name);
	if (Full_name_long != NULL) free(Full_name_long);
}

/************************************************** Interfaces *********************************************************/

WIN32_FIND_DATA File::get_FileData(void) {

	return FileData;
}

BOOLEAN			File::get_if_hashed(void) {

	return is_hashed;
}

LARGE_INTEGER	File::get_HashValue(void) {

	return HashValue;
}

UINT			File::get_hashing_algorithm(void) {

	return hashing_algorithm;
}

errno_t			File::get_Path(wchar_t** Receiving_Path) {

	if (*Receiving_Path != NULL) free(*Receiving_Path);
	size_t totalsize = wcslen(Path) + 1;
	*Receiving_Path = (wchar_t*)calloc(totalsize, sizeof(wchar_t));
	return (wcscpy_s(*Receiving_Path, totalsize, Path));
}

errno_t			File::get_full_name(wchar_t** Receiving_FullName) {

	if (*Receiving_FullName != NULL) free(*Receiving_FullName);
	size_t totalsize = wcslen(Full_name) + 1;
	*Receiving_FullName = (TCHAR*)calloc(totalsize, sizeof(wchar_t));
	return (wcscpy_s(*Receiving_FullName, totalsize, Full_name));
}

errno_t			File::get_full_name_long(wchar_t** Receiving_FullName_long) {

	if (*Receiving_FullName_long != NULL) free(*Receiving_FullName_long);
	size_t totalsize = wcslen(Full_name_long) + 1;
	*Receiving_FullName_long = (TCHAR*)calloc(totalsize, sizeof(wchar_t));
	return (wcscpy_s(*Receiving_FullName_long, totalsize, Full_name_long));
}

class Folder*	File::get_Parent(void) {

	return Parent;
}

UINT			File::get_status(void) {

	return status;
}



void			File::set_Path(wchar_t* _Path) {

	if (Path != NULL) free(Path);
	size_t totalsize = wcslen(_Path) + 1;
	Path = (wchar_t*)calloc(totalsize, sizeof(wchar_t));
	wcscpy_s(Path, totalsize, _Path);
}

void			File::set_Full_name(wchar_t* _Full_name) {

	if (Full_name != NULL) free(Full_name);
	size_t totalsize = wcslen(_Full_name) + 1;
	Full_name = (wchar_t*)calloc(totalsize, sizeof(wchar_t));
	wcscpy_s(Full_name, totalsize, _Full_name);
	set_Full_Name_long();
	return;
}

void			File::set_Full_Name_long() {

	size_t totalsize = wcslen(Full_name) + 5;
	Full_name_long = (wchar_t*)calloc(totalsize, sizeof(wchar_t));
	wcscpy_s(Full_name_long, totalsize, L"\\\\?\\");
	wcscat_s(Full_name_long, totalsize, Full_name);
	return;
}

void			File::set_status(UINT _status) {

	status = _status;
	return;
}

void			File::set_parent(Folder* _Parent) {

	Parent = _Parent;
}

void			File::set_FileData(WIN32_FIND_DATA _FileData) {

	memcpy(&FileData, &_FileData, sizeof(WIN32_FIND_DATA));
	return;
}



INT16			File::compare(File* ToFile, COMPARE_LEVELS Level, COMPARE_LEVELS sync, class m_error** p_error){

	/*	This function return two possible values:

	-1 There was an error that made imposible to do the comparison or the sync (if it applies).
	or a bit mask composed of the COMPARE_LEVELS that match both files.

	Example: If the compare levels dictated that the comparison was on SIZE (10000), WTIME (01000) and  NAME (01) and the result
	was that the files was equals in NAME and WTIME but not in SIZE the return value will be: 01001

	The "sync" bit mask stablish on what levels (from the set of levels that the bit mask "Level" requested comparison) there should
	be made a synchronization (real an virtual) in case the Files classes compared are different.
	*/

	class m_error** p_error_t = p_error;
	
	//DWORD internal_error;
	INT16 return_value = 0;
	BOOL real_sync = FALSE;
	WIN32_FIND_DATA FileDataTmp;
	wchar_t* ToFile_Full_name = NULL;

	errno_t err = 0;

	err = ToFile->get_full_name(&ToFile_Full_name);
	if (err != 0) {

		SET_PRINT_DELETE_SINGLE_MERROR( FILE_ERRORS, (ERRNO_CODE_MASK | err), TEXT("Error obteniendo el nombre del archivo a comparar."))

		free(ToFile_Full_name);
		return -1;
	}

	FileDataTmp = ToFile->get_FileData();

	if ((Level & NAME) == NAME) {	// If there was requested name comparison.
		
		LPTSTR messages[5] = { L"Comparando el nombre del archivo : \"\0",Full_name,L"\" contra el del archivo: \"\0",ToFile_Full_name,L"\".\0" };
		SET_PRINT_DELETE_SINGLE_MERROR_ARRAY( MESSAGE_LOG, MESSAGE, 5, messages)

		if (wcscmp(FileData.cFileName, FileDataTmp.cFileName) == 0) {  // If both files names are equals

			//SET_PRINT_DELETE_SINGLE_MERROR( MESSAGE_LOG, MESSAGE, L"Nombres coincidentes\0")
			return_value = (return_value | NAME);
		}
		else {

			//SET_PRINT_DELETE_SINGLE_MERROR( MESSAGE_LOG, MESSAGE, L"Nombres no coincidentes\0")
			if ((sync & NAME) == NAME) real_sync = TRUE;
		}
	}
	if ((Level & ATTR) == ATTR) {

		if (FileData.dwFileAttributes == FileDataTmp.dwFileAttributes) return_value = (return_value | ATTR);
		else if ((sync & ATTR) == ATTR) real_sync = TRUE;
	}
	if ((Level & ATIME) == ATIME) {
	
		if (CompareFileTime(&(FileData.ftLastAccessTime),&(FileDataTmp.ftLastAccessTime)) == 0) return_value = (return_value | ATIME);
		else if ((sync & ATIME) == ATIME) real_sync = TRUE;
	}
	if ((Level & WTIME) == WTIME) {
	
		//LPTSTR messages[5] = { L"Comparando la fecha de modificación del archivo: \"\0",Full_name,L"\" contra la del archivo: \"\0",ToFile_Full_name,L"\".\0" };
		//SET_PRINT_DELETE_SINGLE_MERROR_ARRAY( MESSAGE_LOG, MESSAGE, 5, messages)

		if (CompareFileTime(&(FileData.ftLastWriteTime), &(FileDataTmp.ftLastWriteTime)) == 0) {

			//SET_PRINT_DELETE_SINGLE_MERROR( MESSAGE_LOG, MESSAGE, L"Fechas de modificación coincidentes\0")
			return_value = (return_value | WTIME);
		}
		else {

			//SET_PRINT_DELETE_SINGLE_MERROR( MESSAGE_LOG, MESSAGE, L"Fechas de modificación no coincidentes\0")
			if ((sync & WTIME) == WTIME) real_sync = TRUE;
		}
	}
	if ((Level & SIZE) == SIZE) {

		//LPTSTR messages[5] = { L"Comparando el tamaño del archivo: \"\0",Full_name,L"\" contra el del archivo: \"\0",ToFile_Full_name,L"\".\0" };
		//SET_PRINT_DELETE_SINGLE_MERROR_ARRAY( MESSAGE_LOG, MESSAGE, 5, messages)


		if ((FileData.nFileSizeHigh == FileDataTmp.nFileSizeHigh) && (FileData.nFileSizeLow == FileDataTmp.nFileSizeLow)) {

			//SET_PRINT_DELETE_SINGLE_MERROR( MESSAGE_LOG, MESSAGE, L"Tamaños coincidentes\0")
			return_value = (return_value | SIZE);
		}
		else {

			//SET_PRINT_DELETE_SINGLE_MERROR( MESSAGE_LOG, MESSAGE, L"Tamaños no coincidentes\0")
			if ((sync & SIZE) == SIZE) real_sync = TRUE;
		}
	
	}
	/*
	if ((Level & HASH) == HASH) {}

	if ((Level & CONTENT) == CONTENT) {}
	*/
	if (real_sync) {

//#ifdef _DEBUG
//		_CrtDbgReportW(_CRT_WARN, TEXT("File.cpp"), 269, TEXT("SyncFolder.exe"), TEXT("Sincronizando los archivos.\r\n"));
//#endif

		class m_error* merror = NULL;
		BOOL band = synchronize(ToFile, &merror);
		if (merror != NULL) ADD_MERROR(p_error_t, merror)
		
		if (!(band)) {

			free(ToFile_Full_name);
			return -1;
		}
		else {	// If the sync was correct doesn't matter the result of the comparison, the files are equals in all respects.

			return_value = Level;
		}
	}

//#ifdef _DEBUG
//	_CrtDbgReportW(_CRT_WARN, TEXT("File.cpp"), 280, TEXT("SyncFolder.exe"), TEXT("Resultado de la comparación: %X\r\n"), return_value);
//#endif
	
	free(ToFile_Full_name);
	return return_value;
}

BOOL			File::delete_from_system(class m_error** p_error) {

	class m_error** p_error_t = p_error;


	if (!DeleteFile(Full_name)) {

		DWORD internal_error = GetLastError();
		SET_PRINT_DELETE_SINGLE_MERROR(FILE_ERRORS, internal_error, Full_name)

			return FALSE;
	}
	else {

		LPTSTR messages[3] = { L"Eliminado el archivo: \"\0",Full_name,L"\" del sistema.\0" };
		SET_PRINT_DELETE_SINGLE_MERROR_ARRAY(MESSAGE_LOG, MESSAGE, 3, messages)
	}

	return TRUE;
}

BOOLEAN			File::synchronize(File* ToFile, class m_error** p_error) {

	/*	This function returns true if the sync was correct, false if not.	*/

	class m_error** p_error_t = p_error;

	DWORD internal_error;
	WIN32_FIND_DATA FileDataTmp;
	wchar_t* ToFile_Full_name = NULL;

	errno_t err = 0;

	err = ToFile->get_full_name(&ToFile_Full_name);
	if (err != 0) {

		SET_PRINT_DELETE_SINGLE_MERROR(FILE_ERRORS, (ERRNO_CODE_MASK | err), TEXT("Error obteniendo el nombre del archivo a salvar."))

			free(ToFile_Full_name);
		return FALSE;
	}
	else {

		//LPTSTR messages[5] = { L"Sincronizando el archivo: \"\0",Full_name,L"\" con el archivo: \"\0",ToFile_Full_name,L"\".\0" };
		//SET_PRINT_DELETE_SINGLE_MERROR_ARRAY( MESSAGE_LOG, MESSAGE, 5, messages)
	}

	if (!(CopyFile(ToFile_Full_name, Full_name, FALSE))) {

		internal_error = GetLastError();
		SET_PRINT_DELETE_SINGLE_MERROR(FILE_ERRORS, internal_error, ToFile_Full_name)

			free(ToFile_Full_name);
		return FALSE;
	}
	else {	// The real file copy was successful

		FileDataTmp = ToFile->get_FileData();
		memcpy(&FileData, &FileDataTmp, sizeof(WIN32_FIND_DATA));
		is_hashed = ToFile->get_if_hashed();
		HashValue = ToFile->get_HashValue();
		hashing_algorithm = ToFile->get_hashing_algorithm();
		//ToFile->get_Path(Path);	//	This does not proceed because Path and Full name respond to the file in the remote file system, not in the local.
		//	Not proceed nether the copy of Parent or status.  
		status = 0;
	}

	free(ToFile_Full_name);
	return TRUE;
}

/************************************************** Private Methods ***************************************************/

LARGE_INTEGER File::calculate_hash(BYTE* content, unsigned int algorithm, class m_error** p_error){ //0x0002

	LARGE_INTEGER temp;
	class m_error** p_error_t = p_error; //0x0002
	 //0x0002


	temp.QuadPart = 0;
	hashing_algorithm = 0;

	 //0x0002
	return temp;
}

BYTE* File::load_content(LPTSTR File, class m_error** p_error){ //0x0002

	// this will use the function ReadFile (carefull with the locking of the file that the documentations alert). A possible solution is to copy the file to a tmp location
	// before reading it to memory.

	// The buffer where the file contents are stored must be allocated with the "new" keyword

	class m_error** p_error_t = p_error; //0x0002
	 //0x0002

	BYTE* return_value = NULL;

	  //0x0002
	return return_value;
}

BOOLEAN File::compare_content(BYTE* content1, BYTE* content2, DWORD size){

	// this function compares byte by byte the content of both buffers, when it finds the first inconsistency it returns FALSE. If it reach the end of
	// both buffers without finding an unmatch it returns TRUE.

	DWORD i;
	for (i = 0; i < size; i++) {

		if (content1[i] != content2[i]) return FALSE;
		i++;

	}
	return TRUE;
}
