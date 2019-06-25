#!/bin/bash

# TEMPLATE SCRIPT FOR ADDING CACAO FPS-ENABLED PROCESSES 

# For each FPS-enabled process:
# - entry is added to fpslist.txt file
# - parameters setup added to file fpssetup.setval.conf


FPSCONFFILE="fpssetup.setval.conf"
rm ${FPSCONFFILE}

rm fpslist.txt
touch fpslist.txt

echo "# File generated by $0"
echo "# Do not edit"
echo ""


# User is expected to set FPS processes to ON or OFF before running this script
# Examples:

#export CACAO_FPSPROC_DMCOMB="ON"
#export CACAO_FPSPROC_STREAMDELAY="ON"
#export CACAO_FPSPROC_SIMMVMGPU="ON"
#export CACAO_FPSPROC_MLAT="ON"





if [ "${CACAO_FPSPROC_DMCOMB}" = "ON" ]; then
# ==============================================================================
# ==========  DM combination ===================================================
# ==============================================================================

# FPS name
fpsname="DMcomb" 
fpsarg0="${CACAO_DMINDEX}"

# FPS full name
fpsfname="${fpsname}-${fpsarg0}" 

echo "${fpsname}      aolcontrolDMcomb       ${CACAO_DMINDEX}"  >> fpslist.txt

echo "setval AOCONF.DMxsize ${CACAO_DMxsize}" >> ${FPSCONFFILE}
echo "setval AOCONF.DMysize ${CACAO_DMxsize}" >> ${FPSCONFFILE}
fi







if [ "${CACAO_FPSPROC_STREAMDELAY}" = "ON" ]; then
# ==============================================================================
# ==========  streamdelay for simulation mode ==================================
# ==============================================================================

# FPS name
fpsname="streamDelay" 
fpsarg0="${CACAO_DMINDEX}"

# FPS full name
fpsfname="${fpsname}-${fpsarg0}" 

echo "${fpsname}    streamdelay         ${fpsarg0}"  >> fpslist.txt

echo "setval ${fpsfname}.in_name aol${CACAO_LOOPNUMBER}_dmdisp" >> ${FPSCONFFILE}
echo "setval ${fpsfname}.out_name aol${CACAO_LOOPNUMBER}_dmdispD" >> ${FPSCONFFILE}
echo "setval ${fpsfname}.delayus 20000" >> ${FPSCONFFILE}
echo "setval ${fpsfname}.option.timeavemode 1" >> ${FPSCONFFILE}
echo "setval ${fpsfname}.option.avedt 0.005" >> ${FPSCONFFILE}
fi






if [ "${CACAO_FPSPROC_SIMMVMGPU}" = "ON" ]; then
# ==============================================================================
# ========== GPU-based modal extraction ========================================
# ==============================================================================

# FPS name
fpsname="simmvmgpu" 
fpsarg0="${CACAO_LOOPNUMBER}"

# FPS full name
fpsfname="${fpsname}-${fpsarg0}" 

echo "${fpsname}      cudaextrmodes       ${fpsarg0}"  >> fpslist.txt

# time delay [us]
LINSIMDT="10000" 

# copy calibration files
mkdir ${CACAO_WORKDIR}/simLHS
cp simLHS/respM.fits ./${CACAO_WORKDIR}/simLHS/simLHS_respM.fits
cp simLHS/wfsref.fits ./${CACAO_WORKDIR}/simLHS/simLHS_wfsref.fits
cp ./${CACAO_WORKDIR}/simLHS/simLHS_respM.fits ./${CACAO_WORKDIR}/conf/shmim.aolsimLHSresmM.fits


echo "setval ${fpsfname}.sname_in aol${CACAO_LOOPNUMBER}_dmdispD" >> ${FPSCONFFILE}

cp ./simLHS/simLHS_respM.fits ./conf/shmim.aolsimLHSresmM.fits
echo "setval ${fpsfname}.sname_modes aolsimLHSresmM" >> ${FPSCONFFILE}

echo "setval ${fpsfname}.sname_outmodesval aol${CACAO_LOOPNUMBER}_linsimWFS" >> ${FPSCONFFILE}

# run simulator at finite frame rate
echo "setval ${fpsfname}.option.twait ${LINSIMDT}" >> ${FPSCONFFILE}

cp ./simLHS/simLHS_wfsref.fits ./conf/shmim.aolsimLHSwfsref.fits
echo "setval ${fpsfname}.option.sname_refout aolsimLHSwfsref" >> ${FPSCONFFILE}

echo "setval ${fpsfname}.option.insem 6" >> ${FPSCONFFILE}
echo "setval ${fpsfname}.option.axmode 1" >> ${FPSCONFFILE}
fi






if [ "${CACAO_FPSPROC_MLAT}" = "ON" ]; then
# ==============================================================================
# ========== Measure Latency ===================================================
# ==============================================================================

# FPS name
fpsname="mlat" 
fpsarg0="${CACAO_LOOPNUMBER}"

# FPS full name
fpsfname="${fpsname}-${fpsarg0}" 

echo "${fpsname}           aoltestlat     ${fpsarg0}" >> fpslist.txt

echo "setval ${fpsfname}.sn_dm aol${CACAO_LOOPNUMBER}_dmRM" >> ${FPSCONFFILE}
echo "setval ${fpsfname}.sn_wfs aol${CACAO_LOOPNUMBER}_wfsim" >> ${FPSCONFFILE}
fi





