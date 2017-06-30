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
    int i = 0;
    int num_senders = 0;

    RTMP *receiver = 0;
    RTMP **senders = 0;
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
    num_senders = argc - 1;

    receiver = RTMP_Alloc();
    if(receiver == 0) {
        strerr_diefu1sys(111, "failed to initialize rtmp library");
    }
    RTMP_Init(receiver);

    if(!RTMP_SetupURL(receiver,(char *)argv[0])) {
        strerr_diefu1sys(111, "failed to setup RTMP URL");
    }

    receiver->Link.lFlags |= RTMP_LF_LIVE;

    if(!RTMP_Connect(receiver,NULL)) {
        strerr_diefu1sys(111, "failed to connect");
    }

    if(!RTMP_ConnectStream(receiver,0)) {
        strerr_diefu1sys(111, "failed to connect");
    }

    senders = (RTMP **)malloc(sizeof(RTMP *) * num_senders);

    if(senders == 0) {
        strerr_diefu1sys(111, "failed to initialize rtmp library");
    }
    memset(senders,0,sizeof(RTMP *) * num_senders);

    for(i=0; i < num_senders; i++) {
        senders[i] = RTMP_Alloc();

        if(senders[i] == 0) {
            strerr_diefu1sys(111, "failed to initialize rtmp library");
        }

        RTMP_Init(senders[i]);

        if(!RTMP_SetupURL(senders[i],(char *)argv[i+1])) {
            strerr_diefu1sys(111, "failed to setup RTMP URL");
        }

        RTMP_EnableWrite(senders[i]);

        if(!RTMP_Connect(senders[i],NULL)) {
            strerr_diefu1sys(111, "failed to connect");
        }

        if(!RTMP_ConnectStream(senders[i],0)) {
            strerr_diefu1sys(111, "failed to connect");
        }
    }

    packet = (RTMPPacket *)malloc(sizeof(RTMPPacket));

    if(packet == 0) {
        strerr_diefu1sys(111, "failed to initialize rtmp library");
    }

    memset(packet,0,sizeof(RTMPPacket));

    RTMPPacket_Reset(packet);

    while(RTMP_ReadPacket(receiver,packet)) {
        if(RTMPPacket_IsReady(packet)) {
            if(RTMP_ClientPacket(receiver,packet) == 1) { /* Audio/Video/Metadata packet */
                for(i=0; i<num_senders; i++) {
                    RTMP_SendPacket(senders[i],packet,0);
                }
            }
            RTMPPacket_Free(packet);
        }
    }

    RTMP_Close(receiver);
    RTMP_Free(receiver);

    for(i=0; i<num_senders; i++) {
        RTMP_Close(senders[i]);
        RTMP_Free(senders[i]);
    }

    return 0;
}
