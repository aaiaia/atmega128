#ifndef _ADC128S102_H_
#define _ADC128S102_H_

#define GET_ADC_PORT_INSTRUCTION(adcAddr)		(adcAddr<<3)//that is instruction of ADC128S102.

#define ADC_DEFAULT_JUNK_SAMPLING_TIMES		0x10

unsigned char SPI_ADC_Init();
unsigned int getAdcValue(unsigned char spiDeviceNumber, unsigned char numberOfPort, unsigned int samplingTimes, unsigned char junkTimes);

#endif
