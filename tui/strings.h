/*
 * $Id$
 */

#ifndef STRINGS_H
#define	STRINGS_H

#ifdef	__cplusplus
extern "C" {
#endif

/* Screen sizing/resizing */
#define NO_RESIZE "Aw, snap! This TUI doesn't handle screen shrinking."
#define NO_SMALL_TERM "You can not make the terminal smaller " \
        "while using the interface."
#define ANY_KEY_EXIT "Press any key to exit..."
#define TOO_SMALL_TERM "Your terminal is not the correct size!"
#define CURR_TERM_SIZE "Current screen size: %d rows X %d columns"
#define REQD_SCREEN_SIZE "(Screen must be at least %d rows and %d columns.)"

/* CDK screen/window/widget error messages */
#define NEWWIN_ERR_MSG          "Couldn't create new window!"
#define CDK_SCR_ERR_MSG         "Couldn't create new CDK screen!"
#define LABEL_ERR_MSG           "Couldn't create label widget!"
#define RADIO_ERR_MSG           "Couldn't create radio widget!"
#define SWINDOW_ERR_MSG         "Couldn't create scrolling window widget!"
#define SCROLL_ERR_MSG          "Couldn't create scroll widget!"
#define FSELECT_ERR_MSG         "Couldn't create file selector widget!"
#define BUTTON_ERR_MSG          "Couldn't create button widget!"
#define ENTRY_ERR_MSG           "Couldn't create entry widget!"
#define ITEM_LIST_ERR_MSG       "Couldn't create item list widget!"
#define SCALE_ERR_MSG           "Couldn't create scale widget!"
#define MENU_ERR_MSG            "Couldn't create menu widget!"
#define SELECTION_ERR_MSG       "Couldn't create selection widget!"
#define HISTOGRAM_ERR_MSG       "Couldn't create histogram widget!"
#define DIALOG_ERR_MSG          "Couldn't create dialog widget!"
#define MENTRY_ERR_MSG          "Couldn't create multiple line entry widget!"
#define CALENDAR_ERR_MSG        "Couldn't create calendar widget!"

/* SCST utility function error messages */
#define TGT_DRIVERS_ERR "An error occurred while retrieving the " \
        "list of drivers."

/* Canned dialog messages */
#define CONTINUE_MSG    "<C></B><Press ENTER to continue...>"
#define NO_SCST_MSG     "<C></B><SCST is not loaded!>"
#define EMPTY_FIELD_ERR "The entry field cannot be empty!"
#define ASK_DEL_GROUP   "Are you sure you want to delete SCST target group %s?"
#define DEL_GROUP_ERR   "Couldn't delete SCST group: %s"
#define ADD_GROUP_ERR   "Couldn't add SCST group: %s"
#define DEL_INIT_ERR    "Couldn't remove initiator: %s"
#define ADD_INIT_ERR    "Couldn't add initiator to SCST group: %s"
#define NOT_MOUNTED_1   "It appears that file system is not mounted; would " \
        "you like to try mounting it now?"
#define NOT_MOUNTED_2   "(The file system must be mounted before proceeding.)"

/* INI parser messages */
#define SET_FILE_VAL_ERR        "Couldn't set configuration file value!"
#define NET_CONF_WRITE_ERR      "Couldn't open network config. file " \
        "for writing: %s"
#define NET_CONF_READ_ERR       "Cannot parse network configuration file!"
#define SSMTP_CONF_WRITE_ERR    "Couldn't open sSMTP config. file " \
        "for writing: %s"
#define SSMTP_CONF_READ_ERR     "Cannot parse sSMTP configuration file!"
#define NET_CONF_COMMENT_1      "# ESOS network configuration file\n"
#define SSMTP_CONF_COMMENT_1    "# sSMTP configuration file\n"
#define INI_CONF_COMMENT        "# This file is generated by esos_tui; " \
        "do not edit\n\n"
#define ESOS_CONF_READ_ERR_1    "Error parsing the ESOS configuration file!"
#define ESOS_CONF_READ_ERR_2    "Continuing anyway..."
#define ESOS_CONF_WRITE_ERR     "Couldn't open ESOS config. file " \
        "for writing: %s"

/* Dialog radio widget options */
extern char *g_no_yes_opts[], *g_auth_meth_opts[], *g_ip_opts[],
        *g_cache_opts[], *g_write_opts[], *g_read_opts[], *g_bbu_opts[],
        *g_raid_opts[], *g_strip_opts[], *g_dsbl_enbl_opts[], *g_fs_type_opts[];

/* Label title strings */
extern char *g_mail_title_msg[], *g_add_user_title_msg[], *g_date_title_msg[];

/* Misc. widget related strings */
extern char *g_choice_char[], *g_bonding_map[], *g_scst_dev_types[],
        *g_scst_bs_list[], *g_fio_types[], *g_sync_label_msg[],
        *g_usage_label_msg[];

/* Button strings */
extern char *g_ok_msg[], *g_ok_cancel_msg[], *g_yes_no_msg[];

/* Other string stuff */
extern char *g_transports[], *g_scst_handlers[];

/* Dialog title strings */
#define LD_LIST_TITLE           "<C></31/B>Choose a Logical Drive\n"
#define ADP_LIST_TITLE          "<C></31/B>Choose an Adapter\n"
#define BLOCK_DEV_LIST_TITLE    "<C></31/B>Choose a Block Device\n"
#define SCSI_DISK_LIST_TITLE    "<C></31/B>Choose a SCSI Disk\n"
#define SCST_DEV_LIST_TITLE     "<C></31/B>Choose a SCST Device Handler\n"

#ifdef	__cplusplus
}
#endif

#endif	/* STRINGS_H */
