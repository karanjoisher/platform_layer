#define PF_FILE
#define PF_TIME
#include "pf.h"
#include <stdio.h>
#include "utility.h"

int GetFilenameStartIndex(char *path)
{
    int result = 0;
    int length = GetStrLength(path);
    
    for(int i = length - 1; i >= 0; i--)
    {
        if(path[i] == '\\' || path[i] == '/') 
        {
            result = i + 1; 
            break;
        }
    }
    return result;
}



int GetDirectoryNameEndIndex(char *path)
{
    int result = 0;
    for(int i = 0; path[i] != 0; i++)
    {
        if(path[i] == '\\' || path[i] == '/')
        {
            result = i - 1;
        }
    }
    return result;
}


struct Filepaths
{
    char *filepath;
    int filepathLength;
    Filepaths *next;
};

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        DEBUG_ERROR("Specify the directory to backup"); 
        return -1;
    }
    
    if(argc < 3)
    {
        DEBUG_ERROR("Specify destination folder for converted files"); 
        return -2;
    }
    
    
    if(argc < 4)
    {
        DEBUG_ERROR("Specify backup periodicity in minutes"); 
        return -2;
    }
    
    int minutes = atoi(argv[3]);
    int destinationPathLength = GetStrLength(argv[2]);
    char destinationPath[MAX_PATH] = {};
    Copy(destinationPath, argv[2], destinationPathLength);
    destinationPath[destinationPathLength] = '\\';
    destinationPathLength++;
    
    Filepaths *f = 0;
    for(int i = 1; i < argc - 2; i++)
    {
        char *dir = argv[i];
        char workingDirectory[128] = {};
        if(AreStringsSame(dir, "."))
        {
            GetCurrentDirectory(128, (char*)&workingDirectory);
        }
        
        
        int dirLength = GetStrLength(dir);
        
        char *types[] = {"/*.cpp", "/*.h", "/*.hbackup", "/*.shader", "/*.bat", "/*.txt", "/*.sh", "/*.py", ".4coder"};
        
        for(int a = 0; a < ARRAY_COUNT(types); a++)
        {
            char searchString[128] = {};
            ConcatenateStrings(dir, types[a], searchString);
            
            WIN32_FIND_DATAA findData = {};
            HANDLE searchHandle = FindFirstFileA(searchString, &findData);
            if(searchHandle != INVALID_HANDLE_VALUE)
            {
                do
                {
                    Filepaths *t = new Filepaths;
                    
                    int filenameLength = GetStrLength(findData.cFileName);
                    t->filepath = new char[dirLength + filenameLength + 2];
                    Copy(t->filepath, dir, dirLength);
                    t->filepath[dirLength] = '\\';
                    Copy(t->filepath + dirLength + 1, findData.cFileName, filenameLength);
                    t->filepath[dirLength + 1 + filenameLength] = 0;
                    
                    t->filepathLength = dirLength + filenameLength + 1;
                    
                    t->next = f;
                    f = t;
                }
                while(FindNextFileA(searchHandle, &findData) != 0);
            }
            FindClose(searchHandle);
        }
    }
    
    while(true)
    {
        fprintf(stdout, "Backing up.. Dont quit the app");
        Filepaths *file = f;
        char *fileData = 0;
        uint64 fileDataSize = 0;
        DEBUG_LOG("\n");
        while(file)
        {
            WIN32_FILE_ATTRIBUTE_DATA fileAttrData = {};
            GetFileAttributesEx(file->filepath,GetFileExInfoStandard,&fileAttrData);
            
            uint64 fileSize = (((uint64)fileAttrData.nFileSizeHigh) << 32) | ((uint64)fileAttrData.nFileSizeLow);
            
            if(fileDataSize < fileSize)
            {
                if(fileData) delete[] fileData;
                
                fileDataSize = fileSize;
                fileData = new char[fileDataSize];
            }
            
            int64 bytesRead = PfReadEntireFile(file->filepath, (void *)fileData);
            ASSERT((uint64)bytesRead == fileSize, "Bytes read not same as file size");
            
            int filenameStartIndex = GetFilenameStartIndex(file->filepath);
            int filenameLength = file->filepathLength - filenameStartIndex;
            
            Copy(destinationPath + destinationPathLength, file->filepath + filenameStartIndex, filenameLength);
            Copy(destinationPath + destinationPathLength + filenameLength, ".backup", 8);
            
            PfWriteEntireFile(destinationPath, fileData, (uint32)fileSize);
            
            DEBUG_LOG("%s -> %s\n", file->filepath, destinationPath);
            file = file->next;
        }
        
        fprintf(stdout, "Sleeping");
        PfSleep(minutes * 60 * 1000);
    }
    return 0;
}