/*****************************************************************************
* | File      	:   DEV_Config.c
* | Author      :   Waveshare team
* | Function    :   Hardware underlying interface
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2025-11-19
* | Info        :   
* -----------------------------------------------------------------------------
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of theex Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
#include "DEV_Config.h"

/*GPIO output init*/
TUYA_GPIO_BASE_CFG_T out_pin_cfg = {
    .mode = TUYA_GPIO_PUSH_PULL, 
    .direct = TUYA_GPIO_OUTPUT, 
    .level = TUYA_GPIO_LEVEL_LOW};

/*GPIO input init*/
TUYA_GPIO_BASE_CFG_T in_pin_cfg = {
    .mode = TUYA_GPIO_PULLUP,
    .direct = TUYA_GPIO_INPUT,
};

/**
 * GPIO read and write
**/
void DEV_Digital_Write(UWORD Pin, UBYTE Value)
{
    tkl_gpio_write(Pin, Value);
}

UBYTE DEV_Digital_Read(UWORD Pin)
{
    TUYA_GPIO_LEVEL_E read_level = 0;

    tkl_gpio_read(Pin, &read_level);

    if(read_level == TUYA_GPIO_LEVEL_LOW)
        return 0;
    else
        return 1;
}

/**
 * SPI
**/
void DEV_SPI_WriteByte(uint8_t Value)
{
    tkl_spi_send(SPI_ID, &Value, 1);
}

void DEV_SPI_Write_nByte(uint8_t *pData, uint32_t Len)
{
    tkl_spi_send(SPI_ID, pData, Len);
}

/**
 * GPIO Mode
**/
void DEV_GPIO_Mode(UWORD Pin, UWORD Mode)
{
    if(Mode == 0) {
		tkl_gpio_init(Pin, &in_pin_cfg);
	} else {
		tkl_gpio_init(Pin, &out_pin_cfg);
	}
}

/**
 * delay x ms
**/
void DEV_Delay_ms(UDOUBLE xms)
{
    tal_system_sleep(xms);
}

void DEV_GPIO_Init(void)
{
    printf("  Configuring BUSY (GPIO %d) as INPUT...\r\n", EPD_BUSY_PIN);
    DEV_GPIO_Mode(EPD_BUSY_PIN, 0);
    
    printf("  Configuring RST (GPIO %d) as OUTPUT...\r\n", EPD_RST_PIN);
	DEV_GPIO_Mode(EPD_RST_PIN, 1);
	
	printf("  Configuring DC (GPIO %d) as OUTPUT...\r\n", EPD_DC_PIN);
	DEV_GPIO_Mode(EPD_DC_PIN, 1);
	
	printf("  Configuring CS (GPIO %d) as OUTPUT...\r\n", EPD_CS_PIN);
	DEV_GPIO_Mode(EPD_CS_PIN, 1);
	
	printf("  Configuring PWR (GPIO %d) as OUTPUT...\r\n", EPD_PWR_PIN);
    DEV_GPIO_Mode(EPD_PWR_PIN, 1);
    
    // MOSI and SCLK will be configured dynamically in software SPI
    // DEV_GPIO_Mode(EPD_MOSI_PIN, 0);
	// DEV_GPIO_Mode(EPD_SCLK_PIN, 1);

    printf("  Setting CS HIGH\r\n");
	DEV_Digital_Write(EPD_CS_PIN, 1);
	
	printf("  Setting PWR HIGH\r\n");
    DEV_Digital_Write(EPD_PWR_PIN, 1);
}

void DEV_SPI_SendnData(UBYTE *Reg)
{
    UDOUBLE size;
    size = sizeof(Reg);
    for(UDOUBLE i=0 ; i<size ; i++)
    {
        DEV_SPI_SendData(Reg[i]);
    }
}

void DEV_SPI_SendData(UBYTE Reg)
{
	UBYTE i,j=Reg;
	DEV_GPIO_Mode(EPD_MOSI_PIN, 1);
	DEV_Digital_Write(EPD_CS_PIN, 0);
	for(i = 0; i<8; i++)
    {
        DEV_Digital_Write(EPD_SCLK_PIN, 0);     
        if (j & 0x80)
        {
            DEV_Digital_Write(EPD_MOSI_PIN, 1);
        }
        else
        {
            DEV_Digital_Write(EPD_MOSI_PIN, 0);
        }
        
        DEV_Digital_Write(EPD_SCLK_PIN, 1);
        j = j << 1;
    }
	DEV_Digital_Write(EPD_SCLK_PIN, 0);
	DEV_Digital_Write(EPD_CS_PIN, 1);
}

// Add initialization for software SPI pins
void DEV_SPI_GPIO_Init(void)
{
    printf("  Configuring SCLK (GPIO %d) as OUTPUT...\r\n", EPD_SCLK_PIN);
    DEV_GPIO_Mode(EPD_SCLK_PIN, 1);
    DEV_Digital_Write(EPD_SCLK_PIN, 0);
    
    printf("  Configuring MOSI (GPIO %d) as OUTPUT...\r\n", EPD_MOSI_PIN);
    DEV_GPIO_Mode(EPD_MOSI_PIN, 1);
    DEV_Digital_Write(EPD_MOSI_PIN, 0);
}

UBYTE DEV_SPI_ReadData()
{
	UBYTE i,j=0xff;
	DEV_GPIO_Mode(EPD_MOSI_PIN, 0);
	DEV_Digital_Write(EPD_CS_PIN, 0);
	for(i = 0; i<8; i++)
	{
		DEV_Digital_Write(EPD_SCLK_PIN, 0);
		j = j << 1;
		if (DEV_Digital_Read(EPD_MOSI_PIN))
		{
				j = j | 0x01;
		}
		else
		{
				j= j & 0xfe;
		}
		DEV_Digital_Write(EPD_SCLK_PIN, 1);
	}
	DEV_Digital_Write(EPD_SCLK_PIN, 0);
	DEV_Digital_Write(EPD_CS_PIN, 1);
	return j;
}

UBYTE DEV_Module_Init(void)
{
    printf("/***********************************/ \r\n");
    printf("E-Paper GPIO Configuration:\r\n");
    printf("  SCLK: GPIO %d\r\n", EPD_SCLK_PIN);
    printf("  MOSI: GPIO %d\r\n", EPD_MOSI_PIN);
    printf("  CS:   GPIO %d\r\n", EPD_CS_PIN);
    printf("  DC:   GPIO %d\r\n", EPD_DC_PIN);
    printf("  RST:  GPIO %d\r\n", EPD_RST_PIN);
    printf("  BUSY: GPIO %d\r\n", EPD_BUSY_PIN);
    printf("  PWR:  GPIO %d\r\n", EPD_PWR_PIN);
    printf("  SPI:  %d (Software SPI)\r\n", SPI_ID);
    printf("/***********************************/ \r\n");
    
    /*spi init*/
    TUYA_SPI_BASE_CFG_T spi_cfg = {.mode = TUYA_SPI_MODE0,
                                   .freq_hz = SPI_FREQ,
                                   .databits = TUYA_SPI_DATA_BIT8,
                                   .bitorder = TUYA_SPI_ORDER_MSB2LSB,
                                   .role = TUYA_SPI_ROLE_MASTER,
                                   .type = TUYA_SPI_SOFT_ONE_WIRE_TYPE};
    
    printf("Initializing SPI %d...\r\n", SPI_ID);
    int ret = tkl_spi_init(SPI_ID, &spi_cfg);
    printf("SPI init returned: %d\r\n", ret);

    printf("Initializing GPIOs...\r\n");
    DEV_GPIO_Init();
    
    printf("Initializing Software SPI GPIOs...\r\n");
    DEV_SPI_GPIO_Init();
    
    printf("GPIO init complete\r\n");
    printf("/***********************************/ \r\n");
	return 0;
}

void DEV_Module_Exit(void)
{
    tkl_spi_deinit(SPI_ID);
    tkl_gpio_deinit(EPD_SCLK_PIN);
    tkl_gpio_deinit(EPD_MOSI_PIN);
    tkl_gpio_deinit(EPD_CS_PIN);
    tkl_gpio_deinit(EPD_DC_PIN);
    tkl_gpio_deinit(EPD_RST_PIN);
    tkl_gpio_deinit(EPD_BUSY_PIN);
    tkl_gpio_deinit(EPD_PWR_PIN);
}
