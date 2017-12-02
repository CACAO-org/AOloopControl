/**
 * @file    AOloopControl_arpf_onoff.c 
 * @brief   AO loop control - Predictive Filter ON/OFF through the ARPF multiplicator
 * 
 * REAL TIME COMPUTING ROUTINES
 *  
 * @author  O. Guyon
 * @date    24 nov 2017
 *
 * 
 * @bug No known bugs.
 * 
 */
 
 
#define _GNU_SOURCE

#include "CommandLineInterface/CLIcore.h"
#include "AOloopControl/AOloopControl.h"

#define AOconfname "/tmp/AOconf.shm"
AOLOOPCONTROL_CONF *AOconf; // configuration - this can be an array
AOloopControl_var aoloopcontrol_var;

extern DATA data;



int_fast8_t AOloopControl_ARPFon()
{
    if(aoloopcontrol_var.AOloopcontrol_meminit==0)
        AOloopControl_InitializeMemory(1);

    AOconf[aoloopcontrol_var.LOOPNUMBER].ARPFon = 1;
    AOloopControl_perfTest_showparams(aoloopcontrol_var.LOOPNUMBER);

    return 0;
}


int_fast8_t AOloopControl_ARPFoff()
{
    if(aoloopcontrol_var.AOloopcontrol_meminit==0)
        AOloopControl_InitializeMemory(1);

    AOconf[aoloopcontrol_var.LOOPNUMBER].ARPFon = 0;
    AOloopControl_perfTest_showparams(aoloopcontrol_var.LOOPNUMBER);

    return 0;
}