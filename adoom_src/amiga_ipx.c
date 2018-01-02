#include <dos.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <devices/sana2.h>
#include <exec/exec.h>
#include <proto/exec.h>

#include "amiga_ipx.h"
#include "i_system.h"

/**********************************************************************/
// This file isn't used in ADoom.
// It's rather buggy.
// It implements an crude IPX layer using any ethernet sana2 device.
// I wrote it out of frustration when I couldn't get amipx.library v1.0
// to work.  Peter Mcgavin, 5 Feb 1998.

/**********************************************************************/
struct request
{
    struct MinNode node;
    struct IPXECB *ecb;
    struct IOSana2Req *io;
    BOOL io_in_progress;
};

static struct MsgPort *sana2_mp = NULL;
static struct IOSana2Req *sana2_io = NULL;
static BOOL sana2_is_open = FALSE;
static UWORD the_socket = 0; /* we handle only one socket at a time */
static int the_packettype = 0;
static struct IPXAddress localaddress = {{'\0', '\0', '\0', '\0'}, {'\0', '\0', '\0', '\0', '\0', '\0'}, 0};
static struct request *req_to_relinquish;
static struct MinList req_list; /* list of possibly outstanding requests */

/**********************************************************************/
static int ecb_length(struct IPXECB *ecb)
{
    int i, len;

    len = 0;
    for (i = 0; i < ecb->FragCount; i++)
        len += ecb->Fragment[i].FragSize;
    return len;
}

/**********************************************************************/
static BOOL __asm __interrupt __saveds CopyFromBuff(  // send
    register __a0 UBYTE *to, register __a1 struct IPXECB *ecb, register __d0 LONG n)
{
    int i, len, rem;
    struct IPXPacketHeader *ph;

    ph = (struct IPXPacketHeader *)ecb->Fragment[0].FragData;

    *to++ = 0xe0; /* Ethernet 802.2 3-byte header */
    *to++ = 0xe0;
    *to++ = 0x03;
    n -= 3;
    len = ecb_length(ecb);
    if (n < len)
        len = n;

    *to++ = 0xff;      /* checksum[0] */
    *to++ = 0xff;      /* checksum[1] */
    *to++ = len >> 8;  /* length[0] */
    *to++ = len & 255; /* length[1] */
    *to++ = ph->Tc;
    *to++ = ph->Type;
    memcpy(to, &ph->Dst, sizeof(struct IPXAddress));
    to += sizeof(struct IPXAddress);
    memcpy(to, &localaddress, sizeof(struct IPXAddress));
    to += sizeof(struct IPXAddress);
    n -= sizeof(struct IPXPacketHeader);

    if (n > 0) {
        rem = ecb->Fragment[0].FragSize - sizeof(struct IPXPacketHeader);
        if (rem > 0) {
            if (n < rem)
                rem = n;
            memcpy(to, &ecb->Fragment[0].FragData[sizeof(struct IPXPacketHeader)], rem);
            to += rem;
            n -= rem;
        }

        for (i = 1; i < ecb->FragCount && n > 0; i++) {
            rem = ecb->Fragment[i].FragSize;
            if (n < rem)
                rem = n;
            memcpy(to, ecb->Fragment[i].FragData, rem);
            to += rem;
            n -= rem;
        }
    }

    /* invoke ESR here */
    ecb->InUse = 0;
    return TRUE;
}

/**********************************************************************/
static BOOL __asm __interrupt __saveds CopyToBuff(  // get
    register __a0 struct IPXECB *ecb, register __a1 UBYTE *from, register __d0 LONG n)
{
    int i, rem;

    from += 3; /* skip Ethernet 802.2 3-byte header */
    n -= 3;
    for (i = 0; i < ecb->FragCount && n > 0; i++) {
        rem = ecb->Fragment[i].FragSize;
        if (n < rem)
            rem = n;
        memcpy(ecb->Fragment[i].FragData, from, rem);
        from += rem;
        n -= rem;
    }
    memcpy(ecb->ImmedAddr, ((struct IPXPacketHeader *)ecb->Fragment[0].FragData)->Src.Node, 6);
    /* invoke ESR here */
    ecb->InUse = 0;
    return TRUE;
}

/**********************************************************************/
/*
static BOOL __asm __interrupt __saveds PacketFilter (
  register __a0 struct Hook *hook,
  register __a2 struct IOSana2Req *ios2,
  register __a1 APTR data)
{
  return TRUE;
}
*/

/**********************************************************************/
void Init_IPX(char *sana2devicename, int unit, int packettype)
{
    /*
      int i;
      static struct Hook packetfilterhook = {
        {NULL},
        (ULONG (*)())PacketFilter,
        NULL,
        NULL
      };
    */
    static struct TagItem ios2_tags[] = {{S2_CopyToBuff, (ULONG)CopyToBuff},
                                         {S2_CopyFromBuff, (ULONG)CopyFromBuff},
                                         /*{S2_PacketFilter, (ULONG)&packetfilterhook},*/
                                         {TAG_DONE, 0}};

    NewList((struct List *)&req_list);

    the_packettype = packettype;

    if ((sana2_mp = CreatePort(NULL, 0)) == NULL ||
        (sana2_io = (struct IOSana2Req *)CreateExtIO(sana2_mp, sizeof(struct IOSana2Req))) == NULL)
        I_Error("Can't create port");

    sana2_io->ios2_BufferManagement = ios2_tags;

    if (OpenDevice(sana2devicename, unit, (struct IORequest *)sana2_io, 0) != 0)
        I_Error("Can't open %s, unit %d", sana2devicename, unit);
    sana2_is_open = TRUE;

    sana2_io->ios2_Req.io_Command = S2_CONFIGINTERFACE;
    sana2_io->ios2_Req.io_Flags = 0; /* SANA2IOF_QUICK */
    memset(sana2_io->ios2_SrcAddr, 0, SANA2_MAX_ADDR_BYTES);
    DoIO((struct IORequest *)sana2_io);

    sana2_io->ios2_Req.io_Command = S2_GETSTATIONADDRESS;
    DoIO((struct IORequest *)sana2_io);
    if (sana2_io->ios2_Req.io_Error != 0)
        I_Error("Error %d getting station address", sana2_io->ios2_Req.io_Error);
    memcpy(localaddress.Node, sana2_io->ios2_SrcAddr, sizeof(localaddress.Node));
    /*
      printf ("Source address =");
      for (i = 0; i < SANA2_MAX_ADDR_BYTES; i++)
          printf (" %02x", sana2_io->ios2_SrcAddr[i]);
      printf ("\n");
      printf ("Dest address   =");
      for (i = 0; i < SANA2_MAX_ADDR_BYTES; i++)
          printf (" %02x", sana2_io->ios2_DstAddr[i]);
      printf ("\n");
    */
}

/**********************************************************************/
void Shutdown_IPX(void)
{
    struct request *req;
    struct MinNode *succ;

    if (sana2_is_open) {
        /* abort and free outstanding requests */
        req = (struct request *)req_list.mlh_Head;
        succ = req->node.mln_Succ;
        while (succ != NULL) {
            if (req->io_in_progress) {
                AbortIO((struct IORequest *)req->io);
                WaitIO((struct IORequest *)req->io);
                req->io_in_progress = FALSE;
                req->ecb->InUse = 0;
            }
            DeleteExtIO((struct IORequest *)req->io);
            Remove((struct Node *)&req->node);
            FreeMem(req, sizeof(struct request));
            req = (struct request *)succ;
            succ = req->node.mln_Succ;
        }
        CloseDevice((struct IORequest *)sana2_io);
        sana2_is_open = FALSE;
    }
    if (sana2_io != NULL) {
        DeleteExtIO((struct IORequest *)sana2_io);
        sana2_io = NULL;
    }
    if (sana2_mp != NULL) {
        DeletePort(sana2_mp);
        sana2_mp = NULL;
    }
}

/**********************************************************************/
UWORD IPXOpenSocket(UWORD socketid)
{
    if (the_socket != 0)
        return 0;
    the_socket = socketid;
    localaddress.Socket = socketid;
    return the_socket;
}

/**********************************************************************/
void IPXCloseSocket(UWORD socketid) { the_socket = 0; }
/**********************************************************************/
int IPXGetLocalTarget(struct IPXAddress *request, UBYTE reply[6])
{
    memcpy(reply, request->Node, 6);
    return 1;
}

/**********************************************************************/
void IPXSendPacket(struct IPXECB *ecb)
{
    struct request *req;
    struct IPXPacketHeader *ph;

    req = *(struct request **)ecb->IPXWork;
    if (req == NULL) {
        if ((req = (struct request *)AllocMem(sizeof(struct request), MEMF_CLEAR)) == NULL ||
            (req->io = (struct IOSana2Req *)CreateExtIO(sana2_mp, sizeof(struct IOSana2Req))) == NULL)
            I_Error("Can't create port or IO");
        *((struct request **)ecb->IPXWork) = req;
        req->io_in_progress = FALSE;
        *req->io = *sana2_io;
        req->io->ios2_Req.io_Flags = 0;                        /* SANA2IOF_RAW | SANA2IOF_QUICK */
        req->io->ios2_PacketType = the_packettype; /* 802.3 */ /* 0x8137 Novell */
                                                               /* 2049 Envoy IP, 2055 Envoy ARP */
        req->ecb = ecb;
        AddTail((struct List *)&req_list, (struct Node *)&req->node);
    } else if (req->io_in_progress) {
        WaitIO((struct IORequest *)sana2_io);
        req->io_in_progress = FALSE;
        ecb->InUse = 0;
    }
    ph = (struct IPXPacketHeader *)ecb->Fragment[0].FragData;
    if (ph->Dst.Node[0] == 0xff && ph->Dst.Node[1] == 0xff && ph->Dst.Node[2] == 0xff && ph->Dst.Node[3] == 0xff &&
        ph->Dst.Node[4] == 0xff && ph->Dst.Node[5] == 0xff) {
        req->io->ios2_Req.io_Command = S2_BROADCAST;
    } else {
        req->io->ios2_Req.io_Command = CMD_WRITE;
        memcpy(req->io->ios2_DstAddr, ph->Dst.Node, 6);
    }
    ph->Checksum = 0xffff;
    ph->Length = ecb_length(ecb);
    ;
    memcpy(&ph->Src, &localaddress, sizeof(localaddress));
    req->io->ios2_DataLength = 3 + ph->Length;
    req->io->ios2_Data = ecb;
    ecb->InUse = 0x1d;
    BeginIO((struct IORequest *)req->io);
    req->io_in_progress = TRUE;
    req_to_relinquish = req;
}

/**********************************************************************/
int IPXListenForPacket(struct IPXECB *ecb)
{
    struct request *req;

    if (ecb->InUse)
        I_Error("IPXListenForPacket(): ECB already InUse");

    req = *(struct request **)ecb->IPXWork;
    if (req == NULL) {
        if ((req = (struct request *)AllocMem(sizeof(struct request), MEMF_CLEAR)) == NULL ||
            (req->io = (struct IOSana2Req *)CreateExtIO(sana2_mp, sizeof(struct IOSana2Req))) == NULL)
            I_Error("Can't create port or IO");
        *((struct request **)ecb->IPXWork) = req;
        req->io_in_progress = FALSE;
        *req->io = *sana2_io;
        req->io->ios2_Req.io_Command = CMD_READ;
        req->io->ios2_Req.io_Flags = 0;                        /* SANA2IOF_RAW | SANA2IOF_QUICK */
        req->io->ios2_PacketType = the_packettype; /* 802.3 */ /* 0x8137 Novell */
                                                               /* 2049 Envoy IP, 2055 Envoy ARP */
        req->ecb = ecb;
        AddTail((struct List *)&req_list, (struct Node *)&req->node);
    } else
        WaitIO((struct IORequest *)req->io);
    req->io->ios2_DataLength = 3 + ecb_length(ecb);
    req->io->ios2_Data = ecb;
    ecb->InUse = 0x1d;
    BeginIO((struct IORequest *)req->io);
    req->io_in_progress = TRUE;

    return 0;
}

/**********************************************************************/
void ScheduleIPXEvent(struct IPXECB *ecb, int ticks) {}
/**********************************************************************/
int IPXCancelEvent(struct IPXECB *ecb)
{
    struct request *req;

    req = *(struct request **)ecb->IPXWork;
    if (req->io_in_progress) {
        AbortIO((struct IORequest *)req->io);
        WaitIO((struct IORequest *)req->io);
        req->io_in_progress = FALSE;
    }
    ecb->InUse = 0;
    return 0;
}

/**********************************************************************/
int IPXGetIntervalMarker(void)
{
    static int time = 0;

    time++;
    return time;
}

/**********************************************************************/
void IPXGetInternetworkAddress(struct IPXInternetworkAddress *reply) {}
/**********************************************************************/
void IPXRelinquishControl(void)
{
    if (!CheckIO((struct IORequest *)req_to_relinquish->io)) {
        WaitIO((struct IORequest *)req_to_relinquish->io);
        req_to_relinquish->io_in_progress = FALSE;
        req_to_relinquish->ecb->InUse = 0;
    }
}

/**********************************************************************/
void IPXDisconnectFromTarget(struct IPXAddress *request) {}
/**********************************************************************/
void IPXGetLocalAddr(struct IPXAddress *reply) { memcpy(reply, &localaddress, sizeof(struct IPXAddress)); }
/**********************************************************************/
void _STDcleanupIPX(void) { Shutdown_IPX(); }
/**********************************************************************/
