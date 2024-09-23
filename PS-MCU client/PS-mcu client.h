/**************************************************************************/
/* LabWindows/CVI User Interface Resource (UIR) Include File              */
/* Copyright (c) National Instruments 2024. All Rights Reserved.          */
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

#define  errPanel                         2       /* callback function: errPanelCallback */
#define  errPanel_textbox                 2

#define  Graph                            3       /* callback function: graphCallback */
#define  Graph_minValue                   2       /* callback function: graphVerticalRange */
#define  Graph_maxValue                   3       /* callback function: graphVerticalRange */
#define  Graph_currentValue               4
#define  Graph_GRAPH                      5

#define  psMcuPanel                       4       /* callback function: adcPanelCallback */


     /* Menu Bars, Menus, and Menu Items: */

#define  BAR                              1
#define  BAR_PROGRAM_MENU                 2
#define  BAR_PROGRAM_MENU_SAVE_VIEW       3       /* callback function: menuProgramSaveView */
#define  BAR_PROGRAM_MENU_LOAD_VIEW       4       /* callback function: menuProgramLoadView */
#define  BAR_COMMANDS                     5
#define  BAR_COMMANDS_CLEAR_ERRS          6       /* callback function: menuCommandsClearAllErrors */
#define  BAR_COMMANDS_ITEM1               7       /* callback function: debugSetErrorsAll */
#define  BAR_COMMANDS_ITEM2               8       /* callback function: debugSetErrorsSingle */
#define  BAR_COMMANDS_SEPARATOR           9
#define  BAR_COMMANDS_EXTRA               10
#define  BAR_COMMANDS_EXTRA_SUBMENU       11
#define  BAR_COMMANDS_EXTRA_RELOAD_NAMES  12      /* callback function: menuExtraReloadNames */


     /* Callback Prototypes: */

int  CVICALLBACK adcPanelCallback(int panel, int event, void *callbackData, int eventData1, int eventData2);
void CVICALLBACK debugSetErrorsAll(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK debugSetErrorsSingle(int menubar, int menuItem, void *callbackData, int panel);
int  CVICALLBACK errPanelCallback(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK graphCallback(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK graphVerticalRange(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);
void CVICALLBACK menuCommandsClearAllErrors(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK menuExtraReloadNames(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK menuProgramLoadView(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK menuProgramSaveView(int menubar, int menuItem, void *callbackData, int panel);
int  CVICALLBACK panelCB(int panel, int event, void *callbackData, int eventData1, int eventData2);
int  CVICALLBACK tick(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif
