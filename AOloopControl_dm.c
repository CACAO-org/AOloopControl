/**
 * @file    AOloopControl_dm.c 
 * @brief   AO loop Control functions wave front sensor and deformable mirror 
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



#ifdef __MACH__   // for Mac OS X - 
//#include <mach/mach_time.h>
//#define CLOCK_REALTIME 0
//#define CLOCK_MONOTONIC 0
int clock_gettime(int clk_id, struct mach_timespec *t) {
    mach_timebase_info_data_t timebase;
    mach_timebase_info(&timebase);
    uint64_t time;
    time = mach_absolute_time();
    double nseconds = ((double)time * (double)timebase.numer)/((double)timebase.denom);
    double seconds = ((double)time * (double)timebase.numer)/((double)timebase.denom * 1e9);
    t->tv_sec = seconds;
    t->tv_nsec = nseconds;
    return 0;
}
#else
#include <time.h>
#endif


#include <string.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_blas.h>
#include <pthread.h>
#include "info/info.h" 

//libraries created by O. Guyon 
#include "CommandLineInterface/CLIcore.h"
#include "AOloopControl/AOloopControl.h"
#include "00CORE/00CORE.h"
#include "COREMOD_memory/COREMOD_memory.h"
#include "COREMOD_iofits/COREMOD_iofits.h"
#include "AOloopControl_IOtools/AOloopControl_IOtools.h"

#ifdef HAVE_CUDA
#include "cudacomp/cudacomp.h"
#endif

# ifdef _OPENMP
# include <omp.h>
#define OMP_NELEMENT_LIMIT 1000000
# endif

#define NB_AOloopcontrol 10 // max number of loops

static int AOlooploadconf_init = 0;

// TIMING
static struct timespec tnow;
static struct timespec tdiff;
static double tdiffv;

static int initWFSref_GPU[100] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static long long aoconfcnt0_contrM_current= -1; 

static long wfsrefcnt0 = -1; 
static long contrMcactcnt0[100] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};;



#define AOconfname "/tmp/AOconf.shm"
extern AOLOOPCONTROL_CONF *AOconf; // configuration - this can be an array
extern AOloopControl_var aoloopcontrol_var;





/**
 * ## Purpose
 * 
 * Send modal commands to DM. \n
 * Converts mode coefficient to DM map by matrix-vector multiplication \n
 * Runs in CPU or GPU.
 * 
 * Takes mode values from aol_DMmode_cmd (ID = aoloopcontrol_var.aoconfID_cmd_modes)
 * 
 * 
 * ## Arguments
 * 
 * @param[in]
 * paramname	long
 * 				number of the loop 
 *
 */ 
int_fast8_t set_DM_modes(long loop)
{
    double a;
    long cnttest;
    int semval;



	

    if(AOconf[loop].AOcompute.GPU1 == 0)
    {
        float *arrayf;
        long i, j, k;

        arrayf = (float*) malloc(sizeof(float)*AOconf[loop].DMctrl.sizeDM);

        for(j=0; j<AOconf[loop].DMctrl.sizeDM; j++)
            arrayf[j] = 0.0;

        for(i=0; i<AOconf[loop].DMctrl.sizeDM; i++)
            for(k=0; k < AOconf[loop].AOpmodecoeffs.NBDMmodes; k++)
                arrayf[i] += data.image[aoloopcontrol_var.aoconfID_cmd_modes].array.F[k] * data.image[aoloopcontrol_var.aoconfID_DMmodes].array.F[k*AOconf[loop].DMctrl.sizeDM+i];

        data.image[aoloopcontrol_var.aoconfID_dmC].md[0].write = 1;
        memcpy (data.image[aoloopcontrol_var.aoconfID_dmC].array.F, arrayf, sizeof(float)*AOconf[loop].DMctrl.sizeDM);
        if(data.image[aoloopcontrol_var.aoconfID_dmC].md[0].sem > 0)
        {
            sem_getvalue(data.image[aoloopcontrol_var.aoconfID_dmC].semptr[0], &semval);
            if(semval<SEMAPHORE_MAXVAL)
                sem_post(data.image[aoloopcontrol_var.aoconfID_dmC].semptr[0]);
        }
        data.image[aoloopcontrol_var.aoconfID_dmC].md[0].cnt0++;
		data.image[aoloopcontrol_var.aoconfID_dmC].md[0].cnt1 = AOconf[loop].aorun.LOOPiteration;
        data.image[aoloopcontrol_var.aoconfID_dmC].md[0].write = 0;

        free(arrayf);
    }
    else
    {
#ifdef HAVE_CUDA


        GPU_loop_MultMat_setup(1, data.image[aoloopcontrol_var.aoconfID_DMmodes].name, data.image[aoloopcontrol_var.aoconfID_cmd_modes].name, data.image[aoloopcontrol_var.aoconfID_dmC].name, AOconf[loop].AOcompute.GPU1, aoloopcontrol_var.GPUset1, 1, AOconf[loop].AOcompute.GPUusesem, 1, loop);
        AOconf[loop].AOtiminginfo.status = 12;
        clock_gettime(CLOCK_REALTIME, &tnow);
        tdiff = info_time_diff(data.image[aoloopcontrol_var.aoconfID_looptiming].md[0].atime.ts, tnow);
        tdiffv = 1.0*tdiff.tv_sec + 1.0e-9*tdiff.tv_nsec;
        data.image[aoloopcontrol_var.aoconfID_looptiming].array.F[32] = tdiffv;

        GPU_loop_MultMat_execute(1, &AOconf[loop].AOtiminginfo.status, &AOconf[loop].AOtiminginfo.GPUstatus[0], 1.0, 0.0, 1, 30);
#endif
    }

    if(aoloopcontrol_var.aoconfID_dmdisp!=-1)
        if(data.image[aoloopcontrol_var.aoconfID_dmdisp].md[0].sem > 1)
        {
            sem_getvalue(data.image[aoloopcontrol_var.aoconfID_dmdisp].semptr[1], &semval);
            if(semval<SEMAPHORE_MAXVAL)
                sem_post(data.image[aoloopcontrol_var.aoconfID_dmdisp].semptr[1]);
        }

    AOconf[loop].aorun.DMupdatecnt ++;

    return(0);
}





/**
 * ## Purpose
 * 
 * Set deformable mirror modes related to the response matrix 
 * 
 * Takes mode values from ????????,
 * 
 * 
 * ## Arguments
 * 
 * @param[in]
 * paramname	long
 * 				number of the loop 
 *
 */ 

int_fast8_t set_DM_modesRM(long loop)
{
    long k;
    long i, j;
    float *arrayf;


    arrayf = (float*) malloc(sizeof(float)*AOconf[loop].DMctrl.sizeDM);

    for(j=0; j<AOconf[loop].DMctrl.sizeDM; j++)
        arrayf[j] = 0.0;

    for(k=0; k < AOconf[loop].AOpmodecoeffs.NBDMmodes; k++)
    {
        for(i=0; i<AOconf[loop].DMctrl.sizeDM; i++)
            arrayf[i] += data.image[aoloopcontrol_var.aoconfID_cmd_modesRM].array.F[k] * data.image[aoloopcontrol_var.aoconfID_DMmodes].array.F[k*AOconf[loop].DMctrl.sizeDM+i];
    }


    data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].write = 1;
    memcpy (data.image[aoloopcontrol_var.aoconfID_dmRM].array.F, arrayf, sizeof(float)*AOconf[loop].DMctrl.sizeDM);
    data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].cnt0++;
    data.image[aoloopcontrol_var.aoconfID_dmRM].md[0].write = 0;

    free(arrayf);
    AOconf[loop].aorun.DMupdatecnt ++;

    return(0);
}






/**
 * ## Purpose
 *
 * Compute DM map from mode values.
 * This is a separate process
 *
 * If offloadMode = 1, apply correction to aol#_dmC
 */

int_fast8_t AOloopControl_GPUmodecoeffs2dm_filt_loop(
    const int   GPUMATMULTCONFindex,
    const char *modecoeffs_name,
    const char *DMmodes_name,
    int         semTrigg,
    const char *out_name,
    int         GPUindex,
    long        loop,
    int         offloadMode
)
{
#ifdef HAVE_CUDA
    long IDmodecoeffs;
    int GPUcnt, k;
    int *GPUsetM;
    int_fast8_t GPUstatus[100];
    int_fast8_t status;
    float alpha = 1.0;
    float beta = 0.0;
    int initWFSref = 0;
    int orientation = 1;
    int use_sem = 1;
    long IDout;
    int write_timing = 0;
    long NBmodes, m;

    float x, x2, x4, x8;
    float gamma;

    uint32_t *sizearray;
    char imnameInput[200];
    long IDmodesC;

    long IDc;
    long dmxsize, dmysize;
    long ii;


    int RT_priority = 80; //any number from 0-99
    struct sched_param schedpar;

    char imname[200];


    schedpar.sched_priority = RT_priority;
#ifndef __MACH__
    sched_setscheduler(0, SCHED_FIFO, &schedpar);
#endif



    PROCESSINFO *processinfo;
    if(data.processinfo==1)
    {
        // CREATE PROCESSINFO ENTRY
        // see processtools.c in module CommandLineInterface for details
        //
        char pinfoname[200];
        sprintf(pinfoname, "aol%ld-GPUmodes2dm", %ld);
        processinfo = processinfo_shm_create(pinfoname, 0);
        processinfo->loopstat = 0; // loop initialization

        strcpy(processinfo->source_FUNCTION, __FUNCTION__);
        strcpy(processinfo->source_FILE,     __FILE__);
        processinfo->source_LINE = __LINE__;

        char msgstring[200];
        sprintf(msgstring, "Running on GPU %d", GPUindex);
        processinfo_WriteMessage(processinfo, msgstring);
    }

 // CATCH SIGNALS
 	
	if (sigaction(SIGTERM, &data.sigact, NULL) == -1)
        printf("\ncan't catch SIGTERM\n");

	if (sigaction(SIGINT, &data.sigact, NULL) == -1)
        printf("\ncan't catch SIGINT\n");    

	if (sigaction(SIGABRT, &data.sigact, NULL) == -1)
        printf("\ncan't catch SIGABRT\n");     

	if (sigaction(SIGBUS, &data.sigact, NULL) == -1)
        printf("\ncan't catch SIGBUS\n");

	if (sigaction(SIGSEGV, &data.sigact, NULL) == -1)
        printf("\ncan't catch SIGSEGV\n");         

	if (sigaction(SIGHUP, &data.sigact, NULL) == -1)
        printf("\ncan't catch SIGHUP\n");         

	if (sigaction(SIGPIPE, &data.sigact, NULL) == -1)
        printf("\ncan't catch SIGPIPE\n");   
 
 
 


    if(aoloopcontrol_var.aoconfID_looptiming == -1)
    {
        // LOOPiteration is written in cnt1 of loop timing array
        if(sprintf(imname, "aol%ld_looptiming", aoloopcontrol_var.LOOPNUMBER) < 1)
            printERROR(__FILE__, __func__, __LINE__, "sprintf wrote <1 char");
        aoloopcontrol_var.aoconfID_looptiming = AOloopControl_IOtools_2Dloadcreate_shmim(imname, " ", aoloopcontrol_var.AOcontrolNBtimers, 1, 0.0);
    }


    if(GPUMATMULTCONFindex==0)
    {
        // read AO loop gain, mult
        if(aoloopcontrol_var.AOloopcontrol_meminit==0)
            AOloopControl_InitializeMemory(1);
    }


    GPUcnt = 1;
    GPUsetM = (int*) malloc(sizeof(int)*GPUcnt);
    for(k=0; k<GPUcnt; k++)
        GPUsetM[k] = k+GPUindex;

    IDout = image_ID(out_name);
    IDmodecoeffs = image_ID(modecoeffs_name);

    NBmodes = data.image[IDmodecoeffs].md[0].size[0];


    // sizearray = (uint32_t*) malloc(sizeof(uint32_t)*2);

    //if(sprintf(imnameInput, "aol%ld_mode_limcorr", loop) < 1)
    //    printERROR(__FILE__, __func__, __LINE__, "sprintf wrote <1 char");


    // sizearray[0] = NBmodes;
    // sizearray[1] = 1;
    //  IDmodesC = create_image_ID(imnameInput, 2, sizearray, _DATATYPE_FLOAT, 0, 0);
    //  COREMOD_MEMORY_image_set_createsem(imnamecorr, 10);
    //  free(sizearray);



	printf(" ====================     SETTING UP GPU COMPUTATION\n");
	fflush(stdout);
    
    GPU_loop_MultMat_setup(GPUMATMULTCONFindex, DMmodes_name, modecoeffs_name, out_name, GPUcnt, GPUsetM, orientation, use_sem, initWFSref, 0);


	

    for(k=0; k<GPUcnt; k++)
        printf(" ====================     USING GPU %d\n", GPUsetM[k]);

    list_image_ID();

    if(offloadMode==1)
    {
        char imnamedmC[200];



        if(sprintf(imnamedmC, "aol%ld_dmC", loop) < 1)
            printERROR(__FILE__, __func__, __LINE__, "sprintf wrote <1 char");

        IDc = image_ID(imnamedmC);
        dmxsize = data.image[IDc].md[0].size[0];
        dmysize = data.image[IDc].md[0].size[1];


        printf("offloadMode = %d  %ld %ld\n", offloadMode, dmxsize, dmysize);
        fflush(stdout);
    }
    else
        printf("offloadMode = %d\n", offloadMode);

    printf("out_name = %s \n", out_name);
    printf("IDout    = %ld\n", IDout);


    if(data.processinfo==1)
        processinfo->loopstat = 1; // loop running
    int loopOK = 1;
    int loopCTRLexit = 0; // toggles to 1 when loop is set to exit cleanly
    long loopcnt = 0;

    while(loopOK == 1)
    {
		// processinfo control
        if(data.processinfo==1)
        {
            while(processinfo->CTRLval == 1)  // pause
                usleep(50);

            if(processinfo->CTRLval == 2) // single iteration
                processinfo->CTRLval = 1;

            if(processinfo->CTRLval == 3) // exit loop
                loopCTRLexit = 1;
        }
		
		
        COREMOD_MEMORY_image_set_semwait(modecoeffs_name, semTrigg);

        //		if(GPUMATMULTCONFindex==0)
        AOconf[loop].AOtiminginfo.statusM = 10;
        clock_gettime(CLOCK_REALTIME, &tnow);
        tdiff = info_time_diff(data.image[aoloopcontrol_var.aoconfID_looptiming].md[0].atime.ts, tnow);
        tdiffv = 1.0*tdiff.tv_sec + 1.0e-9*tdiff.tv_nsec;
        data.image[aoloopcontrol_var.aoconfID_looptiming].array.F[7] = tdiffv;

        //  for(m=0; m<NBmodes; m++)
        //      data.image[IDmodesC].array.F[m] = data.image[IDmodecoeffs].array.F[m];


        GPU_loop_MultMat_execute(GPUMATMULTCONFindex, &status, &GPUstatus[0], alpha, beta, write_timing, 0);

        if(offloadMode==1) // offload back to dmC
        {
            data.image[IDc].md[0].write = 1;
            for(ii=0; ii<dmxsize*dmysize; ii++)
                data.image[IDc].array.F[ii] = data.image[IDout].array.F[ii];

            data.image[IDc].md[0].cnt0++;
            data.image[IDc].md[0].cnt1 = data.image[aoloopcontrol_var.aoconfID_looptiming].md[0].cnt1;
            COREMOD_MEMORY_image_set_sempost_byID(IDc, -1);
            data.image[IDc].md[0].write = 0;
        }


        //		if(GPUMATMULTCONFindex==0)
        AOconf[loop].AOtiminginfo.statusM = 20;
        clock_gettime(CLOCK_REALTIME, &tnow);
        tdiff = info_time_diff(data.image[aoloopcontrol_var.aoconfID_looptiming].md[0].atime.ts, tnow);
        tdiffv = 1.0*tdiff.tv_sec + 1.0e-9*tdiff.tv_nsec;
        data.image[aoloopcontrol_var.aoconfID_looptiming].array.F[8] = tdiffv;
    



      // process signals

		if(data.signal_TERM == 1){
			loopOK = 0;
			if(data.processinfo==1)
				processinfo_SIGexit(processinfo, SIGTERM);
		}
     
		if(data.signal_INT == 1){
			loopOK = 0;
			if(data.processinfo==1)
				processinfo_SIGexit(processinfo, SIGINT);
		}

		if(data.signal_ABRT == 1){
			loopOK = 0;
			if(data.processinfo==1)
				processinfo_SIGexit(processinfo, SIGABRT);
		}

		if(data.signal_BUS == 1){
			loopOK = 0;
			if(data.processinfo==1)
				processinfo_SIGexit(processinfo, SIGBUS);
		}
		
		if(data.signal_SEGV == 1){
			loopOK = 0;
			if(data.processinfo==1)
				processinfo_SIGexit(processinfo, SIGSEGV);
		}
		
		if(data.signal_HUP == 1){
			loopOK = 0;
			if(data.processinfo==1)
				processinfo_SIGexit(processinfo, SIGHUP);
		}
		
		if(data.signal_PIPE == 1){
			loopOK = 0;
			if(data.processinfo==1)
				processinfo_SIGexit(processinfo, SIGPIPE);
		}	
        

        loopcnt++;
        if(data.processinfo==1)
            processinfo->loopcnt = loopcnt;
    
    }
    
    
        if(data.processinfo==1)
        processinfo_cleanExit(processinfo);



    free(GPUsetM);

#endif

    return(0);
}







long AOloopControl_dm2dm_offload(const char *streamin, const char *streamout, float twait, float offcoeff, float multcoeff)
{
    long IDin, IDout;
    long cnt = 0;
    long xsize, ysize, xysize;
    long ii;
    //long IDtmp;
	char imname[200];

    IDin = image_ID(streamin);
    IDout = image_ID(streamout);

    xsize = data.image[IDin].md[0].size[0];
    ysize = data.image[IDin].md[0].size[1];
    xysize = xsize*ysize;
    


	if(aoloopcontrol_var.aoconfID_looptiming == -1)
	{
		// LOOPiteration is written in cnt1 of loop timing array
		if(sprintf(imname, "aol%ld_looptiming", aoloopcontrol_var.LOOPNUMBER) < 1)
			printERROR(__FILE__, __func__, __LINE__, "sprintf wrote <1 char");
		aoloopcontrol_var.aoconfID_looptiming = AOloopControl_IOtools_2Dloadcreate_shmim(imname, " ", aoloopcontrol_var.AOcontrolNBtimers, 1, 0.0);
	}


    while(1)
    {
        printf("%8ld : offloading   %s -> %s\n", cnt, streamin, streamout);

        data.image[IDout].md[0].write = 1;
        for(ii=0; ii<xysize; ii++)
            data.image[IDout].array.F[ii] = multcoeff*(data.image[IDout].array.F[ii] + offcoeff*data.image[IDin].array.F[ii]);
        COREMOD_MEMORY_image_set_sempost_byID(IDout, -1);
        data.image[IDout].md[0].cnt0++;
        data.image[IDout].md[0].cnt1 = data.image[aoloopcontrol_var.aoconfID_looptiming].md[0].cnt1;
        data.image[IDout].md[0].write = 0;

        usleep((long) (1000000.0*twait));
        cnt++;
    }

    return(IDout);
}


