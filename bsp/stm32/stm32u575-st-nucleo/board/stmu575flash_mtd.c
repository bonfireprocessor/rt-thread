
#include <rtthread.h>

#ifdef RT_USING_MTD_NOR
#include <stdbool.h>
#include <ctype.h> 

#include <drivers/mtd_nor.h>

#include "stm32u5xx_hal.h"

#define FLASH_START 0x08100000
#define FLASH_END  0x081FFFFF
#define BLOCK_SIZE 8192
#define NUM_BLOCKS ((LAST_BLOCK-FIRST_BLOCK)/BLOCK_SIZE)


static rt_size_t stm_flash_read(struct rt_mtd_nor_device* device, 
                             rt_off_t offset, rt_uint8_t* data, rt_uint32_t length)
{

  
   rt_uint32_t addr = FLASH_START + offset;
   //rt_kprintf("Fuck %lx, %lx\n",addr+length,LAST_BLOCK);
   RT_ASSERT((addr+length-1) <= FLASH_END);

   rt_memcpy((void*)data,(void*)addr,length);
   return length;
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


static rt_size_t stm_flash_write (struct rt_mtd_nor_device* device, 
                           rt_off_t offset, const rt_uint8_t* data, rt_uint32_t length)
{
 HAL_StatusTypeDef res;


   
    rt_uint32_t addr = FLASH_START + offset;
    if ((addr+length-1) > FLASH_END) {
       rt_kprintf("stm_flash_write out of range errror %lx > %lx\n",addr+length-1,FLASH_END);
       return RT_ERROR;
    }
    

    RT_ASSERT((addr & 0xf)==0); // Check 16 Byte (Quadword) bondary
    RT_ASSERT((length % 16)==0);
    RT_ASSERT(HAL_ICACHE_Disable()==HAL_OK);
    RT_ASSERT(HAL_FLASH_Unlock()==HAL_OK);
    
    int num_quads = length / 16 ; // Remaining quadwords
    
    while(num_quads>0) {
        res = _flash_program_verify(addr,(void*)data);
        if (res != HAL_OK)  {
            RT_ASSERT(HAL_FLASH_Lock()==HAL_OK);
            return -1; 
        }  
        num_quads--;
        addr += 16;
        data += 16;

    }

    RT_ASSERT(HAL_FLASH_Lock()==HAL_OK);
    RT_ASSERT(HAL_ICACHE_Enable()==HAL_OK);
    return length;
}


static rt_err_t stm_flash_erase(struct rt_mtd_nor_device* device, rt_off_t offset, rt_uint32_t length)
{
FLASH_EraseInitTypeDef EraseInitStruct;
uint32_t PageError = 0;


    RT_ASSERT((offset % BLOCK_SIZE)==0); // Check erase block boundary
    RT_ASSERT((length % BLOCK_SIZE)==0); // Check length is multiple of block size

    if (HAL_ICACHE_Disable() != HAL_OK)
    {
      return RT_ERROR;
    }

      /* Unlock the Flash to enable the flash control register access *************/
    if (HAL_FLASH_Unlock() != HAL_OK) {
      HAL_ICACHE_Enable();
      return RT_ERROR;
    }


    /* Fill EraseInit structure*/
    EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Banks       = 2;
    
    EraseInitStruct.Page        = offset / BLOCK_SIZE;
    EraseInitStruct.NbPages     = length / BLOCK_SIZE;

    HAL_StatusTypeDef res = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);

    RT_ASSERT(HAL_FLASH_Lock()==HAL_OK);
    RT_ASSERT(HAL_ICACHE_Enable()==HAL_OK);

    return res==HAL_OK?RT_EOK:RT_ERROR; 
   
}

static rt_err_t _read_id(struct rt_mtd_nor_device* device)
{
  return RT_EOK;
}

static struct rt_mtd_nor_driver_ops ops =  {
   .read_id = _read_id,
   .read = stm_flash_read,
   .write = stm_flash_write,
   .erase_block = stm_flash_erase
};


static struct rt_mtd_nor_device stmflash =
{
  .block_size = BLOCK_SIZE,
  .block_start = 0,
  .block_end = 128, // should be 127, but littlefs config has a wrong size calculation
  .ops = &ops
};



static int mtd_init()
{
  rt_mtd_nor_register_device("stmflash",&stmflash);
  return 1;
  
}

INIT_BOARD_EXPORT(mtd_init);


#endif