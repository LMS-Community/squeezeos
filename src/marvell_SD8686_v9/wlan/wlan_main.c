/** @file wlan_main.c
  *  
  * @brief This file contains the major functions in WLAN
  * driver. It includes init, exit, open, close and main
  * thread etc..
  * 
  * (c) Copyright � 2003-2007, Marvell International Ltd. 
  *   
  * This software file (the "File") is distributed by Marvell International 
  * Ltd. under the terms of the GNU General Public License Version 2, June 1991 
  * (the "License").  You may use, redistribute and/or modify this File in 
  * accordance with the terms and conditions of the License, a copy of which 
  * is available along with the File in the gpl.txt file or by writing to 
  * the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 
  * 02111-1307 or on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
  *
  * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE 
  * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE 
  * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about 
  * this warranty disclaimer.
  *
  */
/**
  * @mainpage M-WLAN Linux Driver
  *
  * @section overview_sec Overview
  *
  * The M-WLAN is a Linux reference driver for Marvell
  * 802.11 (a/b/g) WLAN chipset.
  * 
  * @section copyright_sec Copyright
  *
  * Copyright � Marvell International Ltd. and/or its affiliates, 2003-2007
  *
  */
/********************************************************
Change log:
    09/30/05: Add Doxygen format comments
    12/09/05: Add TX_QUEUE support  
    01/05/06: Add kernel 2.6.x support  
    01/11/06: Conditionalize new scan/join functions.
    01/12/06: Add TxLockFlag for UAPSD power save mode 
              and Proprietary Periodic sleep support
********************************************************/

#include    "include.h"

/********************************************************
        Local Variables
********************************************************/

spinlock_t driver_lock = SPIN_LOCK_UNLOCKED;
ulong driver_flags;

#define WLAN_TX_PWR_DEFAULT     20      /*100mW */
#define WLAN_TX_PWR_US_DEFAULT      20  /*100mW */
#define WLAN_TX_PWR_JP_DEFAULT      16  /*50mW */
#define WLAN_TX_PWR_FR_100MW        20  /*100mW */
#define WLAN_TX_PWR_EMEA_DEFAULT    20  /*100mW */

/* Format { Channel, Frequency (MHz), MaxTxPower } */
/* Band: 'B/G', Region: USA FCC/Canada IC */
static CHANNEL_FREQ_POWER channel_freq_power_US_BG[] = {
    {1, 2412, WLAN_TX_PWR_US_DEFAULT},
    {2, 2417, WLAN_TX_PWR_US_DEFAULT},
    {3, 2422, WLAN_TX_PWR_US_DEFAULT},
    {4, 2427, WLAN_TX_PWR_US_DEFAULT},
    {5, 2432, WLAN_TX_PWR_US_DEFAULT},
    {6, 2437, WLAN_TX_PWR_US_DEFAULT},
    {7, 2442, WLAN_TX_PWR_US_DEFAULT},
    {8, 2447, WLAN_TX_PWR_US_DEFAULT},
    {9, 2452, WLAN_TX_PWR_US_DEFAULT},
    {10, 2457, WLAN_TX_PWR_US_DEFAULT},
    {11, 2462, WLAN_TX_PWR_US_DEFAULT}
};

/* Band: 'B/G', Region: Europe ETSI */
static CHANNEL_FREQ_POWER channel_freq_power_EU_BG[] = {
    {1, 2412, WLAN_TX_PWR_EMEA_DEFAULT},
    {2, 2417, WLAN_TX_PWR_EMEA_DEFAULT},
    {3, 2422, WLAN_TX_PWR_EMEA_DEFAULT},
    {4, 2427, WLAN_TX_PWR_EMEA_DEFAULT},
    {5, 2432, WLAN_TX_PWR_EMEA_DEFAULT},
    {6, 2437, WLAN_TX_PWR_EMEA_DEFAULT},
    {7, 2442, WLAN_TX_PWR_EMEA_DEFAULT},
    {8, 2447, WLAN_TX_PWR_EMEA_DEFAULT},
    {9, 2452, WLAN_TX_PWR_EMEA_DEFAULT},
    {10, 2457, WLAN_TX_PWR_EMEA_DEFAULT},
    {11, 2462, WLAN_TX_PWR_EMEA_DEFAULT},
    {12, 2467, WLAN_TX_PWR_EMEA_DEFAULT},
    {13, 2472, WLAN_TX_PWR_EMEA_DEFAULT}
};

/* Band: 'B/G', Region: Spain */
static CHANNEL_FREQ_POWER channel_freq_power_SPN_BG[] = {
    {10, 2457, WLAN_TX_PWR_DEFAULT},
    {11, 2462, WLAN_TX_PWR_DEFAULT}
};

/* Band: 'B/G', Region: France */
static CHANNEL_FREQ_POWER channel_freq_power_FR_BG[] = {
    {10, 2457, WLAN_TX_PWR_FR_100MW},
    {11, 2462, WLAN_TX_PWR_FR_100MW},
    {12, 2467, WLAN_TX_PWR_FR_100MW},
    {13, 2472, WLAN_TX_PWR_FR_100MW}
};

/* Band: 'B/G', Region: Japan */
static CHANNEL_FREQ_POWER channel_freq_power_JPN41_BG[] = {
    {1, 2412, WLAN_TX_PWR_JP_DEFAULT},
    {2, 2417, WLAN_TX_PWR_JP_DEFAULT},
    {3, 2422, WLAN_TX_PWR_JP_DEFAULT},
    {4, 2427, WLAN_TX_PWR_JP_DEFAULT},
    {5, 2432, WLAN_TX_PWR_JP_DEFAULT},
    {6, 2437, WLAN_TX_PWR_JP_DEFAULT},
    {7, 2442, WLAN_TX_PWR_JP_DEFAULT},
    {8, 2447, WLAN_TX_PWR_JP_DEFAULT},
    {9, 2452, WLAN_TX_PWR_JP_DEFAULT},
    {10, 2457, WLAN_TX_PWR_JP_DEFAULT},
    {11, 2462, WLAN_TX_PWR_JP_DEFAULT},
    {12, 2467, WLAN_TX_PWR_JP_DEFAULT},
    {13, 2472, WLAN_TX_PWR_JP_DEFAULT}
};

/* Band: 'B/G', Region: Japan */
static CHANNEL_FREQ_POWER channel_freq_power_JPN40_BG[] = {
    {14, 2484, WLAN_TX_PWR_JP_DEFAULT}
};

/********************************************************
        Global Variables
********************************************************/

/* add by feng for thread conflict debug */
int threadrunning = 0;

/**
 * the structure for channel, frequency and power
 */
typedef struct _region_cfp_table
{
    u8 region;
    CHANNEL_FREQ_POWER *cfp_BG;
    int cfp_no_BG;
} region_cfp_table_t;

/**
 * the structure for the mapping between region and CFP
 */
region_cfp_table_t region_cfp_table[] = {
    {0x10,                      /*US FCC */
     channel_freq_power_US_BG,
     sizeof(channel_freq_power_US_BG) / sizeof(CHANNEL_FREQ_POWER),
     }
    ,
    {0x20,                      /*CANADA IC */
     channel_freq_power_US_BG,
     sizeof(channel_freq_power_US_BG) / sizeof(CHANNEL_FREQ_POWER),
     }
    ,
    {0x30, /*EU*/ channel_freq_power_EU_BG,
     sizeof(channel_freq_power_EU_BG) / sizeof(CHANNEL_FREQ_POWER),
     }
    ,
    {0x31, /*SPAIN*/ channel_freq_power_SPN_BG,
     sizeof(channel_freq_power_SPN_BG) / sizeof(CHANNEL_FREQ_POWER),
     }
    ,
    {0x32, /*FRANCE*/ channel_freq_power_FR_BG,
     sizeof(channel_freq_power_FR_BG) / sizeof(CHANNEL_FREQ_POWER),
     }
    ,
    {0x40, /*JAPAN*/ channel_freq_power_JPN40_BG,
     sizeof(channel_freq_power_JPN40_BG) / sizeof(CHANNEL_FREQ_POWER),
     }
    ,
    {0x41, /*JAPAN*/ channel_freq_power_JPN41_BG,
     sizeof(channel_freq_power_JPN41_BG) / sizeof(CHANNEL_FREQ_POWER),
     }
    ,
/*Add new region here */
};

/**
 * the rates supported by the card
 */
u8 WlanDataRates[WLAN_SUPPORTED_RATES] =
    { 0x02, 0x04, 0x0B, 0x16, 0x00, 0x0C, 0x12,
    0x18, 0x24, 0x30, 0x48, 0x60, 0x6C, 0x00
};

/**
 * the rates supported
 */
u8 SupportedRates[G_SUPPORTED_RATES] =
    { 0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24, 0x30, 0x48,
    0x60, 0x6c, 0
};

/**
 * the rates supported for ad-hoc G mode
 */
u8 AdhocRates_G[G_SUPPORTED_RATES] =
    { 0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24, 0x30, 0x48, 0x60, 0x6c,
0 };

/**
 * the rates supported for ad-hoc B mode
 */
u8 AdhocRates_B[4] = { 0x82, 0x84, 0x8b, 0x96 };

/**
 * the global variable of a pointer to wlan_private
 * structure variable
 */
wlan_private *wlanpriv = NULL;

u32 DSFreqList[15] = {
    0, 2412000, 2417000, 2422000, 2427000, 2432000, 2437000, 2442000,
    2447000, 2452000, 2457000, 2462000, 2467000, 2472000, 2484000
};

/**
 * the table to keep region code
 */
u16 RegionCodeToIndex[MRVDRV_MAX_REGION_CODE] =
    { 0x10, 0x20, 0x30, 0x31, 0x32, 0x40, 0x41 };

/********************************************************
        Local Functions
********************************************************/

/** 
 *  @brief This function opens the network device
 *  
 *  @param dev     A pointer to net_device structure
 *  @return        WLAN_STATUS_SUCCESS
 */
static int
wlan_open(struct net_device *dev)
{
    wlan_private *priv = (wlan_private *) dev->priv;
    wlan_adapter *adapter = priv->adapter;

    ENTER();

    // On some systems the device open handler will be called before HW ready.
    // Use the following flag check and wait function to work around the issue.
    {
        int i = 0;

        while ((adapter->HardwareStatus != WlanHardwareStatusReady) &&
               (i < MAX_WAIT_DEVICE_READY_COUNT)) {
            i++;
            os_sched_timeout(100);
        }
        if (i >= MAX_WAIT_DEVICE_READY_COUNT) {
            PRINTM(FATAL, "HW not ready, wlan_open() return failure\n");
            LEAVE();
            return WLAN_STATUS_FAILURE;
        }
    }

    MODULE_GET;

    priv->open = TRUE;

    if ((adapter->MediaConnectStatus == WlanMediaStateConnected)
        && (adapter->InfrastructureMode != Wlan802_11IBSS
            || adapter->AdhocLinkSensed == TRUE))
        os_carrier_on(priv);
    else
        os_carrier_off(priv);

    os_start_queue(priv);

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief This function closes the network device
 *  
 *  @param dev     A pointer to net_device structure
 *  @return        WLAN_STATUS_SUCCESS
 */
static int
wlan_close(struct net_device *dev)
{
    wlan_private *priv = dev->priv;

    ENTER();

    if (priv->adapter)
        wlan_clean_txrx(priv);

    MODULE_PUT;

    priv->open = FALSE;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief This function handles packet transmission
 *  
 *  @param skb     A pointer to sk_buff structure
 *  @param dev     A pointer to net_device structure
 *  @return        WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
wlan_hard_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
    int ret;
    wlan_private *priv = dev->priv;

    ret = WLAN_STATUS_SUCCESS;

    ENTER();

    PRINTM(DATA, "Data <= kernel\n");

    if (wlan_tx_packet(priv, skb)) {
        /* Transmit failed */
        ret = WLAN_STATUS_FAILURE;
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief This function handles the timeout of packet
 *  transmission
 *  
 *  @param dev     A pointer to net_device structure
 *  @return        n/a
 */
static void
wlan_tx_timeout(struct net_device *dev)
{
    wlan_private *priv = (wlan_private *) dev->priv;

    ENTER();

    PRINTM(DATA, "Tx timeout\n");
    UpdateTransStart(dev);

    priv->adapter->dbg.num_tx_timeout++;

    priv->adapter->IntCounter++;
    wake_up_interruptible(&priv->MainThread.waitQ);

    LEAVE();
}

/** 
 *  @brief This function returns the network statistics
 *  
 *  @param dev     A pointer to wlan_private structure
 *  @return        A pointer to net_device_stats structure
 */
static struct net_device_stats *
wlan_get_stats(struct net_device *dev)
{
    wlan_private *priv = (wlan_private *) dev->priv;

    return &priv->stats;
}

/** 
 *  @brief This function sets the MAC address to firmware.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param pRxPD   A pointer to RxPD structure of received packet
 *  @return        WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
wlan_set_mac_address(struct net_device *dev, void *addr)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_private *priv = (wlan_private *) dev->priv;
    wlan_adapter *Adapter = priv->adapter;
    struct sockaddr *pHwAddr = (struct sockaddr *) addr;

    ENTER();

    memset(Adapter->CurrentAddr, 0, MRVDRV_ETH_ADDR_LEN);

    /* dev->dev_addr is 8 bytes */
    HEXDUMP("dev->dev_addr:", dev->dev_addr, ETH_ALEN);

    HEXDUMP("addr:", pHwAddr->sa_data, ETH_ALEN);
    memcpy(Adapter->CurrentAddr, pHwAddr->sa_data, ETH_ALEN);

    ret = PrepareAndSendCommand(priv, HostCmd_CMD_802_11_MAC_ADDRESS,
                                HostCmd_ACT_SET,
                                HostCmd_OPTION_WAITFORRSP, 0, NULL);

    if (ret) {
        PRINTM(INFO, "set mac address failed.\n");
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    HEXDUMP("Adapter->MacAddr:", Adapter->CurrentAddr, ETH_ALEN);
    memcpy(dev->dev_addr, Adapter->CurrentAddr, ETH_ALEN);

  done:
    LEAVE();
    return ret;
}

/** 
 *  @brief This function sets multicast addresses to firmware
 *  
 *  @param dev     A pointer to net_device structure
 *  @return        n/a
 */
static void
wlan_set_multicast_list(struct net_device *dev)
{
    wlan_private *priv = dev->priv;
    wlan_adapter *Adapter = priv->adapter;
    int OldPacketFilter;

    ENTER();

    OldPacketFilter = Adapter->CurrentPacketFilter;

    if (dev->flags & IFF_PROMISC) {
        PRINTM(INFO, "Enable Promiscuous mode\n");
        Adapter->CurrentPacketFilter |= HostCmd_ACT_MAC_PROMISCUOUS_ENABLE;
        Adapter->CurrentPacketFilter &= ~HostCmd_ACT_MAC_ALL_MULTICAST_ENABLE;
    } else {
        /* Multicast */
        Adapter->CurrentPacketFilter &= ~HostCmd_ACT_MAC_PROMISCUOUS_ENABLE;

        if (dev->flags & IFF_ALLMULTI || dev->mc_count >
            MRVDRV_MAX_MULTICAST_LIST_SIZE) {
            PRINTM(INFO, "Enabling All Multicast!\n");
            Adapter->CurrentPacketFilter |=
                HostCmd_ACT_MAC_ALL_MULTICAST_ENABLE;
        } else {
            Adapter->CurrentPacketFilter &=
                ~HostCmd_ACT_MAC_ALL_MULTICAST_ENABLE;

            if (!dev->mc_count) {
                PRINTM(INFO, "No multicast addresses - "
                       "disabling multicast!\n");

            } else {
                int i;

                Adapter->NumOfMulticastMACAddr =
                    CopyMulticastAddrs(Adapter, dev);

                PRINTM(INFO, "Multicast addresses: %d\n", dev->mc_count);

                for (i = 0; i < dev->mc_count; i++) {
                    PRINTM(INFO, "Multicast address %d:"
                           "%x %x %x %x %x %x\n", i,
                           Adapter->MulticastList[i][0],
                           Adapter->MulticastList[i][1],
                           Adapter->MulticastList[i][2],
                           Adapter->MulticastList[i][3],
                           Adapter->MulticastList[i][4],
                           Adapter->MulticastList[i][5]);
                }
                /* set multicast addresses to firmware */
                PrepareAndSendCommand(priv, HostCmd_CMD_MAC_MULTICAST_ADR,
                                      HostCmd_ACT_GEN_SET, 0, 0, NULL);
            }
        }
    }

    if (Adapter->CurrentPacketFilter != OldPacketFilter) {
        PrepareAndSendCommand(priv,
                              HostCmd_CMD_MAC_CONTROL,
                              0, 0, 0, &Adapter->CurrentPacketFilter);
    }

    LEAVE();
}

/** 
 *  @brief This function pops rx_skb from the rx queue.
 *  
 *  @param RxSkbQ  A pointer to rx_skb queue
 *  @return        A pointer to skb
 */
static struct sk_buff *
wlan_pop_rx_skb(struct sk_buff *RxSkbQ)
{
    struct sk_buff *skb_data = NULL;

    if (!list_empty((struct list_head *) RxSkbQ)) {
        skb_data = RxSkbQ->next;
        list_del((struct list_head *) RxSkbQ->next);
    }

    return skb_data;
}

/** 
 *  @brief This function hanldes the major job in WLAN driver.
 *  it handles the event generated by firmware, rx data received
 *  from firmware and tx data sent from kernel.
 *  
 *  @param data    A pointer to wlan_thread structure
 *  @return        WLAN_STATUS_SUCCESS
 */
static int
wlan_service_main_thread(void *data)
{
    wlan_thread *thread = data;
    wlan_private *priv = thread->priv;
    wlan_adapter *Adapter = priv->adapter;
    wait_queue_t wait;
    u8 ireg = 0;

    OS_INTERRUPT_SAVE_AREA;

    ENTER();

    wlan_activate_thread(thread);

    init_waitqueue_entry(&wait, current);

    wmm_init(priv);

    for (;;) {
        add_wait_queue(&thread->waitQ, &wait);
        OS_SET_THREAD_STATE(TASK_INTERRUPTIBLE);

        TX_DISABLE;

        if ((Adapter->WakeupTries) ||
            (Adapter->PSState == PS_STATE_SLEEP
             && !Adapter->bWakeupDevRequired) ||
            (!Adapter->IntCounter &&
             Adapter->PSState == PS_STATE_PRE_SLEEP) ||
            (!Adapter->IntCounter
             && (priv->wlan_dev.dnld_sent || Adapter->TxLockFlag
                 || wmm_lists_empty(priv) || wmm_is_queue_stopped(priv))
             && (priv->wlan_dev.dnld_sent || !Adapter->CurrentTxSkb)
             && (priv->wlan_dev.dnld_sent || Adapter->CurCmd ||
                 list_empty(&Adapter->CmdPendingQ))
            )
            ) {
            PRINTM(INFO, "main-thread sleeping... "
                   "HS_Act=%s WakeupReq=%s Conn=%s PS_Mode=%d PS_State=%d\n",
                   (Adapter->HS_Activated) ? "Y" : "N",
                   (Adapter->bWakeupDevRequired) ? "Y" : "N",
                   (Adapter->MediaConnectStatus) ? "Y" : "N",
                   Adapter->PSMode, Adapter->PSState);

            TX_RESTORE;

		/* add by feng for thread debug */
	    threadrunning = 0;

            schedule();
        		/* add by feng for thread debug */
	    threadrunning = 1; 
	       
            PRINTM(INFO, "main-thread waking up: IntCnt=%d "
                   "CurCmd=%s CmdPending=%s\n"
                   "                       Connect=%s "
                   "CurTxSkb=%s dnld_sent=%d\n",
                   Adapter->IntCounter,
                   (Adapter->CurCmd) ? "Y" : "N",
                   list_empty(&Adapter->CmdPendingQ) ? "N" : "Y",
                   (Adapter->MediaConnectStatus) ? "Y" : "N",
                   (Adapter->CurrentTxSkb) ? "Y" : "N",
                   priv->wlan_dev.dnld_sent);
        } else {
            TX_RESTORE;
        }

        OS_SET_THREAD_STATE(TASK_RUNNING);
        remove_wait_queue(&thread->waitQ, &wait);

        if (kthread_should_stop() || Adapter->SurpriseRemoved) {
            PRINTM(INFO, "main-thread: break from main thread: "
                   "SurpriseRemoved=0x%x\n", Adapter->SurpriseRemoved);
            break;
        }

        if (Adapter->IntCounter) {
            OS_INT_DISABLE;
            Adapter->IntCounter = 0;
            OS_INT_RESTORE;

            if (sbi_get_int_status(priv, &ireg)) {
                PRINTM(ERROR,
                       "main-thread: reading HOST_INT_STATUS_REG failed\n");
                continue;
            }
            OS_INT_DISABLE;
            Adapter->HisRegCpy |= ireg;
            OS_INT_RESTORE;
            PRINTM(INTR, "INT: status = 0x%x\n", Adapter->HisRegCpy);
        } else if (Adapter->bWakeupDevRequired
                   && (Adapter->HS_Activated || (Adapter->IsDeepSleep)
                   )
            ) {
            Adapter->WakeupTries++;
            PRINTM(CMND,
                   "Wakeup device... WakeupReq=%s Conn=%s PS_Mode=%d PS_State=%d\n",
                   (Adapter->bWakeupDevRequired) ? "Y" : "N",
                   (priv->adapter->MediaConnectStatus) ? "Y" : "N",
                   priv->adapter->PSMode, priv->adapter->PSState);

            /* Wake up device */
            if (sbi_exit_deep_sleep(priv))
                PRINTM(MSG, "main-thread: wakeup dev failed\n");
            continue;
        }

        /* Command response? */
        if (Adapter->HisRegCpy & HIS_CmdUpLdRdy) {
            OS_INT_DISABLE;
            Adapter->HisRegCpy &= ~HIS_CmdUpLdRdy;
            OS_INT_RESTORE;

            wlan_process_rx_command(priv);
        }

        /* Any received data? */
        if (Adapter->HisRegCpy & HIS_RxUpLdRdy) {
            OS_INT_DISABLE;
            Adapter->HisRegCpy &= ~HIS_RxUpLdRdy;
            OS_INT_RESTORE;

            wlan_send_rxskbQ(priv);
        }

        /* Any Card Event */
        if (Adapter->HisRegCpy & HIS_CardEvent) {
            OS_INT_DISABLE;
            Adapter->HisRegCpy &= ~HIS_CardEvent;
            OS_INT_RESTORE;

            if (sbi_read_event_cause(priv)) {
                PRINTM(ERROR, "main-thread: sbi_read_event_cause failed.\n");
                continue;
            }
            wlan_process_event(priv);
        }

        /* Check if we need to confirm Sleep Request received previously */
        if (Adapter->PSState == PS_STATE_PRE_SLEEP) {
            if (!priv->wlan_dev.dnld_sent && !Adapter->CurCmd) {
                ASSERT(Adapter->MediaConnectStatus ==
                       WlanMediaStateConnected);
                PSConfirmSleep(priv, (u16) Adapter->PSMode);
            }
        }

        /* The PS state is changed during processing of 
         * Sleep Request event above 
         */
        if ((Adapter->PSState == PS_STATE_SLEEP)
            || (Adapter->PSState == PS_STATE_PRE_SLEEP)) {
            continue;
        }

        if (Adapter->IsDeepSleep)
            continue;

        /* The HS_Activated flag is changed during processing of 
           HS_Activate command resp */
        /* We cannot send command or data if HS_Activated and 
           WakeupDevRequired are TRUE */
        if (Adapter->HS_Activated && Adapter->bWakeupDevRequired) {
            PRINTM(INFO, "main-thread: cannot send command or data, "
                   "HS_Activated=%d\n", Adapter->HS_Activated);
            continue;
        }

        /* Execute the next command */
        if (!priv->wlan_dev.dnld_sent && !Adapter->CurCmd) {
            ExecuteNextCommand(priv);
        }

        if (!priv->wlan_dev.dnld_sent
            && !wmm_lists_empty(priv) && !wmm_is_queue_stopped(priv)) {
            if ((Adapter->PSState == PS_STATE_FULL_POWER)
                || (Adapter->sleep_period.period == 0)
                || (Adapter->TxLockFlag == FALSE)) {
                wmm_process_tx(priv);
            }
        }

    }

    wlan_deactivate_thread(thread);

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/**
 * @brief This function adds the card. it will probe the
 * card, allocate the wlan_priv and initialize the device. 
 *  
 *  @param card    A pointer to card
 *  @return        A pointer to wlan_private structure
 */
static wlan_private *
wlan_add_card(void *card)
{
    struct net_device *dev = NULL;
    wlan_private *priv = NULL;

    ENTER();

    /* probe the card */
    if (sbi_probe_card(card) < 0) {
        PRINTM(MSG, "NO card found!\n");
        return NULL;
    }

    /* Allocate an Ethernet device and register it */
    if (!(dev = alloc_netdev(sizeof(wlan_private), "wlan%d", ether_setup))) {
        PRINTM(MSG, "Init ethernet device failed!\n");
        return NULL;
    }

    priv = dev->priv;

    /* allocate buffer for wlan_adapter */
    if (!(priv->adapter = kmalloc(sizeof(wlan_adapter), GFP_KERNEL))) {
        PRINTM(MSG, "Allocate buffer for wlan_adapter failed!\n");
        goto err_kmalloc;
    }

    /* init wlan_adapter */
    memset(priv->adapter, 0, sizeof(wlan_adapter));

    priv->wlan_dev.netdev = dev;
    priv->wlan_dev.card = card;
    wlanpriv = priv;

    //SET_MODULE_OWNER(dev);

    /* Setup the OS Interface to our functions */
    dev->open = wlan_open;
    dev->hard_start_xmit = wlan_hard_start_xmit;
    dev->stop = wlan_close;
    dev->do_ioctl = wlan_do_ioctl;
    dev->set_mac_address = wlan_set_mac_address;

    dev->tx_timeout = wlan_tx_timeout;
    dev->get_stats = wlan_get_stats;
    dev->watchdog_timeo = MRVDRV_DEFAULT_WATCHDOG_TIMEOUT;

#ifdef  WIRELESS_EXT
    //hufh dev->get_wireless_stats = wlan_get_wireless_stats;
    dev->wireless_handlers = (struct iw_handler_def *) &wlan_handler_def;
#endif
#define NETIF_F_DYNALLOC 16
    dev->features |= NETIF_F_DYNALLOC;
    dev->flags |= IFF_BROADCAST | IFF_MULTICAST;
    dev->set_multicast_list = wlan_set_multicast_list;

    init_waitqueue_head(&priv->adapter->ds_awake_q);

    INIT_LIST_HEAD(&priv->adapter->CmdFreeQ);
    INIT_LIST_HEAD(&priv->adapter->CmdPendingQ);

    PRINTM(INFO, "Starting kthread...\n");
    priv->MainThread.priv = priv;
    wlan_create_thread(wlan_service_main_thread,
                       &priv->MainThread, "wlan_main_service");

    ConfigureThreadPriority();

#ifdef REASSOCIATION
    priv->ReassocThread.priv = priv;
    wlan_create_thread(wlan_reassociation_thread,
                       &priv->ReassocThread, "wlan_reassoc_service");
#endif /* REASSOCIATION */

    /*
     * Register the device. Fillup the private data structure with
     * relevant information from the card and request for the required
     * IRQ. 
     */

    if (sbi_register_dev(priv) < 0) {
        PRINTM(FATAL, "Failed to register wlan device!\n");
        goto err_registerdev;
    }

    SET_NETDEV_DEV(dev, priv->hotplug_device);

    PRINTM(WARN, "%s: Marvell Wlan 802.11 Adapter "
           "revision 0x%02X at IRQ %i\n", dev->name,
           priv->adapter->chip_rev, dev->irq);

#ifdef CONFIG_PROC_FS
    wlan_proc_entry(priv, dev);
#ifdef PROC_DEBUG
    wlan_debug_entry(priv, dev);
#endif
#endif /* CPNFIG_PROC_FS */

    /* Get the CIS Table */
    sbi_get_cis_info(priv);

    /* init FW and HW */
    if (wlan_init_fw(priv)) {
        PRINTM(FATAL, "Firmware Init Failed\n");
        goto err_init_fw;
    }

    if (register_netdev(dev)) {
        printk(KERN_ERR "Cannot register network device!\n");
        goto err_init_fw;
    }

    LEAVE();
    return priv;

  err_init_fw:
    sbi_unregister_dev(priv);

#ifdef CONFIG_PROC_FS
#ifdef PROC_DEBUG
    wlan_debug_remove(priv);
#endif
    wlan_proc_remove(priv);
#endif

  err_registerdev:
    /* Stop the thread servicing the interrupts */
    wake_up_interruptible(&priv->MainThread.waitQ);
    wlan_terminate_thread(&priv->MainThread);

#ifdef REASSOCIATION
    wake_up_interruptible(&priv->ReassocThread.waitQ);
    wlan_terminate_thread(&priv->ReassocThread);
#endif /* REASSOCIATION */

  err_kmalloc:
	// modified by Feng
	free_netdev(dev);
    wlan_free_adapter(priv);
    wlanpriv = NULL;

    LEAVE();
    return NULL;
}

/** 
 *  @brief This function removes the card.
 *  
 *  @param priv    A pointer to card
 *  @return        WLAN_STATUS_SUCCESS
 */
static int
wlan_remove_card(void *card)
{
    wlan_private *priv = wlanpriv;
    wlan_adapter *Adapter;
    struct net_device *dev;
    union iwreq_data wrqu;

    ENTER();

    if (!priv) {
        LEAVE();
        return WLAN_STATUS_SUCCESS;
    }

    Adapter = priv->adapter;

    if (!Adapter) {
        LEAVE();
        return WLAN_STATUS_SUCCESS;
    }

    dev = priv->wlan_dev.netdev;

    wake_up_interruptible(&Adapter->ds_awake_q);

    if (Adapter->CurCmd) {
        PRINTM(INFO, "Wake up current cmdwait_q\n");
        wake_up_interruptible(&Adapter->CurCmd->cmdwait_q);
    }

    Adapter->CurCmd = NULL;

    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        wlan_clean_txrx(priv);
        Adapter->MediaConnectStatus = WlanMediaStateDisconnected;
    }

    if (Adapter->PSMode == Wlan802_11PowerModeMAX_PSP) {
        Adapter->PSMode = Wlan802_11PowerModeCAM;
        PSWakeup(priv, HostCmd_OPTION_WAITFORRSP);
    }
    if (Adapter->IsDeepSleep == TRUE) {
        sbi_exit_deep_sleep(priv);
        if (Adapter->IsDeepSleep == TRUE) {
            if (os_wait_interruptible_timeout(Adapter->ds_awake_q,
                                              !Adapter->IsDeepSleep,
                                              MRVDRV_DEEP_SLEEP_EXIT_TIMEOUT)
                == 0) {
                PRINTM(MSG, "ds_awake_q: timer expired\n");
            }
        }
    }

    memset(wrqu.ap_addr.sa_data, 0xaa, ETH_ALEN);
    wrqu.ap_addr.sa_family = ARPHRD_ETHER;
    wireless_send_event(priv->wlan_dev.netdev, SIOCGIWAP, &wrqu, NULL);

    /* Disable interrupts on the card as we cannot handle them after RESET */
    sbi_disable_host_int(priv);

    PrepareAndSendCommand(priv, HostCmd_CMD_802_11_RESET, 0, 0, 0, NULL);

    os_sched_timeout(200);

    Adapter->SurpriseRemoved = TRUE;

    /* Stop the thread servicing the interrupts */
    wake_up_interruptible(&priv->MainThread.waitQ);

#ifdef REASSOCIATION
    wake_up_interruptible(&priv->ReassocThread.waitQ);
#endif /* REASSOCIATION */

#ifdef CONFIG_PROC_FS
#ifdef PROC_DEBUG
    wlan_debug_remove(priv);
#endif
    wlan_proc_remove(priv);
#endif

    PRINTM(INFO, "unregester dev\n");
    sbi_unregister_dev(priv);

    PRINTM(INFO, "Free Adapter\n");
    wlan_free_adapter(priv);

    /* Last reference is our one */
    PRINTM(INFO, "refcnt = %d\n", atomic_read(&dev->refcnt));

    PRINTM(INFO, "netdev_finish_unregister: %s%s.\n", dev->name,
           (dev->features & NETIF_F_DYNALLOC) ? "" : ", old style");

    unregister_netdev(dev);

    PRINTM(INFO, "Unregister finish\n");

    priv->wlan_dev.netdev = NULL;
    free_netdev(dev);
    wlanpriv = NULL;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/********************************************************
        Global Functions
********************************************************/

/**
 * @brief This function sends the rx packets to the os from the skb queue
 *
 * @param priv  A pointer to wlan_private structure
 * @return  n/a
 */
void
wlan_send_rxskbQ(wlan_private * priv)
{
    struct sk_buff *skb;

    ENTER();
    if (priv->adapter) {
        while ((skb = wlan_pop_rx_skb(&priv->adapter->RxSkbQ))) {
            if (ProcessRxedPacket(priv, skb) == -ENOMEM)
                break;
        }
    }
    LEAVE();
}

/** 
 *  @brief This function finds the CFP in 
 *  region_cfp_table based on region and band parameter.
 *  
 *  @param region  The region code
 *  @param band    The band
 *  @param cfp_no  A pointer to CFP number
 *  @return        A pointer to CFP
 */
CHANNEL_FREQ_POWER *
wlan_get_region_cfp_table(u8 region, u8 band, int *cfp_no)
{
    int i;

    ENTER();

    for (i = 0; i < sizeof(region_cfp_table) / sizeof(region_cfp_table_t);
         i++) {
        PRINTM(INFO, "region_cfp_table[i].region=%d\n",
               region_cfp_table[i].region);
        if (region_cfp_table[i].region == region) {
            {
                *cfp_no = region_cfp_table[i].cfp_no_BG;
                LEAVE();
                return region_cfp_table[i].cfp_BG;
            }
        }
    }

    LEAVE();
    return NULL;
}

/** 
 *  @brief This function sets region table. 
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param region  The region code
 *  @param band    The band
 *  @return        WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_set_regiontable(wlan_private * priv, u8 region, u8 band)
{
    wlan_adapter *Adapter = priv->adapter;
    int i = 0;

    CHANNEL_FREQ_POWER *cfp;
    int cfp_no;

    ENTER();

    memset(Adapter->region_channel, 0, sizeof(Adapter->region_channel));

    {
        cfp = wlan_get_region_cfp_table(region, band, &cfp_no);
        if (cfp != NULL) {
            Adapter->region_channel[i].NrCFP = cfp_no;
            Adapter->region_channel[i].CFP = cfp;
        } else {
            PRINTM(INFO, "wrong region code %#x in Band B-G\n", region);
            return WLAN_STATUS_FAILURE;
        }
        Adapter->region_channel[i].Valid = TRUE;
        Adapter->region_channel[i].Region = region;
        Adapter->region_channel[i].Band = band;
        i++;
    }
    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief This function handles the interrupt. it will change PS
 *  state if applicable. it will wake up main_thread to handle
 *  the interrupt event as well.
 *  
 *  @param dev     A pointer to net_device structure
 *  @return        n/a
 */
void
wlan_interrupt(struct net_device *dev)
{
    wlan_private *priv = dev->priv;

    priv->adapter->IntCounter++;

    PRINTM(INTR, "*\n");

    priv->adapter->WakeupTries = 0;
    if (priv->adapter->bWakeupDevRequired)
        sd_set_clock(priv, 0);  /* default clock frequency */

    if (priv->adapter->PSState == PS_STATE_SLEEP) {
        priv->adapter->PSState = PS_STATE_AWAKE;
    }

    wake_up_interruptible(&priv->MainThread.waitQ);
}

/** 
 *  @brief This function initializes module.
 *  
 *  @param     n/a    A pointer to wlan_private structure
 *  @return        WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_init_module(void)
{
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (sbi_register(wlan_add_card, wlan_remove_card, NULL) == NULL) {
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

  done:
    LEAVE();
    return ret;
}

/** 
 *  @brief This function cleans module
 *  
 *  @param priv    n/a
 *  @return        n/a
 */
void
wlan_cleanup_module(void)
{
    ENTER();

    sbi_unregister();

    LEAVE();
}

module_init(wlan_init_module);
module_exit(wlan_cleanup_module);

MODULE_DESCRIPTION("M-WLAN Driver");
MODULE_AUTHOR("Marvell International Ltd.");
MODULE_LICENSE("GPL");
