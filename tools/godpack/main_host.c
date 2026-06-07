#include <stdio.h>

#include <godlib/file/file.h>
#include <godlib/memory/memory.h>
#include <godlib/packer/godpack.h>

int main(int argc, char **argv)
{
    int res = 0;
    U8 *src_data;
    U32 dst_size;
    U32 src_size;
    sGodPackHeader *header;
    U8 *temp;
    const char *src_name;
    const char *dst_name;

    printf("GOD-PACKER\n");
    printf("(c) 2018 Reservoir Gods\n");

    if (argc < 3)
    {
        printf("\nUSAGE: GODPACK srcfile dstfile\n");
        return -1;
    }

    Memory_Init();
    File_Init();

    src_name = argv[1];
    dst_name = argv[2];

    if (!File_Exists(src_name))
    {
        printf("GODPACK : ERROR : can't find file %s\n", src_name);
        res = -2;
        goto done;
    }

    printf("<GODPACK> %s -> %s\n", src_name, dst_name);

    src_size = File_GetSize(src_name);
    if (!src_size)
    {
        printf("GODPACK : ERROR : can't pack 0 size file %s\n", src_name);
        res = -2;
        goto done;
    }

    src_data = File_Load(src_name);
    if (!src_data)
    {
        printf("GODPACK : ERROR : can't load file %s\n", src_name);
        res = -3;
        goto done;
    }

    header = GodPack_Pack(src_data, src_size);
    if (!header)
    {
        printf("couldn't allocate memory for packed file\n");
        res = -4;
    }
    else
    {
        temp = mMEMCALLOC(src_size + dGODPACK_OVERFLOW);
        GodPack_DePack(header, temp);
        if (!Memory_IsEqual(src_data, temp, src_size))
        {
            printf("VALIDATE ERROR! Depacked data doesn't match original\n");
            res = -5;
        }
        else
        {
            U32 perc;
            Endian_ReadBigU32(&header->mPackedSize, dst_size);

            File_Save(dst_name, header, dst_size + sizeof(sGodPackHeader));

            perc = dst_size * 100;
            perc /= src_size;

            printf("%d bytes -> %d bytes (%d%%)\n", src_size, dst_size, perc);
        }

        mMEMFREE(temp);
        mMEMFREE(header);
    }

    File_UnLoad(src_data);

done:
    File_DeInit();
    Memory_DeInit();
    return res;
}
