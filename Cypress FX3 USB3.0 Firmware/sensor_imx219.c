
#include <cyu3system.h>
#include <cyu3os.h>
#include <cyu3dma.h>
#include <cyu3error.h>
#include <cyu3uart.h>
#include <cyu3i2c.h>
#include <cyu3types.h>
#include <cyu3gpio.h>
#include <cyu3utils.h>
#include "sensor_imx219.h"


static const imx219_reg_t mode_default[]={
		 {REG_MODE_SEL,			0x00},
		 {0x30EB,				0x05},	//access sequence
		 {0x30EB,				0x0C},
		 {0x300A,				0xFF},
		 {0x300B,				0xFF},
		 {0x30EB,				0x05},
		 {0x30EB,				0x09},
		 {REG_CSI_LANE,			0x03},	//3-> 4Lane 1-> 2Lane
		 {REG_DPHY_CTRL,		0x00},	//DPHY timing 0-> auot 1-> manual
		 {REG_EXCK_FREQ_MSB,	0x18},	//external oscillator frequncy 0x18 -> 24Mhz
		 {REG_EXCK_FREQ_LSB,	0x00},
		 {REG_FRAME_LEN_MSB,	0x03},	//frame length , Raspberry pi sends this commands continously when recording video @60fps ,writes come at interval of 32ms , Data 355 for resolution 1280x720 command 162 also comes along with data 0DE7 also 15A with data 0200
		 {REG_FRAME_LEN_LSB,	0x55},
		 {REG_LINE_LEN_MSB,		0x0d},	//line length D78 def f88 , does not actually affect how many bits on wire in one line does affect how many clock between lines
		 {REG_LINE_LEN_LSB,		0xE7},	//appears to be having step in value, not every LSb change will reflect on fps
		 {REG_X_ADD_STA_MSB,	0x02},	//x start
		 {REG_X_ADD_STA_LSB,	0xA8},
		 {REG_X_ADD_END_MSB,	0x0A},	//x end
		 {REG_X_ADD_END_LSB,	0x27},
		 {REG_Y_ADD_STA_MSB,	0x02},	//y start
		 {REG_Y_ADD_STA_LSB,	0xB4},
		 {REG_Y_ADD_END_MSB,	0x06},	//y end
		 {REG_Y_ADD_END_LSB,	0xEB},
		 {REG_X_OUT_SIZE_MSB,	0x05},	//resolution 1280 -> 5 00 , 1920 -> 780 , 2048 -> 0x8 0x00
		 {REG_X_OUT_SIZE_LSB,	0x00},
		 {REG_Y_OUT_SIZE_MSB,	0x02},	// 720 -> 0x02D0 | 1080 -> 0x438  | this setting changes how many line over wire does not affect frame rate
		 {REG_Y_OUT_SIZE_LSB,	0xD0},
		 {REG_X_ODD_INC,		0x01},	//increment
		 {REG_Y_ODD_INC,		0x01},	//increment
		 {REG_BINNING_H,		0x00},	//binning H 0 off 1 x2 2 x4 3 x2 analog
		 {REG_BINNING_V,		0x00},	//binning H 0 off 1 x2 2 x4 3 x2 analog
		 {REG_CSI_FORMAT_C,		0x0A},	//CSI Data format A-> 10bit
		 {REG_CSI_FORMAT_D,		0x0A},	//CSI Data format
		 {REG_VTPXCK_DIV,		0x05},	//vtpxclkd_div	5
		 {REG_VTSYCK_DIV,		0x01},	//vtsclk _div  1
		 {REG_PREPLLCK_VT_DIV,	0x03},	//external oscillator /3
		 {REG_PREPLLCK_OP_DIV,	0x03},	//external oscillator /3
		 {REG_PLL_VT_MPY_MSB,	0x00},	//PLL_VT multiplizer
		 {REG_PLL_VT_MPY_LSB,	0x39},	// 0x30 ~33 , 0x57 ~60	//Changes Frame rate with , integration register 0x15a
		 {REG_OPPXCK_DIV,		0x0A},	//oppxck_div
		 {REG_OPSYCK_DIV,		0x01},	//opsysck_div
		 {REG_PLL_OP_MPY_MSB,	0x00},	//PLL_OP
		 {REG_PLL_OP_MPY_LSB,	0x30}, // 8Mhz x 0x57 ->696Mhz -> 348Mhz |  0x30 -> 200Mhz | 0x40 -> 256Mhz
		 {0x455E,				0x00},	//magic?
		 {0x471E,				0x4B},
		 {0x4767,				0x0F},
		 {0x4750,				0x14},
		 {0x4540,				0x00},
		 {0x47B4,				0x14},
		 {0x4713,				0x30},
		 {0x478B,				0x10},
		 {0x478F,				0x10},
		 {0x4793,				0x10},
		 {0x4797,				0x0E},
		 {0x479B,				0x0E},
		 {REG_TEST_PATTERN_MSB, 0x00},	//test pattern
		 {REG_TEST_PATTERN_LSB, 0x00},
		 {REG_TP_X_OFFSET_MSB,  0x00}, //tp offset x 0
		 {REG_TP_X_OFFSET_LSB,  0x00},
		 {REG_TP_Y_OFFSET_MSB,  0x00}, //tp offset y 0
		 {REG_TP_Y_OFFSET_LSB,  0x00},
		 {REG_TP_WIDTH_MSB,   	0x05}, //TP width 1920 ->780 1280->500
		 {REG_TP_WIDTH_LSB,   	0x00},
		 {REG_TP_HEIGHT_MSB,   	0x02}, //TP height 1080 -> 438 720->2D0
		 {REG_TP_HEIGHT_LSB,   	0xD0},
		 {REG_DIG_GAIN_GLOBAL_MSB, 	0x01},
		 {REG_DIG_GAIN_GLOBAL_LSB, 	0x00},
		 {REG_ANA_GAIN_GLOBAL, 		0x80}, //analog gain , raspberry pi constinouly changes this depending on scense
		 {REG_INTEGRATION_TIME_MSB, 0x03},	//integration time , really important for frame rate
		 {REG_INTEGRATION_TIME_LSB, 0x51},
		// {REG_MODE_SEL,			0x01},

};


static image_sensor_config_t sensor_config = {
	.sensor_mode = 0x01,
	.mode_640x480 = {
		.linelength = 0xD78,
		.framelength = 0x534,
		.startx = 2,
		.starty = 2,
	},
	.mode_1280x720_30 = {		//only 1280 x 720 30 and 60 FPS are functional reset WIP
		.integration = 0x0633,
		.gain = 0x80,
		.linelength = 0xD78,
		.framelength = 0x6E3,
		.startx = 4,
		.starty = 4,
		.endx = 0xA27,
		.endy = 0x6EB,
		.width = 1280,
		.height = 720,
		.test_pattern = 0
	},
	.mode_1280x720_60 = {
		.integration = 0x0351,
		.gain = 0x80,
		.linelength = 0xDE7,
		.framelength = 0x355,
		.startx = 0x2A8,
		.starty = 0x2B4,
		.endx = 0xA27,
		.endy = 0x6EB,
		.width = 1280,
		.height = 720,
		.test_pattern = 0
	},
	.mode_1920x1080 = {
		.linelength = 0xD78,
		.framelength = 0x9F0,
		.startx = 4,
		.starty = 4,
	},
	.mode_1280x720_180 = {
		.linelength = 0xD78,
		.framelength = 0x9F0,
		.startx = 4,
		.starty = 4,
	},
	.mode_1920x1080_60 = {
		.linelength = 0xD78,
		.framelength = 0x48e,
		.startx = 0,
		.starty = 0,
	},
	.mode_640x480_200 = {
			.integration = 0x0351,
			.gain = 0x80,
			.linelength = 0xDE7,
			.framelength = 0x355,
			.startx = 0x2A8,
			.starty = 0x2B4,
			.endx = 0xA27,
			.endy = 0x6EB,
			.width = 1280,
			.height = 720,
			.test_pattern = 0
	},
};


static void SensorI2CAccessDelay (CyU3PReturnStatus_t status)
{
    /* Add a 10us delay if the I2C operation that preceded this call was successful. */
    if (status == CY_U3P_SUCCESS)
        CyU3PBusyWait (50);
}

CyU3PReturnStatus_t sensor_i2c_write(uint16_t reg_addr, uint8_t data)
{
	CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
	CyU3PI2cPreamble_t  preamble;
	uint8_t buf[2];


	/* Set the parameters for the I2C API access and then call the write API. */
	preamble.buffer[0] = SENSOR_ADDR_WR;
	preamble.buffer[1] = (reg_addr >> 8) & 0xFF;
	preamble.buffer[2] = (reg_addr) & 0xFF;
	preamble.length    = 3;             /*  Three byte preamble. */
	preamble.ctrlMask  = 0x0000;        /*  No additional start and stop bits. */
	buf[0] = data;
	apiRetStatus = CyU3PI2cTransmitBytes (&preamble, buf, 1, 0);
	SensorI2CAccessDelay (apiRetStatus);

	return apiRetStatus;
}

CyU3PReturnStatus_t sensor_i2c_read(uint16_t reg_addr , uint8_t *buffer)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;
    CyU3PI2cPreamble_t preamble;

	preamble.length    = 4;
    preamble.buffer[0] = SENSOR_ADDR_RD & I2C_SLAVEADDR_MASK;        /*  Mask out the transfer type bit. */
    preamble.buffer[1] = (reg_addr >> 8) & 0xFF;
    preamble.buffer[2] = reg_addr & 0xFF;
    preamble.buffer[3] = SENSOR_ADDR_RD ;
    preamble.ctrlMask  = 1<<2;                                /*  Send start bit after third byte of preamble. */

    apiRetStatus = CyU3PI2cReceiveBytes (&preamble, buffer, 1, 0);

    SensorI2CAccessDelay (apiRetStatus);

    return apiRetStatus;
}



static CyU3PReturnStatus_t camera_stream_on (uint8_t on)
{
   return sensor_i2c_write(REG_MODE_SEL , on);
}


void SensorReset (void)
{
    sensor_i2c_write(REG_SW_RESET , 0x01);
    /* Wait for some time to allow proper reset. */
    CyU3PThreadSleep (10);
    /* Delay the allow the sensor to power up. */
    sensor_i2c_write(REG_SW_RESET , 0x00);
    CyU3PThreadSleep (10);
    return;
}

static void set_mirror_flip(uint8_t image_mirror)
{
//	CyU3PDebugPrint(4,"image_mirror = %d\n", image_mirror);

	/********************************************************
	   *
	   *   0x3820[2] ISP Vertical flip
	   *   0x3820[1] Sensor Vertical flip
	   *
	   *   0x3821[2] ISP Horizontal mirror
	   *   0x3821[1] Sensor Horizontal mirror
	   *
	   *   ISP and Sensor flip or mirror register bit should be the same!!
	   *
	   ********************************************************/
	uint8_t  iTemp;

	image_mirror = IMAGE_NORMAL;
	//CyU3PDebugPrint(4,"set_mirror_flip function\n");
   // iTemp = sensor_i2c_read(0x0172) & 0x03;	//Clear the mirror and flip bits.
    switch (image_mirror)
    {
        case IMAGE_NORMAL:
            sensor_i2c_write(0x0172, iTemp | 0x03);	//Set normal
            break;
        case IMAGE_V_MIRROR:
            sensor_i2c_write(0x0172, iTemp | 0x01);	//Set flip
            break;
        case IMAGE_H_MIRROR:
            sensor_i2c_write(0x0172, iTemp | 0x02);	//Set mirror
            break;
        case IMAGE_HV_MIRROR:
            sensor_i2c_write(0x0172, iTemp);	//Set mirror and flip
            break;
    }
}

void sensor_configure_mode(imgsensor_mode_t * mode)
{
	//set_mirror_flip(mode->mirror);
	sensor_i2c_write(REG_INTEGRATION_TIME_MSB, GET_WORD_MSB(mode->integration));
	sensor_i2c_write(REG_INTEGRATION_TIME_LSB, GET_WORD_LSB(mode->integration));
	sensor_i2c_write(REG_ANALOG_GAIN, 	mode->gain);
	sensor_i2c_write(REG_LINE_LEN_MSB, 	GET_WORD_MSB(mode->linelength));
	sensor_i2c_write(REG_LINE_LEN_LSB, 	GET_WORD_LSB(mode->linelength));

	sensor_i2c_write(REG_FRAME_LEN_MSB, GET_WORD_MSB(mode->framelength));
	sensor_i2c_write(REG_FRAME_LEN_LSB, GET_WORD_LSB(mode->framelength));

	sensor_i2c_write(REG_X_ADD_STA_MSB, GET_WORD_MSB(mode->startx));
	sensor_i2c_write(REG_X_ADD_STA_LSB, GET_WORD_LSB(mode->startx));

	sensor_i2c_write(REG_Y_ADD_STA_MSB, GET_WORD_MSB(mode->starty));
	sensor_i2c_write(REG_Y_ADD_STA_LSB, GET_WORD_LSB(mode->starty));

	sensor_i2c_write(REG_X_ADD_END_MSB, GET_WORD_MSB(mode->endx));
	sensor_i2c_write(REG_X_ADD_END_LSB, GET_WORD_LSB(mode->endx));

	sensor_i2c_write(REG_Y_ADD_END_MSB, GET_WORD_MSB(mode->endy));
	sensor_i2c_write(REG_Y_ADD_END_LSB, GET_WORD_LSB(mode->endy));

	sensor_i2c_write(REG_X_OUT_SIZE_MSB, GET_WORD_MSB(mode->width));
	sensor_i2c_write(REG_X_OUT_SIZE_LSB, GET_WORD_LSB(mode->width));

	sensor_i2c_write(REG_Y_OUT_SIZE_MSB, GET_WORD_MSB(mode->height));
	sensor_i2c_write(REG_Y_OUT_SIZE_LSB, GET_WORD_LSB(mode->height));

	sensor_i2c_write(REG_TEST_PATTERN_LSB, (mode->test_pattern < 8)? mode->test_pattern : 0);

	sensor_i2c_write(REG_TP_WIDTH_MSB, GET_WORD_MSB(mode->width));
	sensor_i2c_write(REG_TP_WIDTH_LSB, GET_WORD_LSB(mode->width));
	sensor_i2c_write(REG_TP_HEIGHT_MSB, GET_WORD_MSB(mode->height));
	sensor_i2c_write(REG_TP_HEIGHT_LSB, GET_WORD_LSB(mode->height));

	camera_stream_on(sensor_config.sensor_mode);
}

uint8_t	SensorI2cBusTest (void)
{
	uint8_t model_lsb;
	uint8_t model_msb;
	sensor_i2c_read (REG_MODEL_ID_MSB, &model_msb);
	sensor_i2c_read (REG_MODEL_ID_LSB, &model_lsb);

	if (((((uint16_t)model_msb & 0x0F) << 8) | model_lsb) == CAMERA_ID )
	{
		CyU3PDebugPrint(4,"I2C Sensor id: 0x%x\n", (((uint16_t)model_msb & 0x0F) << 8) | model_lsb);
		return CY_U3P_SUCCESS;
	}

	return CY_U3P_ERROR_DMA_FAILURE;
}

void SensorInit (void)
{

    if (SensorI2cBusTest() != CY_U3P_SUCCESS)        /* Verify that the sensor is connected. */
    {
        CyU3PDebugPrint (4, "Error: Reading Sensor ID failed!\r\n");
        return;
    }

	for (uint16_t i = 0; i < _countof(mode_default); i++)
	{
		//CyU3PDebugPrint (4, "Reg 0x%x val 0x%x\n", (mode_default + i)->address, (mode_default + i)->val);
		sensor_i2c_write((mode_default + i)->address, (mode_default + i)->val);
	}

	sensor_configure_mode(&sensor_config.mode_1280x720_60);
}





uint8_t SensorGetBrightness (void)
{
    return 0;
}


void SensorSetBrightness (uint8_t input)
{
	sensor_i2c_write (REG_ANALOG_GAIN, input);
}

uint8_t sensor_get_contrast (void)
{
	return 0;
}


void sensor_set_contrast (uint8_t input)
{
	sensor_i2c_write (REG_INTEGRATION_TIME_LSB, input);
}

uint8_t sensor_get_gain (void)
{
	return 0;
}

void sensor_set_gain (uint8_t input)
{
	sensor_i2c_write (REG_TEST_PATTERN_LSB, input);
}