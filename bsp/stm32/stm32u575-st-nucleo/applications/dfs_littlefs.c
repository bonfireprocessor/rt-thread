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

    if (lfs==RT_NULL) return -ENOMEM;

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

    if (lfs) {
        lfs_unmount(lfs);
        rt_free(lfs);
    }    
    return RT_EOK;
}

int dfs_lfs_ioctl(struct dfs_fd *file, int cmd, void *args)
{
    return -RT_EIO;
}

int dfs_lfs_read(struct dfs_fd *file, void *buf, rt_size_t count)
{
    lfs_t *lfs = ( lfs_t *) file->fs->data; 
    RT_ASSERT(lfs!=RT_NULL);
    RT_ASSERT(file->data != RT_NULL);
     if (file->flags & O_DIRECTORY) {
        return  -ENOENT; // TODO Implementation
     } else {
        return lfs_file_read(lfs,(lfs_file_t*)file->data,buf,count);
     }   
}


int   dfs_lfs_write(struct dfs_fd *file, const void *buf, size_t count)
{
    lfs_t *lfs = ( lfs_t *) file->fs->data; 
    RT_ASSERT(lfs!=RT_NULL);
    RT_ASSERT(file->data != RT_NULL);
     if (file->flags & O_DIRECTORY) {
        return  -ENOENT; // TODO Implementation
     } else {
        return lfs_file_write(lfs,(lfs_file_t*)file->data,buf,count);
     }   
}

int dfs_lfs_lseek(struct dfs_fd *file, rt_off_t offset)
{
    lfs_t *lfs = ( lfs_t *) file->fs->data; 
    RT_ASSERT(lfs!=RT_NULL);
    RT_ASSERT(file->data != RT_NULL);
     if (file->flags & O_DIRECTORY) {
        return  -ENOENT; // TODO Implementation
     } else {
        return lfs_file_seek(lfs,(lfs_file_t*)file->data,offset,0);
     }   
}

int dfs_lfs_close(struct dfs_fd *file)
{
lfs_t *lfs = ( lfs_t *) file->fs->data;

    RT_ASSERT(lfs!=RT_NULL);
    RT_ASSERT(file->data!=RT_NULL);

    if (file->flags & O_DIRECTORY)
      lfs_dir_close(lfs,(lfs_dir_t*)file->data);
    else
      lfs_file_close(lfs,(lfs_file_t*)file->data);  

    rt_free(file->data);
    return RT_EOK;
}

int dfs_lfs_open(struct dfs_fd *file)
{
 lfs_t *lfs = ( lfs_t *) file->fs->data;     

    RT_ASSERT(lfs);
    if (file->flags & O_DIRECTORY) {
         if (file->flags & O_CREAT)
        {
            return -ENOSPC; // Implement later
        } else {
            lfs_dir_t * dir = rt_malloc(sizeof(lfs_dir_t));
            if (dir==RT_NULL) return -ENOMEM;
            int err=lfs_dir_open(lfs,dir,file->path);
            if (err!=LFS_ERR_OK) {
                rt_free(dir);
                return -ENOENT; // TODO: Better error mapping
            } else {
                file->data = (void*) dir;
                return RT_EOK;
            }

        }
    } else {
        int mode = LFS_O_RDONLY;
        int accmode = file->flags & O_ACCMODE;
        switch(accmode) {
            case O_RDONLY: 
               mode = LFS_O_RDONLY; 
               break;
            case  O_WRONLY:
                mode = LFS_O_WRONLY; 
                break;
            case  O_RDWR:
               mode = LFS_O_RDWR;      
        }
        
        
        if (file->flags & O_CREAT)
            mode |= LFS_O_CREAT;
        /* Creates a new file. If the file is existing, it is truncated and overwritten. */
        if (file->flags & O_TRUNC)
            mode |= LFS_O_TRUNC;
        /* Creates a new file. The function fails if the file is already existing. */
        if (file->flags & O_EXCL)
            mode |= LFS_O_EXCL;

        lfs_file_t * fd = rt_malloc(sizeof(lfs_file_t));
        if (file==RT_NULL) return -ENOMEM;
        int err = lfs_file_open(lfs,fd,file->path,mode);
        if (err!=LFS_ERR_OK) {
            rt_free(fd);
            return err; // TODO: Map Errors
        } else {
            file->data = fd;
            return RT_EOK;
        }

    }
}

int dfs_lfs_stat(struct dfs_filesystem *fs, const char *path, struct stat *st)
{
struct lfs_info info;

    lfs_t *lfs = ( lfs_t *) fs->data;
    RT_ASSERT(lfs != RT_NULL);
    memset(st,0,sizeof(struct stat));
    int err = lfs_stat(lfs,path,&info);
    if (err==LFS_ERR_OK) {
        if (info.type == LFS_TYPE_DIR ) st->st_mode = S_IFDIR;
        st->st_size = info.size; 
    }

    return RT_EOK;
}

int dfs_lfs_getdents(struct dfs_fd *file, struct dirent *dirp, uint32_t count)
{
lfs_dir_t *dir;
struct lfs_info info;
int err;
rt_uint32_t index;
struct dirent *d;

    lfs_t *lfs = ( lfs_t *) file->fs->data;
    RT_ASSERT(lfs != RT_NULL);
    dir = (lfs_dir_t *)(file->data);
    RT_ASSERT(dir != RT_NULL);

    /* make integer count */
    count = (count / sizeof(struct dirent)) * sizeof(struct dirent);
    if (count == 0)
        return -EINVAL;

    index = 0;
    while (1)
    {

        d = dirp + index;
        err = lfs_dir_read(lfs,dir,&info);
        if (err <= 0 || info.name[0] == 0)
            break;

        if (info.type==LFS_TYPE_DIR)
          d->d_type = DT_DIR;
        else
          d->d_type = DT_REG;

        d->d_namlen = (rt_uint8_t)rt_strlen(info.name);
        d->d_reclen = (rt_uint16_t)sizeof(struct dirent);
        rt_strncpy(d->d_name, info.name, 256);

        index ++;
        if (index * sizeof(struct dirent) >= count)
            break;
    }

    if (index == 0)
        return err;

    file->pos += index * sizeof(struct dirent);

    return index * sizeof(struct dirent); 
}

static const struct dfs_file_ops _lfs_fops =
{
    dfs_lfs_open,
    dfs_lfs_close,
    dfs_lfs_ioctl,
    dfs_lfs_read,
    dfs_lfs_write, 
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


