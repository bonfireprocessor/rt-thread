
#if (!defined (NO_FLASH))
#include <rtthread.h>
#include "lfs.h"
#include <stdbool.h>
#include "stm_flash.h"


// variables used by the filesystem
lfs_t lfs;
lfs_file_t file;

#define FIRST_BLOCK 0x08100000
#define NUM_BLOCKS 128
#define BLOCK_SIZE 8192

/**
  * @brief  Gets the page of a given address
  * @param  Addr: Address of the FLASH Memory
  * @retval The page of a given address
  */
static rt_uint32_t GetPage(uint32_t Addr)
{
  uint32_t page = 0;

  if (Addr < (FLASH_BASE + FLASH_BANK_SIZE))
  {
    /* Bank 1 */
    page = (Addr - FLASH_BASE) / FLASH_PAGE_SIZE;
  }
  else
  {
    /* Bank 2 */
    page = (Addr - (FLASH_BASE + FLASH_BANK_SIZE)) / FLASH_PAGE_SIZE;
  }

  return page;
}

/**
  * @brief  Gets the bank of a given address
  * @param  Addr: Address of the FLASH Memory
  * @retval The bank of a given address
  */
static rt_uint32_t GetBank(uint32_t Addr)
{
  if (Addr < (FLASH_BASE + FLASH_BANK_SIZE))
  {
    /* Bank 1 */
    return FLASH_BANK_1;
  }
  else
  {
    /* Bank 2 */
   return FLASH_BANK_2;
  }
}



static int hal_flash_read(const struct lfs_config *c, lfs_block_t block,
            lfs_off_t off, void *buffer, lfs_size_t size)
{

   RT_ASSERT(block < NUM_BLOCKS);
   void * addr = (void*) ( block * BLOCK_SIZE + FIRST_BLOCK + off );
   
   rt_memcpy(buffer,(void*)addr,size);
   return 0;
}


static HAL_StatusTypeDef _flash_program_verify(rt_uint32_t dest,void* src)
{
    rt_kprintf("Program quadword at %lx\n",dest);
    HAL_StatusTypeDef res = HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD,dest,(uint32_t)src);
    rt_kprintf("HAL_Status: %lx\n",res);
    if (res!=HAL_OK) return res;

    rt_uint32_t * d = (rt_uint32_t*)dest;
    rt_int32_t * s = (rt_uint32_t*)src;

    for(int i=0;i<4;i++) {
        if (*d != *s) {
            rt_kprintf("Flash program error at %lx (%lx != %lx)\n",d,*d,*s);
            return HAL_ERROR;
        }
    }
    return HAL_OK;
}


static int hal_flash_prog(const struct lfs_config *c, lfs_block_t block,
            lfs_off_t off, const void *buffer, lfs_size_t size)
{
// rt_uint32_t  copied = 0;
// rt_uint32_t src_adr;
 HAL_StatusTypeDef res;
// rt_uint8_t buffer[16]; // Quadword Buffer 
rt_uint32_t next_quadword; 


    RT_ASSERT(block < NUM_BLOCKS);
    rt_uint32_t addr = block * BLOCK_SIZE + FIRST_BLOCK + off;

    RT_ASSERT((addr & 0xf)==0); // Check 16 Byte (Quadword) bondary
    RT_ASSERT(HAL_FLASH_Unlock()==HAL_OK);

    // if ((addr & 0xf)) { // Start not on a quadword boundary

    //     void  * base = (void*)(addr & 0xfffffff0); // find quadword boundary 
    //     int offset = addr & 0x0f;  // offset of start address in quadword
    //     int len = 16 - offset;
    //     memcpy(buffer,base,16); // Copy Quadword from Flash to buffer
    //     memcpy(buffer+offset,src,len); 
    //     next_quadword = (rt_uint32_t)base;
    //     res = _flash_program_verify(next_quadword,buffer);
    //     if (res != HAL_OK)  {
    //         RT_ASSERT(HAL_FLASH_Lock()==HAL_OK);
    //         return -1; 
    //     }    
    //     copied += len;      
    //     next_quadword += 16;
    //     src_adr = (rt_uint32_t)(src+len);
    // } else {
    //     src_adr = (rt_uint32_t)src;
    // }

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

    // if (copied < size) { // Copy last partial word
    //     int rem = size - copied;
    //     RT_ASSERT(rem<16);
    //     memcpy(buffer,(void*)next_quadword,16);
    //     memcpy(buffer,(void*)src_adr,rem);
    //     res = _flash_program_verify(next_quadword,buffer);
    //     if (res != HAL_OK)  {
    //         RT_ASSERT(HAL_FLASH_Lock()==HAL_OK);
    //         return -1; 
    //     } 
    // }

    return 0;
}


static int hal_flash_erase(const struct lfs_config *c, lfs_block_t block)
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

static int hal_flash_sync(const struct lfs_config *c)
{
  return 0;
}


const struct lfs_config cfg = {
    // block device operations
    .read  = hal_flash_read,
    .prog  = hal_flash_prog,
    .erase = hal_flash_erase,
    .sync  = hal_flash_sync,

    // block device configuration
    .read_size = 16,
    .prog_size = 16,
    .block_size = 8192,
    .block_count = 128,
    .cache_size = 16,
    .lookahead_size = 16,
    .block_cycles = 500,
};


int lfs_start(int argc,char **argv)
{
   // mount the filesystem
    int err = lfs_mount(&lfs, &cfg);

    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err) {
        lfs_format(&lfs, &cfg);
        lfs_mount(&lfs, &cfg);
    }

    // read current count
    uint32_t boot_count = 0;
    lfs_file_open(&lfs, &file, "boot_count", LFS_O_RDWR | LFS_O_CREAT);
    lfs_file_read(&lfs, &file, &boot_count, sizeof(boot_count));

    // update boot count
    boot_count += 1;
    lfs_file_rewind(&lfs, &file);
    lfs_file_write(&lfs, &file, &boot_count, sizeof(boot_count));

    // remember the storage is not updated until the file is closed successfully
    lfs_file_close(&lfs, &file);

    // release any resources we were using
    lfs_unmount(&lfs);

    // print the boot count
    printf("boot_count: %d\n", boot_count);

}

MSH_CMD_EXPORT_ALIAS(lfs_start, littlefs, test littlefs  );

#endif