/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 */

#include <rtthread.h>
#include <dfs.h>
#include <dfs_fs.h>
#include <dfs_file.h>

#include "lfs.h"

//#include "dfs_lfs_fs.h"

int dfs_lfs_mount(struct dfs_filesystem *fs, unsigned long rwflag, const void *data)
{
const struct lfs_config *c = (const struct lfs_config *)data;

lfs_t *lfs = rt_malloc(sizeof(lfs_t));

    if (lfs==RT_NULL) return -ENOENT;

    // mount the filesystem
    int err = lfs_mount(lfs,c);

      // reformat if we can't mount the filesystem
      // this should only happen on the first boot
    if (err) {
          err = lfs_format(lfs, c);
          if (err==LFS_ERR_OK) err = lfs_mount(lfs, c);
    }
    
    if (err==LFS_ERR_OK) {
        fs->data = lfs;
        return RT_EOK;
    } else {
        rt_free(lfs);
        return -ENOENT;
    }    
}

int dfs_lfs_unmount(struct dfs_filesystem *fs)
{
    lfs_t *lfs = ( lfs_t *) fs->data;

    if (lfs) lfs_unmount(lfs);
    return RT_EOK;
}

int dfs_lfs_ioctl(struct dfs_fd *file, int cmd, void *args)
{
    return -RT_EIO;
}

int dfs_lfs_read(struct dfs_fd *file, void *buf, rt_size_t count)
{
    return count;
}

int dfs_lfs_lseek(struct dfs_fd *file, rt_off_t offset)
{
    return -RT_EIO;
}

int dfs_lfs_close(struct dfs_fd *file)
{
    return RT_EOK;
}

int dfs_lfs_open(struct dfs_fd *file)
{
    return RT_EOK;
}

int dfs_lfs_stat(struct dfs_filesystem *fs, const char *path, struct stat *st)
{
    return RT_EOK;
}

int dfs_lfs_getdents(struct dfs_fd *file, struct dirent *dirp, rt_uint32_t count)
{
    return count * sizeof(struct dirent);
}

static const struct dfs_file_ops _lfs_fops =
{
    dfs_lfs_open,
    dfs_lfs_close,
    dfs_lfs_ioctl,
    dfs_lfs_read,
    NULL, /* write */
    NULL, /* flush */
    dfs_lfs_lseek,
    dfs_lfs_getdents,
};

static const struct dfs_filesystem_ops _lfs_fs =
{
    "lfs",
    DFS_FS_FLAG_DEFAULT,
    &_lfs_fops,

    dfs_lfs_mount,
    dfs_lfs_unmount,
    NULL, /* mkfs */
    NULL, /* statfs */

    NULL, /* unlink */
    dfs_lfs_stat,
    NULL, /* rename */
};

int dfs_lfs_init(void)
{
    /* register rom file system */
    dfs_register(&_lfs_fs);
    return 0;
}
INIT_COMPONENT_EXPORT(dfs_lfs_init);


