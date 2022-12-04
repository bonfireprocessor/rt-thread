#include <rtthread.h>

#ifdef  BSP_USING_HW_I2C3

#include <drivers/i2c.h>

//#include "stm32f4xx_hal_conf.h"
#include "stm32f4xx_hal.h"
//#include "stm32f4xx_ll_i2c.h"


static I2C_HandleTypeDef hi2c3;

static void MX_I2C3_Init(void)
{

  /* USER CODE BEGIN I2C3_Init 0 */

  /* USER CODE END I2C3_Init 0 */

  /* USER CODE BEGIN I2C3_Init 1 */

  /* USER CODE END I2C3_Init 1 */
  hi2c3.Instance = I2C3;
  hi2c3.Init.ClockSpeed = 100000;
  hi2c3.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  RT_ASSERT(HAL_I2C_Init(&hi2c3) == HAL_OK);

  RT_ASSERT(HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) == HAL_OK)
  
  RT_ASSERT(HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) == HAL_OK)
 
}



static rt_size_t stm32_master_xfer(struct rt_i2c_bus_device *bus,
                             struct rt_i2c_msg msgs[],
                             rt_uint32_t num)
{
I2C_HandleTypeDef * hi2c = (I2C_HandleTypeDef *)bus->priv;
HAL_StatusTypeDef res;

    for(int i=0;i<num;i++) {
        if (msgs[i].flags & (RT_I2C_NO_START|RT_I2C_IGNORE_NACK|RT_I2C_NO_READ_ACK)) {
            rt_kprintf("stm32_master_xfer unsuported flags\n");
            return 0;
        } else {
            if (msgs[i].flags & RT_I2C_RD) {
              res = HAL_I2C_Master_Receive(hi2c,msgs[i].addr<<1,msgs[i].buf,msgs[i].len,1000);               
            } else {              
              res = HAL_I2C_Master_Transmit(hi2c,msgs[i].addr<<1,msgs[i].buf,msgs[i].len,1000); 
            }
            if (res!=HAL_OK) return RT_ERROR;
        }
    }
    return num;
}



static struct rt_i2c_bus_device_ops  dev_ops = {
    .master_xfer = stm32_master_xfer,
    .slave_xfer =RT_NULL,
    .i2c_bus_control = RT_NULL
};

static struct rt_i2c_bus_device bus = {
   .ops = &dev_ops,
   .priv = (void*)&hi2c3
};


static int i2c3_device_init() {


    MX_I2C3_Init();
    rt_i2c_bus_device_register(&bus,"i2c3");

}

INIT_COMPONENT_EXPORT(i2c3_device_init);

#endif