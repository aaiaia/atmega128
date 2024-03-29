//#define F_CPU 16000000UL//CPU clock definition
#ifndef F_CPU
	#define F_CPU 16000000UL//CPU clock definition
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <malloc.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <util/twi.h>
#include <math.h>
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* to save and display collected data, definition. */
#define STRING_BUFFER_SIZE 61
struct stringBuffer
{
	char dat[STRING_BUFFER_SIZE];
	char *loc;
	char dir;
}typedef stringBuffer;

stringBuffer g_strBuf;

char g_glcdBuf[STRING_BUFFER_SIZE];
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned char SWITCH=0;
unsigned char testFlag = 0;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//#define TEST_LED_DDR	DDRC
//#define TEST_LED		PORTC
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
	global variables
*/
//char stringBuffer[130] = {0};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*Text file and other paste information*/
const char EndOfTextFile[]={0x0d, 0x0a, 0x00};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
char putPasteWritingInFile(fat32Info *diskInfo, clustorData *clustor, directoryAndFileEntryInformation *physicalDirLocationInfo, lastFileAccessInfo *lastestClustorInfo, char *pastedString, char *pastedTailString)
{
	/*
	upgrade idea
	sum pastedString and pastedTailString.
	*/
	/*empty file file is not file exception and start*/
	if( (*physicalDirLocationInfo).dirStructure.otherInfo.indicateFirstClustor == 0) return -1;
	else if(((*physicalDirLocationInfo).dirStructure.otherInfo.attribute&ATTR_DIR_FILE_MASK)!=ATTR_ARCHIVE) return -2;
	/*empty file exception end*/

	/*to copy string from buffer to sd card buffer variables definition.*/
	CLUSTOR_LOCATION writingLocationClustor;
	if((*lastestClustorInfo).fileIndicateClustor!=(*physicalDirLocationInfo).dirStructure.otherInfo.indicateFirstClustor)
	{
		writingLocationClustor = findFilesLastestClustor(diskInfo, clustor, (*physicalDirLocationInfo).dirStructure.otherInfo.indicateFirstClustor);
		(*lastestClustorInfo).fileIndicateClustor=(*physicalDirLocationInfo).dirStructure.otherInfo.indicateFirstClustor;
		(*lastestClustorInfo).lastFoundedClustor=writingLocationClustor;
	}
	else
	{
		writingLocationClustor = findFilesLastestClustor(diskInfo, clustor, (*lastestClustorInfo).lastFoundedClustor);
		(*lastestClustorInfo).lastFoundedClustor=writingLocationClustor;
	}


	unsigned long writingLocationSecterInClustor = findFilesLastestLocationInClustor(diskInfo, clustor, (*physicalDirLocationInfo).dirStructure.otherInfo.fileSize)/(*diskInfo).bytesPerSecter;//start number of secter in clustor is 0.
	char *writingPositionInSdCard = (*clustor).secterData.data;


	if( (*diskInfo).secterPerClustor <= writingLocationSecterInClustor)
	{//file size multiple exception
		writingPositionInSdCard+=SD_DATA_BUFFER_SIZE;
	}
	else
	{
		writingPositionInSdCard+=(findFilesLastestLocationInClustor(diskInfo, clustor, (*physicalDirLocationInfo).dirStructure.otherInfo.fileSize)%(*diskInfo).bytesPerSecter);//searching and loaded target Clustor.//input charactor location
	}


//	char *testWritingAddedTailStringPointer = pastedTailString;
	const unsigned long writingStringOccupiedSizeMain=strlen(pastedString);
	const unsigned long writingStringOccupiedSizeTail=strlen(pastedTailString);
	unsigned long writingStringOccupiedRestSecterCount;//=writingStringOccupiedSize/SD_DATA_BUFFER_SIZE;
	unsigned int restOfWritingStringLengthAtLastestSecter;//=writingStringOccupiedSize%SD_DATA_BUFFER_SIZE;

	readSecterInClustor(diskInfo, clustor, writingLocationClustor, writingLocationSecterInClustor);


	if((writingPositionInSdCard+writingStringOccupiedSizeMain+writingStringOccupiedSizeTail)<(*clustor).secterData.data+SD_DATA_BUFFER_SIZE)//writing secter have enough empty bytes to write?
	{//yes{//yes//just copy from common buffer to sd card buffer.
		strncpy(writingPositionInSdCard, pastedString, writingStringOccupiedSizeMain);

		pastedString+=writingStringOccupiedSizeMain;
		writingPositionInSdCard+=writingStringOccupiedSizeMain;//have to move sdcardBuffer first.


		strncpy(writingPositionInSdCard, pastedTailString, writingStringOccupiedSizeTail);
		pastedTailString+=writingStringOccupiedSizeTail;
		writingPositionInSdCard+=writingStringOccupiedSizeTail;

		writeSecterInClustor(diskInfo, clustor, writingLocationClustor, writingLocationSecterInClustor);
	}
	else
	{//no//find next secter in same clustor or next clustor is empty.
		if( (writingPositionInSdCard+writingStringOccupiedSizeMain)<((*clustor).secterData.data+SD_DATA_BUFFER_SIZE) )//writingStringOccupiedSize=strlen(g_strBuf.loc)+strlen(pastedTailString);
		{
			strncpy(writingPositionInSdCard, pastedString, writingStringOccupiedSizeMain);
			pastedString+=writingStringOccupiedSizeMain;
			writingPositionInSdCard+=writingStringOccupiedSizeMain;

			/*tail is 3 case. Tail cannot input buffer(not empty space) or tail is cutout.*/
			if( (((*clustor).secterData.data+SD_DATA_BUFFER_SIZE)-writingPositionInSdCard)>0 )
			{
				strncpy(writingPositionInSdCard, pastedTailString, (((*clustor).secterData.data+SD_DATA_BUFFER_SIZE)-writingPositionInSdCard));
				pastedTailString+=(((*clustor).secterData.data+SD_DATA_BUFFER_SIZE)-writingPositionInSdCard);
				writingPositionInSdCard+=(((*clustor).secterData.data+SD_DATA_BUFFER_SIZE)-writingPositionInSdCard);
			}
		}
		else
		{
			strncpy(writingPositionInSdCard, pastedString, (((*clustor).secterData.data+SD_DATA_BUFFER_SIZE)-writingPositionInSdCard));
			pastedString+=(((*clustor).secterData.data+SD_DATA_BUFFER_SIZE)-writingPositionInSdCard);
			writingPositionInSdCard+=(((*clustor).secterData.data+SD_DATA_BUFFER_SIZE)-writingPositionInSdCard);
		}

		writeSecterInClustor(&sdCardInfo, clustor, writingLocationClustor, writingLocationSecterInClustor);
		/*because secter is full,so increase loaded sectre in clustor*/
		writingLocationSecterInClustor++;//writingLocationSecterInClustor have 1, next secter is empty but, it have 2 next clustor is not exist when secter per clustor is 2.
		/*because data that can be insert at lastest secter of target files, next step is find empty next secter in same clustor or clustor.*/

		if((strlen(pastedString)+strlen(pastedTailString))!=0)
		{
			restOfWritingStringLengthAtLastestSecter=(strlen(pastedString)+strlen(pastedTailString))%SD_DATA_BUFFER_SIZE;//calculate string length of lastest secter.
			writingStringOccupiedRestSecterCount=(strlen(pastedString)+strlen(pastedTailString))/SD_DATA_BUFFER_SIZE;//calculate writing secter number, but not this operating is ignore rest value.
			writingPositionInSdCard=(*clustor).secterData.data;//reset buffer pointer.

			if(restOfWritingStringLengthAtLastestSecter!=0)//case 2. just one secter is not full.
			{
				/*
					rest data have clustor case is 2.
					1. secter is full of data accurately.
					2. some secter is full but, any secter is not full. Otherwise, just one secter is not full.
				*/
				writingStringOccupiedRestSecterCount++;
			}

			while(writingStringOccupiedRestSecterCount)
			{
				if(writingLocationSecterInClustor<sdCardInfo.secterPerClustor)//secter in clustor is 0 or 1. secter had clustor -2,
				{//after write secter, increase secterInClustor variable.
					while(writingPositionInSdCard<=((*clustor).secterData.data+SD_DATA_BUFFER_SIZE))
					{
						if(*(pastedString)!=0)
						{
							*(writingPositionInSdCard)=*(pastedString);
							pastedString++;
						}
						else
						{
							if(*pastedTailString != 0)
							{
								*(writingPositionInSdCard)=*(pastedTailString);
								pastedTailString++;
							}
							else
							{
								*(writingPositionInSdCard)=0;
							}


						}
						writingPositionInSdCard++;

					}//escape while loop, when sd card buffer is full or common buffer is ended.

					writeSecterInClustor(diskInfo, clustor, writingLocationClustor, writingLocationSecterInClustor);
					writingPositionInSdCard=(*clustor).secterData.data;//

					writingLocationSecterInClustor++;
					writingStringOccupiedRestSecterCount--;
				}
				else
				{//find emtpty clustor. end set secter in clustor reset(set to zero).
					writingLocationClustor=writeNextClustor(diskInfo, clustor, writingLocationClustor, findEmptyClustor(diskInfo, clustor, writingLocationClustor));

					writingLocationSecterInClustor=0;
				}
			}
		}
	}
	(*lastestClustorInfo).lastFoundedClustor=(*clustor).locatedClustor;
	/*
		below code just write when secter is full and tail of data
		case is 2, one is wrote data size is small then sd card buffer size,
		another is wrote data size is same to sd card buffer size.
	*/
	/*
		writingStringOccupiedRestSecterCount is secter number will be wrote. so, it is decrease after executation of writeSecterInClustor.
		writingLocationSecterInClustor is secter number in target clustor. so, it is increase after executation of writeSecterInClustor.
		don't confuse both variables.
	*/

/*to Test after text file write, add file size start*/
	(*physicalDirLocationInfo).dirStructure.otherInfo.fileSize+=(writingStringOccupiedSizeMain+writingStringOccupiedSizeTail);

	readSecterInClustor(&sdCardInfo, clustor, (*physicalDirLocationInfo).entryInfo.location.clustor, (*physicalDirLocationInfo).entryInfo.location.secterInClustor);

	parsing32BitsToLittleEndian((*clustor).secterData.data+(*physicalDirLocationInfo).entryInfo.entryNumberOrOffset+DIR_FILESIZE ,(*physicalDirLocationInfo).dirStructure.otherInfo.fileSize);

	/*!!!!if varing lastest access time, added this location...*/
	writeSdcard(WRITE_BLOCK, &((*clustor).secterData), (*clustor).locatedClustorStartPhysicalSecter+(*clustor).secterInClustor, 512, 1);

	return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
char putPasteWritingInFileIfNotCreateNewDirEntry(fat32Info *diskInfo, clustorData *clustor, directoryAndFileEntryInformation *physicalDirLocationInfo, lastFileAccessInfo *lastestClustorInfo, CLUSTOR_LOCATION targetClustor, char *fileName, char *pastedString, char *pastedTailString)
{
	if(findDirEntryIfNotCreateNewDirEntry(diskInfo, clustor, physicalDirLocationInfo, targetClustor, ATTR_ARCHIVE, fileName))
	{
		return -1;
	}
	return putPasteWritingInFile(diskInfo, clustor, physicalDirLocationInfo, lastestClustorInfo, pastedString, pastedTailString);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* saved file and saved data name format*/
/*
	file name
	yy-MM-dd.txt
	
	data
	hhmmss(6 characters),deviceNumber(4 characters),value1(4 characters),value2,value3,value4,value5,value6,value7,value8(\r\n)
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*saved file parsing*/
#define ANALOG_DATA_START	12
#define ANALOG_DATA_VALUE_LENGTH	4
#define ANALOG_DATA_OFFSET	(ANALOG_DATA_VALUE_LENGTH+1)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*analog data display*/
void analogDataDisplay(char *str)
{
	char strBuffer[ANALOG_DATA_VALUE_LENGTH+1]={0};
	unsigned char portNum=0;
	unsigned char displayPageNumber=5;


	sprintf(g_glcdBuf, "Analog %c", *(str+10));
	putStringInGlcdAtPage(PAGE0+1, g_glcdBuf);

	sprintf(g_glcdBuf, "Time:%c%c:%c%c:%c%c", *(str+0), *(str+1), *(str+2), *(str+3), *(str+4), *(str+5));
	putStringInGlcdAtPage(PAGE0+2, g_glcdBuf);

	for(portNum=0; portNum<8; portNum+=2)
	{
		strncpy(strBuffer, str+12+(portNum*ANALOG_DATA_OFFSET), ANALOG_DATA_VALUE_LENGTH);
		*(strBuffer+ANALOG_DATA_VALUE_LENGTH)=0;
		sprintf(g_glcdBuf, "PORT%d:%s", portNum+1, strBuffer);

		strncpy(strBuffer, str+12+((portNum+1)*ANALOG_DATA_OFFSET), ANALOG_DATA_VALUE_LENGTH);
		*(strBuffer+ANALOG_DATA_VALUE_LENGTH)=0;
		sprintf(g_glcdBuf, "%s,PORT%d:%s", g_glcdBuf, portNum+2, strBuffer);

	putStringInGlcdAtPage(PAGE0+4+(portNum/2), g_glcdBuf);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*when use readSecterInClustor function first, MSB in clutorData at secterData must setted zero.*/
char savedDataFileInfoParseFromSectorInClustor(fat32Info *diskInfo, clustorData *searchingSecterBuffer, simpleDirectoryAndFileEntryInfomation *p, CLUSTOR_LOCATION startClustor)
{
	unsigned int count=0;
	char *str;

	if(((*p).dirStructure.otherInfo.indicateFirstClustor == 0)||(*p).dirStructure.otherInfo.fileSize == 0)
	{
		putStringInGlcdAtPage(PAGE1, "file is empty.");
		nextSequence();
		return -1;
	}

	/*read file name part start*/
	if((*p).entryInfo.extensionNameEntryCount!=0)
	{
		memset(g_strBuf.dat, 0x00, STRING_BUFFER_SIZE);

		(*searchingSecterBuffer).locatedClustor=(*p).entryInfo.longNameLocation.clustor;
		(*searchingSecterBuffer).secterInClustor=(*p).entryInfo.longNameLocation.secterInClustor;

		checkFatAndLocatNextClustor(diskInfo, searchingSecterBuffer, (*searchingSecterBuffer).locatedClustor);//if want check wrong fat table, added exception process

		readSecterInClustor(diskInfo, searchingSecterBuffer, (*searchingSecterBuffer).locatedClustor, (*searchingSecterBuffer).secterInClustor);

		str = (*searchingSecterBuffer).secterData.data + (*p).entryInfo.longNameEntryOffset;
		for(count=(*p).entryInfo.extensionNameEntryCount; count!=0; count--)
		{
			abstractDirLongNameFromDirectoryEntry(str, g_strBuf.dat);
			str+=DIR_DISCRIPTION_LENGTH;
			if( !(str<(*searchingSecterBuffer).secterData.data+SD_DATA_BUFFER_SIZE) )
			{
				(*searchingSecterBuffer).secterInClustor++;

				if( !((*searchingSecterBuffer).secterInClustor < (*diskInfo).secterPerClustor) )//finding secter is lastest secter of clustor?
				{//lastest secter of clustor. loading next clustor
					(*searchingSecterBuffer).locatedClustor=(*searchingSecterBuffer).nextClustor;
					checkFatAndLocatNextClustor(diskInfo, searchingSecterBuffer, (*searchingSecterBuffer).locatedClustor);
					(*searchingSecterBuffer).secterInClustor=0;
				}

				readSecterInClustor(diskInfo, searchingSecterBuffer, (*searchingSecterBuffer).locatedClustor, (*searchingSecterBuffer).secterInClustor);				
				str = (*searchingSecterBuffer).secterData.data;
			}
		}
	}
	else
	{
		memset(g_strBuf.dat, 0x00, STRING_BUFFER_SIZE);

		(*searchingSecterBuffer).locatedClustor=(*p).entryInfo.location.clustor;
		(*searchingSecterBuffer).secterInClustor=(*p).entryInfo.location.secterInClustor;

		readSecterInClustor(diskInfo, searchingSecterBuffer, (*searchingSecterBuffer).locatedClustor, (*searchingSecterBuffer).secterInClustor);

		str = (*searchingSecterBuffer).secterData.data + (*p).entryInfo.entryNumberOrOffset;

		for(count=0; count<8; count++)
		{
			if(*(str+count) == DIR_NAME_EMPTY_DATA) break;
			*(g_strBuf.dat+count)=*(str+count);

		}

		*(g_strBuf.dat+count)='.';
		count++;
		strncpy(g_strBuf.dat+count, str+DIR_SIMPLE_NAME_MAXIMUM_LENGTH , DIR_EXTENSION_MAXUMUM_LENGTH);//general name copy

		checkAndConvertSimpleNameStringForm(g_strBuf.dat, count+DIR_EXTENSION_MAXUMUM_LENGTH+1);
	}
	/*read file name part end*/

	/*if need file name processing, add code this. file name is loaded g_strBuf.dat. start*/
	putStringInGlcdAtPage(PAGE0+0, g_strBuf.dat);
	/*if need file name processing, add code this. file name is loaded g_strBuf.dat. end*/


	unsigned long readFileSize=0;//This variable is same to file size.

	(*searchingSecterBuffer).locatedClustor = (*p).dirStructure.otherInfo.indicateFirstClustor;
	(*searchingSecterBuffer).secterInClustor = 0;

	checkFatAndLocatNextClustor(diskInfo, searchingSecterBuffer, (*searchingSecterBuffer).locatedClustor);
	readSecterInClustor(diskInfo, searchingSecterBuffer, (*searchingSecterBuffer).locatedClustor, (*searchingSecterBuffer).secterInClustor);	

	/*if skipped data is exist, add code this. Data Info length must not exceed 512 bytes.*/
	str = (*searchingSecterBuffer).secterData.data;
	count = 0;
	while(*(str+count)=='[')
	{
		/*if need to process every non saved data, added code here.*/
		while(*(str+count-1)!=0x0a)
		{
			count++;
		}
	}
	const unsigned int dataFileInfoLength=count;
	str+=count;//pointer move
	readFileSize=count;
	count=0;

	/*
		After data is displaied, pointer is indicating end of buffer. Below is example.
		<- is pointer location.
		/cr is carriage return.
		/nl is line feed.

		before, data infomation not process.
		[<-DataLogger Finfotech]
		[Date:2014:01:24]
		000423,ADC0,1604,1663,2205,1473,2257,1088,3214,2765/cr/nl
		006897,ADC1,3939,2228,2878,0270,1831,2412,0534,3942/cr/nl
		003289,ADC0,1590,1649,2185,1460,2237,1078,3186,2736/cr/nl
		003114,ADC1,3946,2231,2881,0270,1833,2415,0535,3946/cr/nl
		006018,ADC0,1592,1651,2187,1459,2237,1079,3189,2740/cr/nl

		before, display data.
		[DataLogger Finfotech]
		[Date:2014:01:24]
		0<-00423,ADC0,1604,1663,2205,1473,2257,1088,3214,2765/cr/nl
		006897,ADC1,3939,2228,2878,0270,1831,2412,0534,3942/cr/nl
		003289,ADC0,1590,1649,2185,1460,2237,1078,3186,2736/cr/nl
		003114,ADC1,3946,2231,2881,0270,1833,2415,0535,3946/cr/nl
		006018,ADC0,1592,1651,2187,1459,2237,1079,3189,2740/cr/nl

		after 3 line data is displaied.
		[DataLogger Finfotech]
		[Date:2014:01:24]
		000423,ADC0,1604,1663,2205,1473,2257,1088,3214,2765/cr/nl
		0<-06897,ADC1,3939,2228,2878,0270,1831,2412,0534,3942/cr/nl
		003289,ADC0,1590,1649,2185,1460,2237,1078,3186,2736/cr/nl
		003114,ADC1,3946,2231,2881,0270,1833,2415,0535,3946/cr/nl
		006018,ADC0,1592,1651,2187,1459,2237,1079,3189,2740/cr/nl
	*/

	char switchBuffer = '2';

	char *displayPointer;

	do
	{
		switch(switchBuffer)
		{
			case '0'://Reverse
				/* readFileSize is same mean that fileSize. */
				/* if updated string size is longer then file size not updated at GLCD. */

				/*
				bufferDirection is in (struct stringBuffer) have any identification values,
				from identification values MCU know how to calculate buffer string length to read before data.

				buffer size (is already read from sd card) calculating.
				1. case Forward. ('F')
					0 1 2 3 4 5 6 7 8 9 10(buffer offset)
					|string start(offset 0)
					            |string end(offset 6)
								  |buffer pointer indicate point is 7(offset) and there is reserved to 0.
					To calculate string length is buffered in buffer, pointer address minus 1st buffer address(offset 0).
					And bufferPointer is in structure of [struct stringBuffer].

					if bufferDirection have unicode 'F', bufferPointer indicate lastest buffer location.

				2. case Reverse. ('R')
					0 1 2 3 4 5 6 7 8 9 10(reserved, it have 0) | 11(invalid memory region)//string length is 8.
									  |string start(offset 9), offset 10 is reserved to end of array.
					    |string end(offset 2)
						|buffer pointer indicate point is 1(offset)
					To calculate string length is buffered in buffer, 11th buffer address(offset 10) minus pointer address.
					And bufferPointer is in structure of [struct stringBuffer].

					if bufferDirection have unicode 'R', bufferPointer indicate 1st buffer location.
				*/
				if(g_strBuf.dir == 'F')
				{
					count=(g_strBuf.loc-g_strBuf.dat);
				}
				else if(g_strBuf.dir == 'R')
				{
					count=(g_strBuf.dat+STRING_BUFFER_SIZE-2)-g_strBuf.loc;
				}

				// g_strBuf.loc=g_strBuf.dat;
				/*
				classify case.
				1. no rest
					1-1. when values 0.
						clustor is zero.
						secter is zero.
						char pointer is zero.
					1-2. perfectly divide, so no rest.
				2. occur rest.
				*/

				if((count<readFileSize))/*when data buffer is indicate data.*/
				{
					readFileSize-=count;//before data set length

					if((dataFileInfoLength<readFileSize))
					{
						*(g_strBuf.dat + STRING_BUFFER_SIZE - 1) = 0;//End of char array reserved bytes.
						g_strBuf.loc = (g_strBuf.dat+STRING_BUFFER_SIZE-2);//when calculate copied string length, (g_strBuf.dat+STRING_BUFFER_SIZE-2) is reference offset in reserve.
						g_strBuf.dir='R';


						str=((*searchingSecterBuffer).secterData.data+((readFileSize-1)%((CLUSTOR_LOCATION)(*diskInfo).bytesPerSecter)));//rest
						(*searchingSecterBuffer).secterInClustor=(((readFileSize-1)/((CLUSTOR_LOCATION)(*diskInfo).bytesPerSecter))%(*diskInfo).secterPerClustor);
						(*searchingSecterBuffer).locatedClustor=findNthClustor(diskInfo, searchingSecterBuffer, (*p).dirStructure.otherInfo.indicateFirstClustor, (readFileSize-1)/(((CLUSTOR_LOCATION)(*diskInfo).secterPerClustor)*((CLUSTOR_LOCATION)(*diskInfo).bytesPerSecter)));//return number of sum of clustor - 1

						readSecterInClustor(diskInfo, searchingSecterBuffer, (*searchingSecterBuffer).locatedClustor, (*searchingSecterBuffer).secterInClustor);
						do/*pointer is move back. */
						{
							if(str<(*searchingSecterBuffer).secterData.data)
							{

								if((*searchingSecterBuffer).secterInClustor==0)//wrong??
								{
									if((*searchingSecterBuffer).locatedClustor != (*p).dirStructure.otherInfo.indicateFirstClustor)
									{
										(*searchingSecterBuffer).locatedClustor=findBeforeClustor(diskInfo, searchingSecterBuffer, (*p).dirStructure.otherInfo.indicateFirstClustor, (*searchingSecterBuffer).locatedClustor);//return number of sum of clustor - 1
										(*searchingSecterBuffer).secterInClustor=(*diskInfo).secterPerClustor-1;
									}
									else
									{
										break;
									}
								}
								else
								{
									(*searchingSecterBuffer).secterInClustor--;
								}

								readSecterInClustor(diskInfo, searchingSecterBuffer, (*searchingSecterBuffer).locatedClustor, (*searchingSecterBuffer).secterInClustor);

								str=(*searchingSecterBuffer).secterData.data+SD_DATA_BUFFER_SIZE-1;
							}

							for(;!(str<(*searchingSecterBuffer).secterData.data);)
							{
								*(g_strBuf.loc)=*(str);//copy.

								if( (g_strBuf.loc != (g_strBuf.dat + STRING_BUFFER_SIZE - 2)) && (*(g_strBuf.loc)==0x0a) )//always 0x0a is located tail of data set.
								{
									break;
								}
								else
								{
									g_strBuf.loc--;
									str--;
								}
							}



						}
						while((((*searchingSecterBuffer).locatedClustor!=(*p).dirStructure.otherInfo.indicateFirstClustor)||((*searchingSecterBuffer).secterInClustor!=0))&&(*(g_strBuf.loc)!=0x0a));//direction is reverse.

						displayPointer = (g_strBuf.loc + 1);

						break;
					}
				}

				/*when buffer pointer is indicate data info.*/
				str=(*searchingSecterBuffer).secterData.data+dataFileInfoLength;
				(*searchingSecterBuffer).locatedClustor=(*p).dirStructure.otherInfo.indicateFirstClustor;
				(*searchingSecterBuffer).secterInClustor=0;
				readFileSize=dataFileInfoLength;

			case '2'://Forward
				/*Read file size is can't exceed file size.*/
				if((*p).dirStructure.otherInfo.fileSize<=readFileSize) continue;
				/*data abstract*/
				g_strBuf.loc=g_strBuf.dat;
				g_strBuf.dir='F';


				str=(*searchingSecterBuffer).secterData.data+(readFileSize%((CLUSTOR_LOCATION)(*diskInfo).bytesPerSecter));//rest
				(*searchingSecterBuffer).secterInClustor=((readFileSize/((CLUSTOR_LOCATION)(*diskInfo).bytesPerSecter))%(*diskInfo).secterPerClustor);
				(*searchingSecterBuffer).locatedClustor=findNthClustor(diskInfo, searchingSecterBuffer, (*p).dirStructure.otherInfo.indicateFirstClustor, readFileSize/(((CLUSTOR_LOCATION)(*diskInfo).secterPerClustor)*((CLUSTOR_LOCATION)(*diskInfo).bytesPerSecter)));//return number of sum of clustor - 1

				readSecterInClustor(diskInfo, searchingSecterBuffer, (*searchingSecterBuffer).locatedClustor, (*searchingSecterBuffer).secterInClustor);				


				do////direction is forward.
				{
					//if MCU exceed 1 secter
					if(!(str<((*searchingSecterBuffer).secterData).data+SD_DATA_BUFFER_SIZE))
					{
						(*searchingSecterBuffer).secterInClustor++;

						if( !((*searchingSecterBuffer).secterInClustor < ((*diskInfo).secterPerClustor)) )//finding secter is lastest secter of clustor?
						{//lastest secter of clustor. loading next clustor
							(*searchingSecterBuffer).locatedClustor=(*searchingSecterBuffer).nextClustor;
							checkFatAndLocatNextClustor(diskInfo, searchingSecterBuffer, (*searchingSecterBuffer).locatedClustor);

							(*searchingSecterBuffer).secterInClustor=0;
						}

						readSecterInClustor(diskInfo, searchingSecterBuffer, (*searchingSecterBuffer).locatedClustor, (*searchingSecterBuffer).secterInClustor);

						str=(*searchingSecterBuffer).secterData.data;
					}


					for(;str<(((*searchingSecterBuffer).secterData).data+SD_DATA_BUFFER_SIZE);str++)
					{
						*(g_strBuf.loc)=*(str);//copy.

						if(*(g_strBuf.loc-1)==0x0a)
						{
							*(g_strBuf.loc)=0;
							readFileSize+=(g_strBuf.loc-g_strBuf.dat);
							break;
						}
						g_strBuf.loc++;
					}
				}
				while((((*searchingSecterBuffer).nextClustor!=CLUSTOR_IS_END)||((*searchingSecterBuffer).secterInClustor<(*diskInfo).secterPerClustor))&&(*(g_strBuf.loc-1)!=0x0a));//direction is forward.

				displayPointer = (g_strBuf.dat);
				break;

			default:
				continue;
		}


		sprintf(g_glcdBuf, "fileSize:0d%ld", readFileSize);
		putStringInGlcdAtPage(PAGE4, g_glcdBuf);

		count=0;
		/*classify data*/

		if(*(displayPointer+7)=='A')
		{
			analogDataDisplay(displayPointer);
		}
	}
	while((switchBuffer = nextSequence())!='4');/*variable str is must moved, so trace reverse direction, using another variable*/
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//#define SAVED_DATA_VERIFICTION_WORD	"[Finfotech Saved Data]"
const char SAVED_DATA_VERIFICTION_WORD[] = "[Finfotech Saved Data]";
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////*using file browser2222222222222 start*//////////////////////////////
void getNextEntry(fat32Info *diskInfo, clustorData *searchingSecterBuffer, directoryAndFileEntryInformation *foundDirEntryInfo)
{

	(*foundDirEntryInfo).entryInfo.entryNumberOrOffset+=DIR_DISCRIPTION_LENGTH;
	if((*foundDirEntryInfo).entryInfo.entryNumberOrOffset < SD_DATA_BUFFER_SIZE)
	{
		return;
	}
	else
	{
		(*foundDirEntryInfo).entryInfo.entryNumberOrOffset=0;
	}

	(*foundDirEntryInfo).entryInfo.location.secterInClustor++;
	if( (*searchingSecterBuffer).secterInClustor < ((*diskInfo).secterPerClustor) )
	{
		return;
	}
	else
	{
		(*searchingSecterBuffer).secterInClustor=0;
	}
	
	(*foundDirEntryInfo).entryInfo.location.clustor = (*searchingSecterBuffer).nextClustor;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void displayOneEntryComponent(unsigned char index, directoryStructure *dirStructure)
{
	unsigned char columnOffsetBuffer=FONT_5X8_OFFSET*2;
	//dir or file display

	if( ((*dirStructure).otherInfo.attribute&ATTR_DIR_FILE_MASK) ==  ATTR_DIRECTORY )
	{
		putStringInGlcdAtPage(PAGE0+index, "D:");
	}
	else if( ((*dirStructure).otherInfo.attribute&ATTR_DIR_FILE_MASK) ==  ATTR_ARCHIVE )
	{
		putStringInGlcdAtPage(PAGE0+index, "F:");		
	}
	else
	{
		putStringInGlcdAtPage(PAGE0+index, "  ");
	}
	
	if(strlen((*dirStructure).dirName.fullName) != 0)
	{
		putStringInGlcdAtPageUsingOffset(index, (*dirStructure).dirName.fullName, columnOffsetBuffer);//FONT_5X8_OFFSET*2;
	}
	else
	{
		putStringInGlcdAtPageUsingOffset(index, (*dirStructure).dirName.simple, columnOffsetBuffer);
		columnOffsetBuffer+=FONT_5X8_OFFSET*strlen((*dirStructure).dirName.simple);
		
		if(strlen((*dirStructure).dirName.extension) !=0)
		{
			putStringInGlcdAtPageUsingOffset(index, ".", columnOffsetBuffer);
			columnOffsetBuffer+=FONT_5X8_OFFSET;
			
			putStringInGlcdAtPageUsingOffset(index, (*dirStructure).dirName.extension, columnOffsetBuffer);
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
char displayAllLine(fat32Info *diskInfo, clustorData *searchingSecterBuffer, simpleDirStructureExtension *pageInfo)
{
	if((*pageInfo).validPageNumber < 0)
	{
		(*pageInfo).validPageNumber=0;
	}

	(*searchingSecterBuffer).locatedClustor=(*pageInfo).firstDirEntryInfo.location.clustor;
	(*searchingSecterBuffer).secterInClustor=(*pageInfo).firstDirEntryInfo.location.secterInClustor;
	
	
	char findPageNumber=0;
	char findLineNumber=0;	
	char *str;
	unsigned char i;
	

	checkFatAndLocatNextClustor(diskInfo, searchingSecterBuffer, (*searchingSecterBuffer).locatedClustor);//if want check wrong fat table, added exception process

	do
	{
		if( !((*searchingSecterBuffer).secterInClustor < (*diskInfo).secterPerClustor) )//finding secter is lastest secter of clustor?
		{//lastest secter of clustor. loading next clustor
			(*searchingSecterBuffer).locatedClustor=(*searchingSecterBuffer).nextClustor;
			checkFatAndLocatNextClustor(diskInfo, searchingSecterBuffer, (*searchingSecterBuffer).locatedClustor);
			(*searchingSecterBuffer).secterInClustor=0;
		}		


		readSecterInClustor(diskInfo, searchingSecterBuffer, (*searchingSecterBuffer).locatedClustor, (*searchingSecterBuffer).secterInClustor);


		for(str=(*searchingSecterBuffer).secterData.data; str<(*searchingSecterBuffer).secterData.data+SD_DATA_BUFFER_SIZE; str+=DIR_DISCRIPTION_LENGTH)
		{
			if((*str==DIR_DELEDTED)||(*str==DIR_EMPTY))//deleted or empty entry
			{
				(*((*pageInfo).dirEntry+findLineNumber)).entryInfo.longNameLocation.clustor=0;
				(*((*pageInfo).dirEntry+findLineNumber)).entryInfo.longNameLocation.secterInClustor=-1;
				(*((*pageInfo).dirEntry+findLineNumber)).entryInfo.longNameEntryOffset=-1;
				
				(*((*pageInfo).dirEntry+findLineNumber)).entryInfo.extensionNameEntryCount=0;
				continue;
			}
			else if(*( (*pageInfo).findEntry.dirStructure.dirName.fullName+LONG_NAME_MAXIMUM_LENGTH)!=0)//if exceed maximum length of displaied string, passed entry.
			{
				(*( (*pageInfo).findEntry.dirStructure.dirName.fullName+LONG_NAME_MAXIMUM_LENGTH))--;
				continue;
			}
			else if( ( (*(str+DIR_ATTR_OFFSET)) & ATTRIBUTE_MASK ) == ATTRIBUTE_MASK )//encountered long name entry.
			{
				if(findPageNumber<(*pageInfo).validPageNumber)
				{//*(longName+LONG_NAME_MAXIMUM_LENGTH) = ((*(dirEntry))&LONG_NAME_NUMBER_MASK)-1-LONG_NAME_ENTRY_MAXIMUM_NUMBER;
					*((*pageInfo).findEntry.dirStructure.dirName.fullName+LONG_NAME_MAXIMUM_LENGTH) = (((*str)&LONG_NAME_NUMBER_MASK)-1);//to skip finding.
				}
				else
				{
					if( ( (*(str)) & LONG_NAME_LASTEST_MASK ) == LONG_NAME_LASTEST_VALID_VALUE )//save first long name entry location info.
					{
						(*((*pageInfo).dirEntry+findLineNumber)).entryInfo.longNameLocation.clustor=(*searchingSecterBuffer).locatedClustor;
						(*((*pageInfo).dirEntry+findLineNumber)).entryInfo.longNameLocation.secterInClustor=(*searchingSecterBuffer).secterInClustor;
						(*((*pageInfo).dirEntry+findLineNumber)).entryInfo.longNameEntryOffset=str-(*searchingSecterBuffer).secterData.data;
						
						(*((*pageInfo).dirEntry+findLineNumber)).entryInfo.extensionNameEntryCount=(*(str)&LONG_NAME_NUMBER_MASK);
					}
				
					/* abstractDirLongNameFromDirectoryEntry is return -1 when long name entry number is exceed maximum displaied string.
					Finally, skip entry that exceeded entry, that can't present. */
					abstractDirLongNameFromDirectoryEntry(str, (*pageInfo).findEntry.dirStructure.dirName.fullName);
				}
				continue;
			}

			
			if(findPageNumber<(*pageInfo).validPageNumber)
			{
				findLineNumber++;
				if(!(findLineNumber<DIR_DISPLAY_NUMBER))
				{
					findPageNumber++;
					findLineNumber=0;
				}
				continue;
			}
			//different name is filtered above condition!
			dirSimpleNameAbstractFromDirectoryEntry(str, &((*pageInfo).findEntry.dirStructure.dirName));

			/*!!!!if varing lastest access time, added function that abstract lastest access time from dir entry, this location...*/
			dirOtherInfoAbstractFromDirectoryEntry(str, &((*((*pageInfo).dirEntry+findLineNumber)).dirStructure.otherInfo) );
			dirOtherInfoAbstractFromDirectoryEntry(str, &((*pageInfo).findEntry.dirStructure.otherInfo) );

			(*((*pageInfo).dirEntry+findLineNumber)).entryInfo.entryNumberOrOffset=str-(*searchingSecterBuffer).secterData.data;

			(*((*pageInfo).dirEntry+findLineNumber)).entryInfo.location.clustor=(*searchingSecterBuffer).locatedClustor;

			(*((*pageInfo).dirEntry+findLineNumber)).entryInfo.location.secterInClustor=(*searchingSecterBuffer).secterInClustor;

			displayOneEntryComponent(findLineNumber, &((*pageInfo).findEntry.dirStructure));

			//reset findEntry info start//
			*((*pageInfo).findEntry.dirStructure.dirName.simple)=0;
			*((*pageInfo).findEntry.dirStructure.dirName.extension)=0;
			*((*pageInfo).findEntry.dirStructure.dirName.fullName)=0;
			(*pageInfo).findEntry.dirStructure.otherInfo.attribute=0;
			//reset findEntry info end//
			
			findLineNumber++;
			
			if(!(findLineNumber<DIR_DISPLAY_NUMBER))
			{
				(*pageInfo).validPageNumber = findPageNumber;
				(*pageInfo).lastLineNumber = findLineNumber;
				return 0;
			}
		}
		(*searchingSecterBuffer).secterInClustor++;
	}
	while( ((*searchingSecterBuffer).nextClustor != CLUSTOR_IS_END) || ((*searchingSecterBuffer).secterInClustor<((*diskInfo).secterPerClustor)) );
	(*pageInfo).findEntry.entryInfo.location.secterInClustor=(*diskInfo).secterPerClustor-1;

	/*there is last page of dir list.*/		
	//blank dir, print blank start//	
	(*pageInfo).lastLineNumber = findLineNumber;
	
	while(findLineNumber < DIR_DISPLAY_NUMBER)
	{
		(*((*pageInfo).dirEntry+findLineNumber)).dirStructure.otherInfo.indicateFirstClustor=0;
		(*((*pageInfo).dirEntry+findLineNumber)).entryInfo.entryNumberOrOffset=-1;
		(*((*pageInfo).dirEntry+findLineNumber)).entryInfo.location.clustor=0;
		(*((*pageInfo).dirEntry+findLineNumber)).entryInfo.location.secterInClustor=-1;

		displayOneEntryComponent(findLineNumber, &((*pageInfo).findEntry.dirStructure));

		//reset extension entry info.
		(*((*pageInfo).dirEntry+findLineNumber)).entryInfo.longNameLocation.clustor=-1;
		(*((*pageInfo).dirEntry+findLineNumber)).entryInfo.longNameLocation.secterInClustor=-1;
		(*((*pageInfo).dirEntry+findLineNumber)).entryInfo.longNameEntryOffset=-1;
		
		(*((*pageInfo).dirEntry+findLineNumber)).entryInfo.extensionNameEntryCount=0;
		
		findLineNumber++;
	}
	(*pageInfo).validPageNumber=findPageNumber;	
	//blank dir, print blank end//
	//if target is not found, (*pageInfo).findEntry.entryInfo.location.clustor is indicate lastest clustor, that is found by this function.
	return -1;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void testDisplayIndicateClustorInfo(simpleDirectoryAndFileEntryInfomation *dirEntry)
{
	unsigned char i;
	for(i=0; i<DIR_DISPLAY_NUMBER; i++)
	{
		sprintf(g_glcdBuf, "0x%lx", (*(dirEntry+i)).dirStructure.otherInfo.indicateFirstClustor);
		putStringInGlcdAtPageUsingOffset(i, g_glcdBuf, FONT_5X8_OFFSET*15);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void fileBrowser(fat32Info *diskInfo, clustorData *searchingSecterBuffer, simpleDirStructureExtension *browserData)
{
	memset(&fileBrowserData, 0x00, sizeof(simpleDirStructureExtension));

	(*browserData).firstDirEntryInfo.location.clustor = sdCardInfo.rootClustor;
	(*browserData).firstDirEntryInfo.location.secterInClustor = 0;
	(*browserData).firstDirEntryInfo.entryNumberOrOffset = 0;

	(*browserData).validPageNumber=0;
	(*browserData).selectLineNumber=0;//valid range 0~7, 1st(top) line is 0, 7th(bottom) line is 7
	signed int pageNumberBuffer=0;
	signed char lineNumberBuffer=0;
	char switchBuffer;

	
	displayAllLine(diskInfo, searchingSecterBuffer, browserData);

	reversePage((*browserData).selectLineNumber);

	while((switchBuffer=nextSequence()))
	{
		lineNumberBuffer=(*browserData).selectLineNumber;
		pageNumberBuffer=(*browserData).validPageNumber;
		switch(switchBuffer)//Which button is pressed?
		{
			case '1'://down
				(*browserData).selectLineNumber++;
				break;
			
			case '5'://up
				(*browserData).selectLineNumber--;
				break;
			case '0':
			case '2':
				/*find clustor...*/
				if((*((*browserData).dirEntry+(*browserData).selectLineNumber)).dirStructure.otherInfo.attribute == ATTR_DIRECTORY)//dir
				{
					if( ( (*((*browserData).dirEntry+(*browserData).selectLineNumber)).dirStructure.otherInfo.indicateFirstClustor < 2 ) || ( (*((*browserData).dirEntry+(*browserData).selectLineNumber)).dirStructure.otherInfo.indicateFirstClustor > (CLUSTOR_IS_END-1) ) )
					{
						(*browserData).firstDirEntryInfo.location.clustor = sdCardInfo.rootClustor;
					}
					else if( ( (*((*browserData).dirEntry+(*browserData).selectLineNumber)).dirStructure.otherInfo.indicateFirstClustor ==  (*browserData).firstDirEntryInfo.location.clustor ) )
					{
						(*browserData).firstDirEntryInfo.location.secterInClustor = 0;
					}
					else
					{
						(*browserData).firstDirEntryInfo.location.clustor = (*((*browserData).dirEntry+(*browserData).selectLineNumber)).dirStructure.otherInfo.indicateFirstClustor;
						(*browserData).firstDirEntryInfo.location.secterInClustor = 0;
					}
				
					(*browserData).validPageNumber=0;
					(*browserData).selectLineNumber=0;//valid range 0~7, 1st(top) line is 0, 7th(bottom) line is 7
					displayAllLine(diskInfo, searchingSecterBuffer, browserData);

					reversePage((*browserData).selectLineNumber);
					continue;
				}
				else if((*((*browserData).dirEntry+(*browserData).selectLineNumber)).dirStructure.otherInfo.attribute == ATTR_ARCHIVE)//file
				{
					savedDataFileInfoParseFromSectorInClustor(diskInfo, searchingSecterBuffer, (*browserData).dirEntry+(*browserData).selectLineNumber, (*((*browserData).dirEntry+(*browserData).selectLineNumber)).dirStructure.otherInfo.indicateFirstClustor);

					displayAllLine(diskInfo, searchingSecterBuffer, browserData);//display reset(reverse line)
					reversePage((*browserData).selectLineNumber);
					continue;
				}
				else//other...
				{
					(*browserData).firstDirEntryInfo.location.clustor = sdCardInfo.rootClustor;
					
					(*browserData).validPageNumber=0;
					(*browserData).selectLineNumber=0;//valid range 0~7, 1st(top) line is 0, 7th(bottom) line is 7
					displayAllLine(diskInfo, searchingSecterBuffer, browserData);

					reversePage((*browserData).selectLineNumber);
					continue;
				}
				
			default:
				continue;
		}

		
		if((*browserData).selectLineNumber < 0)//page up
		{
			(*browserData).validPageNumber--;
			displayAllLine(diskInfo, searchingSecterBuffer, browserData);//display reset(reverse line)

			
			if((pageNumberBuffer - (*browserData).validPageNumber) !=0 )//move page
			{
				(*browserData).selectLineNumber=DIR_DISPLAY_NUMBER-1;
			}
			else//top page
			{
				(*browserData).selectLineNumber=0;
			}
			reversePage((*browserData).selectLineNumber);
		}
		else if(!((*browserData).selectLineNumber<(*browserData).lastLineNumber))//page down
		{
			(*browserData).validPageNumber++;
			displayAllLine(diskInfo, searchingSecterBuffer, browserData);//display reset(reverse line)
			
			if((pageNumberBuffer - (*browserData).validPageNumber) !=0 )//move page
			{
				(*browserData).selectLineNumber=0;
			}
			else//bottom page
			{
				(*browserData).selectLineNumber=(*browserData).lastLineNumber-1;
			}
			
			if((*browserData).selectLineNumber==((*browserData).lastLineNumber-1))
			{
				reversePage((*browserData).selectLineNumber);
			}
			reversePage((*browserData).selectLineNumber);
		}
		else if( (lineNumberBuffer-(*browserData).selectLineNumber) !=0)//samePage
		{
			reversePage(lineNumberBuffer);
			reversePage((*browserData).selectLineNumber);
			continue;
		}
	}
}
//////////////////////////////*using file browser2222222222222 end*//////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define MAXIMUM_CMD_ARGS_NUM	5
#define CMD_ARGC_SPACE	' '
#define CMD_ARGC_LINEFEED	'\r'
#define CMD_ARGC_RETURN	'\n'
void cmdArgsLocation(char **argc_l, char *cmd)
{
	char i=0;
	
	*(argc_l+i++)=cmd;
	while(((*cmd)!=CMD_ARGC_LINEFEED)||((*cmd)!=CMD_ARGC_RETURN))
	{
		if(!(i<MAXIMUM_CMD_ARGS_NUM))
		{
			*cmd=0;
			return;
		}
		if(*cmd==0)
		{
			for(;i<MAXIMUM_CMD_ARGS_NUM;i++)
			{
				*(argc_l+i)=cmd;
			}
			return;
		}
		else if(((*cmd)==CMD_ARGC_LINEFEED)||((*cmd)==CMD_ARGC_RETURN))
		{
			(*cmd)=0;
			for(;i<MAXIMUM_CMD_ARGS_NUM;i++)
			{
				*(argc_l+i)=cmd;
			}
			return;
		}
		else if((*cmd)==CMD_ARGC_SPACE)
		{
			(*cmd)=0;
			cmd++;
			*(argc_l+i)=cmd;
			i++;
		}
		else
		{
			cmd++;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
struct stringBuffer
{
	char buffer[STRING_BUFFER_SIZE];
	char *bufferPointer;
	char bufferDirection;
}typedef stringBuffer;
*/
char sendDirEntryInfoToUsart0(directoryAndFileEntryInformation *findEntryBuffer)
{
	if(((*findEntryBuffer).dirStructure.otherInfo.attribute&ATTR_DIRECTORY)==ATTR_DIRECTORY)
	{
		sendStringOnly("d ");
	}
	else
	{
		sendStringOnly("- ");
	}
	
	sprintf(g_strBuf.dat, "%10ld ", (*findEntryBuffer).dirStructure.otherInfo.fileSize);
	sendStringOnly(g_strBuf.dat);
	
	sprintf(g_strBuf.dat, "WT %4d/%2d/%2d,%2d:%2d:%2d ", (*findEntryBuffer).dirStructure.writeDateInfo.date.year+1980, (*findEntryBuffer).dirStructure.writeDateInfo.date.month, (*findEntryBuffer).dirStructure.writeDateInfo.date.day, (*findEntryBuffer).dirStructure.writeDateInfo.time.hour, (*findEntryBuffer).dirStructure.writeDateInfo.time.minute, (*findEntryBuffer).dirStructure.writeDateInfo.time.second);
	sendStringOnly(g_strBuf.dat);

	sprintf(g_strBuf.dat, "C.N:0x%000000008x ", (*findEntryBuffer).dirStructure.otherInfo.indicateFirstClustor);
	sendStringOnly(g_strBuf.dat);
	
	if((*findEntryBuffer).entryInfo.extensionNameEntryCount==0)
	{
		// sendStringOnly(" S:");
		sendStringOnly((*findEntryBuffer).dirStructure.dirName.simple);
		if(*((*findEntryBuffer).dirStructure.dirName.extension)!=0)
		{
			sendCharOnly('.');
			sendStringOnly((*findEntryBuffer).dirStructure.dirName.extension);

		}
	}
	else
	{
		// sprintf(g_strBuf.dat, " L%d:", (*findEntryBuffer).entryInfo.extensionNameEntryCount);
		// sendStringOnly(g_strBuf.dat);
		sendStringOnly((*findEntryBuffer).dirStructure.dirName.fullName);
	}
	SEND_COMMON();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
char cmdDisplayDirList(fat32Info *diskInfo, clustorData *searchingSecterBuffer, directoryAndFileEntryInformation *findEntryBuffer, CLUSTOR_LOCATION cmdClustorLocation)
{
	char *str;

	(*searchingSecterBuffer).locatedClustor=cmdClustorLocation;
	(*searchingSecterBuffer).secterInClustor=0;

	
	*((*findEntryBuffer).dirStructure.dirName.simple)=0;
	*((*findEntryBuffer).dirStructure.dirName.extension)=0;
	*((*findEntryBuffer).dirStructure.dirName.fullName)=0;

	(*findEntryBuffer).entryInfo.longNameLocation.clustor=0;
	(*findEntryBuffer).entryInfo.longNameLocation.secterInClustor=-1;
	(*findEntryBuffer).entryInfo.longNameEntryOffset=-1;
	
	(*findEntryBuffer).entryInfo.extensionNameEntryCount=0;


	checkFatAndLocatNextClustor(diskInfo, searchingSecterBuffer, (*searchingSecterBuffer).locatedClustor);//if want check wrong fat table, added exception process

	do
	{
		if( !((*searchingSecterBuffer).secterInClustor < (*diskInfo).secterPerClustor) )//finding secter is lastest secter of clustor?
		{//lastest secter of clustor. loading next clustor
			(*searchingSecterBuffer).locatedClustor=(*searchingSecterBuffer).nextClustor;
			checkFatAndLocatNextClustor(diskInfo, searchingSecterBuffer, (*searchingSecterBuffer).locatedClustor);
			(*searchingSecterBuffer).secterInClustor=0;
		}		


		readSecterInClustor(diskInfo, searchingSecterBuffer, (*searchingSecterBuffer).locatedClustor, (*searchingSecterBuffer).secterInClustor);


		for(str=(*searchingSecterBuffer).secterData.data; str<(*searchingSecterBuffer).secterData.data+SD_DATA_BUFFER_SIZE; str+=DIR_DISCRIPTION_LENGTH)
		{
			if((*str==DIR_DELEDTED)||(*str==DIR_EMPTY))//deleted or empty entry
			{
				(*findEntryBuffer).entryInfo.longNameLocation.clustor=0;
				(*findEntryBuffer).entryInfo.longNameLocation.secterInClustor=-1;
				(*findEntryBuffer).entryInfo.longNameEntryOffset=-1;
				
				(*findEntryBuffer).entryInfo.extensionNameEntryCount=0;
				continue;
			}
			else if(*( (*findEntryBuffer).dirStructure.dirName.fullName+LONG_NAME_MAXIMUM_LENGTH)!=0)//if exceed maximum length of displaied string, passed entry.
			{
				(*( (*findEntryBuffer).dirStructure.dirName.fullName+LONG_NAME_MAXIMUM_LENGTH))--;
				continue;
			}
			else if( ( (*(str+DIR_ATTR_OFFSET)) & ATTRIBUTE_MASK ) == ATTRIBUTE_MASK )//encountered long name entry.
			{
				if( ( (*(str)) & LONG_NAME_LASTEST_MASK ) == LONG_NAME_LASTEST_VALID_VALUE )//save first long name entry location info.
				{
					(*findEntryBuffer).entryInfo.longNameLocation.clustor=(*searchingSecterBuffer).locatedClustor;
					(*findEntryBuffer).entryInfo.longNameLocation.secterInClustor=(*searchingSecterBuffer).secterInClustor;
					(*findEntryBuffer).entryInfo.longNameEntryOffset=str-(*searchingSecterBuffer).secterData.data;
					
					(*findEntryBuffer).entryInfo.extensionNameEntryCount=(*(str)&LONG_NAME_NUMBER_MASK);
				}
			
				/* abstractDirLongNameFromDirectoryEntry is return -1 when long name entry number is exceed maximum displaied string.
				Finally, skip entry that exceeded entry, that can't present. */
				abstractDirLongNameFromDirectoryEntry(str, (*findEntryBuffer).dirStructure.dirName.fullName);//if exc

				continue;
			}

			
			//different name is filtered above condition!
			dirSimpleNameAbstractFromDirectoryEntry(str, &((*findEntryBuffer).dirStructure.dirName));

			/*!!!!if varing lastest access time, added function that abstract lastest access time from dir entry, this location...*/
			dirOtherInfoAbstractFromDirectoryEntry(str, &((*findEntryBuffer).dirStructure.otherInfo) );
			dirOtherInfoAbstractFromDirectoryEntry(str, &((*findEntryBuffer).dirStructure.otherInfo) );

			dirDateAndTimeInfoParseFromDirectoryEntry(str, &((*findEntryBuffer).dirStructure));
			dirDateAndTimeInfoParseFromDirectoryEntry(str, &((*findEntryBuffer).dirStructure));
			
			(*findEntryBuffer).entryInfo.entryNumberOrOffset=str-(*searchingSecterBuffer).secterData.data;
			(*findEntryBuffer).entryInfo.location.clustor=(*searchingSecterBuffer).locatedClustor;
			(*findEntryBuffer).entryInfo.location.secterInClustor=(*searchingSecterBuffer).secterInClustor;

			
			sendDirEntryInfoToUsart0(findEntryBuffer);

			
			//reset findEntry info start//
			*((*findEntryBuffer).dirStructure.dirName.simple)=0;
			*((*findEntryBuffer).dirStructure.dirName.extension)=0;
			*((*findEntryBuffer).dirStructure.dirName.fullName)=0;
			(*findEntryBuffer).dirStructure.otherInfo.attribute=0;
			(*findEntryBuffer).entryInfo.extensionNameEntryCount=0;
			//reset findEntry info end//		
		}
		(*searchingSecterBuffer).secterInClustor++;
	}
	while( ((*searchingSecterBuffer).nextClustor != CLUSTOR_IS_END) || ((*searchingSecterBuffer).secterInClustor<((*diskInfo).secterPerClustor)) );
	(*findEntryBuffer).entryInfo.location.secterInClustor=(*diskInfo).secterPerClustor-1;

	return -1;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
char cmdDisplayArchive(fat32Info *diskInfo, clustorData *searchingSecterBuffer, directoryAndFileEntryInformation *foundFileEntryInfo)
{
	if((*foundFileEntryInfo).dirStructure.otherInfo.indicateFirstClustor==0) return -1;
	if((*foundFileEntryInfo).dirStructure.otherInfo.fileSize==0) return -2;
	if(((*foundFileEntryInfo).dirStructure.otherInfo.attribute&ATTR_ARCHIVE)!=ATTR_ARCHIVE) return -3;
	
	char *str;
	unsigned long readFileSize=0;
	unsigned long maximumFileSize=(*foundFileEntryInfo).dirStructure.otherInfo.fileSize;
	(*searchingSecterBuffer).locatedClustor=(*foundFileEntryInfo).dirStructure.otherInfo.indicateFirstClustor;
	(*searchingSecterBuffer).secterInClustor=0;


	checkFatAndLocatNextClustor(diskInfo, searchingSecterBuffer, (*searchingSecterBuffer).locatedClustor);//if want check wrong fat table, added exception process

				sendString("----- FILE INFORMATION -----");
				sendDirectoryAndFileEntryInfomation1(foundFileEntryInfo);
				sendDirectoryAndFileEntryInfomation2(foundFileEntryInfo);
				sendDirectoryAndFileEntryInfomation3(foundFileEntryInfo);
				sendString("----- FILE VEIW IS START -----");
	
	do
	{
		if( !((*searchingSecterBuffer).secterInClustor < (*diskInfo).secterPerClustor) )//finding secter is lastest secter of clustor?
		{//lastest secter of clustor. loading next clustor
			(*searchingSecterBuffer).locatedClustor=(*searchingSecterBuffer).nextClustor;
			checkFatAndLocatNextClustor(diskInfo, searchingSecterBuffer, (*searchingSecterBuffer).locatedClustor);
			(*searchingSecterBuffer).secterInClustor=0;
		}		


		readSecterInClustor(diskInfo, searchingSecterBuffer, (*searchingSecterBuffer).locatedClustor, (*searchingSecterBuffer).secterInClustor);

		for(str=(*searchingSecterBuffer).secterData.data; str<(*searchingSecterBuffer).secterData.data+SD_DATA_BUFFER_SIZE; str++)
		{
			if(readFileSize==maximumFileSize)
			{
				sendString("----- FILE VEIW IS END -----");
				return 0;
			}
			
			sendCharOnly((*str));
			readFileSize++;
		}
		(*searchingSecterBuffer).secterInClustor++;
	}
	while( ((*searchingSecterBuffer).nextClustor != CLUSTOR_IS_END) || ((*searchingSecterBuffer).secterInClustor<((*diskInfo).secterPerClustor)) );

	return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void sendDirectoryAndFileEntryInfomation1(directoryAndFileEntryInformation *dirEntryInfo)
{
		// sprintf(g_strBuf.dat, "S.N:%002x|%002x|%002x|%002x|%002x|%002x|%002x|%002x, E:%002x|%002x|%002x", *((*dirEntryInfo).dirStructure.dirName.simple+0), *((*dirEntryInfo).dirStructure.dirName.simple+1), *((*dirEntryInfo).dirStructure.dirName.simple+2), *((*dirEntryInfo).dirStructure.dirName.simple+3), *((*dirEntryInfo).dirStructure.dirName.simple+4), *((*dirEntryInfo).dirStructure.dirName.simple+5), *((*dirEntryInfo).dirStructure.dirName.simple+6), *((*dirEntryInfo).dirStructure.dirName.simple+7), *((*dirEntryInfo).dirStructure.dirName.extension+0), *((*dirEntryInfo).dirStructure.dirName.extension+1), *((*dirEntryInfo).dirStructure.dirName.extension+2));
		// sendString(g_strBuf.dat);

		// sprintf(g_strBuf.dat, "S.N:%c|%c|%c|%c|%c|%c|%c|%c, E:%c|%c|%c", *((*dirEntryInfo).dirStructure.dirName.simple+0), *((*dirEntryInfo).dirStructure.dirName.simple+1), *((*dirEntryInfo).dirStructure.dirName.simple+2), *((*dirEntryInfo).dirStructure.dirName.simple+3), *((*dirEntryInfo).dirStructure.dirName.simple+4), *((*dirEntryInfo).dirStructure.dirName.simple+5), *((*dirEntryInfo).dirStructure.dirName.simple+6), *((*dirEntryInfo).dirStructure.dirName.simple+7), *((*dirEntryInfo).dirStructure.dirName.extension+0), *((*dirEntryInfo).dirStructure.dirName.extension+1), *((*dirEntryInfo).dirStructure.dirName.extension+2));
		// sendString(g_strBuf.dat);
		
		sprintf(g_strBuf.dat, "S.N:%s, E:%s", (*dirEntryInfo).dirStructure.dirName.simple, (*dirEntryInfo).dirStructure.dirName.extension);
		sendString(g_strBuf.dat);

		sprintf(g_strBuf.dat, "LongName:%s",(*dirEntryInfo).dirStructure.dirName.fullName);
		sendString(g_strBuf.dat);

		sprintf(g_strBuf.dat, "CheckSumS:0x%002x, LongNameEntryNum:%002d", (*dirEntryInfo).entryInfo.extensionNameChkSum, (*dirEntryInfo).entryInfo.extensionNameEntryCount);
		sendString(g_strBuf.dat);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void sendDirectoryAndFileEntryInfomation2(directoryAndFileEntryInformation *dirEntryInfo)
{		sprintf(g_strBuf.dat, "simple Lo: clustor:0x%000000008lx, secter 0d%002d, entryOffset:0x%002x", (*dirEntryInfo).entryInfo.location.clustor,  (*dirEntryInfo).entryInfo.location.secterInClustor, (*dirEntryInfo).entryInfo.entryNumberOrOffset);
		sendString(g_strBuf.dat);

		sprintf(g_strBuf.dat, "size 0x%000000008lx, attribute 0x%002x, entry indicate:0x%000000008lx", (*dirEntryInfo).dirStructure.otherInfo.fileSize, (*dirEntryInfo).dirStructure.otherInfo.attribute, (*dirEntryInfo).dirStructure.otherInfo.indicateFirstClustor);
		sendString(g_strBuf.dat);
		
		//sprintf(buffer ,"empty C.L 0x%000000008lx", findEmptyClustor(&sdCardInfo, &clustor, (*dirEntryInfo).dirStructure.otherInfo.indicateFirstClustor));
		//putStringInGlcdAtPage(PAGE7, buffer);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void sendDirectoryAndFileEntryInfomation3(directoryAndFileEntryInformation *dirEntryInfo)
{
		sprintf(g_strBuf.dat, "Long Loc: Clustor:0x%000000008lx, secter:0d%002d, entryOffset:0x%002x", (*dirEntryInfo).entryInfo.longNameLocation.clustor, (*dirEntryInfo).entryInfo.longNameLocation.secterInClustor, (*dirEntryInfo).entryInfo.longNameEntryOffset);
		sendString(g_strBuf.dat);

		//sprintf(buffer ,"empty C.L 0x%000000008lx", findEmptyClustor(&sdCardInfo, &clustor, (*dirEntryInfo).dirStructure.otherInfo.indicateFirstClustor));
		//putStringInGlcdAtPage(PAGE7, buffer);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CMD_BUFFER_SIZE	257
char cmdStringNowLocationName[27]={0};
char cmd[CMD_BUFFER_SIZE]={0};

char *cmdArgs[MAXIMUM_CMD_ARGS_NUM]={0};
//char mkdirDirectoryBuffer[CMD_BUFFER_SIZE]={0};


CLUSTOR_LOCATION cmdClustorLocation;
char cmdResult=0;

int main()
{
	signed char resultBuffer = 0;

	CLUSTOR_LOCATION targetClustor;
	char fileName[31]={0};
	
/* GLCD INIT start */
	initGlcd();
	resetGlcd();
	_delay_ms(1);
/* GLCD INIT end */

	initUsart0();
	sendString("Usart Initializing...");

	displayDs1302ReadTime();
	nextSequence();

	SPI_Master_Init();
	sendString("SPI Master Initializing...");

	SPI_SD_CARD_Init(SPI_MODE_SD_CARD);
	sendString("SD Card Initializing...");

	findBootSecter(&sdCardInfo, &(clustor.secterData));



	strcpy(cmdStringNowLocationName, "/");
	cmdClustorLocation=sdCardInfo.rootClustor;

	unsigned char i=0;
	while(1)
	{

		sendStringOnly("Path:");
		sendString(cmdStringNowLocationName);
		sprintf(g_strBuf.dat, "Clus:0x%lx", cmdClustorLocation);
		sendString(g_strBuf.dat);
		sendStringOnly("$ ");

		receiveString(cmd);
		putStringInGlcdAtPage(PAGE2, cmd);
		sendStringOnly(cmd);

		cmdArgsLocation(cmdArgs, cmd);
		
		for(i=0; i<MAXIMUM_CMD_ARGS_NUM; i++)
		{
			sprintf(g_strBuf.dat,"args[%d]:", i);
			sendStringOnly(g_strBuf.dat);
			sendStringOnly((*(cmdArgs+i)));
			sendCharOnly(' ');
			
		}
		sendCharOnly('\r');
		sendCharOnly('\n');
		
		if(!strcmp(*(cmdArgs+0), "ls"))
		{
			cmdDisplayDirList(&sdCardInfo, &clustor, &(fileBrowserData.findEntry), cmdClustorLocation);
		}
		else if(!strcmp(*(cmdArgs+0), "cd"))
		{
			if(strlen(*(cmdArgs+1))!=0)
			{
				if((cmdResult=findDirEntryUsingName(&sdCardInfo, &clustor, &(fileBrowserData.findEntry), cmdClustorLocation, *(cmdArgs+1))))
				{
					sprintf(g_strBuf.dat, "%d:", cmdResult);
					sendStringOnly(g_strBuf.dat);
					sendString("directory is not found!");
					// sendDirectoryAndFileEntryInfomation1(&(fileBrowserData.findEntry));
					// sendDirectoryAndFileEntryInfomation2(&(fileBrowserData.findEntry));
					// sendDirectoryAndFileEntryInfomation3(&(fileBrowserData.findEntry));

				}
				else
				{
											// sendString(" - found directory info - ");
											// sendDirectoryAndFileEntryInfomation1(&(fileBrowserData.findEntry));
											// sendDirectoryAndFileEntryInfomation2(&(fileBrowserData.findEntry));
											// sendDirectoryAndFileEntryInfomation3(&(fileBrowserData.findEntry));
											// sprintf(g_strBuf.dat, "BeBefClus:0x%lx, %lx", cmdClustorLocation, (fileBrowserData.findEntry).dirStructure.otherInfo.indicateFirstClustor);
											// sendString(g_strBuf.dat);
					// if(!strcmp(*(cmdArgs+1), "."))
					// {
						// if((fileBrowserData.findEntry).dirStructure.otherInfo.indicateFirstClustor)
						// {
							// cmdClustorLocation=(fileBrowserData.findEntry).dirStructure.otherInfo.indicateFirstClustor;
							// strcpy(cmdStringNowLocationName, *(cmdArgs+1));
						// }
						// else
						// {
							// cmdClustorLocation=(sdCardInfo).rootClustor;
							// strcpy(cmdStringNowLocationName, "/");
						// }
					// }
					// else if(!strcmp(*(cmdArgs+1), ".."))
					// {
						// if((fileBrowserData.findEntry).dirStructure.otherInfo.indicateFirstClustor)
						// {
							// cmdClustorLocation=(fileBrowserData.findEntry).dirStructure.otherInfo.indicateFirstClustor;
							// strcpy(cmdStringNowLocationName, *(cmdArgs+1));
						// }
						// else
						// {
							// cmdClustorLocation=(sdCardInfo).rootClustor;
							// strcpy(cmdStringNowLocationName, "/");
						// }
					// }
					// else if(((fileBrowserData.findEntry).dirStructure.otherInfo.attribute&ATTR_DIRECTORY)!=ATTR_DIRECTORY)
					if(((fileBrowserData.findEntry).dirStructure.otherInfo.attribute&ATTR_DIRECTORY)!=ATTR_DIRECTORY)
					{
						sendString("target is not directory!");
					}
					else
					{
						if((fileBrowserData.findEntry).dirStructure.otherInfo.indicateFirstClustor!=0)
						{
													// sprintf(g_strBuf.dat, "BefClus:0x%lx, %lx", cmdClustorLocation, (fileBrowserData.findEntry).dirStructure.otherInfo.indicateFirstClustor);
													// putStringInGlcdAtPage(PAGE3, g_strBuf.dat);
													// sendString(g_strBuf.dat);

							cmdClustorLocation=(fileBrowserData.findEntry).dirStructure.otherInfo.indicateFirstClustor;

													// sprintf(g_strBuf.dat, "AftClus:0x%lx, %lx", cmdClustorLocation, (fileBrowserData.findEntry).dirStructure.otherInfo.indicateFirstClustor);
													// putStringInGlcdAtPage(PAGE4, g_strBuf.dat);
													// sendString(g_strBuf.dat);
							strcpy(cmdStringNowLocationName, *(cmdArgs+1));
						}
						else
						{
							cmdClustorLocation=(sdCardInfo).rootClustor;
							strcpy(cmdStringNowLocationName, "/");
						}
					}
				}
			}
			else
			{
				sendString("directory name is not inserted!");
			}
		}
		else if(!strcmp(*(cmdArgs+0), "mkdir"))
		{
			if(*(*(cmdArgs+1))=='/')//mkdir from root directory
			{
				sendString("absolute path is not support yet!");				
			}
			else
			{
				// memset(mkdirDirectoryBuffer, 0, CMD_BUFFER_SIZE);
				// strcpy(mkdirDirectoryBuffer, *(cmdArgs+1));
				// if(findDirEntryUsingName(&sdCardInfo, &clustor, &(fileBrowserData.findEntry), cmdClustorLocation, *(cmdArgs+1)))
				if((cmdResult=createNewDirEntry(&sdCardInfo, &clustor, &(fileBrowserData.findEntry), cmdClustorLocation, ATTR_DIRECTORY, *(cmdArgs+1))))
				// if(createNewDirEntry(&sdCardInfo, &clustor, &(fileBrowserData.findEntry), cmdClustorLocation, ATTR_DIRECTORY, mkdirDirectoryBuffer))
				{
					sprintf(g_strBuf.dat, "%d:", cmdResult);
					sendStringOnly(g_strBuf.dat);

					sendString("directory name is already exist!");
				}
			}
		}
		else if(!strcmp(*(cmdArgs+0), "mkfile"))
		{
			if(*(*(cmdArgs+1))=='/')//mkdir from root directory
			{
				sendString("absolute path is not support yet!");				
			}
			else
			{
				// memset(mkdirDirectoryBuffer, 0, CMD_BUFFER_SIZE);
				// strcpy(mkdirDirectoryBuffer, *(cmdArgs+1));
				// if(findDirEntryUsingName(&sdCardInfo, &clustor, &(fileBrowserData.findEntry), cmdClustorLocation, *(cmdArgs+1)))
				if((cmdResult=createNewDirEntry(&sdCardInfo, &clustor, &(fileBrowserData.findEntry), cmdClustorLocation, ATTR_ARCHIVE, *(cmdArgs+1))))
				// if(createNewDirEntry(&sdCardInfo, &clustor, &(fileBrowserData.findEntry), cmdClustorLocation, ATTR_ARCHIVE, mkdirDirectoryBuffer))
				{
					sprintf(g_strBuf.dat, "%d:", cmdResult);
					sendStringOnly(g_strBuf.dat);

					sendString("directory name is already exist!");
				}
			}
		}
		else if(!strcmp(*(cmdArgs+0), "rm"))
		{
			if(*(*(cmdArgs+1))=='/')//mkdir from root directory
			{
				sendString("absolute path is not support yet!");				
			}
			else
			{
//				if(findDirEntryUsingName(&sdCardInfo, &clustor, &(fileBrowserData.findEntry), cmdClustorLocation, *(cmdArgs+1)))
				if((cmdResult=findDirEntryAndDeleteUsingName(&sdCardInfo, &clustor, &(fileBrowserData.findEntry), cmdClustorLocation,  *(cmdArgs+1))))
				{
					sprintf(g_strBuf.dat, "%d:", cmdResult);
					sendStringOnly(g_strBuf.dat);

					sendString("delete dir or file fail!");
				}
			}
		}
		else if(!strcmp(*(cmdArgs+0), "find"))
		{
				if((cmdResult=findDirEntryUsingName(&sdCardInfo, &clustor, &(fileBrowserData.findEntry), cmdClustorLocation, *(cmdArgs+1))))
				{
					sprintf(g_strBuf.dat, "%d:", cmdResult);
					sendStringOnly(g_strBuf.dat);

					sendStringOnly("cannot find ");
					sprintf(g_strBuf.dat, "=> %s", *(cmdArgs+1));
					sendString(g_strBuf.dat);
					
				}
				else
				{
					sendDirectoryAndFileEntryInfomation1(&(fileBrowserData.findEntry));
					sendDirectoryAndFileEntryInfomation2(&(fileBrowserData.findEntry));
					sendDirectoryAndFileEntryInfomation3(&(fileBrowserData.findEntry));
				}
		}
		else if(!strcmp(*(cmdArgs+0), "put"))
		{
			if((cmdResult=putPasteWritingInFileIfNotCreateNewDirEntry(&sdCardInfo, &clustor, &(fileBrowserData.findEntry), &fileAccessInfo, cmdClustorLocation, *(cmdArgs+1), *(cmdArgs+2), EndOfTextFile)))
			{
				sprintf(g_strBuf.dat, "%d:", cmdResult);
				sendStringOnly(g_strBuf.dat);

				sendStringOnly("cannot file write ");
				sprintf(g_strBuf.dat, "=> %s", *(cmdArgs+1));
				sendString(g_strBuf.dat);
			}
			else
			{
				sendStringOnly("file write success.");
			}
		}
		else if(!strcmp(*(cmdArgs+0), "cat"))
		{
			if((cmdResult=findDirEntryUsingName(&sdCardInfo, &clustor, &(fileBrowserData.findEntry), cmdClustorLocation, *(cmdArgs+1))))
			{
				sprintf(g_strBuf.dat, "%d:", cmdResult);
				sendStringOnly(g_strBuf.dat);

				sendStringOnly("cannot find file ");
				sprintf(g_strBuf.dat, "=> %s", *(cmdArgs+1));
				sendString(g_strBuf.dat);
			}
			else
			{
				cmdDisplayArchive(&sdCardInfo, &clustor, &(fileBrowserData.findEntry));
			}
		}
		
		
		
		else if(!strcmp(*(cmdArgs+0), "adc"))
		{
			if((cmdResult=findDirEntryUsingName(&sdCardInfo, &clustor, &(fileBrowserData.findEntry), cmdClustorLocation, *(cmdArgs+1))))
			{
				sprintf(g_strBuf.dat, "%d:", cmdResult);
				sendStringOnly(g_strBuf.dat);

				sendStringOnly("cannot find file ");
				sprintf(g_strBuf.dat, "=> %s", *(cmdArgs+1));
				sendString(g_strBuf.dat);
			}
			else if((fileBrowserData.findEntry).dirStructure.otherInfo.indicateFirstClustor==0)
			{
				sprintf(g_strBuf.dat, "%d:", cmdResult);
				sendStringOnly(g_strBuf.dat);

				sendStringOnly("file is not have indicate clustor ");
				sprintf(g_strBuf.dat, "=> %s", *(cmdArgs+1));
				sendString(g_strBuf.dat);
			}
			else
			{
				sendString("----- GET ADC#0, #1 VALUES START -----");
				/*get value from ADC #0*/
				SPI_ADC_Init();

				sprintf(g_strBuf.dat ,
				"%0000006d,ADC%d,%00004d,%00004d,%00004d,%00004d,%00004d,%00004d,%00004d,%00004d", 
				(0x00001FFF&rand()),(0), 
				getAdcValue(SPI_MODE_ADC0, 0, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES), 
				getAdcValue(SPI_MODE_ADC0, 1, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES), 
				getAdcValue(SPI_MODE_ADC0, 2, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES), 
				getAdcValue(SPI_MODE_ADC0, 3, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES),
				getAdcValue(SPI_MODE_ADC0, 4, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES), 
				getAdcValue(SPI_MODE_ADC0, 5, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES), 
				getAdcValue(SPI_MODE_ADC0, 6, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES), 
				getAdcValue(SPI_MODE_ADC0, 7, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES)
				);
				sendString(g_strBuf.dat);

				SPI_SD_CARD_Init(SPI_MODE_SD_CARD);
				putPasteWritingInFile(&sdCardInfo, &clustor, &(fileBrowserData.findEntry), &fileAccessInfo, g_strBuf.dat, EndOfTextFile);


				/*get value from ADC #1*/
				SPI_ADC_Init();

				sprintf(g_strBuf.dat ,
				"%0000006d,ADC%d,%00004d,%00004d,%00004d,%00004d,%00004d,%00004d,%00004d,%00004d", 
				(0x00001FFF&rand()),(1), 
				getAdcValue(SPI_MODE_ADC1, 0, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES), 
				getAdcValue(SPI_MODE_ADC1, 1, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES), 
				getAdcValue(SPI_MODE_ADC1, 2, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES), 
				getAdcValue(SPI_MODE_ADC1, 3, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES),
				getAdcValue(SPI_MODE_ADC1, 4, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES), 
				getAdcValue(SPI_MODE_ADC1, 5, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES), 
				getAdcValue(SPI_MODE_ADC1, 6, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES), 
				getAdcValue(SPI_MODE_ADC1, 7, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES)
				 );
				sendString(g_strBuf.dat);


				SPI_SD_CARD_Init(SPI_MODE_SD_CARD);
				putPasteWritingInFile(&sdCardInfo, &clustor, &(fileBrowserData.findEntry), &fileAccessInfo, g_strBuf.dat, EndOfTextFile);

				sendString("----- GET ADC#0, #1 VALUES END -----");
			}
		}
		else if(!strcmp(*(cmdArgs+0), "exit")) break;
		else
		{
			sendString("command is not found.");		
			sendString("command is....");
			sendString("ls | mkdir | rm | mkfile | find | find-long| put");		
		}
	}
	

	/*Setting file name and directory entry*/
	targetClustor=sdCardInfo.rootClustor;
	strcpy(fileName, "dataLoggerData.txt");
	/*using findTargetFileDirectoryEntryUsingName start*/

	
////////////////////////////////////////////////////* get Adc Value example and write sd-card start *////////////////////////////////////////////////////

	if(findDirEntryIfNotCreateNewDirEntry(&sdCardInfo, &clustor, &(fileBrowserData.findEntry), targetClustor,  ATTR_ARCHIVE, fileName)==0)
	{
		unsigned long i=0;
////////////////////////////////////////////////////* get Adc Value example and write sd-card start *////////////////////////////////////////////////////
		for(i=0; i<0x40; i++)
		{
			/*get value from ADC #0*/
			SPI_ADC_Init();

			sprintf(g_strBuf.dat ,
			"%0000006d,ADC%d,%00004d,%00004d,%00004d,%00004d,%00004d,%00004d,%00004d,%00004d", 
			(0x00001FFF&rand()),(0), 
			getAdcValue(SPI_MODE_ADC0, 0, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES), 
			getAdcValue(SPI_MODE_ADC0, 1, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES), 
			getAdcValue(SPI_MODE_ADC0, 2, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES), 
			getAdcValue(SPI_MODE_ADC0, 3, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES),
			getAdcValue(SPI_MODE_ADC0, 4, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES), 
			getAdcValue(SPI_MODE_ADC0, 5, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES), 
			getAdcValue(SPI_MODE_ADC0, 6, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES), 
			getAdcValue(SPI_MODE_ADC0, 7, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES)
			);

			SPI_SD_CARD_Init(SPI_MODE_SD_CARD);
			putPasteWritingInFile(&sdCardInfo, &clustor, &(fileBrowserData.findEntry), &fileAccessInfo, g_strBuf.dat, EndOfTextFile);


			/*get value from ADC #1*/
			SPI_ADC_Init();

			sprintf(g_strBuf.dat ,
			"%0000006d,ADC%d,%00004d,%00004d,%00004d,%00004d,%00004d,%00004d,%00004d,%00004d", 
			(0x00001FFF&rand()),(1), 
			getAdcValue(SPI_MODE_ADC1, 0, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES), 
			getAdcValue(SPI_MODE_ADC1, 1, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES), 
			getAdcValue(SPI_MODE_ADC1, 2, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES), 
			getAdcValue(SPI_MODE_ADC1, 3, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES),
			getAdcValue(SPI_MODE_ADC1, 4, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES), 
			getAdcValue(SPI_MODE_ADC1, 5, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES), 
			getAdcValue(SPI_MODE_ADC1, 6, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES), 
			getAdcValue(SPI_MODE_ADC1, 7, 10, ADC_DEFAULT_JUNK_SAMPLING_TIMES)
			 );

			SPI_SD_CARD_Init(SPI_MODE_SD_CARD);
			putPasteWritingInFile(&sdCardInfo, &clustor, &(fileBrowserData.findEntry), &fileAccessInfo, g_strBuf.dat, EndOfTextFile);
		}
	}
////////////////////////////////////////////////////* get Adc Value example and write sd-card end *////////////////////////////////////////////////////

////////////////////////* finding Dir Entry Using indicate first clustor end *////////////////////////
////////////////////////////////////////* browser test start */////////////////////////////////////////
	SPI_SD_CARD_Init(SPI_MODE_SD_CARD);
	fileBrowser(&sdCardInfo, &clustor, &fileBrowserData);
////////////////////////////////////////* browser test start */////////////////////////////////////////
	return 0;
}

