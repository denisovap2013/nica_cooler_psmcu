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
#define  BAR_PROGRAM_MENU_SEPARATOR_2     5
#define  BAR_PROGRAM_MENU_CONSOLE_VIEW    6       /* callback function: ShowHideConsole */
#define  BAR_COMMANDS                     7
#define  BAR_COMMANDS_CLEAR_ERRS          8       /* callback function: menuCommandsClearAllErrors */
#define  BAR_COMMANDS_RESET_CGW_RECONN    9       /* callback function: resetCgwReconn */
#define  BAR_COMMANDS_SEPARATOR           10
#define  BAR_COMMANDS_EXTRA               11
#define  BAR_COMMANDS_EXTRA_SUBMENU       12
#define  BAR_COMMANDS_EXTRA_RELOAD_NAMES  13      /* callback function: menuExtraReloadNames */
#define  BAR_COMMANDS_EXTRA_ITEM1         14      /* callback function: debugSetErrorsAll */
#define  BAR_COMMANDS_EXTRA_ITEM2         15      /* callback function: debugSetErrorsSingle */


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
void CVICALLBACK resetCgwReconn(int menubar, int menuItem, void *callbackData, int panel);
void CVICALLBACK ShowHideConsole(int menubar, int menuItem, void *callbackData, int panel);
int  CVICALLBACK tick(int panel, int control, int event, void *callbackData, int eventData1, int eventData2);


#ifdef __cplusplus
    }
#endif
