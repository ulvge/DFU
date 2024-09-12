// No doxygen 'group' header because this file is included by both user & kernel implmentations

//------------------------------------------------------------------------------
// Copyright (c) 2013 Cohda Wireless Pty Ltd
//------------------------------------------------------------------------------

#ifndef __LINUX__COHDA__LLC__LLC_API_H__
#define __LINUX__COHDA__LLC__LLC_API_H__

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------
#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/string.h> // for memcpy

// Limits of integral types
#ifndef INT8_MIN
#define INT8_MIN               (-128)
#endif
#ifndef INT16_MIN
#define INT16_MIN              (-32767-1)
#endif
#ifndef INT32_MIN
#define INT32_MIN              (-2147483647-1)
#endif
#ifndef INT8_MAX
#define INT8_MAX               (127)
#endif
#ifndef INT16_MAX
#define INT16_MAX              (32767)
#endif
#ifndef INT32_MAX
#define INT32_MAX              (2147483647)
#endif
#ifndef UINT8_MAX
#define UINT8_MAX              (255U)
#endif
#ifndef UINT16_MAX
#define UINT16_MAX             (65535U)
#endif
#ifndef UINT32_MAX
#define UINT32_MAX             (4294967295U)
#endif

#else
#include <stdint.h>
#include <string.h> // For memcpy()
#endif

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

/// Number of WISPA Tx Gain settings available on WISPA-ITS
#define WISPA_ITS_GAIN_CNT (32)
/// Number of WISPA Tx Gain settings available on WISPA-TC2
#define WISPA_TC2_GAIN_CNT (9)

/// Set the number of WISPA Tx gain settings for the TC2 (ITS doesn't need 32)
#define WISPA_TX_GAIN_CNT (WISPA_TC2_GAIN_CNT)

/// MKx magic value
#define MKX_API_MAGIC (0xC0DA)

/// The size of the Address Matching Table
#define AMS_TABLE_COUNT 8

/// Rename of inline
#ifndef INLINE
#define INLINE __inline
#endif

//------------------------------------------------------------------------------
// Type definitions
//------------------------------------------------------------------------------

/**
 * @section llc_remote Remote LLC Module
 *
 * LLCRemote implements a mechanism to allow the MKx Modem to be used as a
 * remote MAC to a Linux Host machine.
 *
 * @verbatim
                       Source provided -> +----------------+
   +-----------------+                    |  llc debug app |
   |   Stack / Apps  |                    +-------*--------+
   +---*-------*-----+                            |                  User Space
 ------|-------|----------------------------------|---------------------------
       | ioctl | socket(s)                        | socket         Kernel Space
     +-*-------*---+                              |
     |  simTD API  | (optional binary)            |
     +------*------+                              |
            | API (MKx_* functions)               |
     +------*------+                              |
     |  LLCRemote  +------------------------------+
     |             |<- Source code provided
     +--*-------*--+
        | USB   | Ethernet (MKxIF_* structures)
    +---*-+ +---*----+
    | USB | | TCP/IP |
    +-----+ +--------+                                         Client side (uP)
 -----------------------------------------------------------------------------
 +---------------------+ +---------------------+              Server Side (SDR)
 |        WMAC         | |   C2X Security      |
 +---------------------+ +---------------------+
 |     802.11p MAC     |
 +---------------------+
 |     802.11p PHY     |
 +---------------------+
 @endverbatim
 *
 * @subsection llc_remote_design LLCRemote MAC Design
 *
 * The LLCRemote module communicates between Server and Client via two USB bulk
 * endpoints or two UDP sockets.
 */

/// Types for the LLCRemote message transfers
typedef enum
{
  /// A transmit packet (message data is @ref tMKxTxPacket)
  MKXIF_TXPACKET = 0,
  /// A received packet (message data is @ref tMKxRxPacket)
  MKXIF_RXPACKET,
  /// New UTC Time (obtained from NMEA data)
  MKXIF_SET_TSF,
  /// Transmitted packet event (message data is @ref tMKxTxEventData)
  MKXIF_TXEVENT,
  /// Radio config for Radio A (message data is @ref tMKxRadioConfig)
  MKXIF_RADIOACFG,
  /// Radio config for Radio B (message data is @ref tMKxRadioConfig)
  MKXIF_RADIOBCFG,
  /// Radio A statistics (message data is @ref tMKxRadioStats)
  MKXIF_RADIOASTATS,
  /// Radio B statistics (message data is @ref tMKxRadioStats)
  MKXIF_RADIOBSTATS,
  /// Flush a single queue or all queueus
  MKXIF_FLUSHQ,
  /// A generic debug container.
  MKXIF_DEBUG,
  /// Calibration message (message data is @ref tMKxCalMsg)
  MKXIF_CAL,
  /// C2XSEC message
  MKXIF_C2XSEC,
} eMKxIFMsgType;
/// @copydoc eMKxIFMsgType
typedef uint16_t tMKxIFMsgType;

/// LLCRemote message (LLC managed header)
typedef struct MKxIFMsg
{
  /// Message type
  tMKxIFMsgType Type;
  /// Length of the message, including the header itself
  uint16_t Len;
  /// Sequence number
  uint16_t Seq;
  /// Return value
  int16_t Ret;
  /// Message data
  uint8_t Data[];
} __attribute__ ((packed)) tMKxIFMsg;

/**
 * @section llc_api MKx API
 *
 * This section provides an overview of the MKx WAVE MAC usage, in order to clarify its
 * functionality.
 *
 * @subsection general_usage General usage in a WSM/Proprietary Protocol System (user-space implementation)
 *
 * Typical usage would be:
 * - Load the MKx LLC kernel module
 * - Open the MKx interface using the MKx_Init() function.
 * - Enable notifications by setting the pMKx->API.Callbacks.NotifInd() callback
 * - Enable packet reception by setting the pMKx->API.Callbacks.RxAlloc()
 *    pMKx->API.Callbacks.RxInd() and callbacks
 * - Enable transmit confirmations by setting the pMKx->API.Callbacks.TxCnf() callback
 * - Set the Radio A (CCH & SCH-A) parameters using the MKx_Config() function.
 * - Set the Radio B (CCH & SCH-B) parameters using the MKx_Config() function.
 * - Packets can be transmitted using the TxReq() function and the success/failure
 *    of the frame is indicated via the TxCnf() callback
 * - Packets received on either radio will be allocated with the RxAlloc()
 *    callback and delivered via the RxInd() callback
 * - When done, the MKx interface can be gracelully coldes with MKx_Exit()
 *
 * @subsection channel_measurements Channel Measurements
 * - Statistics are updates are notified via the NotifInd() callback every 50ms
 * - Counters can be read directly from the MKx handle or using the MKx_GetStats() helper function
 *   - Channel busy ratio is provided in the per-channel statistics.
 *     This is the ratio of channel busy (virtual carrier sense is asserted)
 *     time to channel idle time.
 *     It is an 8-bit unsigned value, where 100% channel utilisation is indicated by a value of 255.
 *   - Average idle period power is provided in the per-channel statistics.
 *     This is the average RSSI recorded whilst the channel isn't busy (virtual carrier sense is not asserted).
 *
 * @subsection dual_channel_operation  Dual channel operation
 * When operating in a dual-radio configuration, it is possible to configure the MAC channel
 * access function to consider the state of the other radio channel before making transmit
 * decisions. The WMAC allows the following configuration options for the channel access
 * function when operating in a dual-radio system.
 *
 * - No consideration of other radio. In this case, the radio will transmit without regard to
 * the state of the other radio channel. The system will behave effectively as two
 * independent radio systems.
 * - Tx inhibit. In this mode, the MAC will prevent this radio from transmitting while the
 * other radio is transmitting. In this case, when the other radio is transmitting, the local
 * radio behaves as if the local channel is busy.
 * - Rx inhibit. In this mode, the MAC will prevent this radio from transmitting while the
 * other radio is actively receiving a frame. In this case, when the other radio is receiving,
 * the local radio behaves as if the local channel is busy. This prevents transmissions from
 * this radio from corrupting the reception of a frame on the other radio, tuned to a nearby
 * radio channel (in particular when shared or co-located antennas are in use).
 * - TxRx inhibit. In this mode, the MAC will prevent this radio from transmitting while
 * the other radio is either transmitting or receiving.
 *
 * In all cases, the transmission inhibit occurs at the MAC channel-access level, so packets will
 * not be dropped when transmission is inhibited, they will simply be deferred.
 *
 */

/// Forward declaration of the MKx Handle
struct MKx;

/// MKx MLME interface return codes
typedef enum
{
  /// Success return code
  MKXSTATUS_SUCCESS = 0,
  // -1 to -255 reserved for @c errno values (see <errno.h>)
  /// Unspecified failure return code (catch-all)
  MKXSTATUS_FAILURE_INTERNAL_ERROR              = -1000,
  /// Failure due to invalid MKx Handle
  MKXSTATUS_FAILURE_INVALID_HANDLE              = -1001,
  /// Failure due to
  MKXSTATUS_FAILURE_INVALID_CONFIG              = -1002,
  MKXSTATUS_FAILURE_INVALID_TX                  = -1003,
  /// Failure due to invalid parameter setting
  MKXSTATUS_FAILURE_INVALID_PARAM               = -1004,
  /// Auto-cal requested when radio is running auto-cal
  MKXSTATUS_FAILURE_AUTOCAL_REJECT_SIMULTANEOUS = -1005,
  /// Auto-cal requested but radio is not configured
  MKXSTATUS_FAILURE_AUTOCAL_REJECT_UNCONFIGURED = -1006,
  /// Radio config failed (likely to be a hardware fault)
  MKXSTATUS_FAILURE_RADIOCONFIG                 = -1007,
} eMKxStatus;
/// @copydoc eMKxStatus
typedef int tMKxStatus;

/// MKx Radio
typedef enum
{
  /// Selection of Radio A of the MKX
  MKX_RADIO_A = 0,
  /// Selection of Radio B of the MKX
  MKX_RADIO_B = 1,
  // ...
  /// Used for array dimensioning
  MKX_RADIO_COUNT,
  /// Used for bounds checking
  MKX_RADIO_MAX = MKX_RADIO_COUNT - 1,
} eMKxRadio;
/// @copydoc eMKxRadio
typedef int8_t tMKxRadio;

/// MKx Channel
typedef enum
{
  /// Indicates Channel Config 0 is selected
  MKX_CHANNEL_0 = 0,
  /// Indicates Channel Config 1 is selected
  MKX_CHANNEL_1 = 1,
  // ...
  /// Used for array dimensioning
  MKX_CHANNEL_COUNT,
  /// Used for bounds checking
  MKX_CHANNEL_MAX = MKX_CHANNEL_COUNT - 1,

} eMKxChannel;
/// @copydoc eMKxChannel
typedef int8_t tMKxChannel;

/// MKx Bandwidth
typedef enum
{
  /// Indicates 10 MHz
  MKXBW_10MHz = 10,
  /// Indicates 20 MHz
  MKXBW_20MHz = 20,
} eMKxBandwidth;
/// @copydoc eMKxBandwidth
typedef int8_t tMKxBandwidth;

/// The channel's centre frequency [MHz]
typedef uint16_t tMKxChannelFreq;

/**
 * MKx dual radio transmit control
 * Controls transmit behaviour according to activity on the
 * other radio (inactive in single radio configurations)
 */
typedef enum
{
  /// Do not constrain transmissions
  MKX_TXC_NONE,
  /// Prevent transmissions when other radio is transmitting
  MKX_TXC_TX,
  /// Prevent transmissions when other radio is receiving
  MKX_TXC_RX,
  /// Prevent transmissions when other radio is transmitting or receiving
  MKX_TXC_TXRX,
  /// Default behaviour
  MKX_TXC_DEFAULT = MKX_TXC_TX,
} eMKxDualTxControl;
/// @copydoc eMKxDualTxControl
typedef uint8_t tMKxDualTxControl;

/**
 * MKx Modulation and Coding scheme
 */
typedef enum
{
  /// Rate 1/2 BPSK
  MKXMCS_R12BPSK = 0xB,
  /// Rate 3/4 BPSK
  MKXMCS_R34BPSK = 0xF,
  /// Rate 1/2 QPSK
  MKXMCS_R12QPSK = 0xA,
  /// Rate 3/4 QPSK
  MKXMCS_R34QPSK = 0xE,
  /// Rate 1/2 16QAM
  MKXMCS_R12QAM16 = 0x9,
  /// Rate 3/4 16QAM
  MKXMCS_R34QAM16 = 0xD,
  /// Rate 2/3 64QAM
  MKXMCS_R23QAM64 = 0x8,
  /// Rate 3/4 64QAM
  MKXMCS_R34QAM64 = 0xC,
  /// Use default data rate
  MKXMCS_DEFAULT = 0x0,
  /// Use transmit rate control (currently unused)
  MKXMCS_TRC = 0x1,
} eMKxMCS;
/// @copydoc eMKxMCS
typedef uint8_t tMKxMCS;

/// Tx & Rx power of frame, in 0.5dB units.
typedef enum
{
  /// Selects the PHY maximum transmit power
  MKX_POWER_TX_MAX     = INT16_MAX,
  /// Selects the PHY minimum transmit power
  MKX_POWER_TX_MIN     = INT16_MIN,
  /// Selects the PHY default transmit power level
  MKX_POWER_TX_DEFAULT = MKX_POWER_TX_MIN + 1,
} eMKxPower;
/// @copydoc eMKxPower
typedef int16_t tMKxPower;

/**
 * MKx Antenna Selection
 */
typedef enum
{
  /// Transmit packet on neither antenna (dummy transmit)
  MKX_ANT_NONE = 0,
  /// Transmit packet on antenna 1
  MKX_ANT_1 = 1,
  /// Transmit packet on antenna 2 (when available).
  MKX_ANT_2 = 2,
  /// Transmit packet on both antenna
  MKX_ANT_1AND2 = MKX_ANT_1 | MKX_ANT_2,
  /// Selects the default (ChanConfig) transmit antenna setting
  MKX_ANT_DEFAULT = 4,
} eMKxAntenna;
/// Number of antennas that are present for the MKX
#define MKX_ANT_COUNT 2
/// @copydoc eMKxAntenna
typedef uint8_t tMKxAntenna;

/**
 * MKx TSF
 * Indicates absolute 802.11 MAC time in micro seconds
 */
typedef uint64_t tMKxTSF;

/**
 * MKx Rate sets
 * Each bit indicates if corresponding MCS rate is supported
 */
typedef enum
{
  /// Rate 1/2 BPSK rate mask
  MKX_RATE12BPSK_MASK = 0x01,
  /// Rate 3/4 BPSK rate mask
  MKX_RATE34BPSK_MASK = 0x02,
  /// Rate 1/2 QPSK rate mask
  MKX_RATE12QPSK_MASK = 0x04,
  /// Rate 3/4 QPSK rate mask
  MKX_RATE34QPSK_MASK = 0x08,
  /// Rate 1/2 16QAM rate mask
  MKX_RATE12QAM16_MASK = 0x10,
  /// Rate 2/3 64QAM rate mask
  MKX_RATE23QAM64_MASK = 0x20,
  /// Rate 3/4 16QAM rate mask
  MKX_RATE34QAM16_MASK = 0x40,
} eMKxRate;
/// @copydoc eMKxRate
typedef uint8_t tMKxRate;

/**
 * MKx 802.11 service class specification.
 */
typedef enum
{
  /// Packet should be (was) transmitted using normal ACK policy
  MKX_QOS_ACK = 0x00,
  /// Packet should be (was) transmitted without Acknowledgement.
  MKX_QOS_NOACK = 0x01
} eMKxService;
/// @copydoc eMKxService
typedef uint8_t tMKxService;

/**
 * Transmit Event Status. Specifies the status of a transmitted packet.
 */
typedef enum
{
  /// Packet was successfully  transmitted
  MKXTXSTATUS_SUCCESS = 0,
  /// Packet failed by exceeding Time To Live
  MKXTXSTATUS_FAIL_TTL,
  /// Packet failed by exceeding Max Retry count
  MKXTXSTATUS_FAIL_RETRIES,
  /// Packet failed because queue was full
  MKXTXSTATUS_FAIL_QUEUEFULL,
  /// Packet failed because requested radio is not present
  MKXTXSTATUS_FAIL_RADIO_NOT_PRESENT,
} eMKxTxStatus;
/// @copydoc eMKxTxStatus
typedef int16_t tMKxTxStatus;

/**
 * MKx Transmit Descriptor. This header is used to control how the data packet
 * is transmitted by the LLC. This is the header used on all transmitted
 * packets.
 */
typedef struct MKxTxPacketData
{
  /// Indicate the radio that should be used (Radio A or Radio B)
  tMKxRadio RadioID;
  /// Indicate the channel config for the selected radio
  tMKxChannel ChannelID;
  /// Indicate the antennas upon which packet should be transmitted
  /// (may specify default)
  tMKxAntenna TxAntenna;
  /// Indicate the MCS to be used (may specify default)
  tMKxMCS MCS;
  /// Indicate the power to be used (may specify default)
  tMKxPower TxPower;
  /// Request to set the retry bit of the 802.11 MAC Header (internal parameter)
  uint8_t SetRetryBit;
  // Reserved (for 64 bit alignment)
  uint8_t Reserved0;
  /// Indicate the expiry time as an absolute MAC time in microseconds
  /// (0 means never)
  tMKxTSF Expiry;
  /// Length of the frame (802.11 Header + Body, not including FCS)
  uint16_t TxFrameLength;
  // Reserved (for 32 bit alignment)
  uint16_t Reserved1;
  /// Frame (802.11 Header + Body, not including FCS)
  uint8_t TxFrame[];
} __attribute__((__packed__)) tMKxTxPacketData;

/**
 * MKx Transmit Packet format.
 */
typedef struct MKxTxPacket
{
  /// Interface Message Header
  tMKxIFMsg Hdr;
  /// Tx Packet control and frame data
  tMKxTxPacketData TxPacketData;
} __attribute__((__packed__)) tMKxTxPacket;

/**
 * Transmit Event Data. This is the structure of the data field for
 * MKxIFMsg messages of type TxEvent.
 */
typedef struct MKxTxEventData
{
  /// Transmit status (transmitted/retired)
  tMKxTxStatus TxStatus;
  /// The TSF when the packet was transmitted or retired
  tMKxTSF TxTime;
} __attribute__((__packed__)) tMKxTxEventData;

/**
 * MKx Transmit Event format.
 */
typedef struct MKxTxEvent
{
  /// Interface Message Header
  tMKxIFMsg Hdr;
  /// Tx Event Data
  tMKxTxEventData TxEventData;
} __attribute__((__packed__)) tMKxTxEvent;

/**
 * MKx Meta Data type - contains per frame receive meta-data
 *
 * The Trice is a 1/256th of microsecond resolution time stamp of the
 * received frame.  It should be considered as an offset to the 64 bit
 * tMKxTSF value.
 *
 * The fine frequency estimate is the carrier frequency offset.
 * It is a signed 24 bit integer in units of normalized radians per sample
 * (10 MHz or 20 MHz), with a Q notation of SQ23.
 */
typedef struct MKxRxMeta
{
  /// Rx Time Stamp (1/256th of a microsecond)
  uint32_t Trice :8;
  /// Fine Frequency estimate (normalized radians per sample in SQ23)
  uint32_t FineFreq :24;
} __attribute__((__packed__)) tMKxRxMeta;

/**
 * MKx Receive descriptor and frame.
 * This header is used to pass receive packet meta-information from
 * the LLC to upper-layers. This header is pre-pended to all received packets.
 * If only a single receive  power measure is required, then simply take the
 * maximum power of Antenna A and B.
 */
typedef struct MKxRxPacketData
{
  /// Indicate the radio that should be used (Radio A or Radio B)
  tMKxRadio RadioID;
  /// Indicate the channel config for the selected radio
  tMKxChannel ChannelID;
  /// Indicate the data rate that was used
  tMKxMCS MCS;
  // Reserved (for 16 bit alignment)
  uint8_t Reserved0;
  /// Indicate the received power on Antenna A
  tMKxPower RxPowerA;
  /// Indicate the received power on Antenna B
  tMKxPower RxPowerB;
  /// Indicate the receiver noise on Antenna A
  tMKxPower RxNoiseA;
  /// Indicate the receiver noise on Antenna B
  tMKxPower RxNoiseB;
  /// Per Frame Receive Meta Data
  tMKxRxMeta RxMeta;
  /// MAC Rx Timestamp, local MAC TSF time at which packet was received
  tMKxTSF RxTSF;
  /// Length of the Frame (802.11 Header + Body, including FCS)
  uint16_t RxFrameLength;
  // Reserved (for 32 bit alignment)
  uint16_t Reserved1;
  /// Frame (802.11 Header + Body, including FCS)
  uint8_t RxFrame[];
} __attribute__((__packed__)) tMKxRxPacketData;

/**
 * MKx receive packet format.
 */
typedef struct MKxRxPacket
{
  /// Interface Message Header
  tMKxIFMsg Hdr;
  /// Rx Packet control and frame data
  tMKxRxPacketData RxPacketData;
} __attribute__((__packed__)) tMKxRxPacket;

/**
 * MKx Rate Set. See @ref eMKxRate for bitmask for enabled rates
 */
typedef uint8_t tMKxRateSet[8];

/// Address matching control bits
/// (0) = ResponseEnable
/// (1) = BufferEnableCtrl
/// (2) = BufferEnableBadFCS
/// (3) = LastEntry
typedef enum
{
  /// ResponseEnable -- Respond with ACK when a DATA frame is matched.
  MKX_ADDRMATCH_RESPONSE_ENABLE = (1 << 0),
  /// BufferEnableCtrl -- Buffer control frames that match.
  MKX_ADDRMATCH_ENABLE_CTRL = (1 << 1),
  /// BufferEnableBadFCS -- Buffer frames even if FCS error was detected.
  MKX_ADDRMATCH_ENABLE_BAD_FCS = (1 << 2),
  /// LastEntry -- Indicates this is the last entry in the table.
  MKX_ADDRMATCH_LAST_ENTRY = (1 << 3),

} eMKxAddressMatchingCtrl;

/**
 * @brief Receive frame address matching structure
 *
 * General operation of the MKx on receive frame:
 * - bitwise AND of 'Mask' and the incoming frame's DA (DA not modified)
 * - equality check between 'Addr' and the masked DA
 * - If equal: continue
 *  - If 'ResponseEnable' is set: Send 'ACK'
 *  - If 'BufferEnableCtrl' is set: Copy into internal buffer
 *         & deliver via RxInd() if FCS check passes
 *  - If 'BufferEnableBadFCS' is set: Deliver via RxInd() even if FCS check fails
 *
 * To receive broadcast frames:
 * - Addr = 0XFFFFFFFFFFFFULL
 * - Mask = 0XFFFFFFFFFFFFULL
 * - MatchCtrl = 0x0002
 * To receive anonymous IEEE1609 heartbeat (multicast) frames:
 * - Addr = 0X000000000000ULL
 * - Mask = 0XFFFFFFFFFFFFULL
 * - MatchCtrl = 0x0002
 * To receive valid unicast frames for 01:23:45:67:89:AB (our MAC address)
 * - Addr = 0XAB8967452301ULL
 * - Mask = 0XFFFFFFFFFFFFULL
 * - MatchCtrl = 0x0003
 * To monitor the channel in promiscious mode (including failed FCS frames):
 * - Addr = 0X000000000000ULL
 * - Mask = 0X000000000000ULL
 * - MatchCtrl = 0x0006
 */
typedef struct MKxAddressMatching
{
#ifndef LLC_NO_BITFIELDS
  /// 48 bit mask to apply to DA before comapring with Addr field
  uint64_t Mask:48;
  uint64_t :0; // Align to 64 bit boundary

  /// 48 bit MAC address to match after masking
  uint64_t Addr:48;
  /// Bitmask see @ref eMKxAddressMatchingCtrl
  uint64_t MatchCtrl:4;
  uint64_t :0; // Align to 64 bit boundary
#else
  uint8_t Mask[6];
  uint16_t Reserved0;
  uint8_t Addr[6];
  uint16_t MatchCtrl;
#endif
} tMKxAddressMatching;

/// Transmit queues (in priority order, where lowest is highest priority)
typedef enum
{
  MKX_TXQ_NON_QOS = 0, ///< Non QoS (for WSAs etc.)
  MKX_TXQ_AC_VO = 1,   ///< Voice
  MKX_TXQ_AC_VI = 2,   ///< Video
  MKX_TXQ_AC_BK = 3,   ///< Background
  MKX_TXQ_AC_BE = 4,   ///< Best effort
  /// For array dimensioning
  MKX_TXQ_COUNT,
  /// For bounds checking
  MKX_TXQ_MAX = MKX_TXQ_COUNT - 1,
} eMKxTxQueue;
/// @copydoc eMKxTxQueue
typedef uint8_t tMKxTxQueue;

/// MKx transmit queue configuration
typedef struct MKxTxQConfig
{
  /// Arbitration inter-frame-spacing (values of 0 to 16)
  uint8_t AIFS;
  /// Contention window min
  uint8_t CWMIN;
  /// Contention window max
  uint16_t CWMAX;
  /// TXOP duration limit [ms]
  uint16_t TXOP;
} __attribute__((__packed__)) tMKxTxQConfig;


/// PHY specific config
typedef struct MKxChanConfigPHY
{
  /// The channel centre frequency that should be used e.g. 5000 + (5 * 172)
  tMKxChannelFreq ChannelFreq;
  /// Indicate if channel is 10 MHz or 20 MHz
  tMKxBandwidth Bandwidth;
  /// Default Transmit antenna configuration
  /// (can be overridden in @ref tMKxTxPacket)
  /// Antenna selection used for transmission of ACK/CTS
  tMKxAntenna TxAntenna;
  /// Receive antenna configuration
  tMKxAntenna RxAntenna;
  /// Indicate the default data rate that should be used
  tMKxMCS DefaultMCS;
  /// Indicate the default transmit power that should be used
  /// Power setting used for Transmission of ACK/CTS
  tMKxPower DefaultTxPower;
} __attribute__((__packed__)) tMKxChanConfigPHY;

/// MAC specific config
typedef struct MKxChanConfigMAC
{
  /// Dual Radio transmit control (inactive in single radio configurations)
  tMKxDualTxControl DualTxControl;
  /// The RSSI power detection threshold for carrier sense [dBm]
  int8_t CSThreshold;
  /// Slot time/duration, per 802.11-2007
  uint16_t SlotTime;
  /// Distributed interframe space, per 802.11-2007
  uint16_t DIFSTime;
  /// Short interframe space, per 802.11-2007
  uint16_t SIFSTime;
  /// Duration to wait after an erroneously received frame,
  /// before beginning slot periods
  /// @note this should be set to EIFS - DIFS
  uint16_t EIFSTime;
  /// Per queue configuration
  tMKxTxQConfig TxQueue[MKX_TXQ_COUNT];
  /// Address matching filters: DA, broadcast, unicast & multicast
  tMKxAddressMatching AMSTable[AMS_TABLE_COUNT];
  /// Maximum number retries for unicast transmission
  uint16_t MaxRetries;
} __attribute__((__packed__)) tMKxChanConfigMAC;

/// LLC (WMAC) specific config
typedef struct MKxChanConfigLLC
{
  /// Duration of this channel interval, in microseconds. Zero means forever.
  uint32_t IntervalDuration;
  /// Duration of guard interval upon entering this channel, in microseconds
  uint32_t GuardDuration;
} __attribute__((__packed__)) tMKxChanConfigLLC;

/**
 * MKx channel configuration
 */
typedef struct MKxChanConfig
{
  /// PHY specific config
  struct MKxChanConfigPHY PHY;
  /// MAC specific config
  struct MKxChanConfigMAC MAC;
  /// LLC (WMAC) specific config
  struct MKxChanConfigLLC LLC;
} __attribute__((__packed__)) tMKxChanConfig;

typedef enum
{
  /// Radio is off
  MKX_MODE_OFF = 0,
  /// Radio is using channel config 0 configuration only
  MKX_MODE_CHANNEL_0 = 1,
  /// Radio is enabled to use channel config 1 configuration only
  MKX_MODE_CHANNEL_1 = 2,
  /// Radio is enabled to channel switch between config 0 & config 1 configs
  MKX_MODE_SWITCHED = 3,
} eRadioMode;
/// @copydoc eRadioMode
typedef uint16_t tMKxRadioMode;

/// MKx per radio configuration
typedef struct MKxRadioConfigData
{
  /// Operation mode of the radio
  tMKxRadioMode Mode;
  /// Reserved (for 32 bit alignment)
  uint16_t Reserved0;
  /// Channel Configurations for this radio
  tMKxChanConfig ChanConfig[MKX_CHANNEL_COUNT];
} __attribute__((__packed__)) tMKxRadioConfigData;

/**
 * MKx configuration message format.
 */
typedef struct MKxRadioConfig
{
  /// Interface Message Header
  tMKxIFMsg Hdr;
  /// Radio configuration data
  tMKxRadioConfigData RadioConfigData;
} __attribute__((__packed__)) tMKxRadioConfig;

/// Tx Queue stats counters
typedef struct MKxTxQueueStats
{
  /// Number of frames submitted via MKx_TxReq() to the current queue
  uint32_t    TxReqCount;
  /// Number of frames successfully transmitted (excluding retries)
  uint32_t    TxCnfCount;
  /// Number of frames un-successfully transmitted (excluding retries)
  uint32_t    TxErrCount;
  /// Number of packets transmitted on the channel (including retries)
  uint32_t    TxValid;
  /// Number of packets in the queue
  uint32_t    TxPending;
} tMKxTxQueueStats;

/// Channel stats counters
typedef struct MKxChannelStats
{
  /// Number of frames submitted via MKx_TxReq()
  uint32_t    TxReq;
  /// Number of Tx frames discarded by the MKx
  uint32_t    TxFail;
  /// Number of frames successfully transmitted (excluding retries)
  uint32_t    TxCnf;
  /// Number of frames un-successfully transmitted (excluding retries)
  uint32_t    TxErr;
  /// Number of packets transmitted on the channel (including retries)
  uint32_t    TxValid;
  /// Number of frames delivered via MKx_RxInd()
  uint32_t    RxInd;
  /// Number of Rx frames discarded by the MKx
  uint32_t    RxFail;
  /// Total number of duplicate (unicast) packets received on the channel
  uint32_t    RxDup;
  /// Per queue statistics
  tMKxTxQueueStats TxQueue[MKX_TXQ_COUNT];
  /// Proportion of time which the radio is considered busy over the last
  /// measurement period. (255 = 100%)
  uint8_t ChannelBusyRatio;
  /// Average idle period power [dBm]
  int8_t AverageIdlePower;
} tMKxChannelStats;

/// Radio level stats counters
typedef struct MKxRadioStatsData
{
  /// Per channel context statistics
  tMKxChannelStats Chan[MKX_CHANNEL_COUNT];
  /// TSF timer value at the end of the last measurement period [us]
  tMKxTSF TSF;
} __attribute__((__packed__)) tMKxRadioStatsData;

/**
 * MKx Radio stats format
 */
typedef struct MKxRadioStats
{
  /// Interface Message Header
  tMKxIFMsg Hdr;
  /// Radio Stats Data
  tMKxRadioStatsData RadioStatsData;
} __attribute__((__packed__)) tMKxRadioStats;

/// PHY Calibration and configuration structure
typedef struct DCOffset
{
  /// The DC offset I correction component per WISPA gain setting
  int32_t I;
  /// The DC offset Q correction component per WISPA gain setting
  int32_t Q;
} tDCOffset;

/// PHY Calibration and configuration structure
typedef struct TxPHYCalConfig
{
  /// The IQ imbalance correction factor K1
  int32_t IQCorrK1;
  /// The IQ imbalance correction factor K2
  int32_t IQCorrK2;
  /// The DC offset components per WISPA gain setting
  tDCOffset DCOffset[WISPA_TX_GAIN_CNT];
} tTxPHYCalConfig;

/// MKx Cal message type enumeration
typedef enum
{
  /// Request autocal
  MKX_CAL_REQUEST = 0,
  /// Reply to autocal request
  MKX_CAL_CNF = 1,
  /// Reply after autocal is completed
  MKX_CAL_IND = 2,
} eMKxCalType;
/// @copydoc eMKxCalMsgType
typedef uint32_t tMKxCalMsgType;

/// MKx Cal message sub type enumeration
typedef enum
{
  /// Requesting Cal and not supplying initial parameters
  MKX_CAL_SUB_REQUEST_FULL_NOT_SUPPLYING_START_PARAMETERS = 0,
  /// Requesting Cal and supplying initial parameters
  MKX_CAL_SUB_REQUEST_FULL_SUPPLYING_START_PARAMETERS = 1,
  /// Requesting adjustment to Cal and not supplying initial parameters
  MKX_CAL_SUB_REQUEST_ADJUST_NOT_SUPPLYING_START_PARAMETERS = 2,
  /// Requesting adjustment to Cal and supplying initial parameters
  MKX_CAL_SUB_REQUEST_ADJUST_SUPPLYING_START_PARAMETERS = 3,
  /// Confirmation message that the Cal request was accepted or not
  MKX_CAL_SUB_CNF = 4,
  /// Indication message with the output Cal data
  MKX_CAL_SUB_IND_WITH_DATA = 5,
  /// Indication message without the output Cal data
  MKX_CAL_SUB_IND_WITHOUT_DATA = 6,
} eMKxCalMsgSubType;
/// @copydoc eMKxCalMsgSubType
typedef uint32_t tMKxCalMsgSubType;

/// MKx Cal indication
typedef struct MKxCalMsgInd
{
  /// Interface Message Header
  tMKxIFMsg Hdr;
  /// Message type
  tMKxCalMsgType Type;
  /// Message sub-type
  tMKxCalMsgSubType SubType;
  /// Determined Ant1 calibration values
  tTxPHYCalConfig TxAnt1;
  /// Determined Ant2 calibration values
  tTxPHYCalConfig TxAnt2;
} __attribute__((__packed__)) tMKxCalMsgInd;

/// Cal Confirmation message
typedef struct MKxCalMsgCnf
{
  /// Interface Message Header
  tMKxIFMsg Hdr;
  /// Message type
  tMKxCalMsgType Type;
  /// Message sub-type
  tMKxCalMsgSubType SubType;
} __attribute__((__packed__)) tMKxCalMsgCnf;

/// Cal Request message
typedef struct MKxCalMsgReq
{
  /// Interface Message Header
  tMKxIFMsg Hdr;
  /// Message type
  tMKxCalMsgType Type;
  /// Message sub-type
  tMKxCalMsgSubType SubType;
  /// Optional Ant1 starting point for Cal (depending on Cal Type requested)
  tTxPHYCalConfig TxAnt1;
  /// Optional Ant2 starting point for Cal (depending on Cal Type requested)
  tTxPHYCalConfig TxAnt2;
} __attribute__((__packed__)) tMKxCalMsgReq;

/// Cal message (generic across all Cal message types)
typedef struct MKxCalMsgData
{
  /// Message type
  tMKxCalMsgType Type;
  /// Message sub-type
  tMKxCalMsgSubType SubType;
  /// Data
  uint8_t Data[];
} __attribute__ ((packed)) tMKxCalMsgData;

/// Cal message (including the MKxIFMsg)
typedef struct MKxCalMsg
{
  /// Interface Message Header
  tMKxIFMsg Hdr;
  /// Cal message data
  tMKxCalMsgData CalMsgData;
} __attribute__((__packed__)) tMKxCalMsg;

/**
 * C2X security command message
 * +------+------+------+------+------+---...---+------+
 * | CLA  | INS  | USN0 | USN1 |  LC  | Payload |  LE  |
 * +------+------+------+------+------+---...---+------+
 *    1      1       1      1      1      'LC'      1
 */
typedef struct MKxC2XSecCmd
{
  /// CLA - Class Byte.
  uint8_t CLA;
  /// INS - Instruction Byte.
  uint8_t INS;
  /// USN0,USN1 - 2 Byte USN field.P1 and P2 fields are reused for USN
  uint8_t USN[2];
  /// LC - Length in Bytes of the payload. (0x01 ... 0xFF)
  uint8_t LC;
  /// Payload - Command Payload.
  uint8_t Payload[0];
  /// LE - Expected Length of response. (0x01 ... 0xFF)
  uint8_t LE;
} __attribute__((__packed__)) tMKxC2XSecCmd;

/**
 * C2X security response message
 * +------+------+---...---+------+------+
 * | USN0 | USN1 | Payload |  SW1 |  SW2 |
 * +------+------+---...---+------+------+
 *    1      1       'LE'      1      1
 */
typedef struct MKxC2XSecRsp
{
  /// USN0, USN1
  uint8_t USN[2];
  /// Payload - Response Payload.
  uint8_t Payload[0];
  /// SW1,SW2 indicate the response code.
  /// Refer to C2X Security API document for valid error codes
  uint8_t SW[2];
} __attribute__((__packed__)) tMKxC2XSecRsp;

/**
 * C2X security message
 */
typedef struct MKxC2XSecAPDU
{
  union {
    /// Command APDU
    tMKxC2XSecCmd Cmd;
    /// Response APDU
    tMKxC2XSecRsp Rsp;
    /// Message data
    uint8_t Data[1];
  };
} __attribute__((__packed__)) tMKxC2XSecAPDU;

/**
 * C2X security request/indication
 */
typedef struct MKxC2XSec
{
  /// Interface Message Header (reserved area for LLC usage)
  tMKxIFMsg Hdr;
  /// Reserved/unused bytes to ensure the APDU is 64bit aligned
  uint8_t Align[3];
  /// C2X Security API APDU
  tMKxC2XSecAPDU APDU;
} __attribute__((__packed__)) tMKxC2XSec;

//------------------------------------------------------------------------------
// Function Types
//------------------------------------------------------------------------------

/**
 * @brief Request the configuration of a particular radio channel
 * @param pMKx MKx handle
 * @param Radio the selected radio
 * @param pConfig Pointer to the new configuration to apply
 * @return MKXSTATUS_SUCCESS if the request was accepted
 *
 * @code
 * // Get the current/default config
 * tMKxRadioConfig Cfg = {0,};
 * memcpy(&Cfg, &(pMKx->Config.Radio[MKX_RADIO_A]), sizoef(Cfg));
 * // Update the values that we want to change
 * Cfg.Mode = MKX_MODE_SWITCHED
 * Cfg.Chan[MKX_CHANNEL_0].PHY.ChannelFreq = 5000 + (5 * 178)
 * Cfg.Chan[MKX_CHANNEL_1].PHY.ChannelFreq = 5000 + (5 * 182)
 * ...
 * // Apply the configuration
 * Res = MKx_Config(pMKx, MKX_RADIO_A, &Cfg);
 * @endcode
 */
typedef tMKxStatus (*fMKx_Config) (struct MKx *pMKx,
                                   tMKxRadio Radio,
                                   tMKxRadioConfig *pConfig);

/**
 * @brief Request the transmission of an 802.11 frame
 * @param pMKx MKx handle
 * @param pTxPkt The packet pointer (including tx header)
 * @param pPriv Pointer to provide when invoking the @ref fMKx_TxCnf callback
 * @return MKXSTATUS_SUCCESS if the transmit request was accepted
 *
 * @note The buffer must lie in DMA accessible memory and there is usually some
 * relation between pTxPkt and pPriv. A possible stack implementation is
 * shown below:
 * @code
 * Len = <802.11 Frame length> + sizeof(struct MKxTxDescriptor);
 * pSkb = alloc_skb(Len, GFP_DMA);
 * pTxPkt = pSkb->data;
 * pTxPkt->Length = <802.11 Frame length>
 *  ...
 * Res = MKx_TxReq(pMKx, pTxPkt, pSkb);
 * @endcode
 */
typedef tMKxStatus (*fMKx_TxReq) (struct MKx *pMKx,
                                  tMKxTxPacket *pTxPkt,
                                  void *pPriv);

/**
 * @brief Transmit notification callback
 * @param pMKx MKx handle
 * @param pTxPkt As provided in the @ref fMKx_TxReq call
 * @param pPriv As provided in the @ref fMKx_TxReq call
 * @param Result Success: >=0 see @ref eMKxTxStatus , Fail: <0 see @ref eMKxStatus
 * @return MKXSTATUS_SUCCESS if the 'confirm' was successfully handled.
 *         Other values are logged for debug purposes.
 *
 * A callback invoked by the LLC to notify the stack that the provided
 * transmit packet was either successfully transmitted or failed to be
 * queued/transmitted.
 *
 * Contiuing the example from @ref fMKx_TxReq...
 * @code
 * {
 *   ...
 *   free_skb(pPriv);
 *   return MKXSTATUS_SUCCESS;
 * }
 * @endcode
 */
typedef tMKxStatus (*fMKx_TxCnf) (struct MKx *pMKx,
                                  tMKxTxPacket *pTxPkt,
                                  void *pPriv,
                                  tMKxStatus Result);

/**
 * @brief Flush all pending transmit packets
 * @param pMKx MKx handle
 * @param Radio The specific radio (MKX_RADIO_A or MKX_RADIO_B)
 * @param Chan The specific channel (MKX_CHANNEL_0 or MKX_CHANNEL_0)
 * @param TxQ The specific queue (MKX_TXQ_COUNT for all)
 * @return MKXSTATUS_SUCCESS if the flush request was accepted
 *
 */
typedef tMKxStatus (*fMKx_TxFlush) (struct MKx *pMKx,
                                    tMKxRadio Radio,
                                    tMKxChannel Chan,
                                    tMKxTxQueue TxQ);

/**
 * @brief Callback invoked by the LLC to allocate a receive packet buffer
 * @param pMKx MKx handle
 * @param BufLen Maximum length of the receive packet
 * @param ppBuf Pointer to a to-be-allocated buffer for the receive packet.
 *              In the case of an error: *ppBuf == NULL
 * @param ppPriv Pointer to provide when invoking any callback associated with
 *               this receive packet. Usually the provided contents of ppBuf
 *               and ppPriv have some association
 * @return MKXSTATUS_SUCCESS if the receive packet allocation request was
 *         successful. Other values may be logged by the MKx for debug purposes.
 *
 * A callback invoked by the LLC in an interrupt context to request the
 * stack to allocate a receive packet buffer.
 *
 * @note The buffer must lie in DMA accessible memory.
 * A possibleimplementation is shown below:
 * @code
 * *ppPriv = alloc_skb(BufLen, GFP_DMA|GFP_ATOMIC);
 * *ppBuf = (*ppPriv)->data;
 * @endcode
 *
 */
typedef tMKxStatus (*fMKx_RxAlloc) (struct MKx *pMKx,
                                    int BufLen,
                                    uint8_t **ppBuf,
                                    void **ppPriv);

/**
 * @brief A callback invoked by the LLC to deliver a receive packet buffer to
 *        the stack
 * @param pMKx MKx handle
 * @param pRxPkt Pointer to the receive packet.
 *            (same as @c *ppBuf provided in @ref MKx_RxAlloc)
 * @param pPriv Private packet pointer
 *             (same as provided in @ref fMKx_RxAlloc)
 * @return MKXSTATUS_SUCCESS if the receive packet allocation delivery was
 *         successful. Other values may be logged by the MKx for debug purposes.
 *
 */
typedef tMKxStatus (*fMKx_RxInd) (struct MKx *pMKx,
                                  tMKxRxPacket *pRxPkt,
                                  void *pPriv);

/// Signalled notifications via MKx_NotifInd()
typedef enum
{
  // Useful masks
  MKX_NOTIF_MASK_ERROR    = 0x8000000, ///< Error
  MKX_NOTIF_MASK_UTC      = 0x4000000, ///< UTC boundary (PPS)
  MKX_NOTIF_MASK_STATS    = 0x2000000, ///< Statistics updated
  MKX_NOTIF_MASK_ACTIVE   = 0x1000000, ///< Radio channel active
  MKX_NOTIF_MASK_RADIOA   = 0x0000010, ///< Specific to radio A
  MKX_NOTIF_MASK_RADIOB   = 0x0000020, ///< Specific to radio B
  MKX_NOTIF_MASK_CHANNEL0 = 0x0000001, ///< Specific to channel 0
  MKX_NOTIF_MASK_CHANNEL1 = 0x0000002, ///< Specific to channel 1
  /// No notification
  MKX_NOTIF_NONE          = 0x0000000,
  /// Active: Radio A, Channel 0
  MKX_NOTIF_ACTIVE_A0     = MKX_NOTIF_MASK_ACTIVE | MKX_NOTIF_MASK_RADIOA |
                            MKX_NOTIF_MASK_CHANNEL0,
  /// Active: Radio A, Channel 1
  MKX_NOTIF_ACTIVE_A1     = MKX_NOTIF_MASK_ACTIVE | MKX_NOTIF_MASK_RADIOA |
                            MKX_NOTIF_MASK_CHANNEL1,
  /// Active: Radio B, Channel 0
  MKX_NOTIF_ACTIVE_B0     = MKX_NOTIF_MASK_ACTIVE | MKX_NOTIF_MASK_RADIOB |
                            MKX_NOTIF_MASK_CHANNEL0,
  /// Active: Radio B, Channel 1
  MKX_NOTIF_ACTIVE_B1     = MKX_NOTIF_MASK_ACTIVE | MKX_NOTIF_MASK_RADIOB |
                            MKX_NOTIF_MASK_CHANNEL1,
  /// Stats updated: Radio A, Channel 0
  MKX_NOTIF_STATS_A0      = MKX_NOTIF_MASK_STATS  | MKX_NOTIF_MASK_RADIOA |
                            MKX_NOTIF_MASK_CHANNEL0,
  /// Stats updated: Radio A, Channel 1
  MKX_NOTIF_STATS_A1      = MKX_NOTIF_MASK_STATS  | MKX_NOTIF_MASK_RADIOA |
                            MKX_NOTIF_MASK_CHANNEL1,
  /// Stats updated: Radio B, Channel 0
  MKX_NOTIF_STATS_B0      = MKX_NOTIF_MASK_STATS  | MKX_NOTIF_MASK_RADIOB |
                            MKX_NOTIF_MASK_CHANNEL0,
  /// Stats updated: Radio B, Channel 1
  MKX_NOTIF_STATS_B1      = MKX_NOTIF_MASK_STATS  | MKX_NOTIF_MASK_RADIOB |
                            MKX_NOTIF_MASK_CHANNEL1,
  /// UTC second boundary
  MKX_NOTIF_UTC           = MKX_NOTIF_MASK_UTC,
  /// Error
  MKX_NOTIF_ERROR         = MKX_NOTIF_MASK_ERROR,
} eMKxNotif;
/// @copydoc eMKxNotif
typedef uint32_t tMKxNotif;

/**
 * @brief MKx notification callback
 * @param pMKx MKx handle
 * @param Notif The notification
 * @return MKXSTATUS_SUCCESS if the 'notif' was successfully handled.
 *         Other values are logged for debug purposes.
 *
 * Notification that the
 *  - Radio has encountered a UTC boundary
 *  - Channel is now active
 *  - Radio/Channel has expereinced an error
 */
typedef tMKxStatus (*fMKx_NotifInd) (struct MKx *pMKx,
                                     tMKxNotif Notif);


/**
 * @brief Get the underlying MKx TSF.
 * @param pMKx MKx handle
 * @return The TSF counter value
 *
 */
typedef tMKxTSF (*fMKx_GetTSF) (struct MKx *pMKx);

/**
 * @brief Set the MKx TSF at the next PPS
 * @param pMKx MKx handle
 * @param TSF The new TSF counter value to apply at the next PPS
 * @return MKXSTATUS_SUCCESS (0) or a negative error code @sa eMKxStatus
 *
 */
typedef tMKxStatus (*fMKx_SetTSF) (struct MKx *pMKx, tMKxTSF TSF);


/**
 * @brief A function invoked by the stack to deliver a C2X APDU buffer to the SAF5100
 * @param pMKx MKx handle
 * @param pMsg Pointer to the buffer.
 * @return MKXSTATUS_SUCCESS if the buffer was sent successful.
 *         Other values may be logged by the MKx for debug purposes.
 *
 * @note This function blocks until the buffer is sent on-the-wire
 */
typedef tMKxStatus (*fC2XSec_CommandReq) (struct MKx *pMKx,
                                          tMKxC2XSec *pMsg);
/**
 * @brief A callback invoked by the LLC to deliver a C2X APDU buffer to the stack
 * @param pMKx MKx handle
 * @param pMsg Pointer to the buffer.
 * @return MKXSTATUS_SUCCESS if the receive packet allocation delivery was
 *         successful. Other values may be logged by the MKx for debug purposes.
 *
 * @note pBuf must be handled (or copied) in the callback
 */
typedef tMKxStatus (*fC2XSec_ReponseInd) (struct MKx *pMKx,
                                          tMKxC2XSec *pMsg);



/**
 * @brief A function invoked by the stack to deliver a debug buffer to the MKx
 * @param pMKx MKx handle
 * @param pMsg Pointer to the buffer.
 * @return MKXSTATUS_SUCCESS if the buffer was sent successful.
 *         Other values may be logged by the MKx for debug purposes.
 *
 * @note This function blocks until the buffer is sent on-the-wire
 */
typedef tMKxStatus (*fMKx_DebugReq) (struct MKx *pMKx,
                                     struct MKxIFMsg *pMsg);
/**
 * @brief A callback invoked by the LLC to deliver a debug buffer to the stack
 * @param pMKx MKx handle
 * @param pMsg Pointer to the buffer.
 * @return MKXSTATUS_SUCCESS if the receive packet allocation delivery was
 *         successful. Other values may be logged by the MKx for debug purposes.
 *
 * @note pBuf must be handled (or copied) in the callback
 */
typedef tMKxStatus (*fMKx_DebugInd) (struct MKx *pMKx,
                                     struct MKxIFMsg *pMsg);

//------------------------------------------------------------------------------
// Handle Structures
//------------------------------------------------------------------------------

/// MKx LLC status information (including statitstics)
typedef struct MKxState
{
  /// Statistics (read only)
  tMKxRadioStats Stats[MKX_RADIO_COUNT];
} tMKxState;

/// Global MKx MKx API functions
typedef struct MKxFunctions
{
  fMKx_Config Config;
  fMKx_TxReq TxReq;
  fMKx_GetTSF GetTSF;
  fMKx_SetTSF SetTSF;
  fMKx_TxFlush TxFlush;
  fMKx_DebugReq DebugReq;
  fC2XSec_CommandReq C2XSecCmd;
} tMKxFunctions;

/// Global MKx MKx API callbacks (set by the stack)
typedef struct MKxCallbacks
{
  fMKx_TxCnf TxCnf;
  fMKx_RxAlloc RxAlloc;
  fMKx_RxInd RxInd;
  fMKx_NotifInd NotifInd;
  fMKx_DebugInd DebugInd;
  fC2XSec_ReponseInd C2XSecRsp;
} tMKxCallbacks;

/// MKx API functions and callbacks
typedef struct MKxAPI
{
  /// Stack -> SDR
  tMKxFunctions Functions;
  /// SDR -> Stack
  tMKxCallbacks Callbacks;
} tMKxAPI;

/// MKx LLC configuration
typedef struct MKxConfig
{
  /// Configuration (read only)
  tMKxRadioConfigData Radio[MKX_RADIO_COUNT];
} tMKxConfig;

/// MKx LLC handle
typedef struct MKx
{
  /// 'Magic' value used as an indicator that the handle is valid
  uint32_t Magic;
  /// Private data reference (for the stack to store stuff)
  void *pPriv;
  /// State information (read only)
  const tMKxState State;
  /// Configuration (read only)
  const tMKxConfig Config;
  /// MKx API functions and callbacks
  struct MKxAPI API;
} tMKx;

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

/**
 * @brief Initialize the LLC and get a handle
 * @param ppMKx MKx handle to initilize
 * @return MKXSTATUS_SUCCESS (0) or a negative error code @sa eMKxStatus
 *
 * This function will:
 *  - Optionally reset and download the SDR firmware
 *   - The SDR firmware image may be complied into the driver as a binary object
 *  - Intialize the USB or UDP interface
 */
tMKxStatus MKx_Init (tMKx **ppMKx);

/**
 * @brief De-initialize the LLC
 * @param pMKx MKx handle
 * @return MKXSTATUS_SUCCESS (0) or a negative error code @sa eMKxStatus
 *
 */
tMKxStatus MKx_Exit (tMKx *pMKx);


/**
 * @copydoc fMKx_Config
 */
static INLINE tMKxStatus MKx_Config (tMKx *pMKx,
                                     tMKxRadio Radio,
                                     tMKxRadioConfig *pConfig)
{
  if (pMKx == NULL)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;
  if (pMKx->Magic != MKX_API_MAGIC)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;

  if (Radio > MKX_RADIO_MAX)
    return MKXSTATUS_FAILURE_INVALID_PARAM;
  if (pConfig == NULL)
    return MKXSTATUS_FAILURE_INVALID_PARAM;

  return pMKx->API.Functions.Config(pMKx, Radio, pConfig);
}

/**
 * @copydoc fMKx_TxReq
 */
static INLINE tMKxStatus MKx_TxReq (tMKx *pMKx,
                                    tMKxTxPacket *pTxPkt,
                                    void *pPriv)
{
  if (pMKx == NULL)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;
  if (pMKx->Magic != MKX_API_MAGIC)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;

  return pMKx->API.Functions.TxReq(pMKx, pTxPkt, pPriv);
}

/**
 * @copydoc fMKx_TxFlush
 */
static INLINE tMKxStatus MKx_TxFlush (tMKx *pMKx,
                                      tMKxRadio Radio,
                                      tMKxChannel Chan,
                                      tMKxTxQueue TxQ)
{
  if (pMKx == NULL)
    return 0;
  if (pMKx->Magic != MKX_API_MAGIC)
    return 0;

  return pMKx->API.Functions.TxFlush(pMKx, Radio, Chan, TxQ);
}

/**
 * @copydoc fMKx_GetTSF
 */
static INLINE tMKxTSF MKx_GetTSF (tMKx *pMKx)
{
  if (pMKx == NULL)
    return 0;
  if (pMKx->Magic != MKX_API_MAGIC)
    return 0;

  return pMKx->API.Functions.GetTSF(pMKx);
}

/**
 * @copydoc fMKx_SetTSF
 */
static INLINE tMKxStatus MKx_SetTSF (tMKx *pMKx, tMKxTSF TSF)
{
  if (pMKx == NULL)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;
  if (pMKx->Magic != MKX_API_MAGIC)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;

  return pMKx->API.Functions.SetTSF(pMKx, TSF);
}

/**
 * @brief Helper function to read the MKx statistics
 * @param Radio the selected radio
 * @param pStats Storage to place the radio's statistics in
 */
static INLINE tMKxStatus MKx_GetStats (struct MKx *pMKx,
                                       tMKxRadio Radio,
                                       tMKxRadioStats *pStats)
{
  if (pMKx == NULL)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;
  if (pMKx->Magic != MKX_API_MAGIC)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;

  if (Radio > MKX_RADIO_MAX)
    return MKXSTATUS_FAILURE_INVALID_PARAM;
  if (pStats == NULL)
    return MKXSTATUS_FAILURE_INVALID_PARAM;

  memcpy(pStats, &(pMKx->State.Stats[Radio]), sizeof(tMKxRadioStats));

  return MKXSTATUS_SUCCESS;
}


#endif // #ifndef __LINUX__COHDA__LLC__LLC_API_H__

