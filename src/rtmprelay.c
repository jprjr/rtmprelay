#include <librtmp/rtmp.h>
#include <librtmp/log.h>
#include <skalibs/skalibs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

#define VERSION "1.0.0"
#define USAGE "rtmprelay src dest [dest ...]"
#define dieusage() strerr_dieusage(100, USAGE)

int main(int argc, char const *const *argv) {
    PROG = "sockexec";
    subgetopt_t l = SUBGETOPT_ZERO ;
    int opt = 0;
    int i = 0;
    int num_senders = 0;
    int streaming = 1;
    const int fds_len = 2;
    iopause_fd fds[2];
    int events;
    int retval = 0;
    char buf[1];
    static struct termios oldt, newt;

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

    memset(fds,0,sizeof(iopause_fd)*fds_len);

    tcgetattr( STDIN_FILENO, &oldt);
    tcgetattr( STDIN_FILENO, &newt);

    newt.c_lflag &= ~(ICANON);
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);

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

    fds[0].fd = STDIN_FILENO;
    fds[0].events = IOPAUSE_READ;
    fds[0].revents = 0;

    fds[1].fd = RTMP_Socket(receiver);
    fds[1].events = IOPAUSE_READ;
    fds[1].revents = 0;

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

    while(streaming) {

        events = iopause(fds,fds_len,0,0);

        if(events <= 0) {
            retval = 1;
            break;
        }

        if (fds[0].revents & IOPAUSE_READ) {
            fds[0].revents = 0;
            if(fd_read(fds[0].fd,buf,1)) {
                if(buf[0] == 'q' || buf[0] == 'Q') {
                    streaming = 0;
                    continue;
                }
            }
        }

        if(! (fds[1].revents & IOPAUSE_READ) ) {
            continue;
        }

        if(!RTMP_ReadPacket(receiver,packet)) {
            break;
        }

        if(RTMPPacket_IsReady(packet)) {
            if(RTMP_ClientPacket(receiver,packet) == 1) { /* Audio/Video/Metadata packet */
                for(i=0; i<num_senders; i++) {
                    RTMP_SendPacket(senders[i],packet,0);
                }
            }
            else if(packet->m_packetType == RTMP_PACKET_TYPE_CONTROL &&
                    AMF_DecodeInt16(packet->m_body) == 1) {
                /* deleteStream */
                streaming = 0;
            }

            RTMPPacket_Dump(packet);
            RTMPPacket_Free(packet);
        }
    }

    RTMP_Close(receiver);
    RTMP_Free(receiver);

    for(i=0; i<num_senders; i++) {
        RTMP_Close(senders[i]);
        RTMP_Free(senders[i]);
    }

    tcsetattr( STDIN_FILENO, TCSANOW, &oldt);

    return retval;
}
