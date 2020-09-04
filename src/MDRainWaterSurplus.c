/******************************************************************************

GHAAS Water Balance/Transport Model V2.0
Global Hydrologic Archive and Analysis System
Copyright 1994-2020, UNH - ASRC/CUNY

MDWaterSurplus.c

bfekete@gc.cuny.edu

*******************************************************************************/

#include <MF.h>
#include <MD.h>

// Input
static int _MDInSPackChgID          = MFUnset;
static int _MDInRainSMoistChgID     = MFUnset;
static int _MDInRainEvapoTransID    = MFUnset;
static int _MDInCommon_PrecipID            = MFUnset;
static int _MDInIrrigation_AreaFracID       = MFUnset;
// Output
static int _MDOutRainWaterSurplusID = MFUnset;

static void _MDRainWaterSurplus (int itemID) {
// Input
	float sPackChg;
	float sMoistChg;
	float evapoTrans; 
	float precip;
	float irrAreaFrac;
// Output
	float surplus;
 
	irrAreaFrac = _MDInIrrigation_AreaFracID != MFUnset ? MFVarGetFloat (_MDInIrrigation_AreaFracID, itemID, 0.0) : 0.0;
	sPackChg    = MFVarGetFloat (_MDInSPackChgID,       itemID, 0.0) * (1.0 - irrAreaFrac);
	sMoistChg   = MFVarGetFloat (_MDInRainSMoistChgID,  itemID, 0.0);
	evapoTrans  = MFVarGetFloat (_MDInRainEvapoTransID, itemID, 0.0);
	precip      = MFVarGetFloat (_MDInCommon_PrecipID,         itemID, 0.0) * (1.0 - irrAreaFrac);

	surplus = precip - sPackChg - evapoTrans - sMoistChg;

	if (surplus < 0.0) surplus = 0.0;	//RJS 071811

	MFVarSetFloat (_MDOutRainWaterSurplusID, itemID, surplus);
}

int MDRainWaterSurplusDef () {
    int ret;
	if (_MDOutRainWaterSurplusID != MFUnset) return (_MDOutRainWaterSurplusID);

	MFDefEntering ("Rainfed Water Surplus");
    if (((ret = MDIrrGrossDemandDef ()) == CMfailed) ||
        ((ret != MFUnset) && ((_MDInIrrigation_AreaFracID = MDIrrigatedAreaDef ()) ==  CMfailed)))
        return (CMfailed);

    if (((_MDInRainSMoistChgID     = MDRainSMoistChgDef ()) == CMfailed) ||
        ((_MDInCommon_PrecipID            = MDPrecipitationDef ()) == CMfailed) ||
        ((_MDInSPackChgID          = MDSPackChgDef      ()) == CMfailed) ||
        ((_MDInRainEvapoTransID    = MFVarGetID (MDVarCore_RainEvapotranspiration, "mm", MFInput, MFFlux, MFBoundary)) == CMfailed) ||
        ((_MDOutRainWaterSurplusID = MFVarGetID (MDVarCore_RainWaterSurplus, "mm", MFOutput, MFFlux, MFBoundary)) == CMfailed) ||
        (MFModelAddFunction (_MDRainWaterSurplus) == CMfailed)) return (CMfailed);
	MFDefLeaving ("Rainfed Water Surplus");
	return (_MDOutRainWaterSurplusID);
}
