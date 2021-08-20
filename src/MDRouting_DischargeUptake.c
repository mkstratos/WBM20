/******************************************************************************

GHAAS Water Balance/Transport Model
Global Hydrological Archive and Analysis System
Copyright 1994-2021, UNH - ASRC/CUNY

MDRouting_DischargeUptake.c

bfekete@gc.cuny.edu

*******************************************************************************/

#include <string.h>
#include <MF.h>
#include <MD.h>

// Inputs
static int _MDInRouting_DischargeInChannelID      = MFUnset;
static int _MDInIrrigation_UptakeExternalID       = MFUnset;
static int _MDInIrrigation_AccumUptakeExternalID  = MFUnset;
static int _MDInIrrigation_ExtractableReleaseID    = MFUnset;

// Outputs
static int _MDOutRouting_DischargeUptakeID        = MFUnset;
static int _MDOutIrrigation_UptakeRiverID         = MFUnset;
static int _MDOutIrrigation_UptakeExcessID        = MFUnset;

static float _MDRiverUptakeFraction = 0.1; // Fraction of the river flow that can be withdrawn.

static void _MDRouting_DischargeUptake (int itemID) {
// Inputs
	float discharge;            // Discharge [m3/s]
	float irrUptakeExt;         // External irrigational water uptake [mm/dt]
	float irrAccumUptakeExt;    // Accumulated external irrigational uptake upstream [m3/s]
	float irrExtractableRelase; // Consumable release from reservoirs [m3/s]
// Outputs
	float irrUptakeRiver = 0.0; // Irrigational water uptake from river [mm/dt]
	float irrUptakeExcess;      // Irrigational water uptake from unsustainable source [mm/dt]
	
	discharge = MFVarGetFloat (_MDInRouting_DischargeInChannelID, itemID, 0.0);

	if (_MDInIrrigation_UptakeExternalID != MFUnset) { // Irrigation is turned on.
		irrUptakeExt = MFVarGetFloat (_MDInIrrigation_UptakeExternalID, itemID, 0.0);
		if (irrUptakeExt > 0.0) {
			if (_MDOutIrrigation_UptakeRiverID != MFUnset) { // River uptake is turned on
				irrAccumUptakeExt = MFVarGetFloat (_MDInIrrigation_AccumUptakeExternalID, itemID, 0.0)
			    	              + irrUptakeExt * MFModelGetArea (itemID) / (MFModelGet_dt () * 1000.0); // converted to m3/s
				irrExtractableRelase = _MDInIrrigation_ExtractableReleaseID != MFUnset ? MFVarGetFloat (_MDInIrrigation_ExtractableReleaseID, itemID, 0.0) : 0.0;
				irrExtractableRelase = 0.0;
				if (irrExtractableRelase  > 0.0)  { // Satisfying irrigation from extractable reservoir release
					if (irrAccumUptakeExt > irrExtractableRelase) { // consumable reservoir release is exhauted, unmet water uptake is reported as unsustaibale withdrawal
						irrUptakeRiver       = irrExtractableRelase * MFModelGet_dt () * 1000.0 / MFModelGetArea (itemID);
						irrUptakeExcess      = irrAccumUptakeExt    * MFModelGet_dt () * 1000.0 / MFModelGetArea (itemID) - irrUptakeRiver; // converted to mm/dt
						irrAccumUptakeExt    = 0.0;
						irrExtractableRelase = 0.0;
					}
					else { // consumable water release satisfies accumulated irrigational water demand
						irrUptakeRiver       = irrAccumUptakeExt * MFModelGet_dt () * 1000.0 / MFModelGetArea (itemID);
						irrAccumUptakeExt    = 0.0;
						irrUptakeExcess      = 0.0;
						irrExtractableRelase -= irrAccumUptakeExt;
					}
					MFVarSetFloat (_MDInIrrigation_ExtractableReleaseID, itemID, irrExtractableRelase);
				}
				else { // accumulated irrigational water demand is sastisfied from river flow without extractable reservoir release.
					if (discharge * _MDRiverUptakeFraction > irrAccumUptakeExt) { 
						irrUptakeRiver    = irrAccumUptakeExt * MFModelGet_dt () * 1000.0 / MFModelGetArea (itemID); // in mm/dt
						irrAccumUptakeExt = 0.0;
					}
					else {
						irrUptakeRiver     = discharge * _MDRiverUptakeFraction * MFModelGet_dt () * 1000.0 / MFModelGetArea (itemID);
						irrAccumUptakeExt -= discharge * _MDRiverUptakeFraction;
					}
					irrUptakeExcess = 0.0;
				}
				MFVarSetFloat (_MDInIrrigation_AccumUptakeExternalID, itemID, irrAccumUptakeExt);
				MFVarSetFloat (_MDOutIrrigation_UptakeRiverID,        itemID, irrUptakeRiver);
			}
			else { // River uptake is turned off all irrigational demand is from unsustainable sources
				irrUptakeRiver  = 0.0;
				irrUptakeExcess = irrUptakeExt;
			}
		}
		else { // External irrigation uptake is zero;
			irrUptakeRiver  = 0.0;
		    irrUptakeExcess = 0.0;
		}
		MFVarSetFloat (_MDOutIrrigation_UptakeExcessID, itemID, irrUptakeExcess);
	}
    MFVarSetFloat (_MDOutRouting_DischargeUptakeID,  itemID, discharge - irrUptakeRiver * MFModelGetArea (itemID) / (MFModelGet_dt () * 1000.0));
}

int MDRouting_DischargeUptakeDef () {
	int optID = MFoff, ret;
	const char *optStr;
	float par;

	if (_MDOutRouting_DischargeUptakeID != MFUnset) return (_MDOutRouting_DischargeUptakeID);

	MFDefEntering ("Discharge - Uptatakes");
	if (((_MDInRouting_DischargeInChannelID  = MDRouting_DischargeInChannelDef()) == CMfailed) ||
        ((_MDOutRouting_DischargeUptakeID    = MFVarGetID ("__DischLevel2",  "m/3", MFOutput, MFState, false)) == CMfailed))
	    return (CMfailed);
	
	if ((ret = MDIrrigation_GrossDemandDef()) != MFUnset) {
		if (ret == CMfailed) return (CMfailed);
		if ((optStr = MFOptionGet ("IrrUptakeRiver")) != (char *) NULL) optID = CMoptLookup (MFswitchOptions, optStr, true);
		switch (optID) {
			default:      MFOptionMessage ("IrrUptakeRiver", optStr, MFswitchOptions); return (CMfailed);
			case MFhelp:  MFOptionMessage ("IrrUptakeRiver", optStr, MFswitchOptions);
			case MFoff:
				if (((_MDInIrrigation_UptakeExternalID      = MFVarGetID (MDVarIrrigation_UptakeExternal, "mm", MFInput, MFFlux, MFBoundary))  == CMfailed) ||
                    ((_MDOutIrrigation_UptakeExcessID       = MFVarGetID (MDVarIrrigation_UptakeExcess,   "mm", MFOutput, MFFlux, MFBoundary)) == CMfailed))
					return (CMfailed);
				break;
			case MFon:
				if ((optStr = MFOptionGet (MDParRiverUptakeFraction))  != (char *) NULL) {
				if (strcmp(optStr,MFhelpStr) == 0) CMmsgPrint (CMmsgInfo,"%s = %f", MDParRiverUptakeFraction, _MDRiverUptakeFraction);
					_MDRiverUptakeFraction = sscanf (optStr,"%f",&par) == 1 ? par : _MDRiverUptakeFraction;
				}

				if (((_MDOutIrrigation_UptakeRiverID        = MDIrrigation_UptakeRiverDef ())       == CMfailed) ||
					((_MDInIrrigation_ExtractableReleaseID  = MDReservoir_ExtractableReleaseDef ()) == CMfailed) ||
					((_MDInIrrigation_UptakeExternalID      = MFVarGetID (MDVarIrrigation_UptakeExternal,      "mm",   MFInput,  MFFlux,   MFBoundary)) == CMfailed) ||
                    ((_MDOutIrrigation_UptakeExcessID       = MFVarGetID (MDVarIrrigation_UptakeExcess,        "mm",   MFOutput, MFFlux,   MFBoundary)) == CMfailed))
					return (CMfailed);
				if ((_MDOutIrrigation_UptakeRiverID != MFUnset) && 
					((_MDInIrrigation_AccumUptakeExternalID = MFVarGetID (MDVarIrrigation_AccumUptakeExternal, "m3/s", MFRoute,  MFState,  MFBoundary)) == CMfailed))
					return (CMfailed);
				break;
		}
	}
	if (MFModelAddFunction(_MDRouting_DischargeUptake) == CMfailed) return (CMfailed);
	MFDefLeaving ("Discharge - Uptakes");
	return (_MDOutRouting_DischargeUptakeID);
}
