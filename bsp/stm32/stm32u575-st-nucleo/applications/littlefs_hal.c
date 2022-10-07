
#if (!defined (NO_FLASH))
#include <rtthread.h>
#include "lfs.h"
#include <stdbool.h>
#include <ctype.h> 
#include <dfs_fs.h>

#include "stm32u5xx_hal.h"







#define FIRST_BLOCK 0x08100000
#define NUM_BLOCKS 128
#define BLOCK_SIZE 8192


struct rt_mutex lfs_mutex;


static int lfshal_flash_read(const struct lfs_config *c, lfs_block_t block,
            lfs_off_t off, void *buffer, lfs_size_t size)
{

   RT_ASSERT(block < NUM_BLOCKS);
   void * addr = (void*) ( block * BLOCK_SIZE + FIRST_BLOCK + off );
   
   rt_memcpy(buffer,(void*)addr,size);
   return 0;
}


static HAL_StatusTypeDef _flash_program_verify(rt_uint32_t dest,void* src)
{
    //rt_kprintf("Program quadword at %lx\n",dest);
    HAL_StatusTypeDef res = HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD,dest,(uint32_t)src);
    //rt_kprintf("HAL_Status: %lx\n",res);
    if (res!=HAL_OK) return res;

    rt_uint32_t * d = (rt_uint32_t*)dest;
    rt_int32_t * s = (rt_uint32_t*)src;

    HAL_ICACHE_Invalidate();
    for(int i=0;i<4;i++) {
        if (*d != *s) {
            rt_kprintf("Flash program error at %lx (%lx != %lx)\n",d,*d,*s);
            return HAL_ERROR;
        }
    }
    return HAL_OK;
}


static int lfshal_flash_prog(const struct lfs_config *c, lfs_block_t block,
            lfs_off_t off, const void *buffer, lfs_size_t size)
{
 HAL_StatusTypeDef res;


    RT_ASSERT(block < NUM_BLOCKS);
    rt_uint32_t addr = block * BLOCK_SIZE + FIRST_BLOCK + off;

    RT_ASSERT((addr & 0xf)==0); // Check 16 Byte (Quadword) bondary
    RT_ASSERT(HAL_ICACHE_Disable()==HAL_OK);
    RT_ASSERT(HAL_FLASH_Unlock()==HAL_OK);
  

    int num_quads = size / 16 ; // Remaining quadwords
    
    while(num_quads>0) {
        res = _flash_program_verify(addr,(void*)buffer);
        if (res != HAL_OK)  {
            RT_ASSERT(HAL_FLASH_Lock()==HAL_OK);
            return -1; 
        }  
        num_quads--;
        addr += 16;
        buffer += 16;

    }

    RT_ASSERT(HAL_FLASH_Lock()==HAL_OK);
    RT_ASSERT(HAL_ICACHE_Enable()==HAL_OK);
    return 0;
}


static int lfshal_flash_erase(const struct lfs_config *c, lfs_block_t block)
{
FLASH_EraseInitTypeDef EraseInitStruct;
uint32_t PageError = 0;



  if (HAL_ICACHE_Disable() != HAL_OK)
  {
    return -1;
  }

    /* Unlock the Flash to enable the flash control register access *************/
  if (HAL_FLASH_Unlock() != HAL_OK) {
    HAL_ICACHE_Enable();
    return -1;
  }


  /* Fill EraseInit structure*/
  EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
  EraseInitStruct.Banks       = 2;
  RT_ASSERT(block<NUM_BLOCKS); 
  EraseInitStruct.Page        = block;
  EraseInitStruct.NbPages     = 1;

  HAL_StatusTypeDef res = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);

  RT_ASSERT(HAL_FLASH_Lock()==HAL_OK);
  RT_ASSERT(HAL_ICACHE_Enable()==HAL_OK);

  return res; 
   
}

static int lfshal_flash_sync(const struct lfs_config *c)
{
  return 0;
}

static int lfshal_flash_lock(const struct lfs_config *c)
{
    rt_mutex_take(&lfs_mutex,RT_WAITING_FOREVER);
    return 0;
}


static int lfshal_flash_unlock(const struct lfs_config *c)
{
    rt_mutex_release(&lfs_mutex);
    return 0;
}


const struct lfs_config cfg = {
    // block device operations
    .read  = lfshal_flash_read,
    .prog  = lfshal_flash_prog,
    .erase = lfshal_flash_erase,
    .sync  = lfshal_flash_sync,
    .lock  = lfshal_flash_lock,
    .unlock = lfshal_flash_unlock,

    // block device configuration
    .read_size = 16,
    .prog_size = 16,
    .block_size = 8192,
    .block_count = 128,
    .cache_size = 16,
    .lookahead_size = 16,
    .block_cycles = 500,
};


// const struct lfs_config *get_lfs_hal()
// {
//    return &cfg;
// }

int lfs_init()
{
  rt_mutex_init(&lfs_mutex,"lfs",RT_IPC_FLAG_FIFO);
  
}

INIT_ENV_EXPORT(lfs_init);


int lfs_start(int argc,char **argv)
{
// variables used by the filesystem
//static lfs_t lfs;  
//static lfs_file_t file;  

    dfs_mount(RT_NULL,"/lfs","lfs",0,(const void*)&cfg);

    // if (argc==2 &&  toupper(*argv[1])=='F') {
    //   rt_kprintf("Force formating\n");
    //   lfs_format(&lfs, &cfg);
    //   lfs_mount(&lfs, &cfg);
    // } else {
    //  // mount the filesystem
    //   int err = lfs_mount(&lfs, &cfg);

    //   // reformat if we can't mount the filesystem
    //   // this should only happen on the first boot
    //   if (err) {
    //       lfs_format(&lfs, &cfg);
    //       lfs_mount(&lfs, &cfg);
    //   }
    // }

    // for(int i=0;i<100;i++) {
    //   uint32_t boot_count = 0;
    //   lfs_file_open(&lfs, &file, "boot_count", LFS_O_RDWR | LFS_O_CREAT);
    //   lfs_file_read(&lfs, &file, &boot_count, sizeof(boot_count));

    //   // update boot count
    //   boot_count += 1;
    //   lfs_file_rewind(&lfs, &file);
    //   lfs_file_write(&lfs, &file, &boot_count, sizeof(boot_count));

    //   // remember the storage is not updated until the file is closed successfully
    //   lfs_file_close(&lfs, &file);
    //   printf("boot_count: %d\n", boot_count);
    // }
    // // release any resources we were using
    // lfs_unmount(&lfs);

    // print the boot count
   

}

MSH_CMD_EXPORT_ALIAS(lfs_start, littlefs, test littlefs  );

#endif