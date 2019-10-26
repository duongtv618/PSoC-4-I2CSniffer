/**
 * \file main.c
 * \author Duong (duongtv618@gmail.com)
 * \brief I2C Sniffer with PSOC
 * Master communicate with I2C about 15 times per second
 * \version 0.1
 * \date 2019-09-18
 * 
 * @copyright Copyright (c) 2019
 * 
 */
#include "project.h"
#include "stdbool.h"
#include "stdio.h"

/** ACK */
#define ACK 0
/** NACK */
#define NACK 1

/** ACKNACK array */
uint8 ACKNACK[10];
/** Out put hexcode for UART display */
uint8 outHex[10];
/** Out put hex count variable */
uint8 outHexCount = 0;
/** Flag to detect stop condition */
bool meetStopCondition = false;
/** Millis second from when CySysTickStart is called */
uint64 millis = 0;

void callback();

/** CySysTickCallback */
void callback(){
    millis++;
}

/**
 * \brief SDA interrupt with rising egde service to detect stop condition
 * 
 * 
 * 
 */
CY_ISR(SDA_isr){
    SDA_ClearInterrupt();
    /** Has rising edge at SDA when SCL is 1 => Stop condition */
    if (SCL_Read())
        meetStopCondition = true;
}

/**
 * \brief main
 * 
 * \return int 
 */
int main(void)
{
    /** Enable global interrupt */
    CyGlobalIntEnable;

    /** Start component */
    UART_Start();
    UART_UartPutString("Test I2c \r\n");
    SDA_intr_StartEx(SDA_isr);
    CySysTickStart();
    CySysTickSetCallback(0, callback);
    //int count = 0;
    //uint64 lastmillis = 0;
    for (;;)
    {
        /** Wait for start condition */
        bool start = false;
        while (!start)
        {
            /** Wait for SCL = 1 and SDA = 0 => Start condition */
            while (((SCL_Read() == 1) && (SDA_Read() == 0)) == 0);
            /** To except error and exit the while loop */
            start = SCL_Read();
        }
        /** For count start condition in a second, about 15 per second */
        //count++;
        //if (millis - lastmillis >= 1000){
        //    lastmillis = millis;
        //    char msg[20];
        //    sprintf(msg, "Count: %d\r\n", count);
        //    UART_UartPutString(msg);
        //    count = 0;            
        //}
        int k = 0;
        /** Set this before a session begin */
        do
        {
            k = 0;
            /** Read 8 bit and store to outHex */
            while (k < 8)
            {
                
                /** Wait for rising edge of SCL */
                while (SCL_Read() == 1){
                    if (meetStopCondition)
                        goto stop;
                }
                while (SCL_Read() == 0){
                    if (meetStopCondition)
                        goto stop;
                }
                /** Then read SDA to get data */
                outHex[outHexCount] |= SDA_Read() << (7 - k++);
            }
            if (meetStopCondition)
               goto stop;
            /** Read ACK/NACK with the same way */
            while (SCL_Read() == 1){
                if (meetStopCondition)
                    goto stop;
            }
            while (SCL_Read() == 0){
                if (meetStopCondition)
                    goto stop;
            }
            ACKNACK[outHexCount++] = SDA_Read();
        } while ((ACKNACK[outHexCount - 1] == ACK) && (!meetStopCondition));/** This loop will exit if it has a stop condition or NACK */
        
        /** Put data to UART */
        stop:
        UART_UartPutString("\r\nStart  ");
        for (size_t i = 0; i < outHexCount; i++){
            char msg[10];
            sprintf(msg, "%02X", outHex[i]);
            UART_UartPutString(msg);
            if (ACKNACK[i])
            {
                UART_UartPutString("  NACK  ");
            }
            else 
            {
                UART_UartPutString("  ACK  ");
            }
        }
        UART_UartPutString("Stop\r\n");
        
        /** Clear data for new session */
        outHexCount = 0;
        for (size_t i = 0; i < sizeof(outHex); i++){
            outHex[i] = 0;
            ACKNACK[i] = 0;
        }
        meetStopCondition = false;
    }
}