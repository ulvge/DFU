/**
 * @addtogroup cohda_llc LLC module
 * @{
 *
 * @addtogroup cohda_llc_intern LLC internals
 * @{
 *
 * @file
 * LLC generic netlink specific net device
 *
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
#include "llc-internal.h"

#define D_SUBMODULE LLC_NetDev
#include "debug-levels.h"

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------
#define LLC_ETHERTYPE     (0x04ff)

#define LLC_DEV_NAME      ("cw-llc")

#define LLC_DEV_HEADROOM  (72)
#define LLC_DEV_TAILROOM  (16)

//------------------------------------------------------------------------------
// Type definitions
//------------------------------------------------------------------------------

/// The LLC netdev's work descriptor
typedef struct LLCNetDevWork
{
  /// Overaching device handle
  struct LLCDev *pDev;
  /// The buffer to 'transmit'
  struct sk_buff *pSkb;
  /// The kernel work structure
  struct work_struct Work;
} tLLCNetDevWork;

//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------

/// LLC device pointer for use in the Rx processing
struct LLCDev *__pDev = NULL;

//------------------------------------------------------------------------------
// Function Prototypes
//------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// LLC netdev functions
//-----------------------------------------------------------------------------

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30))
#include <linux/etherdevice.h>
#else
int eth_mac_addr (struct net_device *pNetDev,
                  void *pAddr)
{
  struct sockaddr *pSockAddr = pAddr;

  if (netif_running(pNetDev))
    return -EBUSY;
  if (!is_valid_ether_addr(pSockAddr->sa_data))
    return -EADDRNOTAVAIL;
  memcpy(pNetDev->dev_addr, pSockAddr->sa_data, ETH_ALEN);
  return 0;
}

int eth_change_mtu (struct net_device *pNetDev,
                    int NewMTU)
{
  if (NewMTU < 68 || NewMTU > ETH_DATA_LEN)
    return -EINVAL;
  pNetDev->mtu = NewMTU;
  return 0;
}

int eth_validate_addr (struct net_device *pNetDev)
{
  if (!is_valid_ether_addr(pNetDev->dev_addr))
    return -EADDRNOTAVAIL;

  return 0;
}
#endif // (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30))

static int LLC_NetDevOpen(struct net_device *pNetDev)
{
  int Res = -ENOSYS;

  d_fnstart(D_API, NULL, "(pNetDev %p)\n", pNetDev);

  netif_start_queue(pNetDev);//@驱动程序调用这个函数来告诉内核网络子系统，现在可以开始数据包的发送。
  Res = 0;

  d_fnend(D_API, NULL, "(pNetDev %p) = %d\n", pNetDev, Res);
  return Res;
}

static int LLC_NetDevClose(struct net_device *pNetDev)
{
  int Res = -ENOSYS;

  d_fnstart(D_API, NULL, "(pNetDev %p)\n", pNetDev);

  netif_stop_queue(pNetDev);
  Res = 0;

  d_fnend(D_API, NULL, "(pNetDev %p) = %d\n", pNetDev, Res);
  return Res;
}

static int LLC_NetDevConfig(struct net_device *pNetDev,
                             struct ifmap *pMap)
{
  int Res = -ENOSYS;

  d_fnstart(D_API, NULL, "(pNetDev %p, pMap %p)\n", pNetDev, pMap);

  if (pNetDev->flags & IFF_UP)
  {
    return -EBUSY;
  }

  if (pMap->base_addr != pNetDev->base_addr)
  {
    return -EOPNOTSUPP;
  }

  d_fnend(D_API, NULL, "(pNetDev %p, pMap %p) = %d\n", pNetDev, pMap, Res);
  return Res;
}

static void LLC_NetDevTxTimeout(struct net_device *pNetDev)
{
  d_fnstart(D_API, NULL, "(pNetDev %p)\n", pNetDev);

  netif_wake_queue(pNetDev);

  d_fnend(D_API, NULL, "(pNetDev %p) = void\n", pNetDev);
  return;
}

static int LLC_NetDevIoctl(struct net_device *pNetDev,
                           					 struct ifreq *pReq, int Cmd)
{
  int Res = -ENOSYS;

  d_fnstart(D_API, NULL, "(pNetDev %p, pReq %p, Cmd %x)\n", pNetDev, pReq, Cmd);

  // Cmd must be in the the 'SIOCDEVPRIVATE' range
  if ((Cmd >= SIOCDEVPRIVATE) &&
      (Cmd <= SIOCDEVPRIVATE + 15))
  {
    struct LLCDev *pDev = LLC_NetDevToDev(pNetDev);

    // Remove the SIOCDEVPRIVATE offset
    Cmd -= SIOCDEVPRIVATE;

    if (pDev != NULL)
      Res = LLC_Ioctl(pDev, pReq, Cmd);
  }
  else
  {
    Res = 0;
  }

  d_fnend(D_API, NULL, "(pNetDev %p, pReq %p, Cmd %x) = %d\n",
          pNetDev, pReq, Cmd, Res);
  return Res;
}

/**
 * @brief Workqueue based TX of a debug message to the MKx
 */
static void LLC_NetDevTxWorkCallback(struct work_struct *pWork)
{
  int Res = -ENOSYS;
  struct LLCNetDevWork *pNetDevWork = NULL;
  struct LLCDev *pDev = NULL;
  struct sk_buff *pSkb = NULL;
  struct net_device *pNetDev = NULL;

  d_fnstart(D_INTERN, NULL, "(pWork %p)\n", pWork);
  d_assert(pWork != NULL);

  pNetDevWork = container_of(pWork, struct LLCNetDevWork, Work);
  pDev = pNetDevWork->pDev;
  pSkb = pNetDevWork->pSkb;
  if ((pDev == NULL) || (pDev == NULL))
    goto Error;

  pNetDev = pDev->pNetDev;
  if (pNetDev == NULL)
    goto Error;

  Res = LLC_MsgSend(pDev, (struct MKxIFMsg *)(pSkb->data));//????
  if (Res == 0)
  {
    pNetDev->stats.tx_packets++;
    pNetDev->stats.tx_bytes += pSkb->len;
    goto Success;
  }
  else
    goto Error;

Error:
  pNetDev->stats.tx_dropped++;
  pNetDev->stats.tx_errors++;
Success:
  if (pSkb != NULL)
    kfree_skb(pSkb);
  kfree(pNetDevWork);
  d_fnend(D_TST, NULL, "() = void\n");
  return;
}


static int LLC_NetDevTx(struct sk_buff *pTxSkb,
                         struct net_device *pNetDev)
{
  int Res = -ENOSYS;
  struct LLCDev *pDev = NULL;
  struct sk_buff *pSkb = NULL;

  d_fnstart(D_INTERN, NULL, "(pTxSkb %p pNetDev %p)\n", pTxSkb, pNetDev);

  if ((NULL == pTxSkb) || (NULL == pNetDev))
  {
    d_error(D_ERR, NULL, "pTxSkb %p pNetDev %p\n", pTxSkb, pNetDev);
    Res = -EINVAL;
    goto Error;
  }

  // Extract the LLC & associated Dot4 handle from the netdev
  pDev = LLC_NetDevToDev(pNetDev);
  if (pDev == NULL)
  {
    Res = -EINVAL;
    goto Error;
  }

  pNetDev->trans_start = jiffies;

  pSkb = skb_copy_expand((struct sk_buff *) pTxSkb,
                         LLC_DEV_HEADROOM, LLC_DEV_TAILROOM,gfp_any());
  
  if (pSkb == NULL)
    goto Error;
  d_printf(D_DBG, NULL, "pSkb %p %d\n", pSkb, pSkb->len);
  d_dump(D_DBG, NULL, pSkb->data, pSkb->len);

  // Do the rest in the kernel thread
  {
    struct LLCNetDevWork *pNetDevWork = NULL;
    pNetDevWork = kzalloc(sizeof(struct LLCNetDevWork), gfp_any());
    if (pNetDevWork == NULL)
    {
      Res = NETDEV_TX_BUSY;
      goto Error;
    }

    pNetDevWork->pDev = pDev;
    pNetDevWork->pSkb = pSkb;
    INIT_WORK(&(pNetDevWork->Work), LLC_NetDevTxWorkCallback);//???

    Res = schedule_work(&(pNetDevWork->Work));//激活工作队列
    if (Res != 1)
      d_printf(D_WARN, NULL, "schedule_work() = %d\n", Res);
  }

Error:
  pNetDev->stats.tx_dropped++;
  pNetDev->stats.tx_errors++;
  if (pTxSkb != NULL)
    kfree_skb(pTxSkb);
  d_fnend(D_INTERN, NULL, "(pTxSkb %p pNetDev %p) = NETDEV_TX_OK\n",
          pTxSkb, pNetDev);
  return NETDEV_TX_OK;
}

void LLC_NetDevRx(struct LLCDev *pDev,
                   struct sk_buff *pRxSkb)
{
  struct net_device *pNetDev = NULL;
  struct sk_buff *pSkb = NULL;

  d_fnstart(D_API, NULL, "(pRxSkb %p)\n", pRxSkb);

  if (pRxSkb == NULL)
    goto Error;

  if (pDev == NULL)
    goto Error;
  if (pDev->pNetDev == NULL)
    goto Error;
  pNetDev = pDev->pNetDev;

  if (netif_running(pNetDev) == 0)
    goto Error;

  pSkb = skb_copy((struct sk_buff *) pRxSkb, gfp_any());
  if (pSkb == NULL)
    goto Error;

  // Fill in the rest of the relevant pSkb bits
  pSkb->dev = pNetDev;

  skb_orphan(pSkb);
  pSkb->dev = pNetDev;
  pSkb->pkt_type = PACKET_BROADCAST;
  pSkb->protocol = htons(LLC_ETHERTYPE);
  pSkb->ip_summed = CHECKSUM_UNNECESSARY;
  skb_reset_mac_header(pSkb);


  // Update the stats
  pNetDev->last_rx = jiffies;
  pNetDev->stats.rx_packets++;
  pNetDev->stats.rx_bytes += pRxSkb->len;

  d_printf(D_INFO, NULL, "Delivering %d bytes to the network stack\n",
           pRxSkb->len);

  if (in_atomic())
    netif_rx(pSkb);//接收
  else
    netif_rx_ni(pSkb);
Error:
  d_fnend(D_API, NULL, "(pRxSkb %p) = void\n", pRxSkb);
}

int LLC_NetDevInd(struct LLCDev *pDev,
                   struct MKxIFMsg *pMsg)
{
  int Ret = -1;
  struct sk_buff *pSkb = NULL;
  struct MKxIFMsg *pData = NULL;

  d_assert((pDev != NULL) && (pMsg != NULL));

  // Allocate a socket buffer for the message
  pSkb = alloc_skb(pMsg->Len, gfp_any());
  if (pSkb == NULL)
    goto Error;
  pData = (struct MKxIFMsg *)skb_put(pSkb, pMsg->Len);
  if (pData == NULL)
    goto Error;

  // Populate the message
  memcpy(pData, pMsg, pMsg->Len);
  // Deliver it to the netdev
  LLC_NetDevRx(pDev, pSkb);//接收帧
  Ret = 0;

Error:
  if (pSkb != NULL)
    kfree_skb(pSkb);
  return Ret;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30))
static const struct net_device_ops LLCNetDevOps =
{
  .ndo_open 			= LLC_NetDevOpen,
  .ndo_stop 			= LLC_NetDevClose,
  .ndo_set_config 		= LLC_NetDevConfig,
  .ndo_start_xmit 		= LLC_NetDevTx,
  .ndo_do_ioctl 			= LLC_NetDevIoctl,
  .ndo_tx_timeout 		= LLC_NetDevTxTimeout,
  .ndo_change_mtu 		= eth_change_mtu,
  .ndo_set_mac_address 	= eth_mac_addr,
  .ndo_validate_addr 		= eth_validate_addr,
};
#endif // (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30))

static void LLC_NetDevCreate (struct net_device *pNetDev)
{
  d_fnstart(D_API, NULL, "(pNetDev %p)\n", pNetDev);

  pNetDev->destructor = free_netdev;
  ether_setup(pNetDev);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30))
  pNetDev->netdev_ops = &(LLCNetDevOps);
#else
  pNetDev->open = LLC_NetDevOpen;
  pNetDev->stop = LLC_NetDevClose;
  pNetDev->set_config = LLC_NetDevConfig;
  pNetDev->hard_start_xmit = LLC_NetDevTx;///
  pNetDev->do_ioctl = LLC_NetDevIoctl;
  pNetDev->tx_timeout = LLC_NetDevTxTimeout;
  pNetDev->change_mtu = eth_change_mtu;
  pNetDev->set_mac_address = eth_mac_addr;
  pNetDev->validate_addr = eth_validate_addr;
#endif // (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30))
  pNetDev->tx_queue_len = 0;
  pNetDev->hard_header_len = LLC_DEV_HEADROOM;
  //点对点,组播
  pNetDev->flags = IFF_POINTOPOINT | IFF_NOARP | IFF_MULTICAST;
  pNetDev->type = ARPHRD_NONE;///* interface hardware type	*//* zero header length */
  random_ether_addr(pNetDev->dev_addr);//随机的mac地址

  d_fnend(D_API, NULL, "(pNetDev %p) = void\n", pNetDev);
}

/**
 * Initialize the 'cw-llc' network interface
 */
int LLC_NetDevSetup(struct LLCDev *pDev)
{
  int Res = 0;
  struct net_device *pNetDev = NULL;
  struct LLCNetDevPriv *pLLCNetDevPriv = NULL;

  d_fnstart(D_API, NULL, "(pDev %p)\n", pDev);

  if (pDev == NULL)
    return -ENOSYS;

  pNetDev = alloc_netdev(sizeof(struct LLCNetDevPriv),
                         LLC_DEV_NAME,//cw_llc
                         LLC_NetDevCreate);
  if (pNetDev == NULL)
    goto ErrorAlloc;

  Res = register_netdev(pNetDev);
  if (Res < 0)
    goto ErrorRegister;

  // Store the 'LLC' pointer in the net device's private storage section
  pLLCNetDevPriv = netdev_priv(pNetDev);
  pLLCNetDevPriv->pDev = pDev;

  pDev->pNetDev = pNetDev;

  Res = 0;
  goto Success;

ErrorRegister:
  d_error(D_ERR, NULL, "ErrorRegister\n");
  free_netdev(pNetDev);
ErrorAlloc:
  d_error(D_ERR, NULL, "ErrorAlloc\n");
Success:
  d_fnend(D_API, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}

/**
 * @brief De-Initialize the 'cw-llc' network interface
 */
int LLC_NetDevRelease (struct LLCDev *pDev)
{
  int Res = 0;
  struct LLCNetDevPriv *pLLCNetDevPriv = NULL;

  d_fnstart(D_API, NULL, "(pDev %p)\n", pDev);

  // Cleanup the netdev's private data
  pLLCNetDevPriv = netdev_priv(pDev->pNetDev);
  pLLCNetDevPriv->pDev = NULL;

  // 'free_netdev(pNetDev)' not neccessary as it's automatically done for us
  // by setting the pNetDev->destructor.
  unregister_netdev(pDev->pNetDev);
  pDev->pNetDev = NULL;

  d_fnend(D_API, NULL, "(pDev %p) = %d\n", pDev, Res);
  return Res;
}

// Close the Doxygen group
/**
 * @}
 * @}
 */
