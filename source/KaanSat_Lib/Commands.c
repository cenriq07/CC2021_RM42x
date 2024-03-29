/*
 * Commands.c
 *
 *  Created on: 02/02/2021
 *      Author: Enrique
 */


#include "KaanSat_Lib/Commands.h"
#include "KaanSat_Lib/Utilities.h"
#include "sys_core.h"

char *commands[] = {"CX", "ST", "SIM", "SP1X", "SP2X", "SIMP"};
int commandsTam = sizeof (commands)/sizeof (char*);

int sim_ok = 0;
int cmd_cont = 0;


int getCommand(char cmd_char)
{
    if(cmd_char != ';')
    {
        CMD_KEY[cmd_cont] = cmd_char;
        cmd_cont++;
    }
    else
    {
        cmd_cont = 0;
        //sciSendData(sprintf(command, CMD_KEY), command, 0);
        if(CMD_KEY[0] == '+' || CMD_KEY[0] == '-')
        {
            getSPTelemetry(CMD_KEY);
        }
        else
        {
            findCommand(CMD_KEY);
        }
        memset(CMD_KEY, 0, sizeof(CMD_KEY));                   // Clean the array CMD_KEY to be used again
    }
}

void getSPTelemetry(char *telemetry)
{
    int i;
    int selectSP = -1;
    char *values[] = {"A","B","C","D","E","F"};

    if(telemetry[0] == '+')
        selectSP = 0;
    else
        selectSP = 1;

    char *token = strtok(telemetry,",+-");

    for(i=0; i<6; i++)
    {
        values[i] = token;
        token = strtok(NULL, ",");
    }

    switch(selectSP)
    {
        case 0:
            strcpy(SP1_MISSION_TIME, values[0]);
            strcpy(SP1_PACKET_COUNT, values[1]);
            strcpy(SP1_PACKET_TYPE, values[2]);
            strcpy(SP1_ALTITUDE, values[3]);
            strcpy(SP1_TEMPERATURE, values[4]);
            strcpy(SP1_ROTATION_RATE, values[5]);
            break;
        case 1:
            strcpy(SP2_MISSION_TIME, values[0]);
            strcpy(SP2_PACKET_COUNT, values[1]);
            strcpy(SP2_PACKET_TYPE, values[2]);
            strcpy(SP2_ALTITUDE, values[3]);
            strcpy(SP2_TEMPERATURE, values[4]);
            strcpy(SP2_ROTATION_RATE, values[5]);
            break;
    }
}

int findCommand(char *comm)
{
    int i, select = -1;                         // select: Selection variable for entered command.
    char *token = strtok(comm,",");             // strtok Extracs the data separated by commas y los almacena cada que es llamada
    char *values[] = {"A","B","C","D"};        // Almacena CMD,No. Equipo,Comando,Par�metro respectivamente.

    for(i=0; i<4; i++)
    {
        values[i] = token;                      // Almacena cada uno de los valores que est�n separados por comas
        token = strtok(NULL, ",;");
    }
    if(!strcmp(values[0],"CMD") && !strcmp(values[1],TEAM_NUMBER))
    {
        for(i = 0; i<commandsTam; i++)
        {
            if(!strcmp(values[2], commands[i]))
            {
                select = i;
                break;
            }
        }
        if(select != -1)
        {
            switch(select)
            {
                case CX:
                    commCX(&values[3]);
                    break;
                case ST:
                    commST(&values[3]);
                    break;
                case SIM:
                    commSIM(&values[3]);
                    break;
                case SP1X:
                    commSP1X(&values[3]);
                    break;
                case SP2X:
                    commSP2X(&values[3]);
                    break;
                case SIMP:
                    commSIMP(&values[3]);
                    break;
            }
            return 1;
        }
        else
            return 0;
    }
    else
        return 0;
}

void commCX(char *value[])
{
    //memset(CMD_KEY, 0, sizeof(CMD_KEY));                   // Clean the array CMD_KEY to be used again

    if(!strcmp(*value,"ON"))
    {
        telemetry_ON = true;
    }

    if(!strcmp(*value,"OFF"))
    {
        telemetry_ON = false;
    }
}

void commST(char *value[])
{
    int i;
    char *time[3];
    char *token = strtok(*value,":");
    for(i=0; i<3; i++)
    {
        time[i] = token;
        token = strtok(NULL, ":;");
    }
    // TODO: Time updated to %s:%s:%s"
}
void commSIM(char *value[])
{
    if(!strcmp(*value,"ENABLE"))
    {
        //TODO: "SIMULATION ENABLED         "
        sim_ok = 1;
    }

    else if(!strcmp(*value,"DISABLE"))
    {
        //TODO: "SIMULATION DISABLED"
        sim_ok = 0;
    }

    else if(sim_ok == 1 && !strcmp(*value,"ACTIVATE"))
    {
        // TODO: "SIMULATION ACTIVATED"
        toggle_sim=1;
    }
    else
    {
        // TODO: "SIMULATION IS NOT ENABLED"
    }
}
void commSP1X(char *value[])
{
    if(!strcmp(*value,"ON"))
    {
        // TODO: "SCIENCE PAYLOAD 1 ON"}
    }
    if(!strcmp(*value,"OFF"))
    {
        // TODO: "SCIENCE PAYLOAD 1 OFF"
    }
}
void commSP2X(char *value[])
{
    if(!strcmp(*value,"ON"))
    {
        // TODO: "SCIENCE PAYLOAD 2 ON"
    }
    if(!strcmp(*value,"OFF"))
    {
        // TODO: "SCIENCE PAYLOAD 2 OFF"
    }
}
void commSIMP(char *value[])
{
    PRESS_BAR = atof(*value);
}

