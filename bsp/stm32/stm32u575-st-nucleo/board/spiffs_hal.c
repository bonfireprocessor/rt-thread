
#if (!defined (NO_FLASH))
#include "spiffs.h" 
#include "spiffs_config.h"
#include <stdbool.h>
#include "stm_flash.h"


spiffs fs;
spiffs_config fs_config;

static rt_uint8_t work_buffer[SPIFFS_CFG_LOG_PAGE_SZ()*2];
static rt_uint8_t fd_space[256];


static rt_uint8_t * cache = NULL;

/**
  * @brief  Gets the page of a given address
  * @param  Addr: Address of the FLASH Memory
  * @retval The page of a given address
  */
static uint32_t GetPage(uint32_t Addr)
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
static uint32_t GetBank(uint32_t Addr)
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



static s32_t hal_spiffs_read(u32_t addr, u32_t size, u8_t *dst)
{
    RT_ASSERT(addr>=SPIFFS_CFG_PHYS_ADDR());
    rt_memcpy(dst,(void*)addr,size);
    return SPIFFS_OK;
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


static s32_t hal_spiffs_write(u32_t addr, u32_t size, u8_t *src)
{
rt_uint32_t  copied = 0;
rt_uint32_t src_adr;
HAL_StatusTypeDef res;
rt_uint8_t buffer[16]; // Quadword Buffer 
rt_uint32_t next_quadword; 


    RT_ASSERT(addr>=SPIFFS_CFG_PHYS_ADDR());
    RT_ASSERT(HAL_FLASH_Unlock()==HAL_OK);

    if ((addr & 0xf)) { // Start not on a quadword boundary

        void  * base = (void*)(addr & 0xfffffff0); // find quadword boundary 
        int offset = addr & 0x0f;  // offset of start address in quadword
        int len = 16 - offset;
        memcpy(buffer,base,16); // Copy Quadword from Flash to buffer
        memcpy(buffer+offset,src,len); 
        next_quadword = (rt_uint32_t)base;
        res = _flash_program_verify(next_quadword,buffer);
        if (res != HAL_OK)  {
            RT_ASSERT(HAL_FLASH_Lock()==HAL_OK);
            return -1; 
        }    
        copied += len;      
        next_quadword += 16;
        src_adr = (rt_uint32_t)(src+len);
    } else {
        src_adr = (rt_uint32_t)src;
    }

    int num_quads = (size-copied) / 16 ; // Remaining quadwords 

    while(num_quads>0) {
        res = _flash_program_verify(next_quadword,(void*)src_adr);
        if (res != HAL_OK)  {
            RT_ASSERT(HAL_FLASH_Lock()==HAL_OK);
            return -1; 
        }  
        next_quadword += 16;
        src_adr += 16;
        copied += 16;

    }

    if (copied < size) { // Copy last partial word
        int rem = size - copied;
        RT_ASSERT(rem<16);
        memcpy(buffer,(void*)next_quadword,16);
        memcpy(buffer,(void*)src_adr,rem);
        res = _flash_program_verify(next_quadword,buffer);
        if (res != HAL_OK)  {
            RT_ASSERT(HAL_FLASH_Lock()==HAL_OK);
            return -1; 
        } 
    }

    return SPIFFS_OK;
}


static s32_t hal_spiffs_erase(u32_t addr, u32_t size)
{
FLASH_EraseInitTypeDef EraseInitStruct;
uint32_t PageError = 0;

  RT_ASSERT(addr>=SPIFFS_CFG_PHYS_ADDR());
  RT_ASSERT((size % 8192) == 0);

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
  EraseInitStruct.Banks       = GetBank(addr);
  RT_ASSERT(EraseInitStruct.Banks == 2); // SPIFFS should only work in Bank 2
  EraseInitStruct.Page        = GetPage(addr);
  EraseInitStruct.NbPages     = size / 8192;

  HAL_StatusTypeDef res = HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);

  RT_ASSERT(HAL_FLASH_Lock()==HAL_OK);
  RT_ASSERT(HAL_ICACHE_Enable()==HAL_OK);

  return res; 
   
}

static const char * const type_strings[] =
{
    "lookup","index","page"
};

static const char * const report_strings[] =
{
   "progress","error","fix index","fix lookup","delete orphaned index",
   "delete page","delete bad file"
};

static int progress=0;


static void check_callback(spiffs_check_type type, spiffs_check_report report,
    u32_t arg1, u32_t arg2)
{
    if (report==SPIFFS_CHECK_PROGRESS) {
       //if (arg1==0) progress = 0; 
       if (!(progress % 4096)) rt_kprintf("\nfsck: checking %s %lu\n",type_strings[type],arg1); 
       if (!(progress++ % 64)) {
           rt_kprintf(".");
          
       } 
    } else
      rt_kprintf("\nfsck: %s %s %lu %lu\n",type_strings[type],report_strings[report], arg1, arg2);
}    


rt_int32_t spiffs_init(rt_uint32_t cache_size,bool autoformat)
{

rt_int32_t result;    

 

    fs_config.hal_read_f=hal_spiffs_read;
    fs_config.hal_write_f=hal_spiffs_write;
    fs_config.hal_erase_f=hal_spiffs_erase;
    cache = realloc(cache,cache_size);

    result=SPIFFS_mount(&fs,&fs_config,work_buffer,fd_space,sizeof(fd_space),cache,cache_size,check_callback);
    if (result==SPIFFS_ERR_NOT_A_FS && autoformat) {
        rt_kprintf("No filesystem found, formating...");
        result=SPIFFS_format(&fs);
        rt_kprintf("SPIFFS format result: %ld\n",result);
        if (result==SPIFFS_OK) {
           result=SPIFFS_mount(&fs,&fs_config,work_buffer,fd_space,sizeof(fd_space),cache,cache_size,check_callback);
        }
    }
    rt_kprintf("SPIFFS Mount result: %ld\n",result);
    return result;
}


bool handle_error(int code)
{
  if(code<0) {
      rt_kprintf("SPIFFS error %d\n",SPIFFS_errno(&fs));
      return false;
  } else {
      return true;
  }
}

int32_t spiffs_save(char *filename,void *memptr,uint32_t size)
{
spiffs_file fd = SPIFFS_open(&fs,filename,SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR,0);
int r;

    if (!handle_error(fd)) return fd;
    r = SPIFFS_write(&fs,fd,memptr,size);
    if (!handle_error(r)) return r;
    r = SPIFFS_close(&fs,fd);
    handle_error(r);
    return r;
}

int spiffs_start(int argc,char **argv)
{
  spiffs_init(8192,true);

}

MSH_CMD_EXPORT_ALIAS(spiffs_start, spiffs, initalize spiffs fs   );

#endif