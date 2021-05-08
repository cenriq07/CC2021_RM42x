/** @file sys_main.c 
*   @brief Application main file
*   @date 11-Dec-2018
*   @version 04.07.01
*
*   This file contains an empty main function,
*   which can be used for the application.
*/

/* 
* Copyright (C) 2009-2018 Texas Instruments Incorporated - www.ti.com 
* 
* 
*  Redistribution and use in source and binary forms, with or without 
*  modification, are permitted provided that the following conditions 
*  are met:
*
*    Redistributions of source code must retain the above copyright 
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the 
*    documentation and/or other materials provided with the   
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/


/* USER CODE BEGIN (0) */
/*----------------- OS Libraries --------------------*/
#include "FreeRTOS.h"
#include "os_task.h"
#include "sys_core.h"
#include "gio.h"
#include "sci.h"
/*------------- KA'AN SAT Libraries -----------------*/
#include "KaanSat_Lib/Utilities.h"
#include "KaanSat_Lib/Commands.h"
#include "KaanSat_Lib/PWM.h"
/* USER CODE END */

/* Include Files */

#include "sys_common.h"

/* USER CODE BEGIN (1) */
static char receivedData[20];
int i = 0;
/* USER CODE END */

/** @fn void main(void)
*   @brief Application main function
*   @note This function is empty by default.
*
*   This function is called after startup.
*   The user can use this function to implement the application.
*/

/* USER CODE BEGIN (2) */
void sciNotification(sciBASE_t *sci, unsigned flags);
/* USER CODE END */

int main(void)
{
/* USER CODE BEGIN (3) */

    gioInit();
    hetInit();
    adcInit();

    /* ---------------- SCI CONFIG----------------*/
    sciInit();
    sciEnableNotification(scilinREG, SCI_RX_INT);
    _enable_IRQ();
    _enable_interrupt_();
    sciReceive(scilinREG, 1, ( unsigned char *)receivedData);
    /* --------------- READ SD STATE --------------*/

    /* --------------- CREATE TASK --------------*/
    xTaskCreate(vMissionOperations,"Sat Ops",configMINIMAL_STACK_SIZE, NULL, 1, &xWTStartHandle);
    xTaskCreate(vSensors,"Sensores",configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vTelemetry,"T. Container",512, NULL, 1, &xTelemetryHandle);
    xTaskCreate(vWaitToStart,"W.To S.",configMINIMAL_STACK_SIZE, NULL, 1, &xWTStartHandle);

    vTaskSuspend(xTelemetryHandle);
    vTaskStartScheduler();
    while(1);
/* USER CODE END */

    return 0;
}


/* USER CODE BEGIN (4) */
void vWaitToStart(void *pvParameters)
{
    portTickType xWaitTime;
    xWaitTime = xTaskGetTickCount();

    while(1)
    {
        if(telemetry_ON)
        {
            __delay_cycles(106);
            vTaskResume(xTelemetryHandle);
            vTaskSuspend(NULL);
            __delay_cycles(106);
        }
        sciSendData(sprintf(command, "."), command, 0);
        vTaskDelayUntil(&xWaitTime, T_TELEMETRY/portTICK_RATE_MS);
    }
}

void vTelemetry(void *pvParameters)
{
    portTickType xTelemetryTime;
    xTelemetryTime = xTaskGetTickCount();
    STATE = LAUNCH;

    while(1)
    {
        if(!telemetry_ON)
        {
            __delay_cycles(106);
            vTaskResume(xWTStartHandle);
            vTaskSuspend(NULL);
            __delay_cycles(106);
        }

        createTelemetryPacket();
        //TODO V4
        sciSendData(buff_sizeAPI, tramaAPI, 0);

        PACKET_COUNT++;
        SP1_PC++;
        SP2_PC++;

        vTaskDelayUntil(&xTelemetryTime, T_TELEMETRY);
    }
}

void vSensors(void *pvParameters)
{
    int cont=0, i=0, press_i=0, m=0, n=0, y=0,w=0;
    float xT=0, x=0, var=0, desv=0, a=0, b=0;
    int presion_u[9];

    portTickType xSensorsTime;
    xSensorsTime = xTaskGetTickCount();
    toggle_sim = 1;

    while(1)
    {
        if (toggle_sim==1)
        {
            toggle_sim=0;
            for (i=0;i<=9;i++)
            {
                presion_u[i]= PRESS_BAR; //PRIMER ACOMODO DE PRESIONES
                if (toggle_sim) break;
                vTaskDelayUntil(&xSensorsTime, T_SENSORS);
            }
            if (!toggle_sim)
            {
                for (i=0;i<=9;i++)
                {
                    press_i += presion_u[i];
                }
                press_i = press_i/(float)10;
                ALTITUDE_INIT=getAltitude(press_i);
                cont=0;
            }
        }
        else
        {
            for (i=0;i<10;i++)
            {
                if (i==cont)
                {
                    y = presion_u[i];
                    presion_u[i] = 0;
                }
                x += presion_u [i]; //Media
            }
            xT = x/(float)9;

            for (i=0;i<10;i++)
            {
                if (i==cont)
                {
                    w = xT;
                    xT=0;
                }
                var += pow ((presion_u [i] - xT),2.00); //Varianza
                xT=w;
            }
            presion_u[cont] = y;
            desv= sqrt(var/8); //Desviación estándar

            a = xT-3*desv; //Limite inf
            b = xT+3*desv;  //Limite sup

            if ((presion_u[cont]<a)||(presion_u[cont]>b))
            {
                m = presion_u[cont] - presion_u[cont-1];
                n = presion_u[0] - presion_u[9];

                if ((m<-2100)||(n<-2100))
                {
                    ALTITUDE_BAR = getAltitude(PRESS_BAR);
                }
            }
            else
            {
                ALTITUDE_BAR = getAltitude(PRESS_BAR);
            }


            cont2++;

            xT=0;
            a=0;
            b=0;
            desv=0;
            var=0;
            x = 0;

            vTaskDelayUntil(&xSensorsTime, T_SENSORS);

            if (cont==9)
            {
                cont =-1;
            }
            cont++;
            presion_u[cont] = PRESS_BAR; //CAMBIO
        }
    }
}

void vMissionOperations(void *pvParameters)
{
    portTickType xOpsTime;
    xOpsTime = xTaskGetTickCount();

    SERVO_PAYLOAD.period = 20000;
    int angles[3] = {SPOS_ZERO, SPOS_SP1, SPOS_SP2};
    SERVO_PAYLOAD.duty = angles[0];

    int i = 0;
    uint8 land = 0;
    float ALTITUDE_STATES[4];

    while (ALTITUDE_INIT<0)
    {
        vTaskDelayUntil(&xOpsTime, T_OPERATIONS);
    }

    ALTITUDE_STATES[0]=ALTITUDE_INIT+670;
    ALTITUDE_STATES[1]=ALTITUDE_INIT+500;
    ALTITUDE_STATES[2]=ALTITUDE_INIT+400;
    ALTITUDE_STATES[3]=ALTITUDE_INIT;

    i = 0;
    while(1)
    {
        switch(STATE)
        {
            case SP1_RELEASE:
                SERVO_PAYLOAD.duty = angles[1];
                break;
            case SP2_RELEASE:
                SERVO_PAYLOAD.duty = angles[2];
                break;
            default:
                SERVO_PAYLOAD.duty = angles[0];
                break;
        }
        if(land == 0 && (ALTITUDE_BAR >= ALTITUDE_STATES[i]))
        {
            land = 1;
            i++;
            STATE = i+1;
        }
        if(land == 1 && (ALTITUDE_BAR <= ALTITUDE_STATES[i]))
        {
            i++;
            STATE = i+1;
        }
        pwmSetSignal10e3(hetRAM1, PWM_PAYLOAD, SERVO_PAYLOAD);
        vTaskDelayUntil(&xOpsTime, T_OPERATIONS);
    }
}

void sciNotification(sciBASE_t *sci, unsigned flags )
{
    sciReceive(scilinREG, 1, (unsigned char *)&receivedData);
    getCommand(receivedData[0]);
}


/* USER CODE END */
