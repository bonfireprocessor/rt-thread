/*
 * Copyright (c) 2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-09-19     Gavin        first version
 *
 */

#include <rtthread.h>

#include <dfs_fs.h>

#ifdef RT_USING_DFS_ROMFS

#include "dfs_romfs.h"

const static struct romfs_dirent _dummy[] =
{
//    {ROMFS_DIRENT_FILE, "dummy.txt", _dummy_dummy_txt, sizeof(_dummy_dummy_txt)},
};

// const static unsigned char _dummy_txt[] =
// {
//     0x74, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x61, 0x20, 0x66, 0x69, 0x6c, 0x65, 0x21, 0x0d, 0x0a,
// };

RT_WEAK const struct romfs_dirent _root_dirent[] =
{
    {ROMFS_DIRENT_DIR, "sd", (rt_uint8_t *)_dummy, 0},//  sizeof(_dummy) / sizeof(_dummy[0])},
	{ROMFS_DIRENT_DIR, "ram", (rt_uint8_t *)_dummy,0} //sizeof(_dummy) / sizeof(_dummy[0])},
    
};

RT_WEAK const struct romfs_dirent romfs_root =
{
    ROMFS_DIRENT_DIR, "/", (rt_uint8_t *)_root_dirent, sizeof(_root_dirent) / sizeof(_root_dirent[0])
};

#endif

#ifdef RT_USING_DFS_RAMFS
extern struct dfs_ramfs *dfs_ramfs_create(rt_uint8_t *pool, rt_size_t size);
#endif

int mnt_init(void)
{
	

    rt_err_t romfs_mounted = RT_ERROR;
#ifdef RT_USING_DFS_ROMFS
	  romfs_mounted = dfs_mount(RT_NULL,"/","rom",0,&romfs_root);
#endif

#ifdef RT_USING_DFS_RAMFS
	rt_uint8_t *pool = RT_NULL;
	rt_size_t size = 8*1024*1024;
	pool = rt_malloc(size);
	if (pool == RT_NULL)
		return 0;

	if (dfs_mount(RT_NULL,romfs_mounted==RT_EOK?"/ram":"/", "ram", 0, (const void *)dfs_ramfs_create(pool, size)) == 0)
		rt_kprintf("RAM file system initializated!\n");
	else
		rt_kprintf("RAM file system initializate failed!\n");

	return 0;
#endif	
}
INIT_ENV_EXPORT(mnt_init);

#ifdef RT_USING_MSH
//#pragma message "installing mount_sd"

#include <sys/stat.h>

static void mount_sd(int argc, char** argv)
{
struct stat stats;
const char sd[]="/sd";
const char *mountpoint = ((stat(sd,&stats)==0) && S_ISDIR(stats.st_mode))?sd:"/";

	rt_err_t err=dfs_mount("sd0",mountpoint,"elm",0,NULL);
	if (err==RT_EOK) {
		rt_kprintf("sdcard mounted at %s\n",mountpoint);
	} else {
		rt_kprintf("sdcard mount failed with error %d\n",err);
	}        
}

MSH_CMD_EXPORT(mount_sd, mount_sd: mount sd0   );
#endif


