/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/* Copyright (c) National Instruments 2022. All Rights Reserved.          */
/*                                                                        */
/* WARNING: Do not add to, delete from, or otherwise modify the contents  */
/*          of this include file.                                         */
/**************************************************************************/

#include <userint.h>

#ifdef __cplusplus
    extern "C" {
#endif

     /* Panels and Controls: */

#define  BlockMenu                        1       /* callback function: panelCB */
#define  BlockMenu_TIMER                  2       /* callback function: tick */

#define  Graph                            2       /* callback function: graphCallback */
#define  Graph_minValue                   2       /* callback function: graphVerticalRange */
#define  Graph_maxValue                   3       /* callback function: graphVerticalRange */
#define  Graph_currentValue               4
#define  Graph_GRAPH                      5

#define  psMcuPanel                       3       /* callback function: adcPanelCallback */


     /* Menu Bars, Menus, and Menu Items: */

          /* (no menu bars in the resource file) */


     /* Callback Prototypes: */

int  CVICALLBACK adcPanelCallback(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK graphCallback(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK graphVerticalRange(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK panelCB(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK tick(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif
