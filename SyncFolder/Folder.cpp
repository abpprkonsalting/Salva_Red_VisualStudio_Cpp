#include "stdafx.h"
#include "Common.h"


Folder::Folder(){

	Files_hashed = FALSE;
	hashing_algorithm = 0;

	// Initialize the Folder object arrays.

	Files.count = 0;
	Files.items = NULL;
	Subfolders.count = 0;
	Subfolders.items = NULL;
	Path = NULL;
	Full_name = NULL;
	Full_name_long = NULL;
	return;
}

Folder::Folder(Folder* _Parent, class m_error** p_error) {

	class m_error** p_error_t = p_error;

	Path = NULL;
	Full_name = NULL;
	Full_name_long = NULL;
	Files_hashed = FALSE;
	hashing_algorithm = 0;
	
	if (_Parent != NULL) {
		Parent = _Parent;
		class m_error* merror = NULL;
		Parent->insert_SubFolder(this, &merror);
		if (merror != NULL) ADD_MERROR(p_error_t, merror)
	}

	// Initialize the Folder object arrays.

	Files.count = 0;
	Files.items = NULL;
	Subfolders.count = 0;
	Subfolders.items = NULL;

	return;
}

Folder::Folder(LPTSTR Origen, Folder* _Parent, bool hashit, unsigned int algorithm, class m_error** p_error){

	// This is one of the parametric constructors of the Folder class. The variables it accept are the following:
	//
	//	Origen:		Input.	This is a string with the full name and path of the real folder in the file system that is intended be represented by the "Folder object".
	//						THIS CONSTRUCTOR DOES NOT VALIDATE THE EXISTENCE OF THE TERMINATING NULL CHARACTER, SO THE CALLING FUNCTION MUST PASS A NULL TERMINATED STRING.
	//	Parent:		Input	A pointer to a Folder class that contains this one (if exists).
	//	hashit:		Input.	Calculate the hash of the files contained inside the Folder and it's subfolders.
	//	algorithm:	Input.	The hash algorithm selected to do the hashing.
	//	p_error:	Output.	A pointer to a location where a linked list of "m_errors" can be placed. The way the calling function has to request errors report
	//						functionallity is to place in this variable a location where a m_error structure could be placed. This means the address of a variable 
	//						of type m_error* in this way:
	//
	//						class m_error* i = NULL;
	//
	//						Folder* g = new Folder(a,b,c, &i);
	//
	//						The constructor will create then an linked list of error structs and return it in the variable i. 
	//						IT'S MANDATORY THAT, IF THIS FUNCTIONALLITY IS NOT GOING TO BE REQUESTED, THIS PARAMETER MUST BE NULL.

	/*********************** Variables definitions ************************************/

	HANDLE hFind = INVALID_HANDLE_VALUE;
	class m_error** p_error_t = p_error;
	DWORD internal_error;

	/*********************** Body of the function *************************************/

	//LPTSTR messages[3] = { L"Instanciando la clase Folder para la carpeta: \"\0",Origen,L"\".\0" };
	////SET_PRINT_DELETE_SINGLE_MERROR_ARRAY( MESSAGE_LOG, MESSAGE, 3, messages)
	//SET_PRINT_DELETE_SINGLE_MERROR_ARRAY(MESSAGE_LOG, MESSAGE, 3, messages, hLogFile)

	// Initialize the Folder object arrays. These initializations must be here because in the destructor these fields need to have those values.

	Path = NULL;
	Full_name = NULL;
	Full_name_long = NULL;
	Files.count = 0;
	Files.items = NULL;
	Subfolders.count = 0;
	Subfolders.items = NULL;

	if (wcsncmp(Origen, L"\\\\?\\", 4) == 0) { Origen = Origen + 4; }

	set_Path(Origen);

	LPTSTR p_tmp = wcsrchr(Path, '\\');	// Search for the last \ in the string
	if (p_tmp == NULL) {	// There were not '\' characters in the string, so the Origen parameter was wrong.

		//SET_PRINT_DELETE_SINGLE_MERROR( FOLDER_ERRORS, INVALID_FUNCTION_PARAM, Origen)
		SET_MERROR(p_error_t, FOLDER_ERRORS, INVALID_FUNCTION_PARAM, Origen)
		return;
	}
	wmemset(p_tmp + 1, '\0', 1);	//	If the "Origen" variable is a NULL terminated string this will not fail because, in the worth case that there is not a valid 
									//	character after the last '\', there will always be the original \0 character which will be overwritten.

	set_Full_Name(Origen);

	// Find the data of the real folder

	hFind = FindFirstFile(Full_name_long, &FileData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		internal_error = GetLastError();
		// This error is recovable.
		//SET_PRINT_DELETE_SINGLE_MERROR( FOLDER_ERRORS, internal_error, Full_name)
		SET_MERROR(p_error_t, FOLDER_ERRORS, internal_error, Full_name)
		
		return;	// If it's not possible to extract the data of the real folder it has not sense to continue on.
	}
	
	FindClose(hFind);

	if (_Parent != NULL) {
		Parent = _Parent;
		class m_error* merror = NULL;
		Parent->insert_SubFolder(this, &merror);
		if (merror != NULL) ADD_MERROR(p_error_t, merror)
	}

	Files_hashed = hashit;
	hashing_algorithm = algorithm;
	status = 0;

//#ifdef _DEBUG
//	_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 119, TEXT("SyncFolder.exe"), TEXT("FileData.cFileName: %S\r\n"), FileData.cFileName);
//#endif

	// Prepare string for use with FindFile functions.  First, copy the string to a buffer, then append '\*' to the directory name.
	
	size_t szDir_size = wcslen(Full_name_long) + 3;
	wchar_t* szDir = (wchar_t*)calloc(szDir_size, sizeof(wchar_t));
	StringCchCopy(szDir, szDir_size, Full_name_long);
	StringCchCat(szDir, szDir_size, TEXT("\\*"));

	// Find the first file in the directory.

	WIN32_FIND_DATA ffd;
	hFind = INVALID_HANDLE_VALUE;
	BOOL continuar;

	hFind = FindFirstFile(szDir, &ffd);

	if (hFind == INVALID_HANDLE_VALUE){

		internal_error = GetLastError();

		// ERROR_FILE_NOT_FOUND is not really an error in this context because it just means that inside the folder there were not files. 
		// Any other error is reported if that funcionallity has been requested.

		if (internal_error != ERROR_FILE_NOT_FOUND) {

			// Here the error is reported with Full_name as a parameter (instead of szDir) because the folder couldn't be listed at all, so it has not sense to instantiate
			// the folder class. In the function that called this one must be checked the errors reported, and if this is one on them, the Folder class should be deleted and 
			// tried it's instantiation again, if proceed.

			// This error is recovable.
			//SET_PRINT_DELETE_SINGLE_MERROR( FOLDER_ERRORS, internal_error, Origen)
			SET_MERROR(p_error_t, FOLDER_ERRORS, internal_error, Full_name)
		}
		
		free(szDir);
		return;
	}

	do {

		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) { // This is a subdirectory, so we create a Folder class that represent it.

			if ((lstrcmp(ffd.cFileName, TEXT(".")) != 0) && (lstrcmp(ffd.cFileName, TEXT("..")) != 0)) { // If this subfolder is not . or ..

				class m_error* merror = NULL;
				Folder* Subfolder_temp = new Folder(&ffd, this, hashit, algorithm, &merror);
				if (merror != NULL) ADD_MERROR(p_error_t, merror)
			}
		}
		else {	// The file is a regular file, so what is created in this case is a File class

			class m_error* merror = NULL;
			File* file_temp = new File(&ffd, this, hashit, algorithm, &merror);
			if (merror != NULL) ADD_MERROR(p_error_t, merror)
		}
		continuar = FindNextFile(hFind, &ffd);
		internal_error = GetLastError();
	} while (continuar);

	// This point is only reached if FindNextFile if equal to 0, and this set the last error variable, so there is not posibility that the "last error" has been set
	// by a function different that FindNextFile

	// ERROR_NO_MORE_FILES is not really an error in this context, it just means that the directory listing process has finish.

	if (internal_error != ERROR_NO_MORE_FILES) {

		// Here the error is reported with szDir as message because at least one file was listed in the Origen folder, and that make this error not fatal.

		SET_PRINT_DELETE_SINGLE_MERROR( FOLDER_ERRORS, internal_error, szDir)
	}
	FindClose(hFind);
	free(szDir);

	/*LPTSTR messages3[5] = { L"Instanciada la clase Folder: \"\0",Full_name,L", con Path: ",Path,L"\".\0" };
	SET_PRINT_DELETE_SINGLE_MERROR_ARRAY( MESSAGE_LOG, MESSAGE, 5, messages3)

	wchar_t* cantidad_archivos = (wchar_t*)calloc(20, sizeof(wchar_t));
	_ultow_s(Files.count, cantidad_archivos, 20, 10);
	LPTSTR messages1[5] = { L"Cantidad de archivos en la carpeta: \"\0",Origen,L": ",cantidad_archivos,L"\".\0" };
	SET_PRINT_DELETE_SINGLE_MERROR_ARRAY( MESSAGE_LOG, MESSAGE, 5, messages1)
	free(cantidad_archivos);

	wchar_t* cantidad_folders = (wchar_t*)calloc(20, sizeof(wchar_t));
	_ultow_s(Subfolders.count, cantidad_folders, 20, 10);
	LPTSTR messages2[5] = { L"Cantidad de subcarpetas en la carpeta: \"\0",Origen,L": ",cantidad_folders,L"\".\0" };
	SET_PRINT_DELETE_SINGLE_MERROR_ARRAY( MESSAGE_LOG, MESSAGE, 5, messages2)
	free(cantidad_folders);*/

	return;
}

Folder::Folder(WIN32_FIND_DATA* Origin_FileData, Folder* _Parent, bool hashit, unsigned int algorithm, class m_error** p_error){

	// This is one of the parametric constructors of the Folder class. The variables it accept are the following:
	//
	//	Origin_FileData:	Input.	The Folder class is been constructed recursivelly from an upper level Folder, so the information of the real folder in the
	//								file system is passed to this constructor as a WIN32_FIND_DATA struct.
	//	Parent:				Input	A pointer to a Folder class that contains this one (if exists).
	//	hashit:				Input.	Calculate the hash of the files contained inside the Folder and it's subfolders.
	//	algorithm:			Input.	The hash algorithm selected to do the hashing.
	//	p_error:			Output.	A pointer to a location where a linked list of "m_errors" can be placed. The way the calling function has to request errors report
	//						functionallity is to place in this variable a location where a m_error structure could be placed. This means the address of a variable 
	//						of type m_error* in this way:
	//
	//						class m_error* i = NULL;
	//
	//						Folder* g = new Folder(a,b,c, &i);
	//
	//						The constructor will create then an linked list of error structs and return it in the variable i. 
	//						IT'S MANDATORY THAT, IF THIS FUNCTIONALLITY IS NOT GOING TO BE REQUESTED, THIS PARAMETER MUST BE NULL.

	/*********************** Variables definitions ************************************/

	//WIN32_FIND_DATA ffd;
	//HANDLE hFind = INVALID_HANDLE_VALUE;
	//size_t length_of_arg;
	class m_error** p_error_t = p_error;
	DWORD internal_error;

	/*********************** Body of the function *************************************/

	Path = NULL;
	Full_name = NULL;
	Full_name_long = NULL;

	Files_hashed = hashit;	// Carefull with this because we are stating that the files on the folder has been hashed when they has not been yet.
	hashing_algorithm = algorithm;

	// Initialize the Folder object arrays.

	Files.count = 0;
	Files.items = NULL;
	Subfolders.count = 0;
	Subfolders.items = NULL;

	wchar_t* tempo = NULL;
	_Parent->get_full_name(&tempo);

	size_t Path_size = wcslen(tempo) + 2;
	Path = (wchar_t*)calloc(Path_size, sizeof(wchar_t));
	wcscpy_s(Path, Path_size, tempo);
	free(tempo);
	tempo = NULL;
	wcscat_s(Path, Path_size, L"\\");
	
	get_path(&tempo);
	size_t Full_name_size = wcslen(tempo) + wcslen(Origin_FileData->cFileName) + 1;
	Full_name = (wchar_t*)calloc(Full_name_size, sizeof(wchar_t));
	wcscpy_s(Full_name, Full_name_size, tempo);
	free(tempo);
	tempo = NULL;
	wcscat_s(Full_name, Full_name_size, Origin_FileData->cFileName);
	set_Full_Name_long();

	memcpy(&FileData, Origin_FileData, sizeof(WIN32_FIND_DATA));

	status = 0;

	if (_Parent != NULL) {

		Parent = _Parent;
		class m_error* merror = NULL;
		Parent->insert_SubFolder(this, &merror);
		if (merror != NULL) ADD_MERROR(p_error_t, merror)
		
	}

	//LPTSTR messages[3] = { L"Instanciada la carpeta: \"\0",Full_name,L"\".\0" };
	//SET_PRINT_DELETE_SINGLE_MERROR_ARRAY(MESSAGE_LOG, MESSAGE, 3, messages)

	// Prepare string for use with FindFile functions.  First, copy the string to a buffer, then append '\*' to the directory name.

	size_t szDir_size = wcslen(Full_name_long) + 3;
	wchar_t* szDir = (wchar_t*)calloc(szDir_size, sizeof(wchar_t));
	StringCchCopy(szDir, szDir_size, Full_name_long);
	StringCchCat(szDir, szDir_size, TEXT("\\*"));

	// Find the first file in the directory.

	WIN32_FIND_DATA ffd;
	HANDLE hFind;
	BOOL continuar;

	hFind = FindFirstFile(szDir, &ffd);

	if (hFind == INVALID_HANDLE_VALUE){

		internal_error = GetLastError();

		// ERROR_FILE_NOT_FOUND is not really an error in this context because it just means that inside the folder there were not files. 
		// Any other error is reported if that funcionallity has been requested.

		if (internal_error != ERROR_FILE_NOT_FOUND){

			SET_PRINT_DELETE_SINGLE_MERROR( FOLDER_ERRORS, internal_error, Full_name)
		}
		free(szDir);
		return;
	}

	do {

		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {	// This is a subdirectory, so we create a Folder class that represent it.

			if ((lstrcmp(ffd.cFileName, TEXT(".")) != 0) && (lstrcmp(ffd.cFileName, TEXT("..")) != 0)) { // If this subfolder is not . or ..
																										 
				class m_error* merror = NULL;
				Folder* Subfolder_temp = new Folder(&ffd, this, hashit, algorithm, &merror);
				if (merror != NULL) ADD_MERROR(p_error_t, merror)
			}
		}
		else {	// The file is a regular file, so what is created in this case is a File class

			class m_error* merror = NULL;
			File* file_temp = new File(&ffd, this, hashit, algorithm, &merror);
			if (merror != NULL) ADD_MERROR(p_error_t, merror)
		}
		continuar = FindNextFile(hFind, &ffd);
		internal_error = GetLastError();
	} while (continuar);

	internal_error = GetLastError();

	// ERROR_NO_MORE_FILES is not really an error in this context, it just means that the directory listing process has finish.

	if (internal_error != ERROR_NO_MORE_FILES) {

		SET_PRINT_DELETE_SINGLE_MERROR( FOLDER_ERRORS, internal_error, szDir)
	}

	FindClose(hFind);
	free(szDir);
	
	/*LPTSTR messages3[5] = { L"Instanciada la clase Folder: \"\0",Full_name,L", con Path: ",Path,L"\".\0" };
	SET_PRINT_DELETE_SINGLE_MERROR_ARRAY( MESSAGE_LOG, MESSAGE, 5, messages3)

	wchar_t* cantidad_archivos = (wchar_t*)calloc(20, sizeof(wchar_t));
	_ultow_s(Files.count, cantidad_archivos, 20, 10);
	LPTSTR messages1[5] = { L"Cantidad de archivos en la carpeta: \"\0",Full_name,L": ",cantidad_archivos,L"\".\0" };
	SET_PRINT_DELETE_SINGLE_MERROR_ARRAY( MESSAGE_LOG, MESSAGE, 5, messages1)
		free(cantidad_archivos);

	wchar_t* cantidad_folders = (wchar_t*)calloc(20, sizeof(wchar_t));
	_ultow_s(Subfolders.count, cantidad_folders, 20, 10);
	LPTSTR messages2[5] = { L"Cantidad de subcarpetas en la carpeta: \"\0",Full_name,L": ",cantidad_folders,L"\".\0" };
	SET_PRINT_DELETE_SINGLE_MERROR_ARRAY( MESSAGE_LOG, MESSAGE, 5, messages2)
		free(cantidad_folders);*/

	return;
}

BOOL Folder::insert_SubFolder(Folder* Subfolder, class m_error** p_error){

	class m_error** p_error_t = p_error;
	
	/*TCHAR* Inserted_Folder_Full_name;

	errno_t err = 0;

	err = Subfolder->get_full_name(&Inserted_Folder_Full_name);
	if (err != 0) {

		SET_PRINT_DELETE_SINGLE_MERROR( FILE_ERRORS, (ERRNO_CODE_MASK | err), TEXT("Error obteniendo el nombre del archivo a insertar."))
		
		return FALSE;
	}
	else {

		LPTSTR messages[5] = { L"Insertando la carpeta: \"\0",Inserted_Folder_Full_name,L"\" en la carpeta: \"\0",Full_name,L"\".\0" };
		SET_PRINT_DELETE_SINGLE_MERROR_ARRAY( MESSAGE_LOG, MESSAGE,5, messages)
	}*/


	Subfolders.count++;
	Folder** temp = new Folder*[Subfolders.count];
	if (Subfolders.count > 1){

		memcpy(temp, Subfolders.items, sizeof(Folder*) * (Subfolders.count - 1));
		delete[] Subfolders.items;

	}
	Subfolders.items = temp;
	Subfolders.items[Subfolders.count - 1] = Subfolder;
	Subfolder->set_parent(this);
	
	return TRUE;
}

BOOL Folder::insert_file(File* file, class m_error** p_error){ //0x0004

	class m_error** p_error_t = p_error; //0x0004
	 //0x0004
	//DWORD internal_error;
	wchar_t* InsertedFile_Full_name = NULL;
	//TCHAR Remote_New_File[MAX_PATH];

	errno_t err = 0;

	err = file->get_full_name(&InsertedFile_Full_name);
	if (err != 0) {

		SET_PRINT_DELETE_SINGLE_MERROR( FILE_ERRORS, (ERRNO_CODE_MASK | err), TEXT("Error obteniendo el nombre del archivo a insertar."))

		if (InsertedFile_Full_name != NULL) free(InsertedFile_Full_name);
		return FALSE;
	}
	/*else {

		LPTSTR messages[5] = { L"Insertando el archivo: \"\0",InsertedFile_Full_name,L"\" en la carpeta: \"\0",Full_name,L"\".\0" };
		SET_PRINT_DELETE_SINGLE_MERROR_ARRAY( MESSAGE_LOG, MESSAGE,5, messages)
	}*/
	
	
	Files.count++;										//count = 1				count = 2
	File** temp = new File*[Files.count];				//temp     []			temp		[][]
	if (Files.count > 1){

		memcpy(temp, Files.items, sizeof(File*) * (Files.count - 1));	//temp	[][] <= Files.items[0] * 1 = [0][]
		delete[] Files.items;
	}
	Files.items = temp;									//items = temp []	items = temp [0][]
	Files.items[Files.count - 1] = file;				//items[0] = file	items[0][file]
	file->set_parent(this);
	
	if (InsertedFile_Full_name != NULL) free(InsertedFile_Full_name);
	return TRUE;
}

/*File* Folder::search_file(LPTSTR file){

	unsigned int i;
	LPTSTR file_name;

	for (i = 0; i < Files.count; i++){

		file_name = (Files.items[i])->name();
		if (lstrcmpi(file_name, file) == 0) return Files.items[i];
	}
	return NULL;
}*/

Folder::~Folder(){

	/*
		Files.count = 3;	class File** Files.items = [(File*)0x000A{file1}] [(File*)0x000B{file2}] [(File*)0x000C{file3}]
	*/

	while (Files.count > 0){	//count = 3 > 0

//#ifdef _DEBUG
//	_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 79, TEXT("SyncFolder.exe"), TEXT("deleting file class: %S, index: %d from folder class: %S\r\n"),
//					(Files.items[Files.count - 1]->get_FileData()).cFileName,Files.count - 1,Full_name);
//#endif
		delete (Files.items[Files.count - 1]);	// Files.items[2] = 0x000C
		Files.count--;
	}
	delete[] Files.items;

	while (Subfolders.count > 0){

		delete (Subfolders.items[Subfolders.count - 1]);
		Subfolders.count--;
	}
	delete[] Subfolders.items;

	if (Path != NULL) free(Path);
	if (Full_name != NULL) free(Full_name);
	if (Full_name_long != NULL) free(Full_name_long);
}

errno_t Folder::get_name(TCHAR** Receiving_Name) {

	size_t totalsize = wcslen(FileData.cFileName) + 1;
	*Receiving_Name = (TCHAR*)calloc(totalsize, sizeof(wchar_t));
	return (wcscpy_s(*Receiving_Name, totalsize, FileData.cFileName));
}

errno_t Folder::get_path(wchar_t** Receiving_Path) {

	if (*Receiving_Path != NULL) free(*Receiving_Path);
	size_t totalsize = wcslen(Path) + 1;
	*Receiving_Path = (TCHAR*)calloc(totalsize, sizeof(wchar_t));
	return (wcscpy_s(*Receiving_Path, totalsize, Path));
}

Folder* Folder::compare_content(Folder* ToFolder, COMPARE_LEVELS files_levels, class m_error** p_error) {
	
	
	class m_error** p_error_t = p_error;

	class m_error* merror = NULL;
	Folder* return_value = new Folder(NULL, &merror);
	if (merror != NULL) ADD_MERROR(p_error_t, merror)

	return_value->FileData = ToFolder->get_FileData();

	wchar_t* tempo = NULL;
	ToFolder->get_path(&tempo);
	return_value->set_Path(tempo);
	delete tempo;
	tempo = NULL;

	ToFolder->get_full_name(&tempo);
	return_value->set_Full_Name(tempo);
	delete tempo;
	tempo = NULL;

	// return_value->set_Full_Name calls internally set_Full_Name_long(), so there is not need to call it here.

	// Mark the status of all the members of the folder as ghosts.

	for (UINT i = 0; i < Subfolders.count; i++) {

		Subfolders.items[i]->set_status(2);
	}
	for (UINT i = 0; i < Files.count; i++) {

		Files.items[i]->set_status(2);
	}

	BOOL folder_founded;

	for (UINT ToFolder_Subfolders_index = 0; ToFolder_Subfolders_index < ToFolder->get_subfolders_count(); ToFolder_Subfolders_index++) {

		folder_founded = FALSE;
		
		for (UINT i = 0; i < Subfolders.count; i++) {

			if (Subfolders.items[i]->get_status() == 2) {

				if (wcscmp((Subfolders.items[i]->get_FileData()).cFileName, ((ToFolder->get_subfolder(ToFolder_Subfolders_index))->get_FileData()).cFileName) == 0) {

					folder_founded = TRUE;
					class m_error* merror = NULL;
					Folder* Folder_tmp = Subfolders.items[i]->compare_content(ToFolder->get_subfolder(ToFolder_Subfolders_index), files_levels, &merror);
					if (merror != NULL) ADD_MERROR(p_error_t, merror)

					if (Folder_tmp != NULL) {	// This means that there were differences between the two subfolders.

						Subfolders.items[i]->set_status(1);	// The folder is dirty.
						class m_error* merror = NULL;
						return_value->insert_SubFolder(Folder_tmp, &merror);
						if (merror != NULL) ADD_MERROR(p_error_t, merror)
					}
					else Subfolders.items[i]->set_status(0);	// The folder is sync.
				}
			}
			if (folder_founded) break;
		}
		if (!folder_founded) {	// If the folder was not founded in the remote.

			class m_error* merror = NULL;
			LPTSTR tm = NULL;
			LPTSTR tm_long = NULL;
			(ToFolder->get_subfolder(ToFolder_Subfolders_index))->get_full_name(&tm);

			size_t totalsize = wcslen(tm) + 5;
			tm_long = (wchar_t*)calloc(totalsize, sizeof(wchar_t));
			wcscpy_s(tm_long, totalsize, L"\\\\?\\");
			wcscat_s(tm_long, totalsize, tm);

			Folder* Folder_tmp1 = new Folder(tm_long, return_value, FALSE, 0, &merror);
			Folder_tmp1->set_status(1);
			delete tm;
			delete tm_long;
			if (merror != NULL) ADD_MERROR(p_error_t, merror)
		}
	}

	BOOL file_founded;

	for (UINT ToFolder_Files_index = 0; ToFolder_Files_index < ToFolder->get_files_count(); ToFolder_Files_index++) {

		file_founded = FALSE;

		for (UINT i = 0; i < Files.count; i++) {

			if (Files.items[i]->get_status() == 2) {

				if (wcscmp((Files.items[i]->get_FileData()).cFileName, ((ToFolder->get_file(ToFolder_Files_index))->get_FileData()).cFileName) == 0) {

					file_founded = TRUE;
					class m_error* merror = NULL;
					INT16 sal = Files.items[i]->compare(ToFolder->get_file(ToFolder_Files_index), (~CONTENT & files_levels), 0, &merror);
					if (merror != NULL) ADD_MERROR(p_error_t, merror)
					if ((sal & (~CONTENT & files_levels)) == (~CONTENT & files_levels)) {	// if the files are equals, the status of the file is set to 0 (sync).
						Files.items[i]->set_status(0);
					}
					else {

						Files.items[i]->set_status(1);	// The file is dirty
						class m_error* merror = NULL;

						File* file_tmp = new File(ToFolder->get_file(ToFolder_Files_index));

						return_value->insert_file(file_tmp, &merror);
						file_tmp->set_status(1);
						if (merror != NULL) ADD_MERROR(p_error_t, merror)
					}
				}
			}
			if (file_founded) break;
		}
		
		if (!file_founded) {

			class m_error* merror = NULL;
			//LPTSTR tm = NULL;
			//(ToFolder->get_file(ToFolder_Files_index))->get_full_name(&tm);

			File* file_tmp = new File(ToFolder->get_file(ToFolder_Files_index));
			return_value->insert_file(file_tmp, &merror);
			file_tmp->set_status(1);
			if (merror != NULL) ADD_MERROR(p_error_t, merror)


			//File* File_tmp1 = new File(tm, return_value, FALSE, 0, &merror);
			//File_tmp1->set_status(1);

//#ifdef _DEBUG
//			_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 681, TEXT("SyncFolder.exe"), TEXT("status added: %d\r\n"),File_tmp1->get_status());
//#endif
//			delete tm;
//			if (merror != NULL) ADD_MERROR(p_error_t, merror)
		}
	}

//#ifdef _DEBUG
//	_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 552, TEXT("SyncFolder.exe"), TEXT("subfolders: %d, files: %d\r\n"), return_value->get_subfolders_count(),
//						return_value->get_files_count());
//#endif

	if ((return_value->get_subfolders_count() == 0) && (return_value->get_files_count() == 0)) {
		
		delete return_value;
		return_value = NULL;
	}
	else return_value->set_status(1);

	return return_value;
}

BOOLEAN Folder::synchronize(Folder* ToSyncFolder, class m_error** p_error) {

	class m_error** p_error_t = p_error;
	DWORD internal_error;

	for (UINT TSF_files_index = 0; TSF_files_index < ToSyncFolder->get_files_count(); TSF_files_index++) {

//#ifdef _DEBUG
//		_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 681, TEXT("SyncFolder.exe"), TEXT("status: %d\r\n"), (ToSyncFolder->get_file(TSF_files_index))->get_status());
//#endif

		size_t Remote_New_File_size = (wcslen(Full_name) + 2 + wcslen(((ToSyncFolder->get_file(TSF_files_index))->get_FileData()).cFileName));
		wchar_t* Remote_New_File = (wchar_t*)calloc(Remote_New_File_size,sizeof(wchar_t));

		wcscpy_s(Remote_New_File, Remote_New_File_size, Full_name);
		wcscat_s(Remote_New_File, Remote_New_File_size, L"\\");
		wcscat_s(Remote_New_File, Remote_New_File_size, ((ToSyncFolder->get_file(TSF_files_index))->get_FileData()).cFileName);

//#ifdef _DEBUG
//			_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 681, TEXT("SyncFolder.exe"), TEXT("remote file: %S\r\n"), Remote_New_File);
//#endif

		LPTSTR LocalNewFile = NULL;
		(ToSyncFolder->get_file(TSF_files_index))->get_full_name(&LocalNewFile);

//#ifdef _DEBUG
//			_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 681, TEXT("SyncFolder.exe"), TEXT("local file: %S\r\n"), LocalNewFile);
//#endif

		if (!CopyFile(LocalNewFile, Remote_New_File, FALSE)) {

			internal_error = GetLastError();
			SET_PRINT_DELETE_SINGLE_MERROR( FILE_ERRORS, internal_error, LocalNewFile)
		}
		else {

			LPTSTR messages[5] = { L"Salvado el archivo: \"\0",LocalNewFile,L"\" al archivo: \"\0",Remote_New_File,L"\".\0" };
			SET_PRINT_DELETE_SINGLE_MERROR_ARRAY( MESSAGE_LOG, MESSAGE, 5, messages)

			UINT nuevo = TRUE;
			for (UINT i = 0; i < Files.count; i++) {

				//if (Files.items[i]->get_status() == 1) {

					if (wcscmp((Files.items[i]->get_FileData()).cFileName, ((ToSyncFolder->get_file(TSF_files_index))->get_FileData()).cFileName) == 0) {

						Files.items[i]->set_status(0);
						WIN32_FIND_DATA FindDataTemp = (ToSyncFolder->get_file(TSF_files_index))->get_FileData();
						Files.items[i]->set_FileData(FindDataTemp);
						nuevo = FALSE;
						break;
					}
				//}
			}
			if (nuevo) {
				class m_error* merror = NULL;
				File* ff = new File(Remote_New_File, this, Files_hashed, hashing_algorithm, &merror);
				if (merror != NULL) ADD_MERROR(p_error_t, merror)
					ff->set_status(0);
			}
		}
		free(LocalNewFile);
		free(Remote_New_File);
	}

	/*	for (UINT i = 0; i < Files.count; i++) {

	if (Files.items[i]->get_status() == 1) {

	for (UINT TSF_files_index = 0; TSF_files_index < ToSyncFolder->get_files_count(); TSF_files_index++) {

	if ((ToSyncFolder->get_file(TSF_files_index))->get_status() == 1) {

	if (wcscmp((Files.items[i]->get_FileData()).cFileName, ((ToSyncFolder->get_file(TSF_files_index))->get_FileData()).cFileName) == 0) {

	Files.items[i]->set_status(0);
	(ToSyncFolder->get_file(TSF_files_index))->set_status(0);
	class m_error* merror = NULL;
	Files.items[i]->synchronize(ToSyncFolder->get_file(TSF_files_index), &merror);
	if (merror != NULL) ADD_MERROR(p_error, merror)
	break;
	}
	}
	}
	}
	}*/

	/*for (UINT i = 0; i < Subfolders.count; i++) {

		if (Subfolders.items[i]->get_status() == 1) {	// La subcarpeta está sucia.

			for (UINT TSF_Subfolder_index = 0; TSF_Subfolder_index < ToSyncFolder->get_subfolders_count(); TSF_Subfolder_index++) {

				if ((ToSyncFolder->get_subfolder(TSF_Subfolder_index))->get_status() == 1) {

					if (wcscmp((Subfolders.items[i]->get_FileData()).cFileName, ((ToSyncFolder->get_subfolder(TSF_Subfolder_index))->get_FileData()).cFileName) == 0) {

						Subfolders.items[i]->set_status(0);
						(ToSyncFolder->get_subfolder(TSF_Subfolder_index))->set_status(0);
						class m_error* merror = NULL;
						BOOL ba = Subfolders.items[i]->synchronize(ToSyncFolder->get_subfolder(TSF_Subfolder_index), &merror);
						if (merror != NULL) ADD_MERROR(p_error_t, merror)
						break;
					}
				}
			}
		}
	}*/

	for (UINT TSF_Subfolder_index = 0; TSF_Subfolder_index < ToSyncFolder->get_subfolders_count(); TSF_Subfolder_index++) {

		BOOLEAN founded = FALSE;
		for (UINT i = 0; i < Subfolders.count; i++) {

			if (wcscmp(((ToSyncFolder->get_subfolder(TSF_Subfolder_index))->get_FileData()).cFileName, (Subfolders.items[i]->get_FileData()).cFileName) == 0) {

				Subfolders.items[i]->set_status(0);
				(ToSyncFolder->get_subfolder(TSF_Subfolder_index))->set_status(0);
				class m_error* merror = NULL;
				BOOL ba = Subfolders.items[i]->synchronize(ToSyncFolder->get_subfolder(TSF_Subfolder_index), &merror);
				if (merror != NULL) ADD_MERROR(p_error_t, merror)
				founded = TRUE;
				break;
			}
		}
		if (!founded) {

			class m_error* merror = NULL;
			copy_folder(ToSyncFolder->get_subfolder(TSF_Subfolder_index),this, &merror);
			if (merror != NULL) ADD_MERROR(p_error_t, merror)
		}
	}

	
	return TRUE;
}

Folder* Folder::copy_folder(Folder* Origen, Folder* _Parent, class m_error** p_error) {

	class m_error** p_error_t = p_error;
	DWORD internal_error;

	size_t size_path = wcslen(Full_name) + 2;
	LPTSTR new_folder_path = (wchar_t*)calloc(size_path, sizeof(wchar_t));
	wcscpy_s(new_folder_path, size_path,Full_name);
	wcscat_s(new_folder_path,size_path, L"\\");

	size_t name_size = (size_path - 1) + wcslen((Origen->get_FileData()).cFileName) + 1;
	LPTSTR new_folder_name = (wchar_t*)calloc(name_size, sizeof(wchar_t));
	wcscpy_s(new_folder_name,name_size, new_folder_path);
	wcscat_s(new_folder_name,name_size, (Origen->get_FileData()).cFileName);

//#ifdef _DEBUG
//	_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 681, TEXT("SyncFolder.exe"), TEXT("receiver folder name: %S\r\n"), Full_name);
//	_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 681, TEXT("SyncFolder.exe"), TEXT("emiter folder name: %S\r\n"), Origen->Full_name);
//#endif

	Folder* ff = NULL;
	
	if (CreateDirectory(new_folder_name, NULL)) {

		LPTSTR messages[3] = { L"Creado el directorio: \"\0",new_folder_name,L"\".\0" };
		SET_PRINT_DELETE_SINGLE_MERROR_ARRAY( MESSAGE_LOG, MESSAGE, 3, messages)
		
		class m_error* merror = NULL;
		ff = new Folder(_Parent, &merror);
		if (merror != NULL) ADD_MERROR(p_error_t, merror)

		ff->set_FileData(Origen->get_FileData());
		ff->set_Path(new_folder_path);
//#ifdef _DEBUG
//		_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 552, TEXT("SyncFolder.exe"), TEXT("New Folder Path: %S\r\n"), ff->Path);
//#endif
		ff->set_Full_Name(new_folder_name);
//#ifdef _DEBUG
//		_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 552, TEXT("SyncFolder.exe"), TEXT("New Folder Name: %S\r\n"), ff->Full_name);
//#endif
		ff->set_Files_hashed(Origen->get_files_hashed());
		ff->set_hashing_algorithm(Origen->get_hashing_algorithm());
		ff->set_status(0);

//#ifdef _DEBUG
//		_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 552, TEXT("SyncFolder.exe"), TEXT("Origen->files_count: %d\r\n"), Origen->get_files_count());
//#endif
		for (UINT i = 0; i < Origen->get_files_count(); i++) {

			size_t new_file_path_size = name_size + 1;
//#ifdef _DEBUG
//			_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 552, TEXT("SyncFolder.exe"), TEXT("new_file_path_size: %d\r\n"), new_file_path_size);
//#endif
			LPTSTR new_file_path = (wchar_t*)calloc(new_file_path_size, sizeof(wchar_t));
			wcscpy_s(new_file_path,new_file_path_size, new_folder_name);
//#ifdef _DEBUG
//			_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 552, TEXT("SyncFolder.exe"), TEXT("New File Path: %S\r\n"), new_file_path);
//#endif
			wcscat_s(new_file_path, new_file_path_size, L"\\");
//#ifdef _DEBUG
//			_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 552, TEXT("SyncFolder.exe"), TEXT("New File Path: %S\r\n"), new_file_path);
//#endif
			
			size_t file_size = name_size + wcslen(((Origen->get_file(i))->get_FileData()).cFileName) + 1;
			LPTSTR NewFile = (wchar_t*)calloc(file_size, sizeof(wchar_t));
			wcscpy_s(NewFile, file_size, new_file_path);
			wcscat_s(NewFile, file_size, ((Origen->get_file(i))->get_FileData()).cFileName);
//#ifdef _DEBUG
//			_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 552, TEXT("SyncFolder.exe"), TEXT("New File Name: %S\r\n"), NewFile);
//#endif

			LPTSTR Existing_File = NULL;
			(Origen->get_file(i))->get_full_name(&Existing_File);

//#ifdef _DEBUG
//			_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 552, TEXT("SyncFolder.exe"), TEXT("Existing_File: %S\r\n"), Existing_File);
//#endif

			if (CopyFile(Existing_File,NewFile, FALSE)) {

//#ifdef _DEBUG
//				_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 681, TEXT("SyncFolder.exe"), TEXT("file: %S\r\n"), NewFile);
//#endif
				LPTSTR messages[5] = { L"Salvado el archivo: \"\0",Existing_File,L"\" al archivo: \"\0",NewFile,L"\".\0" };
				SET_PRINT_DELETE_SINGLE_MERROR_ARRAY( MESSAGE_LOG, MESSAGE, 5, messages)
				
				File* new_ff = new File(Origen->get_file(i));
				new_ff->set_Path(new_file_path);
				new_ff->set_Full_name(NewFile);

				class m_error* merror = NULL;
				ff->insert_file(new_ff, &merror);
				if (merror != NULL) ADD_MERROR(p_error_t, merror)
			}
			else {

				internal_error = GetLastError();
				SET_PRINT_DELETE_SINGLE_MERROR( FILE_ERRORS, internal_error, NewFile)
			}
			free(NewFile);
			free(new_file_path);
		}

//#ifdef _DEBUG
//		_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 681, TEXT("SyncFolder.exe"), TEXT("subfolders count: %d\r\n"), Origen->get_subfolders_count());
//#endif

		for (UINT i = 0; i < Origen->get_subfolders_count(); i++) {

			class m_error* merror = NULL;
			ff->copy_folder((Origen->get_subfolder(i)), ff, &merror);
			if (merror != NULL) ADD_MERROR(p_error_t, merror)
		}

	}
	else {

		internal_error = GetLastError();
		SET_PRINT_DELETE_SINGLE_MERROR( FILE_ERRORS, internal_error, new_folder_name)
	}

	free(new_folder_name);
	free(new_folder_path);
	return ff;
}

//INT16 Folder::compare(LPTSTR ToFolder, COMPARE_LEVELS Level, COMPARE_LEVELS sync, COMPARE_LEVELS files_levels, COMPARE_LEVELS files_sync, class m_error** p_error) {
//
//	/*	This function return two possible values:
//
//			-1 - There was an error that made imposible to do the comparison.
//			or a bit mask composed of the COMPARE_LEVELS that match both folders.
//
//			Example: If the compare levels dictated that the comparison was on SIZE (10000), WTIME (01000) and  NAME (01) and the result
//					was that the folders was equals in NAME and WTIME but not in SIZE the return value will be: 01001
//
//		sync determines on wich compare level matches the folder autosynchronize.
//	*/
//
//	/*************** Variables *************************/
//
//	class m_error** p_error_t = p_error;	//0x0001[NULL]
//		//0x0001[NULL]
//	DWORD internal_error;
//	INT16 return_value = 0;
//	BOOL real_sync = FALSE;
//
//
//	//WIN32_FILE_ATTRIBUTE_DATA ToFolder_Attributes;
//	WIN32_FIND_DATA ffd;
//	TCHAR szDir[MAX_PATH];
//	HANDLE hFind = INVALID_HANDLE_VALUE;
//
//
//
//	/************** Body of the function **************/
//
//	// Check that ToFolder input variable does not exceed MAX_PATH - 3;
//
//	if FAILED(StringCchLength(ToFolder, (MAX_PATH - 3), NULL)) {
//
//		SET_PRINT_DELETE_SINGLE_MERROR( FOLDER_ERRORS, FILENAME_TOO_LONG, ToFolder)
//		
//		return -1;
//	}
//
//	LPTSTR p_tmp = wcsrchr(ToFolder, '\\');	// Search for the last \ in the string
//
//	if (p_tmp == NULL) {	// There were not '\' characters in the string, so the ToFolder parameter was wrong.
//
//		SET_PRINT_DELETE_SINGLE_MERROR( FOLDER_ERRORS, INVALID_FUNCTION_PARAM, ToFolder)
//		return -1;
//	}
//	p_tmp++;	// Move the pointer to the caracter more to the right of the last \ (i.e: the begining of the name of the folder without it path).
//
//	// Compare the names of the folders, if that was requested
//
//	if ((Level & NAME) == NAME) {	// If there was requested name comparison.
//
//		//LPTSTR messages[5] = { L"Comparando el nombre de la carpeta: \"\0",FileData.cFileName,L"\" contra el de la carpeta: \"\0",p_tmp, L"\".\0" };
//		//SET_PRINT_DELETE_SINGLE_MERROR_ARRAY( MESSAGE_LOG, MESSAGE, 5, messages)
//
//		if (wcscmp(FileData.cFileName, p_tmp) == 0) {
//
//			//SET_PRINT_DELETE_SINGLE_MERROR( MESSAGE_LOG, MESSAGE, L"Nombres coincidentes\0")
//			return_value = (return_value | NAME); // Both folder has the same name.
//		}
//		else {
//
//			//SET_PRINT_DELETE_SINGLE_MERROR( MESSAGE_LOG, MESSAGE, L"Nombres no coincidentes\0")
//				if ((sync & NAME) == NAME) real_sync = TRUE;
//		}
//	}
//
//	if ((Level & CONTENT) == CONTENT) {
//
//		// Compare the contents of the two folders.
//
//		//LPTSTR messages[5] = { L"Comparando el contenido de la carpeta: \"\0",FileData.cFileName,L"\" contra el de la carpeta: \"\0",p_tmp,L"\".\0" };
//		//SET_PRINT_DELETE_SINGLE_MERROR_ARRAY( MESSAGE_LOG, MESSAGE, 5, messages)
//
//		// Mark the status of all the members of the folder as ghosts.
//
//		for (UINT i = 0; i < Subfolders.count; i++) {
//
//			Subfolders.items[i]->set_status(2);
//		}
//		for (UINT i = 0; i < Files.count; i++) {
//
//			Files.items[i]->set_status(2);
//		}
//
//		// Prepare string for use with FindFile functions.  First, copy the
//		// string to a buffer, then append '\*' to the directory name.
//
//		StringCchCopy(szDir, MAX_PATH, ToFolder);
//		StringCchCat(szDir, MAX_PATH, TEXT("\\*"));
////#ifdef _DEBUG
////		_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 594, TEXT("SyncFolder.exe"), TEXT("%S\r\n"), szDir);
////#endif
//		// Find the first file in the directory.
//
//		hFind = FindFirstFile(szDir, &ffd);
//
//		if (hFind == INVALID_HANDLE_VALUE) {
//
//			internal_error = GetLastError();
//
//			// ERROR_FILE_NOT_FOUND is not really an error in this context because it just means that inside the folder there were not files. 
//			// Any other error is reported if that funcionallity has been requested.
//
//			if (internal_error != ERROR_FILE_NOT_FOUND) {
//
//				SET_PRINT_DELETE_SINGLE_MERROR( UNDETERMINED_ERROR_DOMAIN, internal_error, szDir)
//				
//				return -1;	// There was an error listing the "ToFolder" folder, so it's not possible to compare it with this class.
//			}
//		}
//		else {	// The search returned a handle, so there is content in the Folder
//
//			do {
//
//				// Compose the full name of the file/subfolder to compare.
//
//				size_t total_size;
//				total_size = wcslen(ToFolder) + wcslen(ffd.cFileName) + 2;
//				LPTSTR FullName = (wchar_t*)calloc(total_size, sizeof(wchar_t));
//				wcscat_s(FullName, total_size, ToFolder);
//				wcscat_s(FullName, total_size, L"\\");
//				wcscat_s(FullName, total_size, ffd.cFileName);
//
////#ifdef _DEBUG
////				_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 629, TEXT("SyncFolder.exe"), TEXT("Elemento listado: %S\r\n"), FullName);
////#endif
//
//				if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {	// This is a subfolder
//
//					if ((lstrcmp(ffd.cFileName, TEXT(".")) != 0) && (lstrcmp(ffd.cFileName, TEXT("..")) != 0)) { // If this subfolder is not . or ..
//
//						/*class m_error* merror = NULL;
//						class Folder* temp_folder = new Folder(FullName, NULL, Files_hashed, (UINT)0, &merror);
//						if (merror != NULL) ADD_MERROR(p_error_t, merror)*/
//
//						BOOL folder_founded = FALSE;
//						for (UINT i = 0; i < Subfolders.count; i++) {
//
//							if (Subfolders.items[i]->get_status() == 2) {
//
//								class m_error* merror = NULL;
//								// Compare only by name of the folder.
//								INT16 sal = Subfolders.items[i]->compare(FullName, NAME, 0, 0, 0,&merror);
//								if (merror != NULL) ADD_MERROR(p_error_t, merror)
//
//								if ((sal != -1) && (sal & NAME) == NAME) { // If there was not error and both folders have the same name
//
//									folder_founded = TRUE;
//
//									class m_error* merror = NULL;
//									// 
//									INT16 sal = Subfolders.items[i]->compare(FullName,CONTENT, sync, files_levels,files_sync, &merror);
//									if (merror != NULL) ADD_MERROR(p_error_t, merror)
//
//									if (sal != -1) {
//
//										if ((sal & (~CONTENT & files_levels)) == (~CONTENT & files_levels)) {	// if the files are the same (because they were or
//																												// because they were synchred) the status of the
//																												// file is set to 0 (sync).
//											Files.items[i]->set_status(0);
//										}
//									}
//									else Files.items[i]->set_status(1);
//									break;
//								}
//							}
//						}
//					}
//				}
//				else {	// The file is a regular file.
//
//					class m_error* merror = NULL;
//
////#ifdef _DEBUG
////					_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 622, TEXT("SyncFolder.exe"), TEXT("merror before new File: %p[%p]\r\n"), &merror, merror);
////#endif
//
//					class File* tmp_file = new File(FullName, NULL, Files_hashed, (UINT)0, &merror);
//
////#ifdef _DEBUG
////					_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 622, TEXT("SyncFolder.exe"), TEXT("merror after new File: %p[%p]\r\n"), &merror,merror);
////#endif
//
//					if (merror != NULL) ADD_MERROR(p_error_t, merror)
//
//
////#ifdef _DEBUG
////					TCHAR created_file[MAX_PATH];
////					tmp_file->get_full_name(created_file);
////					_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 642, TEXT("SyncFolder.exe"), TEXT("Full name de la clase tmp_file creada: %S\r\n"), created_file);
////#endif
//
//					BOOL file_founded = FALSE;
//					// Check if the tmp_file was correctly created (from the error returned), if not clean and do not make the comparison.
//
////#ifdef _DEBUG
////					_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 682, TEXT("SyncFolder.exe"),
////						TEXT("cantidad de archivos en el arreglo: %d\r\n"), Files.count);
////#endif
//
//					for (UINT i = 0; i < Files.count; i++) {
//
////#ifdef _DEBUG
////						TCHAR file_compared[MAX_PATH];
////						Files.items[i]->get_full_name(file_compared);
////						_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 653, TEXT("SyncFolder.exe"),
////							TEXT("status del archivo \"%S\" antes de la comparación: %d\r\n"), file_compared, Files.items[i]->get_status());
////#endif
//
//						if (Files.items[i]->get_status() == 2) {	// If the file is still marked as a ghost.
//
//							// Compare the files on the NAME Level
//							class m_error* merror = NULL;
//							INT16 sal = Files.items[i]->compare(tmp_file, NAME, 0, &merror);
//							if (merror != NULL) ADD_MERROR(p_error_t, merror)
//
//							if ((sal != -1) && (sal & NAME) == NAME) {	// If there was not error and both files have the same name, it's supposed that the comparison
//																		// at NAME level without sync will not yield an error because all the parameters involved are
//																		// validated.
//
//								file_founded = TRUE;
//
//								// Make the comparison/sync at levels requested, but without the CONTENT level because that's
//								// not implemented yet.
//								class m_error* merror = NULL;
//								INT16 sal = Files.items[i]->compare(tmp_file, (~CONTENT & files_levels), (~CONTENT & files_sync),&merror);
//								if (merror != NULL) ADD_MERROR(p_error_t, merror)
//									
//								if (sal != -1) {
//
//									if ((sal & (~CONTENT & files_levels)) == (~CONTENT & files_levels)) {	// if the files are the same (because they were, or
//																											// because they were synchred) the status of the
//																											// file is set to 0 (sync).
//										Files.items[i]->set_status(0);
//									}
//									else Files.items[i]->set_status(1);	// If the files were different then
//								}
//								else Files.items[i]->set_status(1);
//								break;
//							}
//						}
//					}
//					if (!file_founded) {	// If the file was not found in the Folder class is a new file, so it must be insert it.
//
//						if ((sync & CONTENT) == CONTENT) { // if it was requested CONTENT synchronization
//
//							LPTSTR messages[5] = { L"Copiando el archivo: \"\0",FullName,L"\" en la carpeta: \"\0",Full_name,L"\".\0" };
//							SET_PRINT_DELETE_SINGLE_MERROR_ARRAY( MESSAGE_LOG, MESSAGE, 5, messages)
//							
//							TCHAR Remote_New_File[MAX_PATH];
//
//							wcscpy_s(Remote_New_File, MAX_PATH, Full_name);
//							wcscat_s(Remote_New_File, MAX_PATH, L"\\");
//							wcscat_s(Remote_New_File, MAX_PATH, ffd.cFileName);
//
////#ifdef _DEBUG
////							_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 629, TEXT("SyncFolder.exe"), TEXT("Remote_New_File: %S\r\n"), Remote_New_File);
////#endif
//							if (!CopyFile(FullName, Remote_New_File, FALSE)) {
//
//								internal_error = GetLastError();
//								SET_PRINT_DELETE_SINGLE_MERROR( FILE_ERRORS, internal_error, FullName)
//								//
//								// return_value = -1; // ESto está por verse.
//							}
//							else {	// The file was copied, now it must be inserted in the Folder class.
//
//								LPTSTR messages[5] = { L"Archivo: \"\0",FullName,L"\" copiado en la carpeta: \"\0",Full_name,L"\".\0" };
//								SET_PRINT_DELETE_SINGLE_MERROR_ARRAY( MESSAGE_LOG, MESSAGE, 5, messages)
//
//								class m_error* merror = NULL;
//								File* ff = new File(Remote_New_File, this, Files_hashed, hashing_algorithm, &merror);
//								if (merror != NULL) ADD_MERROR(p_error_t, merror)
//							}
//						}
//					}
//					delete tmp_file;
//				}
//			} while (FindNextFile(hFind, &ffd) != 0);
//
//			internal_error = GetLastError();
//
//			// ERROR_NO_MORE_FILES is not really an error in this context, it just means that the directory listing process has finish.
//
//			if (internal_error != ERROR_NO_MORE_FILES) {
//
//				SET_PRINT_DELETE_SINGLE_MERROR( UNDETERMINED_ERROR_DOMAIN, internal_error, ToFolder)
//			}
//
//			FindClose(hFind);
//		}
//
//		// At this point all the files/subfolders contained in the "ToFolder" folder that match by name a file/subfolder in the Folder class has been
//		// compared an syncronized (if this action was requested), so the files/subfolders that remains in status == 2 (ghosts) does not exist in the
//		// "ToFolder" folder, so they should be removed.
//		
//		if ((sync & CONTENT) == CONTENT) {
//
//			for (UINT i = 0; i < Subfolders.count; i++) {
//
//				if (Subfolders.items[i]->get_status() == 2) {
//
//					class m_error* merror = NULL;
//					remove_SubFolder(Subfolders.items[i], TRUE,&merror);
//					if (merror != NULL) ADD_MERROR(p_error_t, merror)
//				}
//			}
//
////#ifdef _DEBUG
////			_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 653, TEXT("SyncFolder.exe"),
////				TEXT("Cantidad de archivos en el arreglo Files.items antes de la limpieza: %d\r\n"), Files.count);
////#endif
//
///*
//			for (UINT i = 0; i < Files.count; i++) {
//
//#ifdef _DEBUG
//				TCHAR file_compared1[MAX_PATH];
//				Files.items[i]->get_full_name(file_compared1);
//				_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 653, TEXT("SyncFolder.exe"),
//					TEXT("status del archivo \"%S\" antes de ser removido: %d\r\n"), file_compared1, Files.items[i]->get_status());
//#endif
//
//				if (Files.items[i]->get_status() == 2) {
//
//					//p_a_eliminar[i] = i;
//					remove_File(Files.items[i], TRUE, temp_error, &temp_error);
//				}
//			}
//			//remove_File_group(p_a_eliminar, TRUE, temp_error, &temp_error);
//*/
//
//			// [0] [2] [0] count = 3;
//			UINT a = 0;
//			while (a < Files.count) {	// a = 0 < 3; a = 1 < 3; a = 1 < 2
//
////#ifdef _DEBUG
////				//TCHAR file_compared1[MAX_PATH];
////				//Files.items[a]->get_full_name(file_compared1);
////				//_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 653, TEXT("SyncFolder.exe"),
////				//	TEXT("status del archivo \"%S\" antes de ser removido: %d\r\n"), file_compared1, Files.items[a]->get_status());
////
////				_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 653, TEXT("SyncFolder.exe"),TEXT("antes a: %d -- Files.count: %d\r\n"),a,Files.count);
////#endif
//				if (Files.items[a]->get_status() == 2) {	// 0 != 2 ; 2 == 2 ; 0 != 2
//
////#ifdef _DEBUG
////
////					_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 653, TEXT("SyncFolder.exe"), TEXT("%d\r\n"), 25);
////#endif
//
//					class m_error* merror = NULL;
//					BOOL band = remove_File(a, TRUE,&merror);
//					if (merror != NULL) ADD_MERROR(p_error_t, merror)
//
//					if (!band) a++; // rem a = 1 -> [0][0] Files.count = 2
//					//else a--;
//				}
//				else a++;	// a++ = 1; // a++ = 2
//
////#ifdef _DEBUG
////				_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 653, TEXT("SyncFolder.exe"), TEXT("despues a: %d -- Files.count: %d\r\n"), a, Files.count);
////#endif
//			}
//
//			/*int a = 0;
//			for (UINT i = 0; i < Files.count; i++) {
//			
//				if (Files.items[i]->get_status() == 2) {
//
//					if (Files.items[i]->delete_from_system(temp_error, &temp_error)) {
//
//						delete Files.items[i];
//						Files.items[i] = NULL;
//						a++;
//					}
//				}
//			}
//			if ((Files.count - a) > 0) {
//
//				UINT c = 0;
//				File** tmp = new File*[Files.count - a];
//				for (UINT b = 0; b < Files.count; b++) {
//
//					if (Files.items[b] != NULL) {
//
//						tmp[c] = Files.items[b];
//						c++;
//					}
//				}
//				delete Files.items;
//				Files.items = tmp;
//				Files.count = Files.count - a;
//			}
//			else {
//				Files.count = 0;
//				Files.items = NULL;
//			}*/
//		}
//	}
//	
//	return  return_value;
//}

UINT Folder::get_status(void) {

	return status;
}

void Folder::set_status(UINT _status) {

	status = _status;
	return;
}

errno_t Folder::get_full_name(wchar_t** Receiving_FullName) {

	if (*Receiving_FullName != NULL) free(*Receiving_FullName);
	size_t totalsize = wcslen(Full_name) + 1;
	*Receiving_FullName = (TCHAR*)calloc(totalsize, sizeof(wchar_t));
	return (wcscpy_s(*Receiving_FullName, totalsize, Full_name));
}

BOOL Folder::delete_from_system(class m_error** p_error) {

	class m_error** p_error_t = p_error;
	
	BOOL content_deleted = TRUE;

	DWORD internal_error;

	for (UINT i = 0; i < Files.count; i++) {

		//if (!(remove_File(Files.items[i], TRUE, temp_error, &temp_error))) content_deleted = FALSE;

	}

	for (UINT i = 0; i < Subfolders.count; i++) {

		class m_error* merror = NULL;
		BOOL band = remove_SubFolder(Subfolders.items[i], TRUE, &merror);
		if (merror != NULL) ADD_MERROR(p_error_t, merror)
		
		if (!band) content_deleted = FALSE;
	}

	if (content_deleted) {

		if (!(RemoveDirectory(Full_name))) {

			internal_error = GetLastError();
			SET_PRINT_DELETE_SINGLE_MERROR( FILE_ERRORS, internal_error, Full_name)
			
			return FALSE;
		}
		else {

			LPTSTR messages[3] = { L"Eliminado el directorio: \"\0",Full_name,L"\" del sistema.\0"};
			SET_PRINT_DELETE_SINGLE_MERROR_ARRAY( MESSAGE_LOG, MESSAGE, 3, messages)
		}
		
		return TRUE;
	}
	
	return FALSE;
}

BOOL Folder::remove_SubFolder(Folder* Subfolder, BOOL from_system, class m_error** p_error) {

	class m_error** p_error_t = p_error;
	
	BOOL remove = TRUE;

	TCHAR* SubFolder_Full_name;
	errno_t err = 0;

	err = Subfolder->get_full_name(&SubFolder_Full_name);

	if (err != 0) {

		SET_PRINT_DELETE_SINGLE_MERROR( FILE_ERRORS, (ERRNO_CODE_MASK | err), L"Error obteniendo el nombre de la carpeta a eliminar.\0")
		
		return FALSE;
	}
	else {

		LPTSTR messages[3] = { L"Eliminando la carpeta: \"\0",SubFolder_Full_name,L"\".\0" };
		SET_PRINT_DELETE_SINGLE_MERROR_ARRAY( MESSAGE_LOG, MESSAGE, 3, messages)
	}
	
	
	// Remove the files inside the subfolder

	if (from_system) {

		class m_error* merror = NULL;
		BOOL band = Subfolder->delete_from_system(p_error_t);
		if (merror != NULL) ADD_MERROR(p_error_t, merror)
		
		if (!band) remove = FALSE;
	}
	if (remove && (Subfolders.count > 0)) {

		Folder** temp = new Folder*[Subfolders.count - 1];
		for (UINT i = 0; i < Subfolders.count; i++) {

			if (Subfolders.items[i] != Subfolder) temp[i] = Subfolders.items[i];
		}
		delete Subfolder;
		Subfolders.count--;
		delete Subfolders.items;
		Subfolders.items = temp;
	}
	
	return TRUE;
}

BOOL Folder::remove_File(UINT file_index, BOOL from_system, class m_error** p_error) {	// a = 1

	/*	The primary objective of this function is to remove the file from the Folder class, and if the parameter "from_system" is true, previously to that
		action it remove the file from the system.

		If "from_system" is true and the file could not be removed from the system, the file class is not removed from the folder class and the return is FALSE.
		This is so because the Folder class must be a mirrow of the real folder in the file system, and if in it there is file it representation must be
		in the folder class.
		If "from_system" is false this function remove the file class from the folder and return the result of this action.
	*/

	class m_error** p_error_t = p_error;
	
	BOOL remove = FALSE;

	wchar_t* File_Full_name = NULL;
	errno_t err = 0;

	err = Files.items[file_index]->get_full_name(&File_Full_name);

	if (err != 0) {

		SET_PRINT_DELETE_SINGLE_MERROR( FILE_ERRORS, (ERRNO_CODE_MASK | err), L"Error obteniendo el nombre del archivo a eliminar.\0")

		if (File_Full_name != NULL) free(File_Full_name);
		return FALSE;
	}
	else {

		LPTSTR messages[3] = { L"Eliminando el archivo: \"\0",File_Full_name,L"\".\0" };
		SET_PRINT_DELETE_SINGLE_MERROR_ARRAY( MESSAGE_LOG, MESSAGE, 3, messages)
	}
	
	
	/*if (from_system) {
		
		if (!(file->delete_from_system(temp_error, &temp_error))) remove = FALSE;	// If the file could not be removed from the file system cancel the remove action
																					// from the folder class
		//file->delete_from_system(temp_error, &temp_error);
	}
	if (remove && (Files.count > 0)) {	//Files.count = 3

		File** temp = new File*[Files.count - 1];	//temp = [][]
		for (UINT i = 0; i < Files.count; i++) {	// i = 0 i<3 ; i = 1 i < 3 ; i = 2 i < 3 ; i = 3 i == 3

			if (Files.items[i] != file) temp[i] = Files.items[i]; // 0 != 1 -> [0]; 1 = 1 -> [0] ; 2 != 1 -> [0][0]
		}
		delete file;	// delete 1
		Files.count--;	// Files.count = 2
		delete Files.items;
		Files.items = temp;
		
		return TRUE;
	}*/

//#ifdef _DEBUG
//	_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 653, TEXT("SyncFolder.exe"), TEXT("Entrando a remove_File Files.count: %d\r\n"),Files.count);
//#endif
	class m_error* merror = NULL;
	BOOL band = Files.items[file_index]->delete_from_system(&merror);
	if (merror != NULL) ADD_MERROR(p_error_t, merror)
	
	if (band) {

		delete Files.items[file_index];
		Files.items[file_index] = NULL;
		remove = TRUE;
	}

	if (remove) {

//#ifdef _DEBUG
//		_CrtDbgReportW(_CRT_WARN, TEXT("Folder.cpp"), 653, TEXT("SyncFolder.exe"), TEXT("Entrando a remove Files.count: %d\r\n"), Files.count);
//#endif
		if (Files.count > 1) {

			UINT c = 0;
			File** tmp = new File*[Files.count - 1];
			for (UINT b = 0; b < Files.count; b++) {

				if (Files.items[b] != NULL) {

					tmp[c] = Files.items[b];
					c++;
				}
			}
			delete Files.items;
			Files.items = tmp;
			Files.count--;
		}
		else {
			delete Files.items;
			Files.count = 0;
			Files.items = NULL;
		}
		if (File_Full_name != NULL) free(File_Full_name);
		return TRUE;
	}
	if (File_Full_name != NULL) free(File_Full_name);
	return FALSE;
}

UINT Folder::get_files_count() {

	return Files.count;
}

UINT Folder::get_subfolders_count() {

	return Subfolders.count;
}

Folder* Folder::get_subfolder(UINT index) {

	return Subfolders.items[index];
}

File* Folder::get_file(UINT index) {

	return Files.items[index];
}

WIN32_FIND_DATA Folder::get_FileData(void) {

	return FileData;
}

void Folder::set_parent(Folder* _Parent) {

	Parent = _Parent;
}

void Folder::set_FileData(WIN32_FIND_DATA _FileData) {

	memcpy(&FileData, &_FileData,sizeof(WIN32_FIND_DATA));
	return;
}

void Folder::set_Path(wchar_t* _Path) {

	if (Path != NULL) free(Path);
	size_t totalsize = wcslen(_Path) + 1;
	Path = (wchar_t*)calloc(totalsize, sizeof(wchar_t));
	wcscpy_s(Path, totalsize, _Path);
}

void Folder::set_Full_Name(wchar_t* _Full_Name) {

	if (Full_name != NULL) free(Full_name);
	size_t totalsize = wcslen(_Full_Name) + 1;
	Full_name = (wchar_t*)calloc(totalsize, sizeof(wchar_t));
	wcscpy_s(Full_name, totalsize, _Full_Name);
	set_Full_Name_long();
	return;
}

void Folder::set_Full_Name_long() {

	size_t totalsize = wcslen(Full_name) + 5;
	Full_name_long = (wchar_t*)calloc(totalsize, sizeof(wchar_t));
	wcscpy_s(Full_name_long, totalsize, L"\\\\?\\");
	wcscat_s(Full_name_long, totalsize, Full_name);
	return;
}

void Folder::set_Files_hashed(BOOLEAN _Files_hashed) {

	Files_hashed = _Files_hashed;
	return;
}

void Folder::set_hashing_algorithm(UINT _hashing_algorithm) {

	hashing_algorithm = _hashing_algorithm;
}

Folder* Folder::get_Parent() {

	return Parent;
}

BOOL Folder::get_files_hashed() {

	return Files_hashed;
}

BOOL Folder::get_hashing_algorithm() {

	return hashing_algorithm;
}