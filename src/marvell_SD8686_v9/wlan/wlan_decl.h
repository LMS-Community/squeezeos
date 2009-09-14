/** @file wlan_decl.h
 *  @brief This file contains declaration referring to
 *  functions defined in other source files
 *     
 *  (c) Copyright © 2003-2007, Marvell International Ltd.  
 *
 *  This software file (the "File") is distributed by Marvell International 
 *  Ltd. under the terms of the GNU General Public License Version 2, June 1991 
 *  (the "License").  You may use, redistribute and/or modify this File in 
 *  accordance with the terms and conditions of the License, a copy of which 
 *  is available along with the File in the gpl.txt file or by writing to 
 *  the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 
 *  02111-1307 or on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
 *
 *  THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE 
 *  IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE 
 *  ARE EXPRESSLY DISCLAIMED.  The License provides additional details about 
 *  this warranty disclaimer.
 *
 */
/******************************************************
Change log:
	09/29/05: add Doxygen format comments
	01/05/06: Add kernel 2.6.x support	
	01/11/06: Conditionalize new scan/join structures.
	          Move wlan_wext statics to their source file.
******************************************************/

#ifndef _WLAN_DECL_H_
#define _WLAN_DECL_H_

/** Function Prototype Declaration */
int wlan_init_fw(wlan_private * priv);
int wlan_tx_packet(wlan_private * priv, struct sk_buff *skb);
void wlan_free_adapter(wlan_private * priv);

int SendNullPacket(wlan_private * priv, u8 flags);
BOOLEAN CheckLastPacketIndication(wlan_private * priv);

void Wep_encrypt(wlan_private * priv, u8 * Buf, u32 Len);
int FreeCmdBuffer(wlan_private * priv);
void CleanUpCmdCtrlNode(CmdCtrlNode * pTempNode);
CmdCtrlNode *GetFreeCmdCtrlNode(wlan_private * priv);

void SetCmdCtrlNode(wlan_private * priv,
                    CmdCtrlNode * pTempNode,
                    WLAN_OID cmd_oid, u16 wait_option, void *pdata_buf);

BOOLEAN Is_Command_Allowed(wlan_private * priv);

int PrepareAndSendCommand(wlan_private * priv,
                          u16 cmd_no,
                          u16 cmd_action,
                          u16 wait_option, WLAN_OID cmd_oid, void *pdata_buf);

void QueueCmd(wlan_adapter * Adapter, CmdCtrlNode * CmdNode, BOOLEAN addtail);

int SetDeepSleep(wlan_private * priv, BOOLEAN bDeepSleep);
int AllocateCmdBuffer(wlan_private * priv);
int ExecuteNextCommand(wlan_private * priv);
int wlan_process_event(wlan_private * priv);
void wlan_interrupt(struct net_device *);
u32 index_to_data_rate(u8 index);
u8 data_rate_to_index(u32 rate);
void HexDump(char *prompt, u8 * data, int len);
void get_version(wlan_adapter * adapter, char *version, int maxlen);
void wlan_read_write_rfreg(wlan_private * priv);

#ifdef CONFIG_PROC_FS
/** The proc fs interface */
void wlan_proc_entry(wlan_private * priv, struct net_device *dev);
void wlan_proc_remove(wlan_private * priv);
int string_to_number(char *s);
#ifdef PROC_DEBUG
void wlan_debug_entry(wlan_private * priv, struct net_device *dev);
void wlan_debug_remove(wlan_private * priv);
#endif /* PROC_DEBUG */
#endif /* CONFIG_PROC_FS */
int wlan_process_rx_command(wlan_private * priv);
void wlan_process_tx(wlan_private * priv);
void CleanupAndInsertCmd(wlan_private * priv, CmdCtrlNode * pTempCmd);
void MrvDrvCommandTimerFunction(void *FunctionContext);

#ifdef REASSOCIATION
void MrvDrvReassocTimerFunction(void *FunctionContext);
#endif /* REASSOCIATION */

int wlan_set_essid(struct net_device *dev, struct iw_request_info *info,
                   struct iw_point *dwrq, char *extra);
int wlan_set_regiontable(wlan_private * priv, u8 region, u8 band);

void wlan_clean_txrx(wlan_private * priv);

int wlan_host_sleep_activated_event(wlan_private * priv);
int wlan_host_sleep_deactivated_event(wlan_private * priv);
int wlan_host_sleep_gpio_int_event(wlan_private * priv);
int wlan_deep_sleep_ioctl(wlan_private * priv, struct ifreq *rq);

int ProcessRxedPacket(wlan_private * priv, struct sk_buff *);

void PSSleep(wlan_private * priv, int wait_option);
void PSConfirmSleep(wlan_private * priv, u16 PSMode);
void PSWakeup(wlan_private * priv, int wait_option);
//void sdio_clear_imask(mmc_controller_t);
//int sdio_check_idle_state(mmc_controller_t);
//void sdio_print_imask(mmc_controller_t);
//void sdio_clear_imask(mmc_controller_t);

int sd_set_clock(wlan_private * priv, u32 slotFreq);
int sd_start_clock(wlan_private * priv);
int sd_stop_clock(wlan_private * priv);
int sd_set_bus_width(wlan_private * priv, u8 width);
int sd_set_gpo(wlan_private * priv, u8 on);

void wlan_send_rxskbQ(wlan_private * priv);

extern CHANNEL_FREQ_POWER *find_cfp_by_band_and_channel(wlan_adapter *
                                                        adapter, u8 band,
                                                        u16 channel);

extern void MacEventDisconnected(wlan_private * priv);

#if WIRELESS_EXT > 14
void send_iwevcustom_event(wlan_private * priv, s8 * str);
#endif

#endif /* _WLAN_DECL_H_ */
