#include "main.h"

int main(int args, char *comm[])
{

    char targetPath[MAX_PATH]{0};

    DWORD compMethod = 7; // 封包时的加密格式


    if (args > 1)
    {
        ::strcpy(targetPath, comm[1]);
    }
    else
    {
        ::printf("Please enter target file with the path:");
        ::scanf("%[^\n]", targetPath);
    }

    DWORD targetAttribute = GetFileAttributesA(targetPath); // 获取文件属性

    if (targetAttribute == NULL || targetAttribute == INVALID_FILE_ATTRIBUTES)
    {

        ::printf("Sorry, the value you entered isn't valid\n");
    }
    else
    {
        if (targetAttribute == FILE_ATTRIBUTE_DIRECTORY)
        {
            if(args>2){
                compMethod=::atoi(comm[2]);
            }
            else{
                ::printf("please enter compression Mode(4:zlib,7:zstd,anther:no compression):");
                ::scanf("%d",&compMethod);
            }
            if (::fileTopack(targetPath,compMethod))
            {
                ::printf("sucess\n");
            }
            else
            {
                ::printf("failed\n");
            }
        }
        else
        {
            if (::fileUnpack(targetPath))
            {
                ::printf("sucess\n");
            }
            else
            {
                ::printf("failed\n");
            }
        }
    }

    return 0;
}