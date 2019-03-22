/**
* MIT License
*
* Copyright (c) 2019 Infineon Technologies AG
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/asn1.h>

//#include "optiga/ifx_i2c/ifx_i2c_config.h"
//#include "optiga/optiga_util.h"

#include "trustm_helper.h"

/*************************************************************************
*  Global
*************************************************************************/
optiga_util_t * me_util;
optiga_lib_status_t optiga_lib_status;

/*************************************************************************
*  functions
*************************************************************************/
/**
 * Callback when optiga_util_xxxx operation is completed asynchronously
 */

void optiga_util_callback(void * context, optiga_lib_status_t return_status)
{
    optiga_lib_status = return_status;
	TRUSTM_HELPER_DBGFN("optiga_lib_status: %x\n",optiga_lib_status);
}

/*************************************************************************
*  Read Metadata support
*************************************************************************/

static char __ALW[] = "ALW";
//static char __CONF[] = "Conf";
//static char __INT[] = "Int";
//static char __LUC[] = "Luc";
static char __LCSG[] = "LcsG";
static char __LCSA[] = "LcsA";
static char __LCSO[] = "LcsO";
static char __EQ[] = "==";
static char __GT[] = ">";
static char __LT[] = "<";
static char __AND[] = "&&";
static char __OR[] = "||";
static char __NEV[] = "NEV";

//static char __BSTR[] = "BSTR";
//static char __UPCTR[] = "UPCTR";
//static char __TA[] = "TA";
//static char __DEVCERT[] = "DEVCERT";
//static char __PRESSEC[] = "PRESSEC";
//static char __PTFBIND[] = "PTFBIND";
//static char __UPDATSEC[] = "UPDATSEC";

static char __ECC256[] = "ECC256";
static char __ECC384[] = "ECC384";
static char __SHA256[] = "SHA256";
static char __AUTH[] = "Auth";
static char __ENC[] = "Enc";
static char __HFU[] = "HFU";
static char __DM[] = "DM";
static char __SIGN[] = "Sign";
static char __AGREE[] = "Agreement";
static char __E1[100];

static char* __decodeAC(uint8_t data)
{
	char *ret;
	switch (data)
	{
		case 0x00:
			ret = __ALW; 
			break;
		case 0x70:
			ret = __LCSG;
			break;
		case 0xE0:
			ret = __LCSA;
			break;
		case 0xE1:
			ret = __LCSO;
			break;
		case 0xFA:
			ret = __EQ;
			break;
		case 0xFB:
			ret = __GT;
			break;
		case 0xFC:
			ret = __LT;
			break;
		case 0xFD:
			ret = __AND;
			break;
		case 0xFE:
			ret = __OR;
			break;
		case 0xFF:
			ret = __NEV;
			break;
		case 0x03:
			ret = __ECC256;
			break;
		case 0x04:
			ret = __ECC384;
			break;
		case 0xE2:
			ret = __SHA256;
			break;
	}
	return ret;
}

static char* __decodeAC_E1(uint8_t data)
{
	char *ret;
	uint16_t i=0; 
	
	if (data & 0x01) // Auth = 0x01 
	{
		strcpy((__E1+i),__AUTH);
		i += (sizeof(__AUTH));
	}
	if (data & 0x02) // Enc = 0x02 
	{
		__E1[i-1] = '/';
		strcpy((__E1+i),__ENC);
		i += sizeof(__ENC);
	}
	if (data & 0x04) // HFU = 0x04 
	{
		__E1[i-1] = '/';
		strcpy((__E1+i),__HFU);
		i += sizeof(__HFU);
	}
	if (data & 0x08) // DM = 0x08 
	{
		__E1[i-1] = '/';
		strcpy((__E1+i),__DM);
		i += sizeof(__DM);
	}
	if (data & 0x10) // SIGN = 0x10 
	{
		__E1[i-1] = '/';
		strcpy((__E1+i),__SIGN);
		i += sizeof(__SIGN);
	}
	if (data & 0x20) // AGREE = 0x20 
	{
		__E1[i-1] = '/';
		strcpy((__E1+i),__AGREE);
		i += sizeof(__AGREE);
	}
	ret = __E1;
	return ret;
}

void trustmdecodeMetaData(uint8_t * metaData)
{
	uint16_t i,j;
	uint16_t metaLen;
	uint16_t len;
	uint8_t LcsO;
	uint16_t maxDataObjSize;
	
	if(*metaData == 0x20)
	{
		i = 1;
		metaLen = *(metaData+(i++));
		while(i < metaLen)
		{
			
			switch(*(metaData+(i++)))
			{
				case 0xC0:
					// len is always 1
					len = *(metaData+(i++));
					LcsO = *(metaData+(i++));
					printf("LcsO:0x%.2X, ",LcsO);
					break;
				
				case 0xC4:
					// len is 1 or 2
					len = *(metaData+(i++));
					maxDataObjSize = *(metaData+(i++));
					if (len == 2)
						maxDataObjSize = (maxDataObjSize<<8) + *(metaData+(i++));
					printf("Max:%d, ",maxDataObjSize);
					break;
				
				case 0xC5:
					len = *(metaData+(i++));
					maxDataObjSize = *(metaData+(i++));
					if (len == 2)
						maxDataObjSize = (maxDataObjSize<<8) + *(metaData+(i++));
					printf("Used:%d, ",maxDataObjSize);
					break;
				
				case 0xD0:
					len = *(metaData+(i++));
					printf("C:");
					for (j=0; j<len;j++)
					{
						if((*(metaData+(i)) == 0x00)||(*(metaData+(i)) == 0xff) )
							printf("%s, ",__decodeAC(*(metaData+(i++))));
						else
						{
							if((*(metaData+(i)) > 0xfa) && 
								(*(metaData+(i)) <= 0xff))
							{
								printf("%s",__decodeAC(*(metaData+(i++))));
							}
							else
							{
								if((*(metaData+(i)) == 0x70)||(*(metaData+(i)) == 0xe1))
									printf("%s",__decodeAC(*(metaData+(i++))));
								else
								{
									printf("%d",*(metaData+(i++)));
									if ((len-j) < 3)
										printf(", ");
								}
							}
						}
					}
					break;
				
				case 0xD1:
					len = *(metaData+(i++));
					printf("R:");
					for (j=0; j<len;j++)
					{
						if((*(metaData+(i)) == 0x00)||(*(metaData+(i)) == 0xff) )
							printf("%s",__decodeAC(*(metaData+(i++))));
						else
						{
							if((*(metaData+(i)) > 0xfa) && 
								(*(metaData+(i)) <= 0xff))
							{
								printf("%s",__decodeAC(*(metaData+(i++))));
							}
							else
							{
								if((*(metaData+(i)) == 0x70)||(*(metaData+(i)) == 0xe1))
									printf("%s",__decodeAC(*(metaData+(i++))));
								else
								{
									printf("%d",*(metaData+(i++)));
									if ((len-j) < 3)
										printf(", ");
								}
							}
						}
					}
					break;
				
				case 0xD2:
					len = *(metaData+(i++));
					printf("D:");
					for (j=0; j<len;j++)
					{
						if((*(metaData+(i)) == 0x00)||(*(metaData+(i)) == 0xff) )
							printf("%s",__decodeAC(*(metaData+(i++))));
						else
						{
							if((*(metaData+(i)) > 0xfa) && 
								(*(metaData+(i)) <= 0xff))
							{
								printf("%s",__decodeAC(*(metaData+(i++))));
							}
							else
							{
								if((*(metaData+(i)) == 0x70)||(*(metaData+(i)) == 0xe1))
									printf("%s",__decodeAC(*(metaData+(i++))));
								else
								{
									printf("%d",*(metaData+(i++)));
									if ((len-j) < 3)
										printf(", ");
								}
							}
						}
					}				
					break;
					
				case 0xE0:
					// len is always 1
					len = *(metaData+(i++));
					printf("Algo:%s, ",__decodeAC(*(metaData+(i++))));				
					break;
					
				case 0xE1:
					// len is always 1
					len = *(metaData+(i++));
					printf("Key:%s, ",__decodeAC_E1(*(metaData+(i++))));				
					break;
				
				default:
					i = metaLen;
					break;
					
			}
		}
	printf("\n");	
	}
}

/**********************************************************************
* trustmHexDump()
**********************************************************************/
void trustmHexDump(uint8_t *pdata, uint32_t len)
{
	uint32_t i, j;

	printf("\t");	
	j=0;
	for (i=0; i < len; i++)
	{
		printf("%.2x ",*(pdata+i));
		if (j < 15)	
		{
			j++;
		}
		else
		{
			j=0;
			printf("\n\t");
		}
	}
	printf("\n");
}

uint16_t trustmWritePEM(uint8_t *buf, uint32_t len, const char *filename, char *name)
{
	FILE *fp;
	char header[] = "";

	fp = fopen(filename,"wb");
	if (!fp)
	{
		printf("error creating file!!\n");
		return 1;
	}

	//Write cert to file
	PEM_write(fp, name,header,buf,(long int)len);				

	fclose(fp);

	return 0;
}

uint16_t trustmWriteDER(uint8_t *buf, uint32_t len, const char *filename)
{
	FILE *fp;

	fp = fopen(filename,"wb");
	if (!fp)
	{
		printf("error creating file!!\n");
		return 1;
	}

	//Write cert to file
	fwrite(buf, 1, len, fp);				

	fclose(fp);

	return 0;
}

uint16_t trustmReadPEM(uint8_t *buf, uint32_t *len, const char *filename, char *name)
{
	FILE *fp;
	char *tempName;
	char *header;
	uint8_t *data;
	long int dataLen;

	fp = fopen(filename,"r");
	if (!fp)
	{
		printf("failed to open file %s\n",filename);
		return 1;
	}
	
	dataLen = 0;
	PEM_read(fp, &tempName,&header,&data,&dataLen);
	memcpy(buf,data,dataLen);
	*len = dataLen;
	
	strcpy(name,tempName);
	
	fclose(fp);
	return 0;
}

uint16_t trustmReadDER(uint8_t *buf, uint32_t *len, const char *filename)
{
	
	FILE *fp;
	uint16_t tempLen;
	uint8_t data[2048];

	//open 
	fp = fopen((const char *)filename,"rb");
	if (!fp)
	{
		return 1;
	}

	//Read file
	tempLen = fread(data, 1, sizeof(data), fp); 
	if (tempLen > 0)
	{
		memcpy(buf,data,tempLen);
		*len = tempLen;
	}
	else
	{
		*len = 0;
	}

	fclose(fp);

	return 0;
}

uint16_t trustmWriteX509PEM(X509 *x509, const char *filename)
{
	FILE *x509file;
	uint16_t ret;

	//create x509 file pem
	x509file = fopen(filename,"wb");
	if (!x509file)
	{
		printf("error creating x509 file!!\n");
		return 1;
	}

	//Write cert to file
	ret = PEM_write_X509(x509file, x509);
	fclose(x509file);
	if (!ret)
	{
		printf("Unable Cert to write to file!!\n");
		return 1;
	}

	return 0;

}

uint16_t trustmReadX509PEM(X509 **x509, const char *filename)
{
	FILE *x509file;

	//open x509 file pem
	x509file = fopen(filename,"rb");
	if (!x509file)
	{
		printf("error reading x509 file!!\n");
		return 1;
	}

	//Read file to cert 
	*x509 = PEM_read_X509(x509file, NULL, 0, NULL);
	fclose(x509file);
	if (x509 == NULL)
	{
		printf("Unable read cert from file!!\n");
		return 1;
	}

	return 0;

}

/**********************************************************************
* trustm_Open()
**********************************************************************/
optiga_lib_status_t trustm_Open(void)
{
	uint16_t i;
    optiga_lib_status_t return_status;

    do
    {
        //Create an instance of optiga_util to open the application on OPTIGA.
        me_util = optiga_util_create(0, optiga_util_callback, NULL);
		printf("TrustM Open. \n");

        /**
         * Open the application on OPTIGA which is a precondition to perform any other operations
         * using optiga_util_open_application
         */        
        optiga_lib_status = OPTIGA_LIB_BUSY;
        return_status = optiga_util_open_application(me_util, 0);

        if (OPTIGA_LIB_SUCCESS != return_status)
        {
            printf("Fail : optiga_util_open_application[1] \n");
            break;
        }


		TRUSTM_HELPER_DBGFN("waiting (max count: 50)");
        //Wait until the optiga_util_open_application is completed
		i=0;
        while (optiga_lib_status == OPTIGA_LIB_BUSY)
		{
			TRUSTM_HELPER_DBG(".");
			i++;
			usleep(50000);
			if (i == 50)
				break;
		}
		TRUSTM_HELPER_DBG("\n");
		TRUSTM_HELPER_DBGFN("count : %d \n",i);

        if (OPTIGA_LIB_SUCCESS != optiga_lib_status)
        {
            //optiga util open application failed
			printf("Fail : optiga_util_open_application \n");
			printf("optiga_lib_status: %x\n",optiga_lib_status);
			return_status = optiga_lib_status;
            break;
        }
    }while(FALSE);      

	return return_status;
}

/**********************************************************************
* trustX_Close()
**********************************************************************/
optiga_lib_status_t trustm_Close(void)
{
    optiga_lib_status_t return_status;
	
	do{
        optiga_lib_status = OPTIGA_LIB_BUSY;
        //++ty++ OPTIGA_COMMS_PROTECTION_MANAGE_CONTEXT(me_util, OPTIGA_COMMS_SESSION_CONTEXT_NONE);
        return_status = optiga_util_close_application(me_util, 0);
        
        if (OPTIGA_LIB_SUCCESS != return_status)
        {
			printf("Fail : optiga_util_close_application \n");
            break;
        }

        while (optiga_lib_status == OPTIGA_LIB_BUSY)
        {
            //Wait until the optiga_util_close_application is completed
			printf("Waiting : optiga_util_close_application \n");
        }
        
        if (OPTIGA_LIB_SUCCESS != optiga_lib_status)
        {
            //optiga util close application failed
			printf("Fail : optiga_util_close_application \n");
			return_status = optiga_lib_status;
            break;
        }

		printf("Success : optiga_util_close_application \n");

	}while(FALSE);

    // destroy util and crypt instances
    optiga_util_destroy(me_util);	
	
	printf("TrustM Closed.\n");
	
	return return_status;
}