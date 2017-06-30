#include <librtmp/rtmp.h>
#include <skalibs/skalibs.h>
#include <stdio.h>
#include <stdlib.h>

#define VERSION "1.0.0"
#define USAGE "rtmprelay src dest"
#define dieusage() strerr_dieusage(100, USAGE)

int main(int argc, char const *const *argv) {
    PROG = "sockexec";
    subgetopt_t l = SUBGETOPT_ZERO ;
    int opt = 0;

    RTMP *receiver = 0;
    RTMP *sender = 0;
    RTMPPacket *packet = 0;

    while((opt = subgetopt_r(argc,argv,"vh",&l)) > 0 )
    {
        switch(opt)
        {
            case 'v':
            {
                printf("%s version %s\n",PROG,VERSION);
                return 0;

            }
            case 'h':
            {
                dieusage();

            }

        }

    }

    argc -= l.ind ; argv += l.ind ;
    if(argc < 2) dieusage() ;

    receiver = RTMP_Alloc();
    if(receiver == 0) {
        strerr_diefu1sys(111, "failed to initialize rtmp library");
    }

    sender = RTMP_Alloc();
    if(sender == 0) {
        strerr_diefu1sys(111, "failed to initialize rtmp library");
    }

    packet = (RTMPPacket *)malloc(sizeof(RTMPPacket));
    if(sender == 0) {
        strerr_diefu1sys(111, "failed to initialize rtmp library");
    }

    RTMP_Init(receiver);
    RTMP_Init(sender);
    RTMPPacket_Reset(packet);

    if(!RTMP_SetupURL(receiver,(char *)argv[0])) {
        strerr_diefu1sys(111, "failed to setup RTMP URL");
    }

    if(!RTMP_SetupURL(sender,(char *)argv[1])) {
        strerr_diefu1sys(111, "failed to setup RTMP URL");
    }

    receiver->Link.lFlags |= RTMP_LF_LIVE;
    RTMP_EnableWrite(sender);

    if(!RTMP_Connect(receiver,NULL)) {
        strerr_diefu1sys(111, "failed to connect");
    }

    if(!RTMP_ConnectStream(receiver,0)) {
        strerr_diefu1sys(111, "failed to connect");
    }

    if(!RTMP_Connect(sender,NULL)) {
        strerr_diefu1sys(111, "failed to connect");
    }

    if(!RTMP_ConnectStream(sender,0)) {
        strerr_diefu1sys(111, "failed to connect");
    }

    while(RTMP_ReadPacket(receiver,packet)) {
        if(RTMPPacket_IsReady(packet)) {
            RTMP_SendPacket(sender,packet,0);
            RTMPPacket_Free(packet);
        }
    }

    RTMP_Close(receiver);
    RTMP_Close(sender);
    RTMP_Free(receiver);
    RTMP_Free(sender);

    return 0;
}
