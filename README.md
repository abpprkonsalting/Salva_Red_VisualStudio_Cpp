# SaveFolder

General system description
==========================

This program does a simple cycle checking for changes in a folder in the file system and saving them to another location. This save location could be local or remote, provided that the file system has access to it through the Windows file system API.  

The application is composed by an executable file that is at the same time a windows service and it's configurator. A settings file provides the working parameters.

On instantiation, it reads the file system structure of the folder where the file are been saved and creates a virtual folder representing it in memory (just the needed info to make the comparison). From there on, it enters an infinite cycle comparing the local folder's files / folders information to the information stored in the virtual folder. That comparison can be done on three levels, name, size & modification date. The appropiate structures and support functions were created to support comparison on HASH, but up to the current version the hashing algorithms were not implemented. 

Instalation
============

1- Copy both files to a folder in the PC.

2- Configure settings.txt with the appropiate working parameters.

3- Set the appropiate permisions to the folders, as well as the user in which name the service will we invoqued in case of a remote save.

4- Run the executable passing as parameter: install.

   i.e: c:\SyncFolder\> SyncFolder install
   
   This step must be done with an administrative account, and in an OS with UAC, from an elevated command prompt session. 
   
5- If the previous step was executed correctly, the output should be: "Service installed successfully". If the output is an initialization error requesting the file "MSVCR100.dll then should be installed the "Microsoft Visual C++ 2010 SP1 Redistributable Package" (vcredist_x86_sp1.exe), and repeat the step 4.

6- Open "Service Manager": Wnd + R, and then invoque "services.msc".

7- Search the service named: "Servicio Salva Automática de Archivos".

8- Configure the tab "Start Sesion" with the credentials of the user authorized to access the program folders. If the save is to be done in the same PC (e.g.: to an external HDD), this step is not necessary.

9- Start the service.

Use
====

The service creates a log file in the same directory where is located the executable.


Uninstall
==========

1- Run the executable passing as parameter "uninstall".

    i.e: c:\SyncFolder\> SyncFolder uninstall
    
    This step must be done using an administrative account, and in an OS with UAC, from an elevated command prompt session.
    
2- If the previous step was executed correctly, the output should be: "Service deleted successfully".

3- Check in Service Manager that the service does not longer exist. If it's still there, and it's state is "running", stop it manually.

Q&A:

1- How to configure the application for saving in a server that belongs to an Active Directory domain?

The best option to solve this situation is to configure the service to run with the credentials of the user "network service". This user is a local one, but it really is an alias pointing to the local computer account in the Active Directory domain. In the remote server where the save will take place, it's necessary then to grant this user write access to the Folder where the save will be done. 
