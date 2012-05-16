#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libmtp.h>

#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 26
#include <fuse.h>

typedef struct mtpfuse_s mtpfuse_t;

struct mtpfuse_s
{
    LIBMTP_mtpdevice_t *dev;
};

static int mtpfuse_getattr(const char *path, struct stat *stat)
{
    memset(stat, 0, sizeof(*stat));

    return 0;
}

static int mtpfuse_open(const char *path, struct fuse_file_info *fi)
{
    return 0;
}

static int mtpfuse_read(const char *path, char *buf, size_t size, off_t offset,
                        struct fuse_file_info *fi)
{
    return 0;
}

static int mtpfuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                           off_t offset, struct fuse_file_info *fi)
{
    (void) offset;
    (void) fi;

    return 0;
}

static struct fuse_operations mtpfuse_ops = {
    .getattr    = mtpfuse_getattr,
    .open       = mtpfuse_open,
    .read       = mtpfuse_read,
    .readdir    = mtpfuse_readdir,
};

int main(int argc, char **argv)
{
    int mainret = 0;

    if (argc < 2)
    {
        fprintf(stderr, "usage: %s [-d] mountpoint\n", argv[0]);
        exit(1);
    }
    char *mount_point = argv[1];
    argc -= 1;
    argv += 1;

    // LIBMTP_Set_Debug(LIBMTP_DEBUG_PTP | LIBMTP_DEBUG_DATA);
    LIBMTP_Init();

    LIBMTP_error_number_t mtperr;

    LIBMTP_raw_device_t *rawdevs;
    int nrawdev;
    mtperr = LIBMTP_Detect_Raw_Devices(&rawdevs, &nrawdev);
    switch (mtperr)
    {
        case LIBMTP_ERROR_NO_DEVICE_ATTACHED:
            fprintf(stderr, "no raw devices\n");
            exit(0);
        case LIBMTP_ERROR_CONNECTING:
            fprintf(stderr, "ERROR connecting\n");
            exit(1);
        case LIBMTP_ERROR_MEMORY_ALLOCATION:
            fprintf(stderr, "ERROR memory allocation\n");
            exit(1);
        case LIBMTP_ERROR_NONE:
            break;
        default:
            fprintf(stderr, "ERROR unknown\n");
            exit(1);
    }

    printf("%d raw devices:\n", nrawdev);
    int i;
    for (i = 0; i < nrawdev; ++i)
    {
        printf("  device #%d: %s: %s (%04X:%04X) @ bus %d, dev %d\n", i,
               rawdevs[i].device_entry.vendor, rawdevs[i].device_entry.product,
               rawdevs[i].device_entry.vendor_id,
               rawdevs[i].device_entry.product_id,
               rawdevs[i].bus_location,
               rawdevs[i].devnum);
    }

    mtpfuse_t ctx;
    memset(&ctx, 0, sizeof(ctx));

    printf("opening raw device #0...\n");
    ctx.dev = LIBMTP_Open_Raw_Device_Uncached(&rawdevs[0]);
    if (!ctx.dev)
    {
        fprintf(stderr, "ERROR opening raw device #0\n");
        exit(1);
    }

    LIBMTP_Dump_Errorstack(ctx.dev);
    LIBMTP_Clear_Errorstack(ctx.dev);

    char *name = LIBMTP_Get_Friendlyname(ctx.dev);
    printf("opened raw device %s.\n", name);

    int ret = LIBMTP_Get_Storage(ctx.dev, LIBMTP_STORAGE_SORTBY_NOTSORTED);
    if (ret != 0)
    {
        fprintf(stderr, "ERROR get %s's storage:\n", name);
        LIBMTP_Dump_Errorstack(ctx.dev);
        mainret = 1;
        goto done;
    }

    printf("device's storage:\n");
    LIBMTP_devicestorage_t *storage;
    for (storage = ctx.dev->storage, i = 0; storage; storage = storage->next, ++i)
    {
        printf("  storage #%d: %d - %s\n", i, storage->id, storage->StorageDescription);
    }
    if (i <= 0)
    {
        fprintf(stderr, "there is no storage that is usable.\n");
        mainret = 1;
        goto done;
    }

    mainret = fuse_main(argc, argv, &mtpfuse_ops, &ctx);

done:
    LIBMTP_Release_Device(ctx.dev);
    return mainret;
}
