#include <pthread.h>
#include "wsdd.nsmap"
#include "soapStub.h"
#include "soapH.h"
#include "macro.h"
#include <stdio.h>
#include "onvif_server.h"

void *onvif_discovered_server(void *arg)
{
    struct soap soap_udp;
    int socked_fd;

    printf("[%s][%d][%s][%s] start \n", __FILE__, __LINE__, __TIME__, __func__);
    
    // 初始化 SOAP 服务器对象
    soap_init1(&soap_udp, SOAP_IO_UDP | SOAP_XML_IGNORENS);
    soap_udp.bind_flags = SO_REUSEADDR; // 允许重复绑定
    soap_udp.port = ONVIF_LISTEN_PORT;

    soap_set_namespaces(&soap_udp, namespaces);
    printf("[%s][%d][%s][%s] ServerSoap.version = %d \n", __FILE__, __LINE__, __TIME__, __func__, soap_udp.version);
   
    // 绑定端口
    socked_fd = soap_bind(&soap_udp, NULL, soap_udp.port, 10);
    if (socked_fd < 0)
    {
        printf("%d soap_bind failed\n", __LINE__);
        soap_print_fault(&soap_udp, stderr);
        goto end;
    }

    // 加入组播
    struct ip_mreq mreqcon;
    mreqcon.imr_multiaddr.s_addr = inet_addr("239.255.255.250");
    mreqcon.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(soap_udp.master, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreqcon, sizeof(mreqcon)) < 0)
    {
        printf("setsockopt error, error message: %s\n", strerror(errno));
        goto end;
    }

    while (1)
    {
        socked_fd = soap_serve(&soap_udp);
        if (socked_fd != SOAP_OK)
        {
            printf("%d soap_serve failed\n", __LINE__);
            soap_print_fault(&soap_udp, stderr);
        }

        // 客户端的IP地址
        printf("connection from IP = %d.%d.%d.%d socket = %d \n\n", 
               ((soap_udp.ip) >> 24) & 0xFF, ((soap_udp.ip) >> 16) & 0xFF, 
               ((soap_udp.ip) >> 8) & 0xFF, (soap_udp.ip) & 0xFF, (soap_udp.socket));


        soap_destroy(&soap_udp);
        soap_end(&soap_udp);
    }

end:
    soap_done(&soap_udp);
    return NULL;
}

// 处理 HTTP 请求的回调函数
int http_get(struct soap *soap)
{
    FILE *fd = NULL;
    static char buf[1024 * 5] = {0};

    fd = fopen("src/index.html", "rb");
    if (!fd)
        return 404;
    if (soap_response(soap, SOAP_HTML) == SOAP_OK) // HTTP response header with text/html
    {
        size_t r = fread(buf, 1, sizeof(buf), fd);
        if (r > 0)
            soap_send(soap, buf);
    }

    soap_end_send(soap);
    fclose(fd);

    return SOAP_OK;
}

// 处理 HTTP 服务的线程函数
void *onvif_http_server(void *arg)
{
    struct soap soap_tcp;
    int socked_fd;
    fd_set readfds;
    struct timeval timeout;

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    soap_init1(&soap_tcp, SOAP_XML_INDENT);
    soap_tcp.port = DEVICE_PORT;
    soap_tcp.bind_flags = SO_REUSEADDR;
    soap_tcp.send_timeout = 3; // send timeout is 3s
    soap_tcp.recv_timeout = 3; // receive timeout is 3s
    soap_tcp.fget = http_get;

    soap_set_namespaces(&soap_tcp, namespaces);

    socked_fd = soap_bind(&soap_tcp, DEVICE_IP, soap_tcp.port, 10);
    if (socked_fd < 0)
    {
        printf("%d soap_bind failed\n", __LINE__);
        soap_print_fault(&soap_tcp, stderr);
        goto end;
    }

    int socked_fd_new = -1;
    while (1)
    {
        socked_fd_new = soap_accept(&soap_tcp);
        if (!soap_valid_socket(socked_fd_new))
        {
            printf("soap_accept failed\n");
            soap_print_fault(&soap_tcp, stderr);
            goto end;
        }

        FD_ZERO(&readfds);
        FD_SET(socked_fd_new, &readfds);

        int activity = select(socked_fd_new + 1, &readfds, NULL, NULL, &timeout);
        if (activity < 0)
        {
            printf("select error\n");
            goto end;
        }

        if (FD_ISSET(socked_fd_new, &readfds))
        {
            if (soap_serve(&soap_tcp) != SOAP_OK)
            {
                printf("%d soap_serve failed, error: %d\n", __LINE__, soap_tcp.error);
                soap_print_fault(&soap_tcp, stderr);
            }
        }

        soap_destroy(&soap_tcp);
        soap_end(&soap_tcp);
    }

end:
    soap_end(&soap_tcp);
    soap_done(&soap_tcp);
    return NULL;
}

int onvif_server_init()
{
    printf("create_onvif_server success\n");
    pthread_t thread1, thread2;
    pthread_attr_t attr1, attr2;

    pthread_attr_init(&attr1);
    pthread_attr_init(&attr2);

    pthread_attr_setdetachstate(&attr1, PTHREAD_CREATE_DETACHED);
    pthread_attr_setdetachstate(&attr2, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&thread1, &attr1, onvif_discovered_server, NULL) != 0)
        return -1;
    if (pthread_create(&thread2, &attr2, onvif_http_server, NULL) != 0)
        return -1;

    pthread_attr_destroy(&attr1);
    pthread_attr_destroy(&attr2);

    return 0;
}
