/* $Header: /wwg/motif/wavplay.RCS/RCS/xltwavplay.c,v 1.2 1997/04/15 02:13:35 wwg Exp $
 * Warren W. Gay VE3WWG		Thu Feb 13 21:11:22 1997
 *
 * 	X LessTif WAV Play :
 * 
 * 	Copyright (C) 1997  Warren W. Gay VE3WWG
 * 
 * This  program is free software; you can redistribute it and/or modify it
 * under the  terms  of  the GNU General Public License as published by the
 * Free Software Foundation version 2 of the License.
 * 
 * This  program  is  distributed  in  the hope that it will be useful, but
 * WITHOUT   ANY   WARRANTY;   without   even  the   implied   warranty  of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details (see enclosed file COPYING).
 * 
 * You  should have received a copy of the GNU General Public License along
 * with this  program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 * Send correspondance to:
 * 
 * 	Warren W. Gay VE3WWG
 * 	5536 Montevideo Road #17
 *	Mississauga, Ontario L5N 2P4
 * 
 * Email:
 * 	wwg@ica.net			(current ISP of the month :-) )
 * 	bx249@freenet.toronto.on.ca	(backup)
 *
 * $Log: xltwavplay.c,v $
 * Revision 1.2  1997/04/15 02:13:35  wwg
 * A number of small pre-release fixes.
 *
 * Revision 1.1  1997/04/13 23:59:08  wwg
 * Initial revision
 *
 */
#define TIMER1_MS	150		/* TimerProc1() */

#define LessTif_Bug_radioBehavior 1	/* XmNradioBehavior busted in RowColumn widget (T/F) */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <assert.h>

#ifndef NO_EDITRES
#include <X11/Xmu/Editres.h>
#endif

#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/ArrowB.h>
#include <Xm/CascadeB.h>
#include <Xm/Text.h>
#include <Xm/Label.h>
#include <Xm/ToggleB.h>
#include <Xm/Separator.h>
#include <Xm/MessageB.h>
#include <Xm/ScrollBar.h>
#include <Xm/RowColumn.h>
#include <Xm/FileSB.h>
#include <Xm/List.h>
#include <Xm/Frame.h>

#include "xmsprint.h"
#include "xltwavplay.h"
#include "wavplay.h"
#include "client.h"

static char *rcsid[] = {
	"@(#)xltwavplay.c $Revision: 1.2 $",
	_wavplay_h_,
	"@(#)xltwavplay version " WAVPLAY_VERSION
};

#ifndef NO_EDITRES
extern void _XEditResCheckmessages(void);		/* EditRes event handler */
#endif

extern pid_t svrPID;					/* Process ID of the wavplay server */
extern int svrIPC;					/* Message Queue ID for the wavplay server */
int cmdopt_x = 0;					/* This is controlled by bOptionsDebug */
int bRecorded = 0;					/* True after first record event */

/* FALLBACK RESOURCES : */

static String fallback_resources[] = {
#ifndef INSTALLED_RESOURCE_FILE				/* if not installed as file xltwavplay use.. */
	"xltwavplay.geometry: +350+250",
	"xltwavplay*background: grey70",
	"xltwavplay*foreground: black",

	"xltwavplay.*fontList: *helvetica-bold-r-normal--14*",

	"xltwavplay.main.menubar.spacing: 10",
	"xltwavplay.main.menubar.filemenucascade.labelString: File",
	"xltwavplay*filemenu.select_files.labelString: Select Files..",
	"xltwavplay*filemenu.exit.labelString: Exit",

	"xltwavplay*filemenu.save_as.labelString: Save " RECORD_PATH " As..",

	"xltwavplay.main.menubar.optsmenucascade.labelString: Options",
	"xltwavplay*optsmenu.debug.labelString: Debug Mode",
	"xltwavplay*optsmenu.sem_reset.labelString: Reset locking semaphores",

	"xltwavplay.main.menubar.helpmenucascade.labelString: Help",
	"xltwavplay*helpmenu.about.labelString: About",

	/* Copyright notice: adjust font to suit */
	"xltwavplay*AboutDlg.fontList: -adobe-helvetica-medium-r-normal-*-10-100-75-75-p-*-iso8859-1",

	"xltwavplay*FilesDlg.dialogTitle: Select file(s) to play",
	"xltwavplay*FilesDlg.pattern: *.wav",
	"xltwavplay*FilesDlg.fileTypeMask: FILE_REGULAR",

	"xltwavplay*SaveAsDlg.dialogTitle: Select a file name to write",
	"xltwavplay*SaveAsDlg.pattern: *.wav",
	"xltwavplay*SaveAsDlg.fileTypeMask: FILE_REGULAR",

	"xltwavplay*listbox_label.labelString: Selection:",

	"xltwavplay*FileLbl0.labelString: File:",
	"xltwavplay*TypeLbl0.labelString: Type:",
	"xltwavplay*DateLbl0.labelString: Date:",
	"xltwavplay*SizeLbl0.labelString: Size:",
	"xltwavplay*SamplingRateLbl0.labelString: Sampling Rate:",
	"xltwavplay*SamplesLbl0.labelString: Samples:",
	"xltwavplay*TimeLbl0.labelString: Duration:",

	"xltwavplay*FileLbl1.labelString: \\                                                            ",
	"xltwavplay*FileLbl1.foreground: black",
	"xltwavplay*TypeLbl1.labelString: ",
	"xltwavplay*TypeLbl1.foreground: black",
	"xltwavplay*DateLbl1.labelString: ",
	"xltwavplay*DateLbl1.foreground: black",
	"xltwavplay*SizeLbl1.labelString: ",
	"xltwavplay*SizeLbl1.foreground: black",
	"xltwavplay*SamplingRateLbl1.labelString: ",
	"xltwavplay*SamplingRateLbl1.foreground: black",
	"xltwavplay*SamplesLbl1.labelString: ",
	"xltwavplay*SamplesLbl1.foreground: black",
	"xltwavplay*TimeLbl1.labelString: ",
	"xltwavplay*TimeLbl1.foreground: black",

	"xltwavplay*Play.labelString: Play",
	"xltwavplay*StopPlay.labelString: Stop",
	"xltwavplay*Pause.labelString: Pause",
	"xltwavplay*Restore.labelString: Restore",

	"xltwavplay*Record.labelString: Record",
	"xltwavplay*Stop.labelString: Stop",

	"xltwavplay*8Bit.labelString: 8 bit",
	"xltwavplay*16Bit.labelString: 16 bit",

	"xltwavplay*rate_lbl.labelString: Sampling Rate:",

	"xltwavplay*rec_rate_tgl.labelString: Record Sampling Rate:",
#endif /* ndef INSTALLED_RESOURCE_FILE */
	NULL
};

#define NoTMR	0UL					/* Value for "no timer" */

static void SampleRateChg_Show(int newValue);

/*
 * WIDGETS:
 */
Widget wParent;						/* Parent shell widget */
XtAppContext aContext;					/* Application context */

Widget wMain;						/* Main widget */
Widget wMenuBar;					/* Main menu bar */
Widget wFileMenuCascade;				/* Cascade button for File Menu */
Widget wFileMenu;					/* File menu itself */
Widget wSelect;						/* File->Select */
Widget wSaveAs;						/* File->SaveAs.. */
Widget wExit;						/* File->Exit */

Widget wOptionsMenuCascade;
Widget wOptionsMenu;
Widget wOptions_DebugTgl;
Boolean bOptionsDebug = False;				/* Debug mode when True */
Widget wOptions_SemReset;				/* Options->Semaphore Reset */

Widget wHelpMenuCascade;				/* Cascade button for Help Menu */
Widget wHelpMenu;					/* Help menu itself */
Widget wAbout;						/* Help->About */

Widget wErrDlg;						/* Error Dialog Box */
XmString sErrDlgTitle;					/* Title for the Error Dialog Box */

Widget wAboutDlg;					/* About Dialog Box */
XmString sAboutDlgTitle;				/* About Dialog Title String */
XmString sAboutDlgMessage;				/* About Dialog message and copyright */

Widget wFilesDlg;					/* File selection dialog */
Widget wSaveAsDlg;					/* File "Save As..." dialog */

Widget wMainForm;					/* Main form widget */

Widget wLeftRC;						/* Left main Row/Column */
Widget wListFrame;					/* Frame for Selections List Box */
Widget wListForm;					/* Form within Selections List Box */
Widget wSelectionsLbl;					/* Label for Selections List Box */
Widget wSelectionsListBox;				/* The selections List Box */
Widget wRateSBFrame;					/* Frame for the scroll bar */
Widget wRateRC;						/* Row Col widget for Rate scroll bar */
Widget wRateLbl;					/* Label for the scroll bar */
Widget wRateScrollBar;					/* The Sampling Rate Scroll Bar */
int SampRate_value = 0;					/* Current scroll bar value */

Widget wRecRateFrame;					/* The record rate frame */
Widget wRecRateRC;
Widget wRecRateTgl;
Widget wRecRate;
int RecRateTgl_value = 0;

Widget wDetailsFrame;					/* Frame for File Info Details */
Widget wDetailsRC;					/* Horizontal RC for wBotLeftRC* */
Widget wBotLeftRC0;					/* Holds labels File:, Type: etc. */
Widget wFileLbl0;
Widget wTypeLbl0;
Widget wDateLbl0;
Widget wSizeLbl0;
Widget wSamplingRateLbl0;
Widget wSamplesLbl0;
Widget wTimeLbl0;

Widget wBotLeftRC1;					/* Holds labels to right of wBotLeftRC0 */
Widget wFileLbl1;
Widget wTypeLbl1;
Widget wDateLbl1;
Widget wSizeLbl1;
Widget wSamplingRateLbl1;
Widget wSamplesLbl1;
Widget wTimeLbl1;

Widget wRightRC;					/* Right side Row/Column */
Widget wBitsFrame;					/* Frame holding BitsRC */

Widget wBitsRC;						/* RowColumn that holds 8, 12, & 16 bits */

Widget w8BitCB;						/* 8-bit check box */
Widget w16BitCB;					/* 16-bit check box */
int curBits = 0;					/* The current setting */

Widget wPlayFrame;					/* Frame for play buttons */
Widget wPlayRC;						/* RC for play buttons */
Widget wPlayPB;						/* Play push button */
Widget wPausePB;					/* Pause push button */
Widget wStopPlayPB;					/* StopPlay push button */
Widget wRestorePB;					/* Restore push button */

Widget wStereoFrame;					/* Frame for Stereo Toggle */
Widget wStereoCB;					/* Stereo Check Box */

Widget wRecordFrame;
Widget wRecordRC;
Widget wRecordPB;					/* Record push button */
Widget wStopPB;						/* Stop push button */

XmString sRecorded_wav;					/* XmString() of RECORD_PATH */

/*
 * This is an internal conveniance routine:
 * In order for EditRes to function, we must register all shell widgets.
 */
#ifndef NO_EDITRES

static void
RegisterShellWidget(Widget w) {
	XtAddEventHandler(w,(EventMask)0,True,_XEditResCheckMessages,NULL);
}

#else

#define RegisterShellWidget(w)				/* No EDITRES facility */

#endif	/* ndef NO_EDITRES */

/*
 * This function initiates the creation of an error dialog with
 * a meaningful error text and an OK button. Note that the dialog
 * box is not operationaly until we return back to the main loop.
 */
static void
ReportError(const char *ErrMsg) {
	XmString s;							/* XmString form of message */

	if ( XtIsManaged(wErrDlg) )
		return;							/* The dialog is already in use */

	/*
	 * Set the Error Dialog message text, and manage it so that it pops up:
	 */
	s = XmStringCreateLtoR((char *)ErrMsg,XmSTRING_DEFAULT_CHARSET);/* We need an XmString */
	XtVaSetValues(wErrDlg,XmNmessageString,s,NULL);			/* Put msg in there */
	XtManageChild(wErrDlg);						/* Make it pop up */
	XmStringFree(s);						/* Release the string */
}

/*
 * Error function to format a message and report it:
 */
static void
v_erf(const char *format,va_list ap) {
	char buf[1024];

	vsprintf(buf,format,ap);			/* Format the message */
	fputs(buf,stderr);				/* Send to stderr */
	fputc('\n',stderr);				/* ..with a newline */
	ReportError(buf);				/* Now popup error message if we can */
}

/*
 * Format an error report:
 */
void
ReportErrorf(const char *format,...) {
	va_list ap;

	va_start(ap,format);				/* Access the arguments list */
	v_erf(format,ap);				/* Go report the error */
	va_end(ap);					/* A formality for stdargs */
}

/*
 * This function extracts the Record sampling rate from the text input widget:
 */
static UInt32
GetRecRate(void) {
	char *pText = XmTextGetString(wRecRate);			/* Get contents */
	unsigned long ul;

	if ( sscanf(pText,"%lu",&ul) != 1 ) {				/* Convert to u_long */
		if ( !*pText )
			ReportErrorf("No sampling rate has been specified.");
		else	ReportErrorf("Record sampling rate '%s' must be an integer.",pText);
		XtFree(pText);						/* Release this now */
		return 0UL;						/* Return no sampling rate */
	}
	XtFree(pText);							/* Release this now */
	XmSprintfText(wRecRate,"%lu",ul);				/* Reformat what we think we got */
	return (UInt32) ul;						/* Return sampling rate */
}

/*
 * This callback is called when the Record Sampling Rate input text widget gets
 * the focus. When this happens, we turn off the toggle that takes the sampling
 * rate from the slider.
 */
static void
RecRateFocus_CB(Widget w,XtPointer client_data,XtPointer call_data) {

	XtVaSetValues(wRecRateTgl,XmNset,False,NULL);			/* Disable toggle */
	RecRateTgl_value = 0;						/* And tracking value */
	XmTextSetInsertionPosition(wRecRate,0);				/* Cursor at start of field */
}

/*
 * This callback is called when the Record Sampling Rate toggle gets altered.
 */
static void
RecRateTgl_CB(Widget w,XtPointer client_data,XtPointer call_data) {
	XmToggleButtonCallbackStruct *ptr = (XmToggleButtonCallbackStruct *)call_data;

	if ( (RecRateTgl_value = ptr->set) != 0 )			/* Toggle set? */
		XmSprintfText(wRecRate,"%d",SampRate_value);		/* Yes, show rate from slider */
}

/*
 * This callback is entered when the [Play] button is pressed.
 */
static void
Play_CB(Widget w,XtPointer client_data,XtPointer call_data) {
	tosvr_play(0,v_erf);						/* Tell server to play */
}

/*
 * This callback is entered when the [Pause] button is pressed.
 */
static void
Pause_CB(Widget w,XtPointer client_data,XtPointer call_data) {
	tosvr_pause(0,v_erf);						/* Tell server to pause */
}

/*
 * This callback is entered when the [Stop] button is pressed.
 */
static void
Stop_CB(Widget w,XtPointer client_data,XtPointer call_data) {
	tosvr_stop(0,v_erf);						/* Tell the server to stop */
}

/*
 * This is the [Restore] settings button callback:
 */
static void
Restore_CB(Widget w,XtPointer client_data,XtPointer call_data) {

	tosvr_restore(0,v_erf);						/* Tell server to "restore" */
	XtVaSetValues(wRateScrollBar,XmNvalue,0,NULL);			/* Fix slider */
	SampleRateChg_Show(0);						/* Update sampling rate etc. */
}

/*
 * The [Record] button callback:
 */
static void
Record_CB(Widget w,XtPointer client_data,XtPointer call_data) {
	Arg al[20];						/* Arg list */
	Cardinal ac;						/* Args count */
	UInt32 RecRate = GetRecRate();				/* Sampling Rate */
	Chan chan_mode;						/* Stereo / Mono */
	UInt16 data_bits = curBits < 8 ? 8 : curBits;		/* Data bits */
	Boolean b;						/* Current setting of Stereo toggle */

	if ( RecRate < 1 ) {					/* We must have a sampling rate */
		ReportErrorf("Please set a Sampling Rate\nand try again.");
		return;
	}

	ac = 0;
	XtSetArg(al[ac],XmNset,&b); ++ac;
	XtGetValues(wStereoCB,al,ac);				/* Get Stereo toggle setting */

	chan_mode = b ? Stereo : Mono;				/* Channels now known */
	tosvr_record(0,v_erf,chan_mode,RecRate,data_bits);	/* Tell server to start recording */
}

/*
 * This callback is entered when a selection has been made from the list box.
 */
static void
PathSelected_CB(Widget w,XtPointer client_data,XtPointer call_data) {
	XmListCallbackStruct *ptr = (XmListCallbackStruct *)call_data;
	char *path;							/* C string copy */

	XmStringGetLtoR(ptr->item,XmSTRING_DEFAULT_CHARSET,&path);	/* Get C string pathname */
	tosvr_path(path,0,v_erf);					/* Tell server the new path */
	XtFree(path);							/* Release this string */
}

/*
 * Internal routine for updating the sampling rate display fields:
 */
static void
SampleRateChg_Show(int newValue) {

	if ( newValue < (int) DSP_MIN ) {
		/*
		 * If the value is less than the minimum, assume no overrides here:
		 */
		XmSprintfLabel(wRateLbl,"Sampling Rate Override: NONE");
		SampRate_value = 0;
	} else	{
		/*
		 * Otherwise display the overridden sampling rate:
		 */
		XmSprintfLabel(wRateLbl,"Sampling Rate Override: %d Hz",(int)newValue);
		SampRate_value = (int)newValue;
	}

	/*
	 * If the record toggle is set, then text box follows slider's value:
	 */
	if ( RecRateTgl_value != 0 )
		XmSprintfText(wRecRate,"%d",SampRate_value);
}

/*
 * This callback is entered when the sampling rate has changed (via scroll bar):
 */
static void
SampleRateChg_CB(Widget w,XtPointer client_data,XtPointer call_data) {
	XmScrollBarCallbackStruct *ptr = (XmScrollBarCallbackStruct *)call_data;

	SampleRateChg_Show(ptr->value);
	tosvr_sampling_rate(0,v_erf,(UInt32)ptr->value);
}

/*
 * This is the Stereo/Mono toggle callback:
 */
static void
StereoChanged_CB(Widget w,XtPointer client_data,XtPointer call_data) {
	XmToggleButtonCallbackStruct *ptr = (XmToggleButtonCallbackStruct *)call_data;

	tosvr_chan(0,v_erf,ptr->set ? Stereo : Mono);		/* Tell the server about change */
}

/*
 * This internal function is used to set the radio buttons:
 */
static void
BitsShow(void) {
	Widget wSet, wNot;

	switch ( curBits ) {
	default :
		curBits = 8;
	case 8 :
		wSet = w8BitCB;
		wNot = w16BitCB;
		break;
	case 16 :
		wSet = w16BitCB;
		wNot = w8BitCB;
	}

	XtVaSetValues(wSet,XmNset,True,NULL);
#if LessTif_Bug_radioBehavior
	/*
	 * Unless I've goofed somewhere, LessTif does not properly handle radio buttons:
	 */
	XtVaSetValues(wNot,XmNset,False,NULL);
#endif
}

/*
 * This callback is made when the bits per sample radio buttons have changed:
 */
static void
BitsChanged_CB(Widget w,XtPointer client_data,XtPointer call_data) {
	int bits = (int) (long) client_data;

	tosvr_bits(0,v_erf,bits);			/* Tell server the new bits setting */
}

/*
 * This callback, is for the [OK] button in the About dialog:
 */
static void
AboutDlgOkCB(Widget w,XtPointer client_data,XtPointer call_data) {
	if ( XtIsManaged(wAboutDlg) )
		XtUnmanageChild(wAboutDlg);		/* Pop down the About dialog */
}

/*
 * This callback is for the toggle in the Options->Debug menu selection:
 */
static void
OptionsDebugCB(Widget w,XtPointer client_data,XtPointer call_data) {
	Arg al[4];							/* Argument list */
	Cardinal ac;							/* Argument count */
	
	ac = 0;
	XtSetArg(al[ac],XmNset,&bOptionsDebug); ++ac;			/* Request Get Toggle Value */
	XtGetValues(wOptions_DebugTgl,al,ac);				/* Get value now */

	cmdopt_x = bOptionsDebug != False ? 1 : 0;			/* Update cmdopt_x also */
	tosvr_debug(0,v_erf,cmdopt_x);					/* And tell server the same */
}

/*
 * This callback is invoked when menu item Options->Semaphore Reset is activated.
 */
static void
OptionsSemResetCB(Widget w,XtPointer client_data,XtPointer call_data) {
	tosvr_semreset(0,v_erf);				/* Request server semaphore reset */
}

/*
 * This callback is invoked by the Help->About menu selection:
 */
static void
AboutCB(Widget w,XtPointer client_data,XtPointer call_data) {

	if ( XtIsManaged(wAboutDlg) )					/* For safety */
		return;							/* The dialog is already in use */

	XtVaSetValues(wAboutDlg,XmNmessageString,sAboutDlgMessage,NULL);/* Supply message */
	XtManageChild(wAboutDlg);					/* Pop up the dialog now */
}

/*
 * This callback is invoked by Menu File->Select, to pop up the file selections dialog box.
 */
static void
PopupSelectFilesCB(Widget w,XtPointer client_data,XtPointer call_data) {

	if ( XtIsManaged(wFilesDlg) )					/* Already managed? */
		return;							/* yes, just return */
	XtManageChild(wFilesDlg);					/* else, make visible now */
}

/*
 * This callback is invoked by menu File->Save As.., to start a dialog to allow
 * saving the recorded.wav file somewhere else.
 */
static void
PopupSaveAsCB(Widget w,XtPointer client_data,XtPointer call_data) {

	if ( XtIsManaged(wSaveAsDlg) )					/* Is this already managed? */
		return;							/* Yes, just exit */
	XtManageChild(wSaveAsDlg);					/* else manage it now */
}

/*
 * This callback is invoked by Menu File->Exit, to do the fatal thing..
 */
static void
ExitCB(Widget w,XtPointer client_data,XtPointer call_data) {

	exit(0); /* This invokes some atexit() calls */
}

/*
 * This callback is called when the user presses [OK] in the error dialog.
 */
static void
ExitDlgCB(Widget w,XtPointer client_data,XtPointer call_data) {

	if ( XtIsManaged(wErrDlg) )					/* Is it still managed? */
		XtUnmanageChild(wErrDlg);				/* Yes, unmanage it now */
}

/*
 * This internal function is used to add a new pathname to the list box:
 */
static void
PutFileSelection(XmString xms_pathname) {

	if ( XmListItemExists(wSelectionsListBox,xms_pathname) == False )
		XmListAddItemUnselected(wSelectionsListBox,xms_pathname,0); /* Append to list */
}

/*
 * This callback is invoked by the file selection dialog, to add another pathname to the
 * list box.
 */
static void
FilesDlgOKCB(Widget w,XtPointer client_data,XtPointer call_data) {
	XmFileSelectionBoxCallbackStruct *ptr = (XmFileSelectionBoxCallbackStruct *) call_data;

	PutFileSelection(ptr->value);		/* Put this string into the selection box */
}

/*
 * This call back pops down the dialog box (for any dialog). The widget used for the pop down
 * is expected to be passed in the client_data argument.
 */
static void
DialogCancelCB(Widget w,XtPointer client_data,XtPointer call_data) {
	Widget wDlg = (Widget) client_data;				/* Get dialog widget */

	if ( XtIsManaged(wDlg) )					/* Is it still managed? */
		XtUnmanageChild(wDlg);					/* yep, unmanage it now */
}

/*
 * This function dispatches wavplay server response messages:
 */
static void
DispatchMsg(SVRMSG *msg) {
	XmString s;

	if ( msg->msg_type == ToClnt_Bits ) {
		/*
		 * Server has responded with a new bits per sample setting:
		 */
		if ( msg->u.toclnt_bits.DataBits != 0 ) {		/* Ignore if value is zero.. */
			curBits = msg->u.toclnt_bits.DataBits;		/* Server indicates new setting */
			BitsShow();					/* Update our client displays */
		}
	} else if ( msg->msg_type == ToClnt_Settings ) {
		/*
		 * A bunch of settings have changed, according to the server:
		 */
		if ( msg->u.toclnt_settings.DataBits != 0 ) {		/* Ignore bits if zero.. */
			curBits = msg->u.toclnt_settings.DataBits;	/* Current server bit setting */
			BitsShow();					/* Reflect in controls */
		}
		XtVaSetValues(wStereoCB,XmNset,
			msg->u.toclnt_settings.Channels == Stereo ? True : False,
			NULL);						/* Show stereo setting */
		/*
		 * Update label text:
		 */
		XmSprintfLabel(wSamplingRateLbl1,"%lu Hz%s",
			(unsigned long)msg->u.toclnt_settings.SamplingRate,
			msg->u.toclnt_settings.bOvrSampling ? "*" : "");

		if ( msg->u.toclnt_settings.bOvrSampling != 0 ) {
			SampleRateChg_Show((int)msg->u.toclnt_settings.SamplingRate);
			XtVaSetValues(wRateScrollBar,XmNvalue,msg->u.toclnt_settings.SamplingRate,NULL);
		} else	{
			SampleRateChg_Show(0);
			XtVaSetValues(wRateScrollBar,XmNvalue,0,NULL);
		}

		/*
		 * Currently, this value should be "PCM" (wav file type)
		 */
		msg->u.toclnt_settings.WavType[sizeof msg->u.toclnt_settings.WavType - 1] = 0;
		XmSprintfLabel(wTypeLbl1,"%s",msg->u.toclnt_settings.WavType);

		/*
		 * Update samples, bits and channels:
		 */
		XmSprintfLabel(wSamplesLbl1,"%lu %u-bit%s %s%s",
			(unsigned long)msg->u.toclnt_settings.Samples,
			(unsigned)msg->u.toclnt_settings.DataBits,
			msg->u.toclnt_settings.bOvrBits ? "*" : "",
			msg->u.toclnt_settings.Channels == Stereo ? "stereo" : "mono",
			msg->u.toclnt_settings.bOvrMode ? "*" : "");

		/*
		 * Update duration time:
		 */
		XmSprintfLabel(wTimeLbl1,"%.2lf seconds",
			(double)msg->u.toclnt_settings.Samples
				/ (double)msg->u.toclnt_settings.SamplingRate);

	} else if ( msg->msg_type == ToClnt_WavInfo ) {

		if ( msg->u.toclnt_wavinfo.errnox != 0 ) {
			/*
			 * Format an error message where the pathname should go:
			 */
			XmSprintfLabel(wFileLbl1,"<%s>",sys_errlist[msg->u.toclnt_wavinfo.errnox]);
			XmSprintfLabel(wDateLbl1,"");
			XmSprintfLabel(wSizeLbl1,"");
			XmSprintfLabel(wSamplesLbl1,"");
			XmSprintfLabel(wTimeLbl1,"");
		} else	{
			if ( msg->u.toclnt_wavinfo.wavinfo.DataBits != 0 ) {
				curBits = msg->u.toclnt_wavinfo.wavinfo.DataBits; /* Current server bit setting */
				BitsShow();				/* Reflect in controls */
			}
			XtVaSetValues(wStereoCB,XmNset,
				msg->u.toclnt_wavinfo.wavinfo.Channels == Stereo ? True : False,
				NULL);					/* Show stereo setting */
			/*
			 * Update label text:
			 */
			XmSprintfLabel(wSamplingRateLbl1,"%lu Hz",
				(unsigned long)msg->u.toclnt_wavinfo.wavinfo.SamplingRate);

			XmSprintfLabel(wTypeLbl1,"%s","PCM");	/* For now... */

			XmSprintfLabel(wSamplesLbl1,"%lu %u-bit %s",
				(unsigned long)msg->u.toclnt_wavinfo.wavinfo.Samples,
				(unsigned)msg->u.toclnt_wavinfo.wavinfo.DataBits,
				msg->u.toclnt_wavinfo.wavinfo.Channels == Stereo ? "stereo" : "mono");

			XmSprintfLabel(wTimeLbl1,"%.2lf seconds",
				(double)msg->u.toclnt_wavinfo.wavinfo.Samples
					/ (double)msg->u.toclnt_wavinfo.wavinfo.SamplingRate);
		}
	} else if ( msg->msg_type == ToClnt_Stat ) {
		/*
		 * Update stat() info:
		 */
		if ( msg->u.toclnt_stat.errnox > 0 ) {
			/*
			 * Show stat() error:
			 */
			XmSprintfLabel(wFileLbl1,"<%s>",sys_errlist[msg->u.toclnt_stat.errnox]);
			XmSprintfLabel(wDateLbl1,"");
			XmSprintfLabel(wSizeLbl1,"");
			XmSprintfLabel(wSamplesLbl1,"");
			XmSprintfLabel(wTimeLbl1,"");
		} else	{
			/*
			 * Show stat() info:
			 */
			XmSprintfLabel(wDateLbl1,"%s",StrDate(msg->u.toclnt_stat.sbuf.st_mtime));
			XmSprintfLabel(wSizeLbl1,"%lu bytes",(unsigned long)msg->u.toclnt_stat.sbuf.st_size);
		}

	} else if ( msg->msg_type == ToClnt_Path ) {
		/*
		 * Update pathname in panel display
		 */
		msg->u.toclnt_path.path[msg->bytes] = 0;
		XmSprintfLabel(wFileLbl1,"%s",msg->u.toclnt_path.path);

		/*
		 * Highlight the pathname in the list box if we can (it should already be there)
		 */
		s = XmStringCreate(msg->u.toclnt_path.path,XmSTRING_DEFAULT_CHARSET);
		XmListSelectItem(wSelectionsListBox,s,False);
		XmStringFree(s);

		if ( !bRecorded && !strcmp(msg->u.toclnt_path.path,RECORD_PATH) ) {
			s = XmStringCreateLtoR(RECORD_PATH,XmSTRING_DEFAULT_CHARSET);
			PutFileSelection(s);
			XmStringFree(s);
			bRecorded = 1;
		}
	} else if ( msg->msg_type == ToClnt_ErrMsg )
		ReportError(msg->u.toclnt_errmsg.msg);
}

/*
 * This timed procedure just looks for incoming messages from the server:
 */
static void
TimerProc1(XtPointer client_data,XtIntervalId *timer_id) {
	SVRMSG msg;
	int z;

	/*
	 * Process messages until we run out of messages:
	 */
rpt:	while ( (z = MsgFromServer(svrIPC,&msg,IPC_NOWAIT)) == 0 )
		DispatchMsg(&msg);				/* Act upon the server message */
	
	if ( z != 0 && errno != ENOMSG ) {
		ReportErrorf("%s: Internal error from MsgFromServer()",sys_errlist[errno]);
		goto rpt;					/* There may be more messages */
	}

	XtAppAddTimeOut(aContext,TIMER1_MS,TimerProc1,NULL);
}

/*
 * The main program needs no introduction:
 */
int
main(int argc,char **argv) {
	Arg al[20];						/* Arg list */
	Cardinal ac;						/* Args count */
	
	/*
	 * CreateApplication Context:
	 */
	ac = 0;
	XtSetArg(al[ac],XmNtitle,"X LessTif WAV Play version " WAVPLAY_VERSION); ++ac;
	wParent = XtAppInitialize(&aContext,
		"xltwavplay",				/* app. class */
		(XrmOptionDescList) NULL, 0,		/* options */
		&argc, argv,				/* cmd line */
		fallback_resources,			/* fallback resources */
		al,ac);					/* hard coded resources */

	/*
	 * Create XmString() version of RECORD_PATH for list box processing:
	 */
	sRecorded_wav = XmStringCreate(RECORD_PATH,XmSTRING_DEFAULT_CHARSET);

	/*
	 * Create Main widget:
	 */
	ac = 0;
	wMain = XmCreateMainWindow(wParent,"main",al,ac);
	XtManageChild(wMain);
	RegisterShellWidget(wMain);			/* This is for Editres */

	/*
	 * Create the menu bar at the top of the main window:
	 */
	ac = 0;
	wMenuBar = XmCreateMenuBar(wMain,"menubar",al,ac);
	XtManageChild(wMenuBar);

	/*
	 * Create the FILE menu in the menu bar:
	 */
	CreateMenu(wMenuBar,&wFileMenu,"filemenu",&wFileMenuCascade,"filemenucascade",
		"select_files",'M',PopupSelectFilesCB,&wSelect,	/* File->Select */
		"save_as",'M',PopupSaveAsCB,&wSaveAs,		/* File->Save As.. */
		"exit",'M',ExitCB,&wExit,			/* File->Exit */
		NULL);

	/*
	 * Create the OPTIONS menu in the menu bar:
	 */
	CreateMenu(wMenuBar,&wOptionsMenu,"optsmenu",&wOptionsMenuCascade,"optsmenucascade",
		"debug",'T',OptionsDebugCB,&wOptions_DebugTgl,	/* Options->Debug (toggle) */
		"sem_reset",'M',OptionsSemResetCB,&wOptions_SemReset, /* Options->Semaphore Reset */
		NULL);

	/*
	 * Create the HELP menu in the menu bar:
	 */
	CreateMenu(wMenuBar,&wHelpMenu,"helpmenu",&wHelpMenuCascade,"helpmenucascade",
		"about",'M',AboutCB,&wAbout,			/* Help->About */
		NULL);

	/*
	 * Create the error message dialog:
	 */
	sErrDlgTitle = XmStringCreate("X LessTif WAV Play : Error Message",XmSTRING_DEFAULT_CHARSET);
	ac = 0;
	XtSetArg(al[ac],XmNdefaultPosition,FALSE); ac++;
	XtSetArg(al[ac],XmNdialogStyle,XmDIALOG_APPLICATION_MODAL); ac++;
	XtSetArg(al[ac],XmNtitle,"Error Report"); ac++;
	XtSetArg(al[ac],XmNdefaultButtonType,XmDIALOG_OK_BUTTON); ++ac;
	XtSetArg(al[ac],XmNmessageAlignment,XmALIGNMENT_CENTER); ++ac;
	XtSetArg(al[ac],XmNdialogTitle,sErrDlgTitle); ++ac;
	XtSetArg(al[ac],XmNallowResize,FALSE); ++ac;
	wErrDlg = XmCreateErrorDialog(wParent,"ErrDlg",al,ac);
	XtAddCallback(wErrDlg,XmNokCallback,ExitDlgCB,NULL);
	XtUnmanageChild(XmMessageBoxGetChild(wErrDlg,XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild(XmMessageBoxGetChild(wErrDlg,XmDIALOG_HELP_BUTTON));
	RegisterShellWidget(wErrDlg);

	/*
	 * Create About Dialog Message String:
	 */
	sAboutDlgTitle = XmStringCreate("About X LessTif WAV Play "WAVPLAY_VERSION,XmSTRING_DEFAULT_CHARSET);
	sAboutDlgMessage = XmStringCreateLtoR(
		"xltwavplay version " WAVPLAY_VERSION "\n"
		"was written by and is\n"
		"Copyright (C) 1997 by Warren W. Gay VE3WWG\n"
		"---\n"
		"Special thanks to Erik de Castro Lopo (erikd@zip.com.au)\n"
		"for his contributed WAV header code\n"
		"(files wavfile.c and wavfile.h)\n"
		"---\n"
		"Thanks also go to Andre Fuechsel for his original\n"
		"wavplay that inspired the current work (though\n"
		"this version does not contain his code)\n"
		"---\n"
		"Many thanks to the LINUX folks,\n"
		"the XFree86 folks,\n"
		"and the LessTif group\n"
		"for making this application possible.\n"
		"---\n"
		"This program comes with\n"
		"ABSOLUTELY NO WARRANTY.",
		XmSTRING_DEFAULT_CHARSET);

	/*
	 * Create the About Dialog box:
	 */
	ac = 0;
	XtSetArg(al[ac],XmNdefaultPosition,FALSE); ac++;
	XtSetArg(al[ac],XmNtitle,"Error Report"); ac++;
	XtSetArg(al[ac],XmNdefaultButtonType,XmDIALOG_OK_BUTTON); ++ac;
	XtSetArg(al[ac],XmNmessageAlignment,XmALIGNMENT_CENTER); ++ac;
	XtSetArg(al[ac],XmNdialogTitle,sAboutDlgTitle); ++ac;
	XtSetArg(al[ac],XmNallowResize,FALSE); ++ac;
	wAboutDlg = XmCreateMessageDialog(wParent,"AboutDlg",al,ac);
	XtAddCallback(wAboutDlg,XmNokCallback,AboutDlgOkCB,NULL);
	XtUnmanageChild(XmMessageBoxGetChild(wAboutDlg,XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild(XmMessageBoxGetChild(wAboutDlg,XmDIALOG_HELP_BUTTON));
	RegisterShellWidget(wAboutDlg);

	/*
	 * Create Files Selection Dialog:
	 */
	ac = 0;
	XtSetArg(al[ac],XmNfileTypeMask,XmFILE_REGULAR); ++ac;
	wFilesDlg = XmCreateFileSelectionDialog(wParent,"FilesDlg",al,ac);
	XtAddCallback(wFilesDlg,XmNokCallback,FilesDlgOKCB,NULL);
	XtAddCallback(wFilesDlg,XmNcancelCallback,DialogCancelCB,(XtPointer)wFilesDlg);
	XtUnmanageChild(XmFileSelectionBoxGetChild(wFilesDlg,XmDIALOG_HELP_BUTTON));
	RegisterShellWidget(wFilesDlg);

	/*
	 * Create "Save As..." Dialog
	 */
	ac = 0;
	XtSetArg(al[ac],XmNfileTypeMask,XmFILE_REGULAR); ++ac;
	wSaveAsDlg = XmCreateFileSelectionDialog(wParent,"SaveAsDlg",al,ac);
	XtAddCallback(wSaveAsDlg,XmNokCallback,SaveAsDlgOKCB,NULL);
	XtAddCallback(wSaveAsDlg,XmNcancelCallback,DialogCancelCB,(XtPointer)wSaveAsDlg);
	XtUnmanageChild(XmFileSelectionBoxGetChild(wSaveAsDlg,XmDIALOG_HELP_BUTTON));
	RegisterShellWidget(wSaveAsDlg);

	/*
	 * Create the main form widget:
	 */
	ac = 0;
	wMainForm = XmCreateForm(wMain,"main_form",al,ac);
	XtManageChild(wMainForm);

	ac = 0;
	XtSetArg(al[ac],XmNmarginHeight,6); ++ac;
	XtSetArg(al[ac],XmNmarginWidth,6); ++ac;
	wLeftRC = XmCreateRowColumn(wMainForm,"LeftRC",al,ac);
	XtManageChild(wLeftRC);

	ac = 0;
	XtSetArg(al[ac],XmNmarginHeight,6); ++ac;
	XtSetArg(al[ac],XmNmarginWidth,6); ++ac;
	wListFrame = XmCreateFrame(wLeftRC,"list_frame",al,ac);
	XtManageChild(wListFrame);

	ac = 0;
	wListForm = XmCreateForm(wListFrame,"list_form",al,ac);
	XtManageChild(wListForm);

	/*
	 * Create the Selections List Box Label:
	 */
	ac = 0;
	XtSetArg(al[ac],XmNmarginWidth,6); ++ac;
	wSelectionsLbl = XmCreateLabel(wListForm,"listbox_label",al,ac);
	XtManageChild(wSelectionsLbl);

	/*
	 * Create the Selections List Box:
	 */
	ac = 0;
	XtSetArg(al[ac],XmNvisibleItemCount,7); ++ac;
	XtSetArg(al[ac],XmNscrollingPolicy,XmSTATIC); ++ac;
	XtSetArg(al[ac],XmNvisualPolicy,XmCONSTANT); ++ac;
	XtSetArg(al[ac],XmNscrolledWindowMarginHeight,6); ++ac;
	XtSetArg(al[ac],XmNscrolledWindowMarginWidth,6); ++ac;
	XtSetArg(al[ac],XmNselectionPolicy,XmSINGLE_SELECT); ++ac;
	XtSetArg(al[ac],XmNwidth,420); ++ac;
	wSelectionsListBox = XmCreateScrolledList(wListForm,"lstbox",al,ac);
	XtManageChild(wSelectionsListBox);
	XtAddCallback(wSelectionsListBox,XmNsingleSelectionCallback,PathSelected_CB,NULL);

	XtVaSetValues(wSelectionsLbl,
		XmNtopAttachment,XmATTACH_FORM,
		XmNleftAttachment,XmATTACH_FORM,
		NULL);

	XtVaSetValues(XtParent(wSelectionsListBox),
		XmNtopAttachment,XmATTACH_WIDGET,		XmNtopWidget,wSelectionsLbl,
		XmNleftAttachment,XmATTACH_FORM,
		NULL);

	ac = 0;
	XtSetArg(al[ac],XmNmarginHeight,12); ++ac;
	XtSetArg(al[ac],XmNmarginWidth,6); ++ac;
	wDetailsFrame = XmCreateFrame(wLeftRC,"DetailsFrame",al,ac);
	XtManageChild(wDetailsFrame);

	ac = 0;
	XtSetArg(al[ac],XmNorientation,XmHORIZONTAL); ++ac;
	wDetailsRC = XmCreateRowColumn(wDetailsFrame,"DetailsRC",al,ac);
	XtManageChild(wDetailsRC);

	ac = 0;
	XtSetArg(al[ac],XmNorientation,XmVERTICAL); ++ac;
	XtSetArg(al[ac],XmNentryAlignment,XmALIGNMENT_END); ++ac;
	wBotLeftRC0 = XmCreateRowColumn(wDetailsRC,"BotLeftRC0",al,ac);
	XtManageChild(wBotLeftRC0);

	ac = 0;
	XtSetArg(al[ac],XmNorientation,XmVERTICAL); ++ac;
	XtSetArg(al[ac],XmNentryAlignment,XmALIGNMENT_BEGINNING); ++ac;
	wBotLeftRC1 = XmCreateRowColumn(wDetailsRC,"BotLeftRC1",al,ac);
	XtManageChild(wBotLeftRC1);

	ac = 0;
	wFileLbl0 = XmCreateLabel(wBotLeftRC0,"FileLbl0",al,ac);
	XtManageChild(wFileLbl0);
	
	ac = 0;
	wTypeLbl0 = XmCreateLabel(wBotLeftRC0,"TypeLbl0",al,ac);
	XtManageChild(wTypeLbl0);
	
	ac = 0;
	wDateLbl0 = XmCreateLabel(wBotLeftRC0,"DateLbl0",al,ac);
	XtManageChild(wDateLbl0);
	
	ac = 0;
	wSizeLbl0 = XmCreateLabel(wBotLeftRC0,"SizeLbl0",al,ac);
	XtManageChild(wSizeLbl0);
	
	ac = 0;
	wSamplingRateLbl0 = XmCreateLabel(wBotLeftRC0,"SamplingRateLbl0",al,ac);
	XtManageChild(wSamplingRateLbl0);
	
	ac = 0;
	wSamplesLbl0 = XmCreateLabel(wBotLeftRC0,"SamplesLbl0",al,ac);
	XtManageChild(wSamplesLbl0);
	
	ac = 0;
	wTimeLbl0 = XmCreateLabel(wBotLeftRC0,"TimeLbl0",al,ac);
	XtManageChild(wTimeLbl0);
	
	ac = 0;
	wFileLbl1 = XmCreateLabel(wBotLeftRC1,"FileLbl1",al,ac);
	XtManageChild(wFileLbl1);
	
	ac = 0;
	wTypeLbl1 = XmCreateLabel(wBotLeftRC1,"TypeLbl1",al,ac);
	XtManageChild(wTypeLbl1);
	
	ac = 0;
	wDateLbl1 = XmCreateLabel(wBotLeftRC1,"DateLbl1",al,ac);
	XtManageChild(wDateLbl1);
	
	ac = 0;
	wSizeLbl1 = XmCreateLabel(wBotLeftRC1,"SizeLbl1",al,ac);
	XtManageChild(wSizeLbl1);
	
	ac = 0;
	wSamplingRateLbl1 = XmCreateLabel(wBotLeftRC1,"SamplingRateLbl1",al,ac);
	XtManageChild(wSamplingRateLbl1);
	
	ac = 0;
	wSamplesLbl1 = XmCreateLabel(wBotLeftRC1,"SamplesLbl1",al,ac);
	XtManageChild(wSamplesLbl1);
	
	ac = 0;
	wTimeLbl1 = XmCreateLabel(wBotLeftRC1,"TimeLbl1",al,ac);
	XtManageChild(wTimeLbl1);
	
	/*
	 * Create the sampling rate scroll bar:
	 */
	ac = 0;
	XtSetArg(al[ac],XmNmarginHeight,6); ++ac;
	XtSetArg(al[ac],XmNmarginWidth,6); ++ac;
	wRateSBFrame = XmCreateFrame(wLeftRC,"rateSBframe",al,ac);
	XtManageChild(wRateSBFrame);
	
	ac = 0;
	wRateRC = XmCreateRowColumn(wRateSBFrame,"rate_rolcol",al,ac);
	XtManageChild(wRateRC);

	ac = 0;
	wRateLbl = XmCreateLabel(wRateRC,"rate_lbl",al,ac);
	XtManageChild(wRateLbl);

	ac = 0;
	XtSetArg(al[ac],XmNorientation,XmHORIZONTAL); ++ac;
	XtSetArg(al[ac],XmNminimum,0); ++ac;			/* DSP_MIN is actual min */
	XtSetArg(al[ac],XmNsliderSize,4000); ++ac;
	XtSetArg(al[ac],XmNmaximum,DSP_MAX+4000); ++ac;
	XtSetArg(al[ac],XmNincrement,100); ++ac;
	wRateScrollBar = XmCreateScrollBar(wRateRC,"rate_scrollbar",al,ac);
	XtManageChild(wRateScrollBar);
	XtAddCallback(wRateScrollBar,XmNvalueChangedCallback,SampleRateChg_CB,NULL);

	ac = 0;
	XtSetArg(al[ac],XmNmarginWidth,6); ++ac;
	XtSetArg(al[ac],XmNmarginHeight,6); ++ac;
	wRecRateFrame = XmCreateFrame(wRateRC,"rec_rate_frame",al,ac);
	XtManageChild(wRecRateFrame);
	
	ac = 0;
	XtSetArg(al[ac],XmNorientation,XmHORIZONTAL); ++ac;
	XtSetArg(al[ac],XmNpacking,XmPACK_COLUMN); ++ac;
	XtSetArg(al[ac],XmNnumColumns,1); ++ac;
	wRecRateRC = XmCreateRowColumn(wRecRateFrame,"rec_rate_rc",al,ac);
	XtManageChild(wRecRateRC);
	
	ac = 0;
	wRecRateTgl = XmCreateToggleButton(wRecRateRC,"rec_rate_tgl",al,ac);
	XtManageChild(wRecRateTgl);
	XtAddCallback(wRecRateTgl,XmNvalueChangedCallback,RecRateTgl_CB,NULL);
	
	ac = 0;
	XtSetArg(al[ac],XmNeditable,True); ++ac;
	wRecRate = XmCreateText(wRecRateRC,"rec_rate",al,ac);
	XtManageChild(wRecRate);
	XtAddCallback(wRecRate,XmNfocusCallback,RecRateFocus_CB,NULL);
	
	/*
	 * Now build the right hand side:
	 */
	ac = 0;
	XtSetArg(al[ac],XmNorientation,XmVERTICAL); ++ac;
	XtSetArg(al[ac],XmNmarginHeight,6); ++ac;
	XtSetArg(al[ac],XmNmarginWidth,6); ++ac;
	XtSetArg(al[ac],XmNpacking,XmPACK_TIGHT); ++ac;
	wRightRC = XmCreateRowColumn(wMainForm,"RightRC",al,ac);
	XtManageChild(wRightRC);

	ac = 0;
	XtSetArg(al[ac],XmNmarginHeight,6); ++ac;
	XtSetArg(al[ac],XmNmarginWidth,6); ++ac;
	wBitsFrame = XmCreateFrame(wRightRC,"BitsFrame",al,ac);
	XtManageChild(wBitsFrame);

	ac = 0;
	XtSetArg(al[ac],XmNorientation,XmVERTICAL); ++ac;
	XtSetArg(al[ac],XmNmarginHeight,6); ++ac;
	XtSetArg(al[ac],XmNmarginWidth,6); ++ac;
	XtSetArg(al[ac],XmNpacking,XmPACK_TIGHT); ++ac;
	XtSetArg(al[ac],XmNradioBehavior,True); ++ac;
	wBitsRC = XmCreateRowColumn(wBitsFrame,"BitsRC",al,ac);
	XtManageChild(wBitsRC);

	ac = 0;
	w8BitCB = XmCreateToggleButton(wBitsRC,"8Bit",al,ac);
	XtManageChild(w8BitCB);
	XtAddCallback(w8BitCB,XmNvalueChangedCallback,BitsChanged_CB,(XtPointer)8UL);

	ac = 0;
	w16BitCB = XmCreateToggleButton(wBitsRC,"16Bit",al,ac);
	XtManageChild(w16BitCB);
	XtAddCallback(w16BitCB,XmNvalueChangedCallback,BitsChanged_CB,(XtPointer)16UL);

	ac = 0;
	XtSetArg(al[ac],XmNmarginHeight,6); ++ac;
	XtSetArg(al[ac],XmNmarginWidth,6); ++ac;
	wPlayFrame = XmCreateFrame(wRightRC,"PlayFrame",al,ac);
	XtManageChild(wPlayFrame);

	ac = 0;
	XtSetArg(al[ac],XmNorientation,XmVERTICAL); ++ac;
	XtSetArg(al[ac],XmNmarginHeight,6); ++ac;
	XtSetArg(al[ac],XmNmarginWidth,6); ++ac;
	XtSetArg(al[ac],XmNpacking,XmPACK_TIGHT); ++ac;
	wPlayRC = XmCreateRowColumn(wPlayFrame,"PlayRC",al,ac);
	XtManageChild(wPlayRC);

	ac = 0;
	wPlayPB = XmCreatePushButton(wPlayRC,"Play",al,ac);
	XtManageChild(wPlayPB);
	XtAddCallback(wPlayPB,XmNactivateCallback,Play_CB,NULL);

	ac = 0;
	wPausePB = XmCreatePushButton(wPlayRC,"Pause",al,ac);
	XtManageChild(wPausePB);
	XtAddCallback(wPausePB,XmNactivateCallback,Pause_CB,NULL);

	ac = 0;
	wStopPlayPB = XmCreatePushButton(wPlayRC,"Stop",al,ac);
	XtManageChild(wStopPlayPB);
	XtAddCallback(wStopPlayPB,XmNactivateCallback,Stop_CB,NULL);

	ac = 0;
	wRestorePB = XmCreatePushButton(wPlayRC,"Restore",al,ac);
	XtManageChild(wRestorePB);
	XtAddCallback(wRestorePB,XmNactivateCallback,Restore_CB,NULL);

	ac = 0;
	XtSetArg(al[ac],XmNmarginHeight,6); ++ac;
	XtSetArg(al[ac],XmNmarginWidth,6); ++ac;
	wStereoFrame = XmCreateFrame(wRightRC,"StereoFrame",al,ac);
	XtManageChild(wStereoFrame);

	ac = 0;
	wStereoCB = XmCreateToggleButton(wStereoFrame,"Stereo",al,ac);
	XtManageChild(wStereoCB);
	XtAddCallback(wStereoCB,XmNvalueChangedCallback,StereoChanged_CB,NULL);

	ac = 0;
	XtSetArg(al[ac],XmNmarginHeight,6); ++ac;
	XtSetArg(al[ac],XmNmarginWidth,6); ++ac;
	wRecordFrame = XmCreateFrame(wRightRC,"RecordFrame",al,ac);
	XtManageChild(wRecordFrame);

	ac = 0;
	XtSetArg(al[ac],XmNorientation,XmVERTICAL); ++ac;
	XtSetArg(al[ac],XmNmarginHeight,6); ++ac;
	XtSetArg(al[ac],XmNmarginWidth,6); ++ac;
	XtSetArg(al[ac],XmNpacking,XmPACK_TIGHT); ++ac;
	wRecordRC = XmCreateRowColumn(wRecordFrame,"RecordRC",al,ac);
	XtManageChild(wRecordRC);

	ac = 0;
	wRecordPB = XmCreatePushButton(wRecordRC,"Record",al,ac);
	XtManageChild(wRecordPB);
	XtAddCallback(wRecordPB,XmNactivateCallback,Record_CB,NULL);

	ac = 0;
	wStopPB = XmCreatePushButton(wRecordRC,"Stop",al,ac);
	XtManageChild(wStopPB);
	XtAddCallback(wStopPB,XmNactivateCallback,Stop_CB,NULL);

	/*
	 * Main form layout details:
	 */
	XtVaSetValues(wLeftRC,
		XmNtopAttachment,XmATTACH_FORM,
		XmNleftAttachment,XmATTACH_FORM,
		NULL);

	XtVaSetValues(wRightRC,
		XmNtopAttachment,XmATTACH_FORM,
		XmNleftAttachment,XmATTACH_WIDGET,		XmNleftWidget,wLeftRC,
		XmNrightAttachment,XmATTACH_FORM,
		NULL);

	/*
	 * Realize the widgets now:
	 */
	BitsShow();						/* Initialize bit settings */
	XtVaSetValues(wOptions_DebugTgl,XmNset,bOptionsDebug ? True : False,NULL);
	cmdopt_x = bOptionsDebug != False ? 1 : 0;
	XtRealizeWidget(wParent);				/* OK, realize it all.. */

	/*
	 * Fork a new process, and try to start the server:
	 */
	if ( tosvr_start(v_erf) < 0 ) {
		ReportErrorf("Failure to start the wavplay server is fatal.\n"
			"Please check the executability of the 'wavplay' command.");
		exit(3);
	}

	/*
	 * Tell the server our debug setting:
	 */
	tosvr_debug(0,NULL,cmdopt_x);

	/*
	 * This work process receives and operates on messages
	 * from the server to this client program:
	 */
	XtAppAddTimeOut(aContext,TIMER1_MS,TimerProc1,NULL);

	/*
	 * The MOTIF Main Loop:
	 */
	XtAppMainLoop(aContext);
	return 0;						/* A cookie for the compiler */
}                       

/* $Source: /wwg/motif/wavplay.RCS/RCS/xltwavplay.c,v $ */
