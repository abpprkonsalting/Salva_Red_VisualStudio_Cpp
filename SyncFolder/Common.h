#pragma once

#ifndef STRICT
#define STRICT
#endif // !STRICT

#include <windows.h>
#include <tchar.h> 
#include <stdio.h>
#include <strsafe.h>
#include <Shlwapi.h>
#include <crtdbg.h>
#include <errno.h>
#include <Shellapi.h>

#include "file_error.h"

/*
The following are the two related struct used to store the information this object needs to represent a folder. The first is the one used in the search functions
of the constructors, the second one is used in the GetFileAttributesEx function that will be used in the compare function. The second does not include a field for
the name (as weird as it could seems), so the first is the one to be used.

typedef struct _WIN32_FIND_DATA {
DWORD    dwFileAttributes;
FILETIME ftCreationTime;
FILETIME ftLastAccessTime;
FILETIME ftLastWriteTime;
DWORD    nFileSizeHigh;
DWORD    nFileSizeLow;
DWORD    dwReserved0;
DWORD    dwReserved1;
TCHAR    cFileName[MAX_PATH];
TCHAR    cAlternateFileName[14];
} WIN32_FIND_DATA, *PWIN32_FIND_DATA, *LPWIN32_FIND_DATA;

typedef struct _WIN32_FILE_ATTRIBUTE_DATA {
DWORD    dwFileAttributes;
FILETIME ftCreationTime;
FILETIME ftLastAccessTime;
FILETIME ftLastWriteTime;
DWORD    nFileSizeHigh;
DWORD    nFileSizeLow;
} WIN32_FILE_ATTRIBUTE_DATA, *LPWIN32_FILE_ATTRIBUTE_DATA;

*/


/*typedef enum _COMPARE_LEVELS {
	Name,									//	Compare just the names.
	Name_Attributes,						//	Plus the attributes of the files.
	Name_Attributes_Access_Time,			//	Plus the LastAccessTime.
	Name_Attributes_Access_Write_Time,		//	Plus the LastWriteTime.
	Name_Attributes_Times_Size,				//	Plus the size of the file.
	Name_Attributes_Times_Size_Hash,		//	Plus the hash of the file.
	Name_Attributes_Times_Size_Hash_Binary	//	Plus a comparison byte by byte.
} COMPARE_LEVELS;*/

/*struct _COMPARE_LEVELS {

	UINT16	RESERVED : 9,
			CONTENT : 1,
			HASH : 1,
			SIZE : 1,
			WTIME : 1,	// Last Write time.
			ATIME : 1,	// Last Access time.
			ATTR : 1,	// Attributes.
			NAME : 1;

} COMPARE_LEVELS;*/

#ifndef COMPARE_LEVELS
#define COMPARE_LEVELS UINT16

#ifndef CONTENT
#define CONTENT 0x40
#endif

#ifndef HASH
#define HASH 0x20
#endif

#ifndef SIZE
#define SIZE 0x10
#endif

#ifndef WTIME
#define WTIME 0x8
#endif

#ifndef ATIME
#define ATIME 0x4
#endif

#ifndef ATTR
#define ATTR 0x2
#endif

#ifndef NAME
#define NAME 0x1
#endif

#endif

struct file_array {

	UINT count;
	class File** items;
};

struct folder_array {

	UINT count;
	class Folder** items;
};

class File {

/************************************************* Properties *********************************************************/

private:

	WIN32_FIND_DATA		FileData;
	BOOLEAN				is_hashed;			// Determine if the file was hashed.
	LARGE_INTEGER		HashValue;			// The type of this variable is temporal, it is not clear that all posible kind of hash returned by the different hashing algorithms
											// will fit in a LARGE_INTEGER. Any way I think I must experiment with a UNION datatype.
	unsigned int		hashing_algorithm;	// The algorithm used to calculate the hash.

	wchar_t*			Path;				// The path of the file without it's name. i.e: if the file full name is "c:\tmp\new_file.txt" the path is "c:\tmp\" including the last
											// back slash.
	wchar_t*			Full_name;
	wchar_t*			Full_name_long;
	class Folder*		Parent;				// Parent folder.

											// The status member class is relative to the folder/subfolder class the files belongs to.
	unsigned int		status;				// 0 -	The file is sync with it's real counterpart.
											// 1 -	The file is dirty, this means it's different than it's real counterpart. 
											// 2 -	The file is a ghost, this means that it's marked for deletion in the folder/subfolder class but still exists in the file system.

public:

/**********************************************************************************************************************/
/************************************************** Methods ***********************************************************/

private:

	LARGE_INTEGER	calculate_hash(BYTE* content, unsigned int algorithm, class m_error** p_error);	// Not implemented yet.
	BYTE*			load_content(LPTSTR File, class m_error** p_error);										// Not implemented yet.
	BOOLEAN			compare_content(BYTE* content1, BYTE* content2, DWORD size);						// Implemented but not funcional yet.
	void			set_Full_Name_long();

public:

/************************************************** Constructors/Destructor ******************************************/

	File();
	File(File* From_file);
	File(LPTSTR Origen, Folder* _Parent, BOOLEAN hashit, unsigned int algorithm, class m_error** p_error);
	File(WIN32_FIND_DATA* Origin_FileData, Folder* _Parent, BOOLEAN hashit, unsigned int algorithm, class m_error** p_error);
	~File();

/************************************************** Interfaces *********************************************************/

	WIN32_FIND_DATA get_FileData(void);
	BOOLEAN			get_if_hashed(void);
	LARGE_INTEGER	get_HashValue(void);
	UINT			get_hashing_algorithm(void);
	errno_t			get_Path(wchar_t** Receiving_Path);
	errno_t			get_full_name(wchar_t** Receiving_FullName);
	errno_t			get_full_name_long(wchar_t** Receiving_FullName_long);
	class Folder*	get_Parent(void);
	UINT			get_status(void);
	//errno_t			get_name(TCHAR* Receiving_Name);
	
	void			set_Path(wchar_t* _Path);
	void			set_Full_name(wchar_t* _Full_name);
	void			set_status(UINT _status);
	void			set_parent(Folder* _Parent);
	void			set_FileData(WIN32_FIND_DATA _FileData);

	INT16			compare(File* ToFile, COMPARE_LEVELS Level, COMPARE_LEVELS sync, class m_error** p_error);
	BOOL			delete_from_system(class m_error** p_error);
	BOOLEAN			synchronize(File* ToFile, class m_error** p_error);
	
};

class Folder
{
	//Properties

private:

	WIN32_FIND_DATA FileData;	// The struct where is saved the pertinent information of the real Folder been represented by this "Folder Object" in memory.
	wchar_t* Path;				// The path of the folder without it's name. i.e: if the folder full name is "c:\tmp\new folder" the path is "c:\tmp\" including the last
								// back slash.
	wchar_t* Full_name;
	wchar_t* Full_name_long;
	Folder* Parent;				// A pointer to the Folder class that contains this very class.

	struct file_array Files;
	struct folder_array Subfolders;

	BOOLEAN	Files_hashed;			// Determine if the files inside this Folder has been hashed.
	unsigned int hashing_algorithm;	// The algorithm used to calculate the hash of the files.
	unsigned int status;			// 0 -	The folder is sync with it's real counterpart.
									// 1 -	The folder is dirty, it means that, with respect to other folder with wich it's been compared is different.
									//		This applies to the folder class and it's real counterpart.
									// 2 -	The folder is a ghost, it means that, with respect to other folder with wich it's been compared it does not exist.
									//		This only applies to the real counterpart.

public:

	//Methods

private:

	//int syncronize(class m_error** p_error);
	//File* search_file(LPTSTR file);
	void set_Full_Name_long();

public:

	Folder();
	Folder(Folder* _Parent, class m_error** p_error);

	//	This parametric constructor is used to create a Folder object from the name of a real folder in the file system.
	Folder(LPTSTR Origen, Folder* _Parent, bool hashit, unsigned int algorithm, class m_error** p_error);	
	
	//	This one is to create the object from a WIN32_FIND_DATA struct that already contain the information about 
	//	the Folder, but not it's internal structure (the files and subfolders contained within it).
	Folder(WIN32_FIND_DATA* Origin_FileData, Folder* _Parent, bool hashit, unsigned int algorithm, class m_error** p_error);	
	

	~Folder();
	//INT16 compare(LPTSTR ToFolder, COMPARE_LEVELS Level, COMPARE_LEVELS sync, COMPARE_LEVELS files_levels, COMPARE_LEVELS files_sync, class m_error** p_error);
	Folder* compare_content(Folder* ToFolder, COMPARE_LEVELS files_levels, class m_error** p_error);
	BOOLEAN synchronize(Folder* ToSyncFolder, class m_error** p_error);
	Folder* copy_folder(Folder* Origen, Folder* _Parent, class m_error** p_error);

	BOOL remove_SubFolder(Folder* Subfolder, BOOL from_system, class m_error** p_error);
	//BOOL remove_File(File* file, BOOL from_system, class m_error** p_error);
	BOOL remove_File(UINT file_index, BOOL from_system, class m_error** p_error);
	BOOL delete_from_system(class m_error** p_error);
	
	WIN32_FIND_DATA get_FileData(void);
	errno_t get_path(wchar_t** Receiving_Path);
	errno_t get_full_name(wchar_t** Receiving_FullName);
	Folder* get_Parent();
	UINT get_files_count();
	File* get_file(UINT index);
	UINT get_subfolders_count();
	Folder* get_subfolder(UINT index);
	BOOL get_files_hashed();
	BOOL get_hashing_algorithm();
	UINT get_status(void);
	errno_t get_name(TCHAR** Receiving_Name);
	
	void set_FileData(WIN32_FIND_DATA _FileData);
	void set_Path(wchar_t* _Path);
	void set_Full_Name(wchar_t* _Full_Name);
	void set_parent(Folder* _Parent);
	BOOL insert_file(File* file, class m_error** p_error);
	BOOL insert_SubFolder(Folder* Subfolder, class m_error** p_error);
	void set_Files_hashed(BOOLEAN _Files_hashed);
	void set_hashing_algorithm(UINT _hashing_algorithm);
	void set_status(UINT _status);

};

