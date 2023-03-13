#pragma once
#include "drivers/stm32xx.h"

namespace SamplerKit
{
const RCC_OscInitTypeDef osc_conf = {
	.PLL3 =
		{
			.PLLState = RCC_PLL_ON,
			.PLLSource = RCC_PLL4SOURCE_HSE,
			.PLLM = 2,
			.PLLN = 34,
			.PLLP = 2,	// 208.896MHZ for MCU
			.PLLQ = 34, //=>12.288014MHz for SAI
			.PLLR = 2,	// 208.89624MHz for ...?
			.PLLRGE = RCC_PLL4IFRANGE_1,
			.PLLFRACV = 6685,
			.PLLMODE = RCC_PLL_FRACTIONAL,
		},
	.PLL4 =
		{
			.PLLState = RCC_PLL_ON,
			.PLLSource = RCC_PLL4SOURCE_HSE,
			.PLLM = 4,
			.PLLN = 99,
			.PLLP = 6,
			.PLLQ = 112,
			.PLLR = 9,
			.PLLRGE = RCC_PLL4IFRANGE_1,
			.PLLFRACV = 0,
			.PLLMODE = RCC_PLL_INTEGER,
		},
};

const RCC_ClkInitTypeDef clk_conf = {
	.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_PCLK3 |
				 RCC_CLOCKTYPE_PCLK4 | RCC_CLOCKTYPE_PCLK5,
	.MCUInit =
		{
			.MCU_Clock = RCC_MCUSSOURCE_PLL3,
			.MCU_Div = RCC_MCU_DIV1,
		},
	.APB4_Div = RCC_APB4_DIV2,
	.APB5_Div = RCC_APB5_DIV4,
	.APB1_Div = RCC_APB1_DIV2,
	.APB2_Div = RCC_APB2_DIV2,
	.APB3_Div = RCC_APB3_DIV2,
};

const RCC_PeriphCLKInitTypeDef periph_clk_conf = {
	.PeriphClockSelection = RCC_PERIPHCLK_I2C12 | RCC_PERIPHCLK_I2C35 | RCC_PERIPHCLK_SAI2 | RCC_PERIPHCLK_SAI3 |
							RCC_PERIPHCLK_SPI1 | RCC_PERIPHCLK_SPI45 | RCC_PERIPHCLK_USART6 | RCC_PERIPHCLK_SDMMC12 |
							RCC_PERIPHCLK_ADC,
	.I2c12ClockSelection = RCC_I2C12CLKSOURCE_PCLK1,
	.I2c35ClockSelection = RCC_I2C35CLKSOURCE_PCLK1,
	.Sai2ClockSelection = RCC_SAI2CLKSOURCE_PLL3_Q,
	// .Sai3ClockSelection = RCC_SAI3CLKSOURCE_PLL3_Q,
	// .Spi45ClockSelection = RCC_SPI45CLKSOURCE_PCLK2,
	.Usart6ClockSelection = RCC_USART6CLKSOURCE_HSI,
	.Sdmmc12ClockSelection =
		RCC_SDMMC12CLKSOURCE_HSI, // See MP15x Errata: 2.3.5 Incorrect reset of glitch-free kernel clock switch
	.AdcClockSelection = RCC_ADCCLKSOURCE_PLL4, // PLL4R: 66MHz vs PER vs PLL3

};

// const RCC_OscInitTypeDef osc_conf{
// 	.OscillatorType = RCC_OSCILLATORTYPE_HSE,
// 	.HSEState = RCC_HSE_ON,
// 	.PLL =
// 		{
// 			.PLLState = RCC_PLL_ON,
// 			.PLLSource = RCC_PLLSOURCE_HSE,
// 			.PLLM = 16,
// 			.PLLN = 432,
// 			.PLLP = RCC_PLLP_DIV2,
// 			.PLLQ = 9,
// 		},
// };

// const RCC_ClkInitTypeDef clk_conf{
// 	.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2,
// 	.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK,
// 	.AHBCLKDivider = RCC_SYSCLK_DIV1,
// 	.APB1CLKDivider = RCC_HCLK_DIV4,
// 	.APB2CLKDivider = RCC_HCLK_DIV2,
// };

// const RCC_PeriphCLKInitTypeDef rcc_periph_conf = {
// 	.PeriphClockSelection = RCC_PERIPHCLK_SAI1 | RCC_PERIPHCLK_UART4 | RCC_PERIPHCLK_CLK48 | RCC_PERIPHCLK_SDMMC1,
// 	.PLLSAI =
// 		{
// 			.PLLSAIN = 197,
// 			.PLLSAIQ = 8,
// 			.PLLSAIP = RCC_PLLSAIP_DIV2,
// 		},
// 	.PLLSAIDivQ = 2,
// 	.Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLLSAI,
// 	.Uart4ClockSelection = RCC_UART4CLKSOURCE_PCLK1,
// 	.Clk48ClockSelection = RCC_CLK48SOURCE_PLL,
// 	.Sdmmc1ClockSelection = RCC_SDMMC1CLKSOURCE_CLK48,
// };

} // namespace SamplerKit
