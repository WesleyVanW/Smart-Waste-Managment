#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include "periph_conf.h"
#include "periph/i2c.h"
#include "minmea.h"
#include "shell.h"
#include <unistd.h>
#include <xtimer.h>

#ifndef I2C_ACK
#define I2C_ACK         (0)
#endif

//static float returnvalues (float *latitude, float *longitude);

void getGPSCoordinates(float* lat, float* lon)
{

    char  reg[255];
    //char * token;
    struct minmea_sentence_rmc frame;
    //float tempLat,tempLongi;
    int  k=0;
    char data[74];
    bool sentenceNotFound=true;
     i2c_init(0);

    while(sentenceNotFound)
    {
    memset(reg, 0, 255);
        memset(data,0,78);
        xtimer_usleep(1000000);
       

        printf("\n");

       for(int j=0;j<255;j++)
        {
            i2c_acquire(0);
            i2c_read_reg(0, 0x10,0x21,&reg[j], 0);
            //i2c_read_byte(0,0x10,&reg[j],0);
            xtimer_usleep(2000);
            printf("%c",reg[j]);
            i2c_release(0);
        }
        
         printf("\n--------------------------------------------\n");
    
   
       
        for(int j=0;j<255;j++)
       {

         if((reg[j] == '$') )//&& (reg[j+1]== 'G') && (reg[j+2] == 'N') && (reg[j+3] == 'R') && (reg[j+4] == 'M') && (reg[j+5] == 'C'))
            {
               
            
                if(reg[j+1]== 'G')
                {

                if(reg[j+2] == 'N')
                {
                    if(reg[j+3] == 'R')
                    {
                        if(reg[j+4] == 'M')
                        {
                            if(reg[j+5] == 'C')
                            {
                                 //printf("I'm here");
                                 printf("Dit is reg: ");
                                 for(int i=j;i<j+74;i++)
                                {
                                      data[k]=reg[i];
                                      printf("%c",reg[i]);
                                      k++;
                                }
                            }
                        }
                    }
                }
                }
           }
        }
        
        
        printf("\nDit is data: ");
        for(int i=0;i<74;i++)   
        {
            printf("%c",data[i]);
            /*if(data[i]=='*')
            {
                printf("%c%c",data[i+1],data[i+2]);
                i=74;
            }*/
        }
        
    
    int res = minmea_parse_rmc(&frame, "$GNRMC,105824.000,A,5110.577055,N,00420.844651,E,0.42,285.58,080119,,,A*73");
    if (!res) {
        puts("FAILURE: error parsing GPS sentence");
    }
        printf("\n");
        *lat = minmea_tocoord(&frame.latitude);
        *lon = minmea_tocoord(&frame.longitude);
        //tempLat = minmea_tocoord(&frame.latitude);
        //tempLongi = minmea_tocoord(&frame.longitude);
        
        printf("First parsed coordinates: lat=%f lon=%f\n",
                minmea_tocoord(&frame.latitude),
                minmea_tocoord(&frame.longitude));
        printf("\n");

        puts("SUCCESS");
        //printf("parsed coordinates: lat=%f lon=%f\n",tempLat,tempLongi);
       // printf("\n");
        //*lat=tempLat;
        //*lon=tempLongi;
     sentenceNotFound = false;

}
        printf("Coordinates received");

}

//$GNRMC,105824.000,A,5110.577055,N,00420.844651,E,0.42,285.58,080119,,,A*73