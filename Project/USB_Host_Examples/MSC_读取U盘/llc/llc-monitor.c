/**
 * @addtogroup cohda_llc LLC module
 * @{
 *
 * @addtogroup cohda_llc_intern LLC internals
 * @{
 *
 * @addtogroup cohda_llc_monitor LLC monitoring interfaces
 * @{
 *
 * @file
 * LLC: monitoring interfaces
 */

//------------------------------------------------------------------------------
// Copyright (c) 2013 Cohda Wireless Pty Ltd
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------

#include <linux/version.h>
#include <linux/if_arp.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <net/sock.h> // for gfp_any()

#include "cohda/llc/llc.h"
#include "llc-monitor.h"
#include "llc-device.h"
#include "llc-internal.h"

#define D_SUBMODULE LLC_Monitor
#include "debug-levels.h"

//-----------------------------------------------------------------------------
// Local Macros & Constants
//-----------------------------------------------------------------------------
#define LLC_MONITOR_HEADER_LEN  (sizeof(struct Radiotap))
#define LLC_MONITOR_MTU         (2500)
#define LLC_MONITOR_RX_PROTO    (0x04D0)
#define LLC_MONITOR_TX_PROTO    (0x04D1)

//-----------------------------------------------------------------------------
// Local Type definitions
//-----------------------------------------------------------------------------

/// Per netdev private data
typedef struct LLCMonPriv
{
  /// Parent pointer
  struct LLCDev *pDev;
  /// Monitor type: TX or RX
  tLLCMonType Type;
  /// Indicate the radio that should be used (Radio A or Radio B)
  tMKxRadio Radio;
  /// Indicate the channel config for the selected radio
  tMKxChannel Channel;
} tLLCMonPriv;

/// Values for a radiotap header's it_present
enum {
  RADIOTAP_PRESENT_FLAGS   = (1 << 1),
  RADIOTAP_PRESENT_RATE    = (1 << 2),
  RADIOTAP_PRESENT_VENDOR  = (1 << 30),
  RADIOTAP_PRESENT         = (RADIOTAP_PRESENT_FLAGS |
                              RADIOTAP_PRESENT_VENDOR),
} eRadiotapPresent;

/// The radiotap header srtucture
typedef struct Radiotap
{
  /// Set to 0
  uint8_t     Version;
  uint8_t     Padding;
  /// Entire radiotap header length
  int16_t    Len;
  /// Bitfield of values present (RADIOTAP_PRESENT)
  uint32_t    Present;
  /// The typical radiotap headers
  struct {
    /// FCS
    uint8_t   Flags;
	/// Rate
    uint8_t   Rate;
  } Radiotap;
  /// Cohda's metadata
  struct {
    /// 0x04 0xe5 0x48
    uint8_t   OUI[3];
    /// == SkipLen
    uint8_t   SubNamespace;
    /// Length of Meta[]
    uint16_t  SkipLen;
	/// Rx or Tx meta data
    uint8_t   Meta[0];
  } Vendor;
} __attribute__((__packed__)) tRadiotap;


//-----------------------------------------------------------------------------
// Global variable definitions
//-----------------------------------------------------------------------------
static struct Radiotap _Radiotap =
{
  .Version = 0x00,
  .Padding = 0x00,
  .Len = 0x0000,
  .Present = RADIOTAP_PRESENT,
  .Radiotap.Flags = 0x00, // FCS not present
  .Radiotap.Rate = 0x00,
  .Vendor.OUI = { 0x04, 0xe5, 0x48 },
  .Vendor.SubNamespace = 0x00,
  .Vendor.SkipLen = 0x0000,
};


//-----------------------------------------------------------------------------
// Local (static) Function Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// LLC monitor netdevice interface related functionality
//-----------------------------------------------------------------------------

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30))
#include <linux/etherdevice.h>
#else
static int eth_mac_addr (struct net_device *pNetDev, void *pAddr)
{
  struct sockaddr *pSockAddr = pAddr;

  if (netif_running(pNetDev))
    return -EBUSY;
  if (!is_valid_ether_addr(pSockAddr->sa_data))
    return -EADDRNOTAVAIL;
  memcpy(pNetDev->dev_addr, pSockAddr->sa_data, ETH_ALEN);
  return 0;
}

static int eth_change_mtu (struct net_device *pNetDev, int NewMTU)
{
  if (NewMTU < 68 || NewMTU > ETH_DATA_LEN)
    return -EINVAL;
  pNetDev->mtu = NewMTU;
  return 0;
}

static int eth_validate_addr (struct net_device *pNetDev)
{
  if (!is_valid_ether_addr(pNetDev->dev_addr))
    return -EADDRNOTAVAIL;

  return 0;
}
#endif // (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30))

static int LLC_MonitorOpen (struct net_device *pNetDev)
{
  int Res = -ENOSYS;
  struct LLCMonPriv *pLLCMonPriv = NULL;
  struct LLCDev *pDev = NULL;

  d_fnstart(D_TST, NULL, "(pNetDev %p)\n", pNetDev);
  d_assert(pNetDev != NULL);

  // Lazy asignement of the pDev parent pointer
  pDev = LLC_DeviceGet();
  if (pDev == NULL)
  {
    Res = -ENODEV;
    goto Error;
  }
  pLLCMonPriv = netdev_priv(pNetDev);
  pLLCMonPriv->pDev = pDev;

  netif_start_queue(pNetDev);
  Res = 0;

Error:
  d_fnend(D_TST, NULL, "(pNetDev %p) = %d\n", pNetDev, Res);
  return Res;
}

static int LLC_MonitorClose (struct net_device *pNetDev)
{
  int Res = -ENOSYS;

  d_fnstart(D_TST, NULL, "(pNetDev %p)\n", pNetDev);

  netif_stop_queue(pNetDev);
  Res = 0;

  d_fnend(D_TST, NULL, "(pNetDev %p) = 0\n", pNetDev);
  return Res;
}

static int LLC_MonitorConfig (struct net_device *pNetDev, struct ifmap *pMap)
{
  int Res = -ENOSYS;

  d_fnstart(D_TST, NULL, "(pNetDev %p, pMap %p)\n", pNetDev, pMap);

  if (pNetDev->flags & IFF_UP)
  {
    return -EBUSY;
  }

  if (pMap->base_addr != pNetDev->base_addr)
  {
    return -EOPNOTSUPP;
  }

  d_fnend(D_TST, NULL, "(pNetDev %p, pMap %p) = %d\n", pNetDev, pMap, Res);
  return Res;
}

static void LLC_MonitorTimeout (struct net_device *pNetDev)
{
  d_fnstart(D_TST, NULL, "(pNetDev %p)\n", pNetDev);

  netif_wake_queue(pNetDev);

  d_fnend(D_TST, NULL, "(pNetDev %p) = void\n", pNetDev);
  return;
}

static int LLC_MonitorIoctl (struct net_device *pNetDev,
                             struct ifreq *pReq,
                             int Cmd)
{
  int Res = -ENOSYS;

  d_fnstart(D_TST, NULL, "(pNetDev %p, pReq %p, Cmd %x)\n", pNetDev, pReq, Cmd);

  // Cmd must be in the the 'SIOCDEVPRIVATE' range
  if ((Cmd >= SIOCDEVPRIVATE) && (Cmd <= SIOCDEVPRIVATE + 15))
  {
    // TODO: Nothing comes to mind...
    Res = 0;
  }
  else
  {
    Res = 0;
  }

  d_fnend(D_TST, NULL, "(pNetDev %p, pReq %p, Cmd %x) = %d\n", pNetDev, pReq, Cmd,
          Res);
  return Res;
}

/**
 * @brief Inject a packet (TX or RX) into the LLC
 * @param pTxSkb the packet to inject
 * @param pNetDev The network device
 * @return
 */
static int LLC_MonitorTransmit (struct sk_buff *pSkb,
                                struct net_device *pNetDev)
{
  struct LLCMonPriv *pLLCMonPriv = NULL;
  int Res = -ENOSYS;

  d_fnstart(D_TST, NULL, "(pSkb %p pNetDev %p)\n", pSkb, pNetDev);
  d_assert(pNetDev != NULL);
  if (pSkb == NULL)
    goto Error;

  pLLCMonPriv = netdev_priv(pNetDev);
  if ((pLLCMonPriv->Radio > MKX_RADIO_MAX) ||
      (pLLCMonPriv->Channel > MKX_CHANNEL_MAX))
    goto Error;

  if (pLLCMonPriv->Type == LLC_MONTYPE_TX)
  {
    Res = LLC_MsgTxReq(pLLCMonPriv->pDev,
                       (tMKxTxPacket *)(pSkb->data),
                       NULL);
    if (Res != 0)
      d_printf(D_WARN, NULL, "Inject TX = %d", Res);
  }
  else if (pLLCMonPriv->Type == LLC_MONTYPE_RX)
  {
    Res = LLC_RxInd(&(pLLCMonPriv->pDev->MKx),
        (tMKxRxPacket *)(pSkb->data),
                    NULL);
    if (Res != 0)
      d_printf(D_WARN, NULL, "Inject RX = %d", Res);
  }

Error:
  kfree_skb(pSkb);

  d_fnend(D_TST, NULL, "(pSkb %p pNetDev %p) = NETDEV_TX_OK\n", pSkb, pNetDev);
  return NETDEV_TX_OK;
}

/**
 * @brief Deliver the buffer to the Linux networking stack
 * @param pNetDev
 * @param pRxSkb
 */
static void LLC_MonitorReceive (struct sk_buff *pRxSkb,
                                struct net_device *pNetDev,
                                uint16_t Proto)
{
  struct sk_buff *pSkb = NULL;

  d_fnstart(D_TST, NULL, "(pRxSkb %p pNetDev %p)\n", pRxSkb, pNetDev);

  if (pNetDev == NULL)
    goto Error;

  pNetDev->last_rx = jiffies;///* Time of last Rx
  pNetDev->stats.rx_packets++;
  pNetDev->stats.rx_bytes += pRxSkb->len;

  if (netif_running(pNetDev) == 0)
    goto Error;

  pSkb = skb_clone(pRxSkb, gfp_any());
  if (pSkb == NULL)
    goto Error;

  skb_orphan(pSkb);
  pSkb->dev       = pNetDev;
  pSkb->pkt_type  = PACKET_BROADCAST;
  pSkb->protocol  = htons(Proto);
  pSkb->ip_summed = CHECKSUM_UNNECESSARY;
  skb_reset_mac_header(pSkb);

  d_printf(D_DEBUG, NULL, "pSkb: data %p len %d\n", pSkb->data, pSkb->len);
  if (in_atomic())
    netif_rx(pSkb);//网卡驱动通过调用netif_rx函数，把数据送入到协议栈中。
  else
    netif_rx_ni(pSkb);

Error:
  d_fnend(D_TST, NULL, "(pRxSkb %p pNetDev %p) = void\n", pRxSkb, pNetDev);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30))
static const struct net_device_ops LLCNetDevOps =
{
  .ndo_open            = LLC_MonitorOpen,
  .ndo_stop            = LLC_MonitorClose,
  .ndo_set_config      = LLC_MonitorConfig,
  .ndo_start_xmit      = LLC_MonitorTransmit,
  .ndo_do_ioctl        = LLC_MonitorIoctl,
  .ndo_tx_timeout      = LLC_MonitorTimeout,
  .ndo_change_mtu      = eth_change_mtu,
  .ndo_set_mac_address = eth_mac_addr,
  .ndo_validate_addr   = eth_validate_addr,
};
#endif // (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30))

static void LLC_MonitorCreate (struct net_device *pNetDev,
                               tLLCMonType Type,
                               tMKxRadio Radio,
                               tMKxChannel Channel)
{

  struct LLCMonPriv *pLLCMonPriv = NULL;

  d_fnstart(D_TST, NULL, "(pNetDev %p)\n", pNetDev);
  d_assert(pNetDev != NULL);

  pNetDev->destructor = NULL;
  ether_setup(pNetDev);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30))
  pNetDev->netdev_ops = &(LLCNetDevOps);
#else
  pNetDev->open            = LLC_MonitorOpen;
  pNetDev->stop            = LLC_MonitorClose;
  pNetDev->set_config      = LLC_MonitorConfig;
  pNetDev->hard_start_xmit = LLC_MonitorTransmit;
  pNetDev->do_ioctl        = LLC_MonitorIoctl;
  pNetDev->tx_timeout      = LLC_MonitorTimeout;
  pNetDev->change_mtu      = eth_change_mtu;
  pNetDev->set_mac_address = eth_mac_addr;
  pNetDev->validate_addr   = eth_validate_addr;
#endif // (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30))
  pNetDev->tx_queue_len    = 0;
  pNetDev->hard_header_len = LLC_MONITOR_HEADER_LEN;
  pNetDev->flags           = IFF_NOARP;
  if (LLC_GetMonitorFormat() == LLC_MON_FORMAT_80211)
    pNetDev->type          = ARPHRD_IEEE80211;
  else if (LLC_GetMonitorFormat() == LLC_MON_FORMAT_RADIOTAP)
    pNetDev->type          = ARPHRD_IEEE80211_RADIOTAP;
  else
    pNetDev->type          = ARPHRD_VOID;
  random_ether_addr(pNetDev->dev_addr);
  pNetDev->addr_len        = ETH_ALEN;
  pNetDev->mtu             = LLC_MONITOR_MTU;

  pLLCMonPriv = netdev_priv(pNetDev);
  pLLCMonPriv->Type    = Type;
  pLLCMonPriv->Radio   = Radio;
  pLLCMonPriv->Channel = Channel;

  d_fnend(D_TST, NULL, "(pNetDev %p) = void\n", pNetDev);
}

static void LLC_MonitorCreateTx (struct net_device *pNetDev)
{
  LLC_MonitorCreate(pNetDev, LLC_MONTYPE_TX, MKX_RADIO_COUNT, MKX_CHANNEL_COUNT);
}

static void LLC_MonitorCreateRx (struct net_device *pNetDev)
{
  LLC_MonitorCreate(pNetDev, LLC_MONTYPE_RX, MKX_RADIO_COUNT, MKX_CHANNEL_COUNT);
}

static void LLC_MonitorCreateTxA (struct net_device *pNetDev)
{
  LLC_MonitorCreate(pNetDev, LLC_MONTYPE_TX, MKX_RADIO_A, MKX_CHANNEL_COUNT);
}

static void LLC_MonitorCreateRxA (struct net_device *pNetDev)
{
  LLC_MonitorCreate(pNetDev, LLC_MONTYPE_RX, MKX_RADIO_A, MKX_CHANNEL_COUNT);
}

static void LLC_MonitorCreateTxA0 (struct net_device *pNetDev)
{
  LLC_MonitorCreate(pNetDev, LLC_MONTYPE_TX, MKX_RADIO_A, MKX_CHANNEL_0);
}

static void LLC_MonitorCreateRxA0 (struct net_device *pNetDev)
{
  LLC_MonitorCreate(pNetDev, LLC_MONTYPE_RX, MKX_RADIO_A, MKX_CHANNEL_0);
}

static void LLC_MonitorCreateTxA1 (struct net_device *pNetDev)
{
  LLC_MonitorCreate(pNetDev, LLC_MONTYPE_TX, MKX_RADIO_A, MKX_CHANNEL_1);
}

static void LLC_MonitorCreateRxA1 (struct net_device *pNetDev)
{
  LLC_MonitorCreate(pNetDev, LLC_MONTYPE_RX, MKX_RADIO_A, MKX_CHANNEL_1);
}

static void LLC_MonitorCreateTxB (struct net_device *pNetDev)
{
  LLC_MonitorCreate(pNetDev, LLC_MONTYPE_TX, MKX_RADIO_B, MKX_CHANNEL_COUNT);
}

static void LLC_MonitorCreateRxB (struct net_device *pNetDev)
{
  LLC_MonitorCreate(pNetDev, LLC_MONTYPE_RX, MKX_RADIO_B, MKX_CHANNEL_COUNT);
}

static void LLC_MonitorCreateTxB0 (struct net_device *pNetDev)
{
  LLC_MonitorCreate(pNetDev, LLC_MONTYPE_TX, MKX_RADIO_B, MKX_CHANNEL_0);
}

static void LLC_MonitorCreateRxB0 (struct net_device *pNetDev)
{
  LLC_MonitorCreate(pNetDev, LLC_MONTYPE_RX, MKX_RADIO_B, MKX_CHANNEL_0);
}

static void LLC_MonitorCreateTxB1 (struct net_device *pNetDev)
{
  LLC_MonitorCreate(pNetDev, LLC_MONTYPE_TX, MKX_RADIO_B, MKX_CHANNEL_1);
}

static void LLC_MonitorCreateRxB1 (struct net_device *pNetDev)
{
  LLC_MonitorCreate(pNetDev, LLC_MONTYPE_RX, MKX_RADIO_B, MKX_CHANNEL_1);
}

/**
 * @brief Initialize the LLC monitoring network interface(s)
 * @param pDev the LLC device structure
 * @return Zero for success or a negative errno
 */
int LLC_MonitorSetup (struct LLCDev *pDev)
{
  typedef struct LLCMonInfo {
    char *pName;
    void *pFunc;
    tMKxRadio Radio;
    tMKxChannel Channel;
    tLLCMonType Type;
  } tLLCMonInfo;

  int Res = -ENOSYS;
  int i;
  tLLCMonInfo Info[] = {
    { "cw-mon-tx",   LLC_MonitorCreateTx,   MKX_RADIO_COUNT, MKX_CHANNEL_COUNT, LLC_MONTYPE_TX },
    { "cw-mon-rx",   LLC_MonitorCreateRx,   MKX_RADIO_COUNT, MKX_CHANNEL_COUNT, LLC_MONTYPE_RX },
    { "cw-mon-txa",  LLC_MonitorCreateTxA,  MKX_RADIO_A,     MKX_CHANNEL_COUNT, LLC_MONTYPE_TX },
    { "cw-mon-rxa",  LLC_MonitorCreateRxA,  MKX_RADIO_A,     MKX_CHANNEL_COUNT, LLC_MONTYPE_RX },
    { "cw-mon-txa0", LLC_MonitorCreateTxA0, MKX_RADIO_A,     MKX_CHANNEL_0,     LLC_MONTYPE_TX },
    { "cw-mon-rxa0", LLC_MonitorCreateRxA0, MKX_RADIO_A,     MKX_CHANNEL_0,     LLC_MONTYPE_RX },
    { "cw-mon-txa1", LLC_MonitorCreateTxA1, MKX_RADIO_A,     MKX_CHANNEL_1,     LLC_MONTYPE_TX },
    { "cw-mon-rxa1", LLC_MonitorCreateRxA1, MKX_RADIO_A,     MKX_CHANNEL_1,     LLC_MONTYPE_RX },
    { "cw-mon-txb",  LLC_MonitorCreateTxB,  MKX_RADIO_B,     MKX_CHANNEL_COUNT, LLC_MONTYPE_TX },
    { "cw-mon-rxb",  LLC_MonitorCreateRxB,  MKX_RADIO_B,     MKX_CHANNEL_COUNT, LLC_MONTYPE_RX },
    { "cw-mon-txb0", LLC_MonitorCreateTxB0, MKX_RADIO_B,     MKX_CHANNEL_0,     LLC_MONTYPE_TX },
    { "cw-mon-rxb0", LLC_MonitorCreateRxB0, MKX_RADIO_B,     MKX_CHANNEL_0,     LLC_MONTYPE_RX },
    { "cw-mon-txb1", LLC_MonitorCreateTxB1, MKX_RADIO_B,     MKX_CHANNEL_1,     LLC_MONTYPE_TX },
    { "cw-mon-rxb1", LLC_MonitorCreateRxB1, MKX_RADIO_B,     MKX_CHANNEL_1,     LLC_MONTYPE_RX },
    { "",            NULL,                  -1,              -1,                -1 },
  };
  struct net_device *pNetDev = NULL;

  d_fnstart(D_TST, NULL, "(pDev %p)\n", pDev);
  d_assert(pDev != NULL);

  d_printf(D_DBG, NULL, "alloc & register netdevs\n");
  for (i = 0; Info[i].pFunc != NULL; i++)
  {
    d_printf(D_DBG, NULL, "[%d] Radio %d Chan %d Type %d Name %s Func %p\n",
             i, Info[i].Radio, Info[i].Channel,
             Info[i].Type, Info[i].pName, Info[i].pFunc);

    pNetDev = alloc_netdev(sizeof(struct LLCMonPriv),
                           Info[i].pName, Info[i].pFunc);
    if (pNetDev == NULL)
      goto Error;
    {
      tMKxRadio Radio = Info[i].Radio;
      tMKxChannel Channel = Info[i].Channel;
      tLLCMonType Type = Info[i].Type;

      pDev->Monitor.pNetDev[Radio][Channel][Type] = pNetDev;
    }
    Res = register_netdev(pNetDev);
    if (Res < 0)
      goto Error;
  }

  Res = 0;
  goto Success;

Error:
  (void)LLC_MonitorRelease(pDev);
Success:
  d_fnend(D_TST, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}

/**
 * @brief De-Initialize the LLC monitoring network interface(s)
 * @param pDev the LLC device structure
 * @return Zero for success or a negative errno
 */
int LLC_MonitorRelease (struct LLCDev *pDev)
{
  int Res = -ENOSYS;
  int i, j, k;
  struct net_device *pNetDev = NULL;
  d_fnstart(D_TST, NULL, "(pDev %p)\n", pDev);

  for (i = 0; i < (MKX_RADIO_COUNT + 1); i++)
  {
    for (j = 0; j < (MKX_CHANNEL_COUNT + 1); j++)
    {
      for (k = 0; k < (LLC_MONTYPE_COUNT + 1); k++)
      {
        pNetDev = pDev->Monitor.pNetDev[i][j][k];
        d_printf(D_DBG, NULL, "Radio %d Chan %d Type %d NetDev %p\n",
                 i, j ,k, pNetDev);
        if (pNetDev != NULL)
        {
          unregister_netdev(pNetDev);
          free_netdev(pNetDev);
        }
      }
    }
  }

  Res = 0;

  d_fnend(D_TST, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}

/**
 * @brief Echo an MKx Rx packet to the relevant monitor interface
 * @param pDev the LLC device structure
 * @param pPkt The pointer to the LLC/MKx formatted rx packet
 */
void LLC_MonitorRx (struct LLCDev *pDev,
                    tMKxRxPacket *pPkt)
{
  int Len;
  struct sk_buff *pSkb = NULL;
  struct net_device *pNetDev[3];
  tMKxRadio RadioID;
  tMKxChannel ChannelID;

  if (pPkt == NULL)
    goto Error;

  RadioID = pPkt->RxPacketData.RadioID;
  ChannelID = pPkt->RxPacketData.ChannelID;
  if ((RadioID > MKX_RADIO_MAX) || (ChannelID > MKX_CHANNEL_MAX))
    goto Error;

  // cw-mon-rx[a|b][0|1]
  pNetDev[0] = pDev->Monitor.pNetDev[RadioID][ChannelID][LLC_MONTYPE_RX];
  // cw-mon-rx[a|b]
  pNetDev[1] = pDev->Monitor.pNetDev[RadioID][MKX_CHANNEL_COUNT][LLC_MONTYPE_RX];
  // cw-mon-rx
  pNetDev[2] = pDev->Monitor.pNetDev[MKX_RADIO_COUNT][MKX_CHANNEL_COUNT][LLC_MONTYPE_RX];

  // Try to exit early if possible ((cw-mon-rx* !up)
  if (!(((pNetDev[0] != NULL) && (netif_running(pNetDev[0]) != 0)) ||
        ((pNetDev[1] != NULL) && (netif_running(pNetDev[1]) != 0)) ||
        ((pNetDev[2] != NULL) && (netif_running(pNetDev[2]) != 0))))
    goto Error;

  // Length adjustment
  Len = sizeof(tMKxRxPacket) + pPkt->RxPacketData.RxFrameLength;
  d_printf(D_DEBUG, NULL, "Len %d\n", Len);

  // Create an socket buffer and put the buffer in
  pSkb = alloc_skb(LLC_MONITOR_HEADER_LEN + Len, gfp_any());
  if (pSkb == NULL)
    goto Error;
  skb_reserve(pSkb, LLC_MONITOR_HEADER_LEN);
  skb_put(pSkb, Len);
  skb_store_bits(pSkb, 0, (const void *)pPkt, Len);

  if (pDev->Cfg.MonitorFormat != LLC_MON_FORMAT_LLC)
  {
    if (pDev->Cfg.MonitorFormat == LLC_MON_FORMAT_RADIOTAP)
    {
      _Radiotap.Len = LLC_MONITOR_HEADER_LEN + \
	                    sizeof(tMKxRxPacket);
      _Radiotap.Radiotap.Rate = 0x00; // TODO
      _Radiotap.Vendor.SkipLen = sizeof(tMKxRxPacket);
      skb_push(pSkb, LLC_MONITOR_HEADER_LEN);
      skb_store_bits(pSkb, 0, (const void *)&_Radiotap, LLC_MONITOR_HEADER_LEN);
    }
    else // (pDev->Cfg.MonitorFormat == LLC_MON_FORMAT_80211)
    {
      // Skip the header (it's just a raw 802.11 packet now)
      skb_pull(pSkb, sizeof(tMKxRxPacket));
    }

    LLC_MonitorReceive(pSkb, pNetDev[0], ETH_P_802_2);
    LLC_MonitorReceive(pSkb, pNetDev[1], ETH_P_802_2);
    LLC_MonitorReceive(pSkb, pNetDev[2], ETH_P_802_2);
  }
  else
  {
    LLC_MonitorReceive(pSkb, pNetDev[0], LLC_MONITOR_RX_PROTO);
    LLC_MonitorReceive(pSkb, pNetDev[1], LLC_MONITOR_RX_PROTO);
    LLC_MonitorReceive(pSkb, pNetDev[2], LLC_MONITOR_RX_PROTO);
  }

Error:
  kfree_skb(pSkb);
  return;
}

/**
 * @brief Echo an LLC Tx frame to the relevant monitor interface
 * @param pDev the LLC device structure
 * @param pPkt The pointer to the LLC/MKx formatted tx packet
 */
void LLC_MonitorTx (struct LLCDev *pDev,
                    tMKxTxPacket *pPkt)
{
  int Len;
  struct sk_buff *pSkb = NULL;
  struct net_device *pNetDev[3];
  tMKxRadio RadioID;
  tMKxChannel ChannelID;

  if (pPkt == NULL)
    goto Error;

  RadioID = pPkt->TxPacketData.RadioID;
  ChannelID = pPkt->TxPacketData.ChannelID;
  if ((RadioID > MKX_RADIO_MAX) || (ChannelID > MKX_CHANNEL_MAX))
    goto Error;

  // cw-mon-tx[a|b][0|1]
  pNetDev[0] = pDev->Monitor.pNetDev[RadioID][ChannelID][LLC_MONTYPE_TX];
  // cw-mon-tx[a|b]
  pNetDev[1] = pDev->Monitor.pNetDev[RadioID][MKX_CHANNEL_COUNT][LLC_MONTYPE_TX];
  // cw-mon-tx
  pNetDev[2] = pDev->Monitor.pNetDev[MKX_RADIO_COUNT][MKX_CHANNEL_COUNT][LLC_MONTYPE_TX];

  // Try to exit early if possible ((!cw-mon-tx && (cw-mon-tx* !up))
  if (!(((pNetDev[0] != NULL) && (netif_running(pNetDev[0]) != 0)) ||
        ((pNetDev[1] != NULL) && (netif_running(pNetDev[1]) != 0)) ||
        ((pNetDev[2] != NULL) /* && (netif_running(pNetDev[2]) != 0)*/)))
    goto Error;

  // Length adjustment
  Len = sizeof(tMKxTxPacket) + pPkt->TxPacketData.TxFrameLength;
  d_printf(D_DEBUG, NULL, "Len %d\n", Len);

  // Create an socket buffer and put the buffer in
  pSkb = alloc_skb(LLC_MONITOR_HEADER_LEN + Len, gfp_any());
  if (pSkb == NULL)
    goto Error;
  skb_reserve(pSkb, LLC_MONITOR_HEADER_LEN);
  skb_put(pSkb, Len);
  skb_store_bits(pSkb, 0, (const void *)pPkt, Len);

  if (pDev->Cfg.MonitorFormat != LLC_MON_FORMAT_LLC)
  {
    if (pDev->Cfg.MonitorFormat == LLC_MON_FORMAT_RADIOTAP)
    {
      _Radiotap.Len = LLC_MONITOR_HEADER_LEN + \
	                    sizeof(tMKxTxPacket);
      _Radiotap.Radiotap.Rate = 0x00; // TODO
      _Radiotap.Vendor.SkipLen = sizeof(tMKxTxPacket);
      skb_push(pSkb, LLC_MONITOR_HEADER_LEN);
      skb_store_bits(pSkb, 0, (const void *)&_Radiotap, LLC_MONITOR_HEADER_LEN);
    }
    else
    {
      // Skip the header (it's just a raw 802.11 packet now)
      skb_pull(pSkb, sizeof(tMKxTxPacket));
    }
    LLC_MonitorReceive(pSkb, pNetDev[0], ETH_P_802_2);
    LLC_MonitorReceive(pSkb, pNetDev[1], ETH_P_802_2);
    LLC_MonitorReceive(pSkb, pNetDev[2], ETH_P_802_2);
  }
  else
  {
    LLC_MonitorReceive(pSkb, pNetDev[0], LLC_MONITOR_TX_PROTO);
    LLC_MonitorReceive(pSkb, pNetDev[1], LLC_MONITOR_TX_PROTO);
    LLC_MonitorReceive(pSkb, pNetDev[2], LLC_MONITOR_TX_PROTO);
  }

Error:
  kfree_skb(pSkb);
  return;
}

/**
 * @}
 * @}
 * @}
 */

