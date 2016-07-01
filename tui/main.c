/**
 * @file main.c
 * @brief Contains the main() implementation and supporting functions.
 * @author Copyright (c) 2012-2016 Marc A. Smith
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <cdk.h>
#include <sys/wait.h>
#include <syslog.h>
#include <fcntl.h>
#include <iniparser.h>
#include <uuid/uuid.h>
#include <curl/curl.h>
#include <assert.h>

#include "prototypes.h"
#include "system.h"
#include "dialogs.h"
#include "strings.h"


/* A few global variables */
ThemeNum g_curr_theme;
chtype g_color_main_text[MAX_TUI_THEMES];
chtype g_color_main_box[MAX_TUI_THEMES];
chtype g_color_dialog_text[MAX_TUI_THEMES];
chtype g_color_dialog_box[MAX_TUI_THEMES];
chtype g_color_dialog_select[MAX_TUI_THEMES];
chtype g_color_dialog_input[MAX_TUI_THEMES];
chtype g_color_error_text[MAX_TUI_THEMES];
chtype g_color_error_box[MAX_TUI_THEMES];
chtype g_color_error_select[MAX_TUI_THEMES];
chtype g_color_menu_text[MAX_TUI_THEMES];
chtype g_color_status_bar[MAX_TUI_THEMES];
int g_color_menu_letter[MAX_TUI_THEMES];
char *g_color_menu_bg[MAX_TUI_THEMES];
int g_color_info_header[MAX_TUI_THEMES];
int g_color_dialog_title[MAX_TUI_THEMES];


int main(int argc, char** argv) {
    CDKSCREEN *cdk_screen = 0;
    WINDOW *main_window = 0, *sub_window = 0;
    CDKMENU *menu_1 = 0, *menu_2 = 0;
    CDKLABEL *targets_label = 0, *sessions_label = 0;
    char *menu_list_1[MAX_MENU_ITEMS][MAX_SUB_ITEMS] = {{NULL}, {NULL}},
            *menu_list_2[MAX_MENU_ITEMS][MAX_SUB_ITEMS] = {{NULL}, {NULL}};
    char *tgt_label_msg[MAX_INFO_LABEL_ROWS] = {NULL},
            *sess_label_msg[MAX_INFO_LABEL_ROWS] = {NULL};
    char *error_msg = NULL;
    int selection = 0, key_pressed = 0, menu_choice = 0, submenu_choice = 0,
            screen_x = 0, screen_y = 0, latest_scr_y = 0, latest_scr_x = 0,
            i = 0, child_status = 0, proc_status = 0, tty_fd = 0,
            labels_last_scr_y = 0, labels_last_scr_x = 0,
            last_tgt_lbl_rows = 0, last_sess_lbl_rows = 0,
            menu_1_cnt = 0, menu_2_cnt = 0;
    int submenu_size_1[CDK_MENU_MAX_SIZE] = {0},
            menu_loc_1[CDK_MENU_MAX_SIZE] = {0},
            submenu_size_2[CDK_MENU_MAX_SIZE] = {0},
            menu_loc_2[CDK_MENU_MAX_SIZE] = {0};
    pid_t child_pid = 0;
    uid_t saved_uid = 0;
    boolean inet_works = FALSE;

    /* The "blue" TUI theme */
    g_color_main_text[BLUE_TUI]         = COLOR_PAIR(5);
    g_color_main_box[BLUE_TUI]          = COLOR_PAIR(53);
    g_color_dialog_text[BLUE_TUI]       = COLOR_PAIR(7);
    g_color_dialog_box[BLUE_TUI]        = COLOR_PAIR(7);
    g_color_dialog_select[BLUE_TUI]     = COLOR_PAIR(29) | A_BOLD;
    g_color_dialog_input[BLUE_TUI]      = COLOR_PAIR(5);
    g_color_error_text[BLUE_TUI]        = COLOR_PAIR(2);
    g_color_error_box[BLUE_TUI]         = COLOR_PAIR(2);
    g_color_error_select[BLUE_TUI]      = COLOR_PAIR(57);
    g_color_menu_text[BLUE_TUI]         = COLOR_PAIR(31);
    g_color_status_bar[BLUE_TUI]        = COLOR_PAIR(7);
    g_color_menu_letter[BLUE_TUI]       = 29;
    g_color_menu_bg[BLUE_TUI]           = "</5>";
    g_color_info_header[BLUE_TUI]       = 21;
    g_color_dialog_title[BLUE_TUI]      = 31;

    /* The "black" TUI theme */
    g_color_main_text[BLACK_TUI]        = COLOR_PAIR(8);
    g_color_main_box[BLACK_TUI]         = COLOR_PAIR(24);
    g_color_dialog_text[BLACK_TUI]      = COLOR_PAIR(3);
    g_color_dialog_box[BLACK_TUI]       = COLOR_PAIR(3);
    g_color_dialog_select[BLACK_TUI]    = COLOR_PAIR(32) | A_BOLD;
    g_color_dialog_input[BLACK_TUI]     = COLOR_PAIR(8);
    g_color_error_text[BLACK_TUI]       = COLOR_PAIR(2);
    g_color_error_box[BLACK_TUI]        = COLOR_PAIR(2);
    g_color_error_select[BLACK_TUI]     = COLOR_PAIR(57);
    g_color_menu_text[BLACK_TUI]        = COLOR_PAIR(27);
    g_color_status_bar[BLACK_TUI]       = COLOR_PAIR(3);
    g_color_menu_letter[BLACK_TUI]      = 32;
    g_color_menu_bg[BLACK_TUI]          = "</8>";
    g_color_info_header[BLACK_TUI]      = 40;
    g_color_dialog_title[BLACK_TUI]     = 27;

    /* Make sure the umask is something sane (per the man
     * page, this call always succeeds) */
    umask(0022);

    /* Setup logging for debug messages */
    openlog(TUI_LOG_PREFIX, TUI_LOG_OPTIONS, TUI_LOG_FACILITY);
    DEBUG_LOG("TUI start-up...");

    /* Check if there is Internet access */
    inet_works = checkInetAccess();

    /* Set the TUI interface theme (colors) */
    setTheme();

    /* Initialize screen and check size */
start:
    main_window = initscr();
    curs_set(0);
    noecho();
    getmaxyx(main_window, screen_y, screen_x);
    if (screen_y < MIN_SCR_Y || screen_x < MIN_SCR_X)
        termSize(main_window);

    /* Setup CDK */
    sub_window = newwin((LINES - 2), (COLS - 2), 1, 1);
    latest_scr_y = LINES;
    latest_scr_x = COLS;
    wbkgd(main_window, g_color_main_text[g_curr_theme]);
    wbkgd(sub_window, g_color_main_text[g_curr_theme]);
    cdk_screen = initCDKScreen(sub_window);
    initCDKColor();

    /* Create the menu lists */
    SAFE_ASPRINTF(&menu_list_1[SYSTEM_MENU][0],
            "</%d/B/U>S<!%d><!U>ystem  <!B>",
            g_color_menu_letter[g_curr_theme],
            g_color_menu_letter[g_curr_theme]);
    menu_list_1[SYSTEM_MENU][SYSTEM_SYNC_CONF] = \
            "</B>Sync Configuration<!B>";
    menu_list_1[SYSTEM_MENU][SYSTEM_NETWORK] = \
            "</B>Network Settings  <!B>";
    menu_list_1[SYSTEM_MENU][SYSTEM_RESTART_NET] = \
            "</B>Restart Networking<!B>";
    menu_list_1[SYSTEM_MENU][SYSTEM_MAIL] = \
            "</B>Mail Setup        <!B>";
    menu_list_1[SYSTEM_MENU][SYSTEM_TEST_EMAIL] = \
            "</B>Send Test Email   <!B>";
    menu_list_1[SYSTEM_MENU][SYSTEM_ADD_USER] = \
            "</B>Add User          <!B>";
    menu_list_1[SYSTEM_MENU][SYSTEM_DEL_USER] = \
            "</B>Delete User       <!B>";
    menu_list_1[SYSTEM_MENU][SYSTEM_CHG_PASSWD] = \
            "</B>Change Password   <!B>";
    menu_list_1[SYSTEM_MENU][SYSTEM_SCST_INFO] = \
            "</B>SCST Information  <!B>";
    menu_list_1[SYSTEM_MENU][SYSTEM_CRM_STATUS] = \
            "</B>CRM Status        <!B>";
    menu_list_1[SYSTEM_MENU][SYSTEM_DRBD_STATUS] = \
            "</B>DRBD Status       <!B>";
    menu_list_1[SYSTEM_MENU][SYSTEM_DATE_TIME] = \
            "</B>Date/Time Settings<!B>";

    SAFE_ASPRINTF(&menu_list_1[HW_RAID_MENU][0],
            "</B>H</%d/U>a<!%d><!U>rdware RAID  <!B>",
            g_color_menu_letter[g_curr_theme],
            g_color_menu_letter[g_curr_theme]);
    menu_list_1[HW_RAID_MENU][HW_RAID_ADD_VOL] = \
            "</B>Add Volume      <!B>";
    menu_list_1[HW_RAID_MENU][HW_RAID_REM_VOL] = \
            "</B>Remove Volume   <!B>";
    menu_list_1[HW_RAID_MENU][HW_RAID_MOD_VOL] = \
            "</B>Modify Volume   <!B>";
    menu_list_1[HW_RAID_MENU][HW_RAID_ADD_HSP] = \
            "</B>Add Hot Spare   <!B>";
    menu_list_1[HW_RAID_MENU][HW_RAID_REM_HSP] = \
            "</B>Remove Hot Spare<!B>";

    SAFE_ASPRINTF(&menu_list_1[SW_RAID_MENU][0],
            "</B>S</%d/U>o<!%d><!U>ftware RAID  <!B>",
            g_color_menu_letter[g_curr_theme],
            g_color_menu_letter[g_curr_theme]);
    menu_list_1[SW_RAID_MENU][SW_RAID_MD_STAT] = \
            "</B>Linux MD Status  <!B>";
    menu_list_1[SW_RAID_MENU][SW_RAID_ADD_ARRAY] = \
            "</B>Add Array        <!B>";
    menu_list_1[SW_RAID_MENU][SW_RAID_REM_ARRAY] = \
            "</B>Remove Array     <!B>";
    menu_list_1[SW_RAID_MENU][SW_RAID_FAULT_DEV] = \
            "</B>Set Device Faulty<!B>";
    menu_list_1[SW_RAID_MENU][SW_RAID_ADD_DEV] = \
            "</B>Add Device       <!B>";
    menu_list_1[SW_RAID_MENU][SW_RAID_REM_DEV] = \
            "</B>Remove Device    <!B>";

    SAFE_ASPRINTF(&menu_list_1[LVM_MENU][0],
            "</%d/B/U>L<!%d><!U>VM  <!B>",
            g_color_menu_letter[g_curr_theme],
            g_color_menu_letter[g_curr_theme]);
    menu_list_1[LVM_MENU][LVM_LV_LIST] = \
            "</B>Logical Volume (LV) List   <!B>";
    menu_list_1[LVM_MENU][LVM_ADD_PV] = \
            "</B>Add Physical Volume (PV)   <!B>";
    menu_list_1[LVM_MENU][LVM_REM_PV] = \
            "</B>Remove Physical Volume (PV)<!B>";
    menu_list_1[LVM_MENU][LVM_ADD_VG] = \
            "</B>Add Volume Group (VG)      <!B>";
    menu_list_1[LVM_MENU][LVM_REM_VG] = \
            "</B>Remove Volume Group (VG)   <!B>";
    menu_list_1[LVM_MENU][LVM_ADD_LV] = \
            "</B>Add Logical Volume (LV)    <!B>";
    menu_list_1[LVM_MENU][LVM_REM_LV] = \
            "</B>Remove Logical Volume (LV) <!B>";

    SAFE_ASPRINTF(&menu_list_1[FILE_SYS_MENU][0],
            "</%d/B/U>F<!%d><!U>ile Systems  <!B>",
            g_color_menu_letter[g_curr_theme],
            g_color_menu_letter[g_curr_theme]);
    menu_list_1[FILE_SYS_MENU][FILE_SYS_VDISK_LIST] = \
            "</B>VDisk File List   <!B>";
    menu_list_1[FILE_SYS_MENU][FILE_SYS_ADD_FS] = \
            "</B>Add File System   <!B>";
    menu_list_1[FILE_SYS_MENU][FILE_SYS_REM_FS] = \
            "</B>Remove File System<!B>";
    menu_list_1[FILE_SYS_MENU][FILE_SYS_ADD_VDISK] = \
            "</B>Add VDisk File    <!B>";
    menu_list_1[FILE_SYS_MENU][FILE_SYS_REM_VDISK] = \
            "</B>Remove VDisk File <!B>";

    SAFE_ASPRINTF(&menu_list_2[HOSTS_MENU][0],
            "</%d/B/U>H<!%d><!U>osts  <!B>",
            g_color_menu_letter[g_curr_theme],
            g_color_menu_letter[g_curr_theme]);
    menu_list_2[HOSTS_MENU][HOSTS_ADD_GROUP] = \
            "</B>Add Group       <!B>";
    menu_list_2[HOSTS_MENU][HOSTS_REM_GROUP] = \
            "</B>Remove Group    <!B>";
    menu_list_2[HOSTS_MENU][HOSTS_ADD_INIT] = \
            "</B>Add Initiator   <!B>";
    menu_list_2[HOSTS_MENU][HOSTS_REM_INIT] = \
            "</B>Remove Initiator<!B>";

    SAFE_ASPRINTF(&menu_list_2[DEVICES_MENU][0],
            "</%d/B/U>D<!%d><!U>evices  <!B>",
            g_color_menu_letter[g_curr_theme],
            g_color_menu_letter[g_curr_theme]);
    menu_list_2[DEVICES_MENU][DEVICES_LUN_LAYOUT] = \
            "</B>LUN/Group Layout     <!B>";
    menu_list_2[DEVICES_MENU][DEVICES_DEV_INFO] = \
            "</B>Device Information   <!B>";
    menu_list_2[DEVICES_MENU][DEVICES_ADD_DEV] = \
            "</B>Add Device           <!B>";
    menu_list_2[DEVICES_MENU][DEVICES_REM_DEV] = \
            "</B>Remove Device        <!B>";
    menu_list_2[DEVICES_MENU][DEVICES_MAP_TO] = \
            "</B>Map to Host Group    <!B>";
    menu_list_2[DEVICES_MENU][DEVICES_UNMAP_FROM] = \
            "</B>Unmap from Host Group<!B>";

    SAFE_ASPRINTF(&menu_list_2[TARGETS_MENU][0],
            "</%d/B/U>T<!%d><!U>argets  <!B>",
            g_color_menu_letter[g_curr_theme],
            g_color_menu_letter[g_curr_theme]);
    menu_list_2[TARGETS_MENU][TARGETS_TGT_INFO] = \
            "</B>Target Information    <!B>";
    menu_list_2[TARGETS_MENU][TARGETS_ADD_ISCSI] = \
            "</B>Add iSCSI Target      <!B>";
    menu_list_2[TARGETS_MENU][TARGETS_REM_ISCSI] = \
            "</B>Remove iSCSI Target   <!B>";
    menu_list_2[TARGETS_MENU][TARGETS_LIP] = \
            "</B>Issue LIP             <!B>";
    menu_list_2[TARGETS_MENU][TARGETS_TOGGLE] = \
            "</B>Enable/Disable Target <!B>";
    menu_list_2[TARGETS_MENU][TARGETS_SET_REL_TGT_ID] = \
            "</B>Set Relative Target ID<!B>";

    SAFE_ASPRINTF(&menu_list_2[ALUA_MENU][0],
            "</B>AL</%d/U>U<!%d><!U>A  <!B>",
            g_color_menu_letter[g_curr_theme],
            g_color_menu_letter[g_curr_theme]);
    menu_list_2[ALUA_MENU][ALUA_DEV_GRP_LAYOUT] = \
            "</B>Device/Target Group Layout<!B>";
    menu_list_2[ALUA_MENU][ALUA_ADD_DEV_GRP] = \
            "</B>Add Device Group          <!B>";
    menu_list_2[ALUA_MENU][ALUA_REM_DEV_GRP] = \
            "</B>Remove Device Group       <!B>";
    menu_list_2[ALUA_MENU][ALUA_ADD_TGT_GRP] = \
            "</B>Add Target Group          <!B>";
    menu_list_2[ALUA_MENU][ALUA_REM_TGT_GRP] = \
            "</B>Remove Target Group       <!B>";
    menu_list_2[ALUA_MENU][ALUA_ADD_DEV_TO_GRP] = \
            "</B>Add Device to Group       <!B>";
    menu_list_2[ALUA_MENU][ALUA_REM_DEV_FROM_GRP] = \
            "</B>Remove Device from Group  <!B>";
    menu_list_2[ALUA_MENU][ALUA_ADD_TGT_TO_GRP] = \
            "</B>Add Target to Group       <!B>";
    menu_list_2[ALUA_MENU][ALUA_REM_TGT_FROM_GRP] = \
            "</B>Remove Target from Group  <!B>";

    SAFE_ASPRINTF(&menu_list_2[INTERFACE_MENU][0],
            "</%d/B/U>I<!%d><!U>nterface  <!B>",
            g_color_menu_letter[g_curr_theme],
            g_color_menu_letter[g_curr_theme]);
    menu_list_2[INTERFACE_MENU][INTERFACE_QUIT] = \
            "</B>Quit          <!B>";
    menu_list_2[INTERFACE_MENU][INTERFACE_SHELL] = \
            "</B>Exit to Shell <!B>";
    menu_list_2[INTERFACE_MENU][INTERFACE_THEME] = \
            "</B>Color Theme   <!B>";
    menu_list_2[INTERFACE_MENU][INTERFACE_HELP] = \
            "</B>Help          <!B>";
    menu_list_2[INTERFACE_MENU][INTERFACE_SUPPORT_PKG] = \
            "</B>Support Bundle<!B>";
    menu_list_2[INTERFACE_MENU][INTERFACE_ABOUT] = \
            "</B>About         <!B>";

    /* Set top menu sizes and locations */
    submenu_size_1[SYSTEM_MENU]       = 13;
    menu_loc_1[SYSTEM_MENU]           = LEFT;
    submenu_size_1[HW_RAID_MENU]      = 6;
    menu_loc_1[HW_RAID_MENU]          = LEFT;
    submenu_size_1[SW_RAID_MENU]      = 7;
    menu_loc_1[SW_RAID_MENU]          = LEFT;
    submenu_size_1[LVM_MENU]          = 8;
    menu_loc_1[LVM_MENU]              = LEFT;
    submenu_size_1[FILE_SYS_MENU]     = 6;
    menu_loc_1[FILE_SYS_MENU]         = LEFT;

    /* Set bottom menu sizes and locations */
    submenu_size_2[HOSTS_MENU]        = 5;
    menu_loc_2[HOSTS_MENU]            = LEFT;
    submenu_size_2[DEVICES_MENU]      = 7;
    menu_loc_2[DEVICES_MENU]          = LEFT;
    submenu_size_2[TARGETS_MENU]      = 7;
    menu_loc_2[TARGETS_MENU]          = LEFT;
    submenu_size_2[ALUA_MENU]         = 10;
    menu_loc_2[ALUA_MENU]             = LEFT;
    submenu_size_2[INTERFACE_MENU]    = 7;
    menu_loc_2[INTERFACE_MENU]        = RIGHT;

    /* Create the top menu */
    menu_1_cnt = 5;
    menu_1 = newCDKMenu(cdk_screen, menu_list_1, menu_1_cnt, submenu_size_1,
            menu_loc_1, 0, A_NORMAL, g_color_menu_text[g_curr_theme]);
    if (!menu_1) {
        errorDialog(cdk_screen, MENU_ERR_MSG, NULL);
        goto quit;
    }
    setCDKMenuBackgroundColor(menu_1, g_color_menu_bg[g_curr_theme]);

    /* Create the bottom menu */
    menu_2_cnt = 5;
    menu_2 = newCDKMenu(cdk_screen, menu_list_2, menu_2_cnt, submenu_size_2,
            menu_loc_2, 1, A_NORMAL, g_color_menu_text[g_curr_theme]);
    if (!menu_2) {
        errorDialog(cdk_screen, MENU_ERR_MSG, NULL);
        goto quit;
    }
    setCDKMenuBackgroundColor(menu_2, g_color_menu_bg[g_curr_theme]);

    /* We need root privileges; for the short term I don't see any other way
     * around this; long term we can hopefully do something else */
    saved_uid = getuid();
    if (setresuid(0, -1, saved_uid) == -1) {
        SAFE_ASPRINTF(&error_msg, "setresuid(): %s", strerror(errno));
        errorDialog(cdk_screen, error_msg,
                "Your capabilities may be reduced...");
        FREE_NULL(error_msg);
    }

    /* Draw the CDK screen */
    statusBar(main_window);
    refreshCDKScreen(cdk_screen);

    /* Usage count (only if we have Internet) */
    if (inet_works)
        reportUsage(cdk_screen);

    /* Check and see if SCST is loaded */
    if (! isSCSTLoaded()) {
        errorDialog(cdk_screen,
                "It appears SCST is not loaded; a number of the TUI",
                "functions will not work. Check the '/var/log/boot' file.");
    }

    /* Loop, refreshing the labels and waiting for input */
    halfdelay(REFRESH_DELAY);
    for (;;) {
        /* Update the information labels */
        if (!updateInfoLabels(cdk_screen, &targets_label, &sessions_label,
                tgt_label_msg, sess_label_msg,
                &labels_last_scr_y, &labels_last_scr_x,
                &last_tgt_lbl_rows, &last_sess_lbl_rows))
            goto quit;

        /* Get user input */
        wrefresh(sub_window);
        keypad(sub_window, TRUE);
        key_pressed = wgetch(sub_window);

        /* Check and see what we got */
        if (key_pressed == 's' || key_pressed == 'S') {
            /* Start with the System menu */
            cbreak();
            setCDKMenu(menu_1, SYSTEM_MENU, 0, A_NORMAL,
                    g_color_menu_text[g_curr_theme]);
            selection = activateCDKMenu(menu_1, 0);

        } else if (key_pressed == 'a' || key_pressed == 'A') {
            /* Start with the Hardware RAID menu */
            cbreak();
            setCDKMenu(menu_1, HW_RAID_MENU, 0, A_NORMAL,
                    g_color_menu_text[g_curr_theme]);
            selection = activateCDKMenu(menu_1, 0);

        } else if (key_pressed == 'o' || key_pressed == 'O') {
            /* Start with the Software RAID menu */
            cbreak();
            setCDKMenu(menu_1, SW_RAID_MENU, 0, A_NORMAL,
                    g_color_menu_text[g_curr_theme]);
            selection = activateCDKMenu(menu_1, 0);

        } else if (key_pressed == 'l' || key_pressed == 'L') {
            /* Start with the LVM menu */
            cbreak();
            setCDKMenu(menu_1, LVM_MENU, 0, A_NORMAL,
                    g_color_menu_text[g_curr_theme]);
            selection = activateCDKMenu(menu_1, 0);

        } else if (key_pressed == 'f' || key_pressed == 'F') {
            /* Start with the File System menu */
            cbreak();
            setCDKMenu(menu_1, FILE_SYS_MENU, 0, A_NORMAL,
                    g_color_menu_text[g_curr_theme]);
            selection = activateCDKMenu(menu_1, 0);

        } else if (key_pressed == 'h' || key_pressed == 'H') {
            /* Start with the Hosts menu */
            cbreak();
            setCDKMenu(menu_2, HOSTS_MENU, 0, A_NORMAL,
                    g_color_menu_text[g_curr_theme]);
            selection = activateCDKMenu(menu_2, 0);

        } else if (key_pressed == 'd' || key_pressed == 'D') {
            /* Start with the Devices menu */
            cbreak();
            setCDKMenu(menu_2, DEVICES_MENU, 0, A_NORMAL,
                    g_color_menu_text[g_curr_theme]);
            selection = activateCDKMenu(menu_2, 0);

        } else if (key_pressed == 't' || key_pressed == 'T') {
            /* Start with the Targets menu */
            cbreak();
            setCDKMenu(menu_2, TARGETS_MENU, 0, A_NORMAL,
                    g_color_menu_text[g_curr_theme]);
            selection = activateCDKMenu(menu_2, 0);

        } else if (key_pressed == 'u' || key_pressed == 'U') {
            /* Start with the ALUA menu */
            cbreak();
            setCDKMenu(menu_2, ALUA_MENU, 0, A_NORMAL,
                    g_color_menu_text[g_curr_theme]);
            selection = activateCDKMenu(menu_2, 0);

        } else if (key_pressed == 'i' || key_pressed == 'I') {
            /* Start with the Interface menu */
            cbreak();
            setCDKMenu(menu_2, INTERFACE_MENU, 0, A_NORMAL,
                    g_color_menu_text[g_curr_theme]);
            selection = activateCDKMenu(menu_2, 0);

        } else if (key_pressed == KEY_RESIZE) {
            /* Screen re-size */
            screenResize(cdk_screen, main_window, sub_window,
                    &latest_scr_y, &latest_scr_x);
            continue;

        } else if (key_pressed == ERR) {
            /* Looks like the user didn't press anything (half-delay mode) */
            continue;

        } else {
            beep();
            continue;
        }

        if (menu_1->exitType == vNORMAL) {
            /* Get the selected menu choice */
            menu_choice = selection / 100;
            submenu_choice = selection % 100;

            if (menu_choice == SYSTEM_MENU &&
                    submenu_choice == SYSTEM_SYNC_CONF - 1) {
                /* Sync. Configuration dialog */
                syncConfig(cdk_screen);

            } else if (menu_choice == SYSTEM_MENU &&
                    submenu_choice == SYSTEM_NETWORK - 1) {
                /* Networking dialog */
                networkDialog(cdk_screen);

            } else if (menu_choice == SYSTEM_MENU &&
                    submenu_choice == SYSTEM_RESTART_NET - 1) {
                /* Restart Networking dialog */
                restartNetDialog(cdk_screen);

            } else if (menu_choice == SYSTEM_MENU &&
                    submenu_choice == SYSTEM_MAIL - 1) {
                /* Mail Setup dialog */
                mailDialog(cdk_screen);

            } else if (menu_choice == SYSTEM_MENU &&
                    submenu_choice == SYSTEM_TEST_EMAIL - 1) {
                /* Test Email dialog */
                testEmailDialog(cdk_screen);

            } else if (menu_choice == SYSTEM_MENU &&
                    submenu_choice == SYSTEM_ADD_USER - 1) {
                /* Add User dialog */
                addUserDialog(cdk_screen);

            } else if (menu_choice == SYSTEM_MENU &&
                    submenu_choice == SYSTEM_DEL_USER - 1) {
                /* Delete User dialog */
                delUserDialog(cdk_screen);

            } else if (menu_choice == SYSTEM_MENU &&
                    submenu_choice == SYSTEM_CHG_PASSWD - 1) {
                /* Change Password dialog */
                chgPasswdDialog(cdk_screen);

            } else if (menu_choice == SYSTEM_MENU &&
                    submenu_choice == SYSTEM_SCST_INFO - 1) {
                /* SCST Information dialog */
                scstInfoDialog(cdk_screen);

            } else if (menu_choice == SYSTEM_MENU &&
                    submenu_choice == SYSTEM_CRM_STATUS - 1) {
                /* CRM Status dialog */
                crmStatusDialog(cdk_screen);

            } else if (menu_choice == SYSTEM_MENU &&
                    submenu_choice == SYSTEM_DRBD_STATUS - 1) {
                /* DRBD Status dialog */
                drbdStatDialog(cdk_screen);

            } else if (menu_choice == SYSTEM_MENU &&
                    submenu_choice == SYSTEM_DATE_TIME - 1) {
                /* Date & Time Settings dialog */
                dateTimeDialog(cdk_screen);

            } else if (menu_choice == HW_RAID_MENU &&
                    submenu_choice == HW_RAID_ADD_VOL - 1) {
                /* Add Volume dialog */
                addVolumeDialog(cdk_screen);
                //addVolDialog(cdk_screen);

            } else if (menu_choice == HW_RAID_MENU &&
                    submenu_choice == HW_RAID_REM_VOL - 1) {
                /* Remove Volume dialog */
                delVolumeDialog(cdk_screen);
                //remVolDialog(cdk_screen);

            } else if (menu_choice == HW_RAID_MENU &&
                    submenu_choice == HW_RAID_MOD_VOL - 1) {
                /* Modify Volume dialog */
                //modVolDialog(cdk_screen);

            } else if (menu_choice == HW_RAID_MENU &&
                    submenu_choice == HW_RAID_ADD_HSP - 1) {
                /* Add Hot Spare dialog */
                //addHSPDialog(cdk_screen);

            } else if (menu_choice == HW_RAID_MENU &&
                    submenu_choice == HW_RAID_REM_HSP - 1) {
                /* Remove Hot Spare dialog */
                //remHSPDialog(cdk_screen);

            } else if (menu_choice == SW_RAID_MENU &&
                    submenu_choice == SW_RAID_MD_STAT - 1) {
                /* Linux MD RAID Status dialog */
                softRAIDStatDialog(cdk_screen);

            } else if (menu_choice == SW_RAID_MENU &&
                    submenu_choice == SW_RAID_ADD_ARRAY - 1) {
                /* Add Array dialog */
                //addArrayDialog(cdk_screen);

            } else if (menu_choice == SW_RAID_MENU &&
                    submenu_choice == SW_RAID_REM_ARRAY - 1) {
                /* Remove Array dialog */
                //remArrayDialog(cdk_screen);

            } else if (menu_choice == SW_RAID_MENU &&
                    submenu_choice == SW_RAID_FAULT_DEV - 1) {
                /* Set Device Faulty dialog */
                //faultDevDialog(cdk_screen);

            } else if (menu_choice == SW_RAID_MENU &&
                    submenu_choice == SW_RAID_ADD_DEV - 1) {
                /* Add Device dialog */
                //addDevDialog(cdk_screen);

            } else if (menu_choice == SW_RAID_MENU &&
                    submenu_choice == SW_RAID_REM_DEV - 1) {
                /* Remove Device dialog */
                //remDevDialog(cdk_screen);

            } else if (menu_choice == LVM_MENU &&
                    submenu_choice == LVM_LV_LIST - 1) {
                /* Logical Volume List dialog */
                lvm2InfoDialog(cdk_screen);

            } else if (menu_choice == LVM_MENU &&
                    submenu_choice == LVM_ADD_PV - 1) {
                /* Add PV dialog */
                //addPVDialog(cdk_screen);

            } else if (menu_choice == LVM_MENU &&
                    submenu_choice == LVM_REM_PV - 1) {
                /* Remove PV dialog */
                //remPVialog(cdk_screen);

            } else if (menu_choice == LVM_MENU &&
                    submenu_choice == LVM_ADD_VG - 1) {
                /* Add VG dialog */
                //addVGDialog(cdk_screen);

            } else if (menu_choice == LVM_MENU &&
                    submenu_choice == LVM_REM_VG - 1) {
                /* Remove VG dialog */
                //remVGDialog(cdk_screen);

            } else if (menu_choice == LVM_MENU &&
                    submenu_choice == LVM_ADD_LV - 1) {
                /* Add LV dialog */
                //addLVDialog(cdk_screen);

            } else if (menu_choice == LVM_MENU &&
                    submenu_choice == LVM_REM_LV - 1) {
                /* Remove LV dialog */
                //remLVDialog(cdk_screen);

            } else if (menu_choice == FILE_SYS_MENU &&
                    submenu_choice == FILE_SYS_VDISK_LIST - 1) {
                /* VDisk File List dialog */
                vdiskFileListDialog(cdk_screen);

            } else if (menu_choice == FILE_SYS_MENU &&
                    submenu_choice == FILE_SYS_ADD_FS - 1) {
                /* Add File System dialog */
                createFSDialog(cdk_screen);

            } else if (menu_choice == FILE_SYS_MENU &&
                    submenu_choice == FILE_SYS_REM_FS - 1) {
                /* Remove File System dialog */
                removeFSDialog(cdk_screen);

            } else if (menu_choice == FILE_SYS_MENU &&
                    submenu_choice == FILE_SYS_ADD_VDISK - 1) {
                /* Add VDisk File dialog */
                addVDiskFileDialog(cdk_screen);

            } else if (menu_choice == FILE_SYS_MENU &&
                    submenu_choice == FILE_SYS_REM_VDISK - 1) {
                /* Remove VDisk File dialog */
                delVDiskFileDialog(cdk_screen);
            }

            /* At this point we've finished the dialog, so we make
             * the screen look nice again and reset the menu exit status */
            menu_1->exitType = vNEVER_ACTIVATED;
            wbkgd(cdk_screen->window, g_color_main_text[g_curr_theme]);
            wrefresh(cdk_screen->window);
            refreshCDKScreen(cdk_screen);
            curs_set(0);
        }

        if (menu_2->exitType == vNORMAL) {
            /* Get the selected menu choice */
            menu_choice = selection / 100;
            submenu_choice = selection % 100;

            if (menu_choice == HOSTS_MENU &&
                    submenu_choice == HOSTS_ADD_GROUP - 1) {
                /* Add Group dialog */
                addGroupDialog(cdk_screen);

            } else if (menu_choice == HOSTS_MENU &&
                    submenu_choice == HOSTS_REM_GROUP - 1) {
                /* Remove Group dialog */
                remGroupDialog(cdk_screen);

            } else if (menu_choice == HOSTS_MENU &&
                    submenu_choice == HOSTS_ADD_INIT - 1) {
                /* Add Initiator dialog */
                addInitDialog(cdk_screen);

            } else if (menu_choice == HOSTS_MENU &&
                    submenu_choice == HOSTS_REM_INIT - 1) {
                /* Remove Initiator dialog */
                remInitDialog(cdk_screen);

            } else if (menu_choice == DEVICES_MENU &&
                    submenu_choice == DEVICES_LUN_LAYOUT - 1) {
                /* LUN Layout dialog */
                lunLayoutDialog(cdk_screen);

            } else if (menu_choice == DEVICES_MENU &&
                    submenu_choice == DEVICES_DEV_INFO - 1) {
                /* Device Information dialog */
                devInfoDialog(cdk_screen);

            } else if (menu_choice == DEVICES_MENU &&
                    submenu_choice == DEVICES_ADD_DEV - 1) {
                /* Add Device dialog */
                addDeviceDialog(cdk_screen);

            } else if (menu_choice == DEVICES_MENU &&
                    submenu_choice == DEVICES_REM_DEV - 1) {
                /* Delete Device dialog */
                remDeviceDialog(cdk_screen);

            } else if (menu_choice == DEVICES_MENU &&
                    submenu_choice == DEVICES_MAP_TO - 1) {
                /* Map to Group dialog */
                mapDeviceDialog(cdk_screen);

            } else if (menu_choice == DEVICES_MENU &&
                    submenu_choice == DEVICES_UNMAP_FROM - 1) {
                /* Unmap from Group dialog */
                unmapDeviceDialog(cdk_screen);

            } else if (menu_choice == TARGETS_MENU &&
                    submenu_choice == TARGETS_TGT_INFO - 1) {
                /* Target Information dialog */
                tgtInfoDialog(cdk_screen);

            } else if (menu_choice == TARGETS_MENU &&
                    submenu_choice == TARGETS_ADD_ISCSI - 1) {
                /* Add iSCSI Target dialog */
                addiSCSITgtDialog(cdk_screen);

            } else if (menu_choice == TARGETS_MENU &&
                    submenu_choice == TARGETS_REM_ISCSI - 1) {
                /* Remove iSCSI Target dialog */
                remiSCSITgtDialog(cdk_screen);

            } else if (menu_choice == TARGETS_MENU &&
                    submenu_choice == TARGETS_LIP - 1) {
                /* Issue LIP dialog */
                issueLIPDialog(cdk_screen);

            } else if (menu_choice == TARGETS_MENU &&
                    submenu_choice == TARGETS_TOGGLE - 1) {
                /* Enable/Disable Target dialog */
                enblDsblTgtDialog(cdk_screen);

            } else if (menu_choice == TARGETS_MENU &&
                    submenu_choice == TARGETS_SET_REL_TGT_ID - 1) {
                /* Set Relative Target ID dialog */
                setRelTgtIDDialog(cdk_screen);

            } else if (menu_choice == ALUA_MENU &&
                    submenu_choice == ALUA_DEV_GRP_LAYOUT - 1) {
                /* Device/Target Group Layout dialog */
                devTgtGrpLayoutDialog(cdk_screen);

            } else if (menu_choice == ALUA_MENU &&
                    submenu_choice == ALUA_ADD_DEV_GRP - 1) {
                /* Add Device Group dialog */
                addDevGrpDialog(cdk_screen);

            } else if (menu_choice == ALUA_MENU &&
                    submenu_choice == ALUA_REM_DEV_GRP - 1) {
                /* Remove Device Group dialog */
                remDevGrpDialog(cdk_screen);

            } else if (menu_choice == ALUA_MENU &&
                    submenu_choice == ALUA_ADD_TGT_GRP - 1) {
                /* Add Target Group dialog */
                addTgtGrpDialog(cdk_screen);

            } else if (menu_choice == ALUA_MENU &&
                    submenu_choice == ALUA_REM_TGT_GRP - 1) {
                /* Remove Target Group dialog */
                remTgtGrpDialog(cdk_screen);

            } else if (menu_choice == ALUA_MENU &&
                    submenu_choice == ALUA_ADD_DEV_TO_GRP - 1) {
                /* Add Device to Group dialog */
                addDevToGrpDialog(cdk_screen);

            } else if (menu_choice == ALUA_MENU &&
                    submenu_choice == ALUA_REM_DEV_FROM_GRP - 1) {
                /* Remove Device from Group dialog */
                remDevFromGrpDialog(cdk_screen);

            } else if (menu_choice == ALUA_MENU &&
                    submenu_choice == ALUA_ADD_TGT_TO_GRP - 1) {
                /* Add Target to Group dialog */
                addTgtToGrpDialog(cdk_screen);

            } else if (menu_choice == ALUA_MENU &&
                    submenu_choice == ALUA_REM_TGT_FROM_GRP - 1) {
                /* Remove Target from Group dialog */
                remTgtFromGrpDialog(cdk_screen);

            } else if (menu_choice == INTERFACE_MENU &&
                    submenu_choice == INTERFACE_QUIT - 1) {
                /* Synchronize the configuration and quit */
                syncConfig(cdk_screen);
                goto quit;

            } else if (menu_choice == INTERFACE_MENU &&
                    submenu_choice == INTERFACE_SHELL - 1) {
                /* Set the UID to what was saved at the start */
                if (setresuid(saved_uid, 0, -1) == -1) {
                    SAFE_ASPRINTF(&error_msg, "setresuid(): %s",
                            strerror(errno));
                    errorDialog(cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                }
                /* Fork and execute a shell */
                if ((child_pid = fork()) < 0) {
                    /* Could not fork */
                    SAFE_ASPRINTF(&error_msg, "fork(): %s", strerror(errno));
                    errorDialog(cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                } else if (child_pid == 0) {
                    /* Child; fix up the terminal and execute the shell */
                    endwin();
                    curs_set(1);
                    echo();
                    system(CLEAR_BIN);
                    /* Execute the shell; if we fail, print something
                     * useful to syslog for debugging */
                    if ((execl(SHELL_BIN, SHELL_BIN, "--rcfile", GLOBAL_BASHRC,
                            "-i", (char *) NULL)) == -1) {
                        DEBUG_LOG("Calling execl() failed: %s",
                                strerror(errno));
                    }
                    exit(2);
                } else {
                    /* Parent; wait for the child to finish */
                    while ((proc_status = wait(&child_status)) != child_pid) {
                        if (proc_status < 0 && errno == ECHILD)
                            break;
                        errno = 0;
                    }
                    /* Ending everything and starting fresh seems to work
                     * best when switching between the shell and UI */
                    for (i = 0; i < menu_1_cnt; i++)
                        FREE_NULL(menu_list_1[i][0]);
                    for (i = 0; i < menu_2_cnt; i++)
                        FREE_NULL(menu_list_2[i][0]);
                    destroyCDKLabel(targets_label);
                    targets_label = NULL;
                    destroyCDKLabel(sessions_label);
                    sessions_label = NULL;
                    destroyCDKScreenObjects(cdk_screen);
                    destroyCDKScreen(cdk_screen);
                    endCDK();
                    cdk_screen = NULL;

                    /* Check and see if we're still attached to our terminal */
                    if ((tty_fd = open("/dev/tty", O_RDONLY)) == -1) {
                        /* Guess not, so we're done */
                        goto quit;
                    } else {
                        close(tty_fd);
                        goto start;
                    }
                }

            } else if (menu_choice == INTERFACE_MENU &&
                    submenu_choice == INTERFACE_THEME - 1) {
                /* Color Theme dialog */
                themeDialog(cdk_screen);

            } else if (menu_choice == INTERFACE_MENU &&
                    submenu_choice == INTERFACE_HELP - 1) {
                /* Help dialog */
                helpDialog(cdk_screen);

            } else if (menu_choice == INTERFACE_MENU &&
                    submenu_choice == INTERFACE_SUPPORT_PKG - 1) {
                /* Support Bundle dialog */
                supportArchDialog(cdk_screen);

            } else if (menu_choice == INTERFACE_MENU &&
                    submenu_choice == INTERFACE_ABOUT - 1) {
                /* About dialog */
                aboutDialog(cdk_screen);
            }

            /* At this point we've finished the dialog, so we make
             * the screen look nice again and reset the menu exit status */
            menu_2->exitType = vNEVER_ACTIVATED;
            wbkgd(cdk_screen->window, g_color_main_text[g_curr_theme]);
            wrefresh(cdk_screen->window);
            refreshCDKScreen(cdk_screen);
            curs_set(0);
        }

        /* Done with the menus, go back to half-delay mode */
        halfdelay(REFRESH_DELAY);
    }

    /* All done -- clean up */
quit:
    DEBUG_LOG("Quitting...");
    closelog();
    if (cdk_screen != NULL) {
        destroyCDKScreenObjects(cdk_screen);
        destroyCDKScreen(cdk_screen);
        endCDK();
    }
    delwin(sub_window);
    delwin(main_window);
    for (i = 0; i < menu_1_cnt; i++)
        FREE_NULL(menu_list_1[i][0]);
    for (i = 0; i < menu_2_cnt; i++)
        FREE_NULL(menu_list_2[i][0]);
    for (i = 0; i < MAX_INFO_LABEL_ROWS; i++) {
        FREE_NULL(tgt_label_msg[i]);
        FREE_NULL(sess_label_msg[i]);
    }
    system(CLEAR_BIN);
    exit(EXIT_SUCCESS);
}


/**
 * @brief Helper to fix initial terminal/screen size (before CDK
 * initialization). If the screen size is less than the minimum, keep
 * displaying the message until the user resizes to the minimum values.
 */
void termSize(WINDOW *screen) {
    int input_char = 0, screen_x = 0, screen_y = 0;
    char curr_term_size[MISC_STRING_LEN] = {0},
            reqd_screen_size[MISC_STRING_LEN] = {0};
    boolean first_run = TRUE;

    /* Start half-delay mode */
    halfdelay(REFRESH_DELAY);

    for (;;) {
        /* We only want to wait for input if this is not the first loop */
        if (!first_run) {
            wrefresh(screen);
            input_char = wgetch(screen);
        } else {
            input_char = KEY_RESIZE;
        }

        /* Wait for a resize event or escape */
        if (input_char == KEY_RESIZE) {
            first_run = FALSE;
            clear();
            refresh();
            getmaxyx(screen, screen_y, screen_x);

            /* Check new screen size and return if we meet the minimums */
            if (screen_y >= MIN_SCR_Y && screen_x >= MIN_SCR_X) {
                return;
            } else {
                mvaddstr(((screen_y / 2) - 1),
                        ((screen_x - strlen(TOO_SMALL_TERM)) / 2),
                        TOO_SMALL_TERM);
                snprintf(curr_term_size, MISC_STRING_LEN,
                        CURR_TERM_SIZE, screen_y, screen_x);
                mvaddstr((screen_y / 2),
                        ((screen_x - strlen(curr_term_size)) / 2),
                        curr_term_size);
                snprintf(reqd_screen_size, MISC_STRING_LEN,
                        REQD_SCREEN_SIZE, MIN_SCR_Y, MIN_SCR_X);
                mvaddstr(((screen_y / 2) + 1),
                        ((screen_x - strlen(reqd_screen_size)) / 2),
                        reqd_screen_size);
            }

        } else if (input_char == KEY_ESC) {
            printf("\n");
            endwin();
            exit(EXIT_SUCCESS);
        }
    }
    return;
}


/**
 * @brief Handle terminal resize (SIGWINCH / KEY_RESIZE). If the screen is made
 * smaller than the last size, we display a message to the user until they
 * exit. If its larger, we try to handle that gracefully.
 */
void screenResize(CDKSCREEN *cdk_screen, WINDOW *main_window,
        WINDOW *sub_window, int *latest_term_y, int *latest_term_x) {
    int input_char = 0, screen_y = 0, screen_x = 0;
    boolean first_run = TRUE;

    getmaxyx(main_window, screen_y, screen_x);
    if ((screen_y < *latest_term_y) || (screen_x < *latest_term_x)) {
        /* Screen is smaller than it should be */
        destroyCDKScreenObjects(cdk_screen);
        destroyCDKScreen(cdk_screen);
        endCDK();
        halfdelay(REFRESH_DELAY);

        for (;;) {
            /* We only want to wait for input if this is not the first loop */
            if (!first_run) {
                wrefresh(main_window);
                input_char = wgetch(main_window);
            } else {
                input_char = KEY_RESIZE;
            }

            /* A SIGWINCH generates a KEY_RESIZE character with our ncurses */
            if (input_char == KEY_RESIZE) {
                first_run = FALSE;
                clear();
                refresh();
                getmaxyx(main_window, screen_y, screen_x);
                /* The screen size has changed so print a fancy message */
                mvaddstr(((screen_y / 2) - 1),
                        ((screen_x - strlen(NO_RESIZE)) / 2),
                        NO_RESIZE);
                mvaddstr((screen_y / 2),
                        ((screen_x - strlen(NO_SMALL_TERM)) / 2),
                        NO_SMALL_TERM);
                mvaddstr(((screen_y / 2) + 1),
                        ((screen_x - strlen(ANY_KEY_EXIT)) / 2),
                        ANY_KEY_EXIT);

            } else if (input_char == ERR) {
                /* Timed out waiting for input -- loop */
                continue;

            } else {
                /* User hit a key to exit */
                endwin();
                printf("\n");
                exit(EXIT_SUCCESS);
            }
        }

    } else {
        /* Screen grew in size so make it pretty */
        wresize(sub_window, (screen_y - 2), (screen_x - 2));
        wclear(main_window);
        wrefresh(main_window);
        statusBar(main_window);
        refreshCDKScreen(cdk_screen);
        /* Set these for the next trip */
        *latest_term_y = screen_y;
        *latest_term_x = screen_x;
        return;
    }
}


/**
 * @brief Make a nice pretty status bar; may be called multiple times in case
 * of a screen resize. Also draws the box around the window.
 */
void statusBar(WINDOW *window) {
    char esos_ver_str[STAT_BAR_ESOS_VER_MAX] = {0},
            username_str[STAT_BAR_UNAME_MAX] = {0};
    int esos_ver_size = 0, username_size = 0, bar_space = 0, junk = 0;
    uid_t ruid = 0, euid = 0, suid = 0;
    struct passwd *pwd = NULL;
    char *status_msg = NULL;
    chtype *status_bar = NULL;

    /* Set the ESOS status bar name/version */
    snprintf(esos_ver_str, STAT_BAR_ESOS_VER_MAX,
            "ESOS - Enterprise Storage OS %s", ESOS_VERSION);
    esos_ver_size = strlen(esos_ver_str);

    /* Get username */
    getresuid(&ruid, &euid, &suid);
    pwd = getpwuid(suid);
    strncpy(username_str, pwd->pw_name, STAT_BAR_UNAME_MAX);
    username_size = strlen(username_str);

    /* Figure out spacing for status bar */
    bar_space = (COLS - 2) - 2 - esos_ver_size - username_size;

    /* Box the window and make the status bar */
    boxWindow(window, g_color_main_box[g_curr_theme]);
    SAFE_ASPRINTF(&status_msg, "</B> %s%*s%s <!B>", esos_ver_str, bar_space,
            "", username_str);
    status_bar = char2Chtype(status_msg, &junk, &junk);
    writeChtypeAttrib(window, 1, (LINES - 1), status_bar,
            g_color_status_bar[g_curr_theme], HORIZONTAL, 0, (COLS - 2));
    freeChtype(status_bar);
    wrefresh(window);

    /* Done */
    FREE_NULL(status_msg);
    return;
}


/**
 * @brief Read the ESOS configuration file and prompt the user to participate
 * in anonymous usage statistics; if user participates, whenever the ESOS
 * version changes (upgrade), an HTTP POST request is made.
 */
void reportUsage(CDKSCREEN *main_cdk_screen) {
    boolean question = FALSE;
    dictionary *ini_dict = NULL;
    uuid_t install_id = {0};
    CDKLABEL *transmit_msg = 0;
    char *error_msg = NULL, *conf_install_id = NULL, *conf_last_ver = NULL,
            *conf_participate = NULL;
    char install_id_str[UUID_STR_SIZE] = {0};
    FILE *ini_file = NULL;
    CURL *curl = NULL;
    CURLcode result = 0;
    struct curl_httppost *form_post = NULL, *last_ptr = NULL;

    while (1) {
        if (access(ESOS_CONF, F_OK) != 0) {
            if (errno == ENOENT) {
                /* The iniparser_load function expects a file (even empty) */
                if ((ini_file = fopen(ESOS_CONF, "w")) == NULL) {
                    SAFE_ASPRINTF(&error_msg, ESOS_CONF_WRITE_ERR,
                            strerror(errno));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    break;
                }
                fclose(ini_file);
            } else {
                /* Missing file is okay, but fail otherwise */
                SAFE_ASPRINTF(&error_msg, "access(): %s", strerror(errno));
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                break;
            }
        }

        /* Read ESOS configuration file (INI file) */
        ini_dict = iniparser_load(ESOS_CONF);
        if (ini_dict == NULL) {
            errorDialog(main_cdk_screen, ESOS_CONF_READ_ERR_1,
                    ESOS_CONF_READ_ERR_2);
            break;
        }

        /* We set the section in every case */
        if (iniparser_set(ini_dict, "usage", NULL) == -1) {
            errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
            break;
        }

        /* Parse INI file values (if any) */
        conf_participate = iniparser_getstring(ini_dict,
                "usage:participate", "");
        conf_install_id = iniparser_getstring(ini_dict,
                "usage:install_id", "");
        conf_last_ver = iniparser_getstring(ini_dict,
                "usage:last_ver", "");

        /* This may be a new configuration file, prompt the user */
        if (conf_participate[0] == '\0') {
            question = questionDialog(main_cdk_screen,
                    "Anonymously report usage statistics?", NULL);
            if (iniparser_set(ini_dict, "usage:participate",
                    (question ? "yes" : "no")) == -1) {
                errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
                break;
            }
            /* Read the string value again, and save the file */
            conf_participate = iniparser_getstring(ini_dict,
                    "usage:participate", "");
            if ((ini_file = fopen(ESOS_CONF, "w")) == NULL) {
                SAFE_ASPRINTF(&error_msg, ESOS_CONF_WRITE_ERR, strerror(errno));
                errorDialog(main_cdk_screen, error_msg, NULL);
                FREE_NULL(error_msg);
                break;
            }
            iniparser_dump_ini(ini_dict, ini_file);
            fclose(ini_file);
        }

        if (strcmp(conf_participate, "no") == 0) {
            /* Nothing to do */
            break;

        } else if (strcmp(conf_participate, "yes") == 0) {
            /* If we don't have an installation ID, generate one */
            if (conf_install_id[0] == '\0') {
                uuid_generate(install_id);
                uuid_unparse_lower(install_id, install_id_str);
                if (iniparser_set(ini_dict, "usage:install_id",
                        install_id_str) == -1) {
                    errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
                    break;
                }
                /* Read the string value again, and save the file */
                conf_install_id = iniparser_getstring(ini_dict,
                        "usage:install_id", "");
                if ((ini_file = fopen(ESOS_CONF, "w")) == NULL) {
                    SAFE_ASPRINTF(&error_msg, ESOS_CONF_WRITE_ERR, strerror(errno));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    break;
                }
                iniparser_dump_ini(ini_dict, ini_file);
                fclose(ini_file);
            }

            /* Last reported version doesn't match what we have, report it */
            if (strcmp(conf_last_ver, ESOS_VERSION) != 0) {
                /* Display a message while we transmit */
                transmit_msg = newCDKLabel(main_cdk_screen, CENTER, CENTER,
                        g_usage_label_msg, g_usage_label_msg_size(),
                        TRUE, FALSE);
                if (!transmit_msg) {
                    errorDialog(main_cdk_screen, LABEL_ERR_MSG, NULL);
                    break;
                }
                setCDKLabelBackgroundAttrib(transmit_msg,
                        g_color_dialog_text[g_curr_theme]);
                setCDKLabelBoxAttribute(transmit_msg,
                        g_color_dialog_box[g_curr_theme]);
                refreshCDKScreen(main_cdk_screen);

                /* Initialize cURL */
                curl_global_init(CURL_GLOBAL_ALL);
                if ((curl = curl_easy_init()) != NULL) {
                    /* Fill in our form fields */
                    curl_formadd(&form_post, &last_ptr, CURLFORM_COPYNAME,
                            "branch", CURLFORM_COPYCONTENTS,
                            GIT_BRANCH, CURLFORM_END);
                    curl_formadd(&form_post, &last_ptr, CURLFORM_COPYNAME,
                            "ver_string", CURLFORM_COPYCONTENTS,
                            ESOS_VERSION, CURLFORM_END);
                    curl_formadd(&form_post, &last_ptr, CURLFORM_COPYNAME,
                            "install_id", CURLFORM_COPYCONTENTS,
                            conf_install_id, CURLFORM_END);

                    /* Set cURL options for our HTTP POST */
                    curl_easy_setopt(curl, CURLOPT_URL, USAGE_POST_URL);
                    curl_easy_setopt(curl, CURLOPT_HTTPPOST, form_post);
                    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
                    curl_easy_setopt(curl, CURLOPT_USERAGENT, TUI_LOG_PREFIX);

                    /* Perform the request */
                    result = curl_easy_perform(curl);
                    if (result != CURLE_OK) {
                        SAFE_ASPRINTF(&error_msg, "curl_easy_perform(): %s",
                                curl_easy_strerror(result));
                        errorDialog(main_cdk_screen, error_msg, NULL);
                        FREE_NULL(error_msg);
                        break;
                    }

                    /* Cleanup */
                    curl_easy_cleanup(curl);
                    curl_formfree(form_post);
                }

                /* Success if we made it this far, set the last version */
                if (iniparser_set(ini_dict, "usage:last_ver",
                        ESOS_VERSION) == -1) {
                    errorDialog(main_cdk_screen, SET_FILE_VAL_ERR, NULL);
                    break;
                }
                /* Save the file */
                if ((ini_file = fopen(ESOS_CONF, "w")) == NULL) {
                    SAFE_ASPRINTF(&error_msg, ESOS_CONF_WRITE_ERR,
                            strerror(errno));
                    errorDialog(main_cdk_screen, error_msg, NULL);
                    FREE_NULL(error_msg);
                    break;
                }
                iniparser_dump_ini(ini_dict, ini_file);
                fclose(ini_file);
            }
        }

        /* We got this far successfully, exit the loop */
        break;
    }

    /* Done */
    if (ini_dict != NULL)
        iniparser_freedict(ini_dict);
    destroyCDKLabel(transmit_msg);
    refreshCDKScreen(main_cdk_screen);
    return;
}


/**
 * @brief Parse the ESOS configuration file (if any), and set the theme to the
 * user-set option, or if no configuration file exists, set the default theme.
 */
void setTheme() {
    dictionary *ini_dict = NULL;
    char *conf_theme = NULL;
    FILE *ini_file = NULL;

    while (1) {
        if (access(ESOS_CONF, F_OK) != 0) {
            if (errno == ENOENT) {
                /* The iniparser_load function expects a file (even empty) */
                if ((ini_file = fopen(ESOS_CONF, "w")) == NULL) {
                    DEBUG_LOG(ESOS_CONF_WRITE_ERR, strerror(errno));
                    break;
                }
                fclose(ini_file);
            } else {
                /* Missing file is okay, but fail otherwise */
                DEBUG_LOG("access(): %s", strerror(errno));
                break;
            }
        }

        /* Read ESOS configuration file (INI file) */
        ini_dict = iniparser_load(ESOS_CONF);
        if (ini_dict == NULL) {
            DEBUG_LOG("%s %s", ESOS_CONF_READ_ERR_1, ESOS_CONF_READ_ERR_2);
            break;
        }

        /* We set the section in every case */
        if (iniparser_set(ini_dict, "tui", NULL) == -1) {
            DEBUG_LOG(SET_FILE_VAL_ERR);
            break;
        }

        /* Parse INI file values (if any) */
        conf_theme = iniparser_getstring(ini_dict,
                "tui:theme", "");

        /* Set the default value if we have nothing for the theme */
        if (conf_theme[0] == '\0') {
            if (iniparser_set(ini_dict, "tui:theme", "blue") == -1) {
                DEBUG_LOG(SET_FILE_VAL_ERR);
                break;
            }
            /* Read the string value again, and save the file */
            conf_theme = iniparser_getstring(ini_dict, "tui:theme", "");
            if ((ini_file = fopen(ESOS_CONF, "w")) == NULL) {
                DEBUG_LOG(ESOS_CONF_WRITE_ERR, strerror(errno));
                break;
            }
            iniparser_dump_ini(ini_dict, ini_file);
            fclose(ini_file);
        }

        if (strcmp(conf_theme, "blue") == 0) {
            g_curr_theme = BLUE_TUI;
        } else if (strcmp(conf_theme, "black") == 0) {
            g_curr_theme = BLACK_TUI;
        } else {
            g_curr_theme = BLUE_TUI;
        }

        /* We got this far successfully, exit the loop */
        break;
    }

    /* Done */
    if (ini_dict != NULL)
        iniparser_freedict(ini_dict);
    return;
}
