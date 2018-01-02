struct IPXInternetworkAddress {
  UBYTE Network[4];
  UBYTE Node[6];
};

struct IPXAddress {
  UBYTE Network[4];
  UBYTE Node[6];
  UWORD Socket;   /* big endian */
};

struct IPXPacketHeader {
  UWORD Checksum;
  UWORD Length;   /* big endian */
  UBYTE Tc;
  UBYTE Type;
  struct IPXAddress Dst;
  struct IPXAddress Src;
};

struct IPXFragment {
 UBYTE *FragData;
 UWORD FragSize;
};

struct IPXECB {
 APTR Link;
 APTR ESR;
 UBYTE InUse;
 UBYTE CompletionCode;   /* non-zero in case of error */
 WORD Socket;
 UBYTE IPXWork[4];	 /* private! */
 UBYTE DWork[12];        /* private! */
 UBYTE ImmedAddr[6];
 UWORD FragCount;
 struct IPXFragment Fragment[2];
};

void Init_IPX (char *sana2devicename, int unit, int packettype);

void Shutdown_IPX (void);

UWORD IPXOpenSocket (UWORD socketid);

void IPXCloseSocket (UWORD socketid);

int IPXGetLocalTarget (struct IPXAddress *request, UBYTE reply[6]);

void IPXSendPacket (struct IPXECB *ecb);

int IPXListenForPacket (struct IPXECB *ecb);

void ScheduleIPXEvent (struct IPXECB *ecb, int ticks);

int IPXCancelEvent (struct IPXECB *ecb);

int IPXGetIntervalMarker (void);

void IPXGetInternetworkAddress (struct IPXInternetworkAddress *reply);

void IPXRelinquishControl (void);

void IPXDisconnectFromTarget (struct IPXAddress *request);

void IPXGetLocalAddr (struct IPXAddress *reply);
