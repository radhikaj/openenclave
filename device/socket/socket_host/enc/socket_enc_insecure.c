/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* Licensed under the MIT License. */
#include <errno.h>
#include <openenclave/bits/safecrt.h>
#include <openenclave/bits/socket.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/enclavelibc.h>
#include "socket_t.h"

typedef unsigned long u_long;

static void copy_input_fds(oe_fd_set_internal* dest, oe_provider_fd_set* src)
{
    unsigned int i;
    dest->fd_count = src->fd_count;
    for (i = 0; i < src->fd_count; i++)
    {
        dest->fd_array[i] = src->fd_array[i];
    }
    for (; i < FD_SETSIZE; i++)
    {
        dest->fd_array[i] = 0;
    }
}

static void copy_output_fds(oe_provider_fd_set* dest, oe_fd_set_internal* src)
{
    unsigned int i;
    dest->fd_count = src->fd_count;
    for (i = 0; i < src->fd_count; i++)
    {
        dest->fd_array[i] = src->fd_array[i];
    }
}

static int oe_insecure_select(
    int64_t a_nFds,
    _Inout_opt_ oe_provider_fd_set* a_readfds,
    _Inout_opt_ oe_provider_fd_set* a_writefds,
    _Inout_opt_ oe_provider_fd_set* a_exceptfds,
    _In_opt_ const struct timeval* a_Timeout)
{
    select_Result result = {0};
    oe_fd_set_internal readFds = {0};
    oe_fd_set_internal writeFds = {0};
    oe_fd_set_internal exceptFds = {0};
    if (a_readfds != NULL)
    {
        copy_input_fds(&readFds, a_readfds);
    }
    if (a_writefds != NULL)
    {
        copy_input_fds(&writeFds, a_writefds);
    }
    if (a_exceptfds != NULL)
    {
        copy_input_fds(&exceptFds, a_exceptfds);
    }
    oe_result_t oe_result = ocall_select(
        &result,
        a_nFds,
        readFds,
        writeFds,
        exceptFds,
        *(struct timeval*)a_Timeout);
    if (oe_result != OE_OK)
    {
        return 0;
    }

    if (result.error != 0)
    {
        return 0;
    }

    if (a_readfds != NULL)
    {
        copy_output_fds(a_readfds, &result.readFds);
    }
    if (a_writefds != NULL)
    {
        copy_output_fds(a_writefds, &result.writeFds);
    }
    if (a_exceptfds != NULL)
    {
        copy_output_fds(a_exceptfds, &result.exceptFds);
    }
    return (int)result.socketsSet;
}

int oe_gethostname_OE_NETWORK_INSECURE(
    _Out_writes_(a_uiBufferLength) char* a_pBuffer,
    size_t a_uiBufferLength)
{
    oe_result_t oe_result;
    gethostname_Result result;

    oe_result = ocall_gethostname(&result);
    if (oe_result != OE_OK)
    {
        result.error = OE_ENETDOWN;
    }
    if (result.error == 0)
    {
        oe_strncpy_s(
            a_pBuffer, a_uiBufferLength, result.name, a_uiBufferLength);
    }
    oe_wsa_set_last_error(OE_NETWORK_INSECURE, (int)result.error);
    return (result.error == 0) ? 0 : OE_SOCKET_ERROR;
}

int oe_wsa_startup_OE_NETWORK_INSECURE(
    uint16_t wVersionRequired,
    _Out_ oe_wsa_data_t* lpWSAData)
{
    OE_UNUSED(wVersionRequired);
    OE_UNUSED(lpWSAData);

    oe_socket_error_t apiResult;
    oe_result_t oe_result = ocall_WSAStartup(&apiResult);
    if (oe_result == OE_OK && apiResult == 0)
    {
        return 0;
    }
    return OE_SYSNOTREADY;
}

int oe_wsa_cleanup_OE_NETWORK_INSECURE(void)
{
    oe_socket_error_t error;
    oe_result_t oe_result = ocall_WSACleanup(&error);
    if (oe_result != OE_OK)
    {
        error = OE_ENETDOWN;
    }
    oe_wsa_set_last_error(OE_NETWORK_INSECURE, (int)error);
    return (error == 0) ? 0 : OE_SOCKET_ERROR;
}

/* We use a simple global variable since we're single threaded. */
static oe_socket_error_t g_WSALastError = 0;

void oe_wsa_set_last_error_OE_NETWORK_INSECURE(_In_ int iError)
{
    g_WSALastError = (oe_socket_error_t)iError;
}

int oe_wsa_get_last_error_OE_NETWORK_INSECURE(void)
{
    return (int)g_WSALastError;
}

static int oe_insecure_shutdown(_In_ intptr_t s, int how)
{
    oe_socket_error_t socketError = 0;
    oe_result_t oe_result =
        ocall_shutdown(&socketError, s, (oe_shutdown_how_t)how);
    if (oe_result != OE_OK)
    {
        socketError = OE_ENETDOWN;
    }
    oe_wsa_set_last_error(OE_NETWORK_INSECURE, (int)socketError);
    return (socketError == 0) ? 0 : OE_SOCKET_ERROR;
}

static int oe_insecure_close(_In_ intptr_t s)
{
    oe_socket_error_t socketError = 0;
    oe_result_t oe_result = ocall_closesocket(&socketError, s);
    if (oe_result != OE_OK)
    {
        socketError = OE_ENETDOWN;
    }
    oe_wsa_set_last_error(OE_NETWORK_INSECURE, (int)socketError);
    return (socketError == 0) ? 0 : OE_SOCKET_ERROR;
}

static int oe_insecure_listen(_In_ intptr_t s, int backlog)
{
    oe_socket_error_t socketError = 0;
    oe_result_t oe_result = ocall_listen(&socketError, s, backlog);
    if (oe_result != OE_OK)
    {
        socketError = OE_ENETDOWN;
    }
    oe_wsa_set_last_error(OE_NETWORK_INSECURE, (int)socketError);
    return (socketError == 0) ? 0 : OE_SOCKET_ERROR;
}

static int oe_insecure_getsockopt(
    _In_ intptr_t s,
    int level,
    int optname,
    _Out_writes_(*optlen) char* optval,
    _Inout_ socklen_t* optlen)
{
    getsockopt_Result result = {0};
    if ((size_t)*optlen > sizeof(result.buffer))
    {
        oe_wsa_set_last_error(OE_NETWORK_INSECURE, OE_EINVAL);
        return OE_SOCKET_ERROR;
    }
    oe_result_t oe_result =
        ocall_getsockopt(&result, s, level, optname, *optlen);
    if ((oe_result != OE_OK) || (result.len > *optlen))
    {
        result.error = OE_ENETDOWN;
        *optlen = 0;
    }
    else
    {
        *optlen = result.len;
        oe_memcpy_s(
            optval, (size_t)result.len, result.buffer, (size_t)result.len);
    }
    oe_wsa_set_last_error(OE_NETWORK_INSECURE, (int)result.error);
    return (result.error == 0) ? 0 : OE_SOCKET_ERROR;
}

static ssize_t oe_insecure_recv(
    _In_ intptr_t s,
    _Out_writes_(len) void* buf,
    size_t len,
    int flags)
{
    ssize_t bytesReceived;
    oe_result_t oe_result;
    oe_socket_error_t sock_error;

    oe_result = ocall_recv(&bytesReceived, s, buf, len, flags, &sock_error);
    if ((oe_result != OE_OK) ||
        (bytesReceived > 0 && (size_t)bytesReceived > len))
    {
        sock_error = OE_ENETDOWN;
    }
    oe_wsa_set_last_error(OE_NETWORK_INSECURE, (int)sock_error);
    return (sock_error != 0) ? OE_SOCKET_ERROR : bytesReceived;
}

static ssize_t oe_insecure_send(
    _In_ intptr_t s,
    _In_reads_bytes_(len) const char* buf,
    int len,
    int flags)
{
    oe_result_t oeResult;
    send_Result result;

    oeResult = ocall_send(&result, s, buf, (size_t)len, flags);
    if (oeResult != OE_OK)
    {
        result.error = OE_ENETDOWN;
        result.bytesSent = 0;
    }
    oe_wsa_set_last_error(OE_NETWORK_INSECURE, (int)result.error);
    return (result.error != 0) ? OE_SOCKET_ERROR : result.bytesSent;
}

static int oe_insecure_setsockopt(
    _In_ intptr_t s,
    int level,
    int optname,
    _In_reads_bytes_(optlen) const char* optval,
    socklen_t optlen)
{
    oe_socket_error_t socketError = 0;
    oe_result_t oe_result;

    oe_result =
        ocall_setsockopt(&socketError, s, level, optname, optval, optlen);
    if (oe_result != OE_OK)
    {
        socketError = OE_ENETDOWN;
    }
    oe_wsa_set_last_error(OE_NETWORK_INSECURE, (int)socketError);
    return (socketError == 0) ? 0 : OE_SOCKET_ERROR;
}

static int oe_insecure_ioctl(_In_ intptr_t s, long cmd, _Inout_ u_long* argp)
{
    ioctlsocket_Result result = {0};
    oe_result_t oe_result;

    oe_result = ocall_ioctlsocket(&result, s, (int)cmd, (unsigned int)*argp);
    if (oe_result != OE_OK)
    {
        result.error = OE_ENETDOWN;
    }
    oe_wsa_set_last_error(OE_NETWORK_INSECURE, (int)result.error);
    *argp = result.outputValue;
    return (result.error == 0) ? 0 : OE_SOCKET_ERROR;
}

static int oe_insecure_connect(
    _In_ intptr_t s,
    _In_reads_bytes_(namelen) const oe_sockaddr* name,
    int namelen)
{
    oe_socket_error_t socketError = 0;
    oe_result_t oe_result;

    oe_result = ocall_connect(&socketError, s, name, namelen);
    if (oe_result != OE_OK)
    {
        socketError = OE_ENETDOWN;
    }
    oe_wsa_set_last_error(OE_NETWORK_INSECURE, (int)socketError);
    return (socketError == 0) ? 0 : OE_SOCKET_ERROR;
}

static intptr_t oe_insecure_accept(
    _In_ intptr_t a_Socket,
    _Out_writes_bytes_(*addrlen) struct sockaddr* a_SockAddr,
    _Inout_ socklen_t* a_pAddrLen)
{
    accept_Result result;
    socklen_t addrlen = (a_pAddrLen != NULL) ? *a_pAddrLen : 0;
    oe_result_t oe_result = ocall_accept(&result, a_Socket, addrlen);
    if ((oe_result != OE_OK) || (result.addrlen > addrlen))
    {
        result.error = OE_ENETDOWN;
        addrlen = 0;
    }
    else
    {
        oe_memcpy_s(
            a_SockAddr,
            (size_t)result.addrlen,
            result.addr,
            (size_t)result.addrlen);
        addrlen = result.addrlen;
    }
    oe_wsa_set_last_error(OE_NETWORK_INSECURE, (int)result.error);
    if (a_pAddrLen != NULL)
    {
        *a_pAddrLen = addrlen;
    }
    return (result.error == 0) ? result.hNewSocket
                               : (intptr_t)OE_INVALID_SOCKET;
}

static int oe_insecure_getpeername(
    _In_ intptr_t s,
    _Out_writes_bytes_(*addrlen) struct sockaddr* addr,
    _Inout_ int* addrlen)
{
    GetSockName_Result result;
    oe_result_t oe_result = ocall_getpeername(&result, s, *addrlen);
    if ((oe_result != OE_OK) || (result.addrlen > *addrlen))
    {
        result.error = OE_ENETDOWN;
        *addrlen = 0;
    }
    else
    {
        oe_memcpy_s(
            addr, (size_t)result.addrlen, result.addr, (size_t)result.addrlen);
        *addrlen = result.addrlen;
    }
    oe_wsa_set_last_error(OE_NETWORK_INSECURE, (int)result.error);
    return (result.error == 0) ? 0 : OE_SOCKET_ERROR;
}

static int oe_insecure_getsockname(
    _In_ intptr_t s,
    _Out_writes_bytes_(*addrlen) struct sockaddr* addr,
    _Inout_ int* addrlen)
{
    GetSockName_Result result;
    oe_result_t oe_result = ocall_getsockname(&result, s, *addrlen);
    if ((oe_result != OE_OK) || (result.addrlen > *addrlen))
    {
        result.error = OE_ENETDOWN;
        *addrlen = 0;
    }
    else
    {
        oe_memcpy_s(
            addr, (size_t)result.addrlen, result.addr, (size_t)result.addrlen);
        *addrlen = result.addrlen;
    }
    oe_wsa_set_last_error(OE_NETWORK_INSECURE, (int)result.error);
    return (result.error == 0) ? 0 : OE_SOCKET_ERROR;
}

static int oe_insecure_bind(
    _In_ intptr_t s,
    _In_reads_bytes_(addrlen) const oe_sockaddr* addr,
    int addrlen)
{
    oe_socket_error_t socketError = 0;
    oe_result_t oe_result;

    oe_result = ocall_bind(&socketError, s, addr, addrlen);
    if (oe_result != OE_OK)
    {
        socketError = OE_ENETDOWN;
    }
    oe_wsa_set_last_error(OE_NETWORK_INSECURE, (int)socketError);
    return (socketError == 0) ? 0 : OE_SOCKET_ERROR;
}

int oe_getaddrinfo_OE_NETWORK_INSECURE(
    _In_z_ const char* pNodeName,
    _In_z_ const char* pServiceName,
    _In_ const oe_addrinfo* pHints,
    _Out_ oe_addrinfo** ppResult)
{
    oe_result_t oe_result;
    int eai_result;
    oe_addrinfo* ailist = NULL;
    oe_addrinfo* ai;
    oe_addrinfo** pNext = &ailist;

    socklen_t length_needed = 0;
    struct addrinfo_Buffer* response = NULL;
    socklen_t len = 0;

    *ppResult = NULL;

    for (;;)
    {
        oe_result = ocall_getaddrinfo(
            &eai_result,
            (char*)pNodeName,
            (char*)pServiceName,
            (pHints != NULL) ? pHints->ai_flags : 0,
            (pHints != NULL) ? pHints->ai_family : 0,
            (pHints != NULL) ? pHints->ai_socktype : 0,
            (pHints != NULL) ? pHints->ai_protocol : 0,
            response,
            len,
            &length_needed);

        if (len < length_needed)
        {
            free(response);
            response =
                (struct addrinfo_Buffer*)oe_malloc((size_t)length_needed);
            if (response == NULL)
            {
                return OE_ENOMEM; // TODO: EAI_MEMORY
            }
            len = length_needed;
            continue;
        }

        if (oe_result != OE_OK)
        {
            // TODO: we need EAI_* codes.
            return OE_EFAULT; // TODO: EAI_FAIL
        }
        if (eai_result != 0)
        {
            return eai_result;
        }
        break;
    }

    /* We now have a response to deserialize. */
    int count = (int)((size_t)len / sizeof(addrinfo_Buffer));
    for (int i = 0; i < count; i++)
    {
        if ((size_t)response[i].ai_addrlen > sizeof(oe_sockaddr_storage) ||
            (size_t)response[i].ai_addrlen > sizeof(response[i].ai_addr))
        {
            eai_result = OE_ENOMEM; // TODO: EAI_MEMORY
            break;
        }
        ai = oe_malloc(sizeof(*ai));
        if (ai == NULL)
        {
            eai_result = OE_ENOMEM; // TODO: EAI_MEMORY
            break;
        }
        ai->ai_addr = oe_malloc((size_t)response[i].ai_addrlen);
        if (ai->ai_addr == NULL)
        {
            oe_free(ai);
            eai_result = OE_ENOMEM; // TODO: EAI_MEMORY
            break;
        }
        oe_memcpy_s(
            ai->ai_addr,
            (size_t)response[i].ai_addrlen,
            response[i].ai_addr,
            (size_t)response[i].ai_addrlen);

        ai->ai_flags = (int)response[i].ai_flags;
        ai->ai_family = (int)response[i].ai_family;
        ai->ai_socktype = (int)response[i].ai_socktype;
        ai->ai_protocol = (int)response[i].ai_protocol;
        ai->ai_addrlen = (size_t)response[i].ai_addrlen;
        if (response[i].ai_canonname[0] != 0)
        {
            ai->ai_canonname = oe_malloc(sizeof(response[i].ai_canonname) + 1);
            if (ai->ai_canonname == NULL)
            {
                oe_free(ai);
                eai_result = OE_ENOMEM; // TODO: EAI_MEMORY
                break;
            }
            oe_strncpy_s(
                ai->ai_canonname,
                sizeof(response[i].ai_canonname),
                response[i].ai_canonname,
                sizeof(response[i].ai_canonname));
            ai->ai_canonname[sizeof(response[i].ai_canonname)] = 0;
        }
        else
        {
            ai->ai_canonname = NULL;
        }
        ai->ai_next = NULL;

        /* Insert at end of list. */
        *pNext = ai;
        pNext = &ai->ai_next;
    }
    oe_wsa_set_last_error_OE_NETWORK_INSECURE(eai_result);

    if (eai_result != 0 && ailist != NULL)
    {
        freeaddrinfo(ailist);
    }
    else
    {
        *ppResult = ailist;
    }
    free(response);
    return eai_result;
}

int oe_getnameinfo_OE_NETWORK_INSECURE(
    _In_ const struct oe_sockaddr* sa,
    _In_ oe_socklen_t salen,
    _Out_writes_opt_z_(hostlen) char* host,
    _In_ socklen_t hostlen,
    _Out_writes_opt_z_(servlen) char* serv,
    _In_ socklen_t servlen,
    _In_ int flags)
{
    getnameinfo_Result result = {0};
    oe_result_t oe_result;

    oe_result = ocall_getnameinfo(&result, sa, salen, flags);
    if (oe_result != OE_OK)
    {
        result.error = OE_ENOTRECOVERABLE;
    }

    if (host != NULL)
    {
        oe_strncpy_s(host, hostlen, result.host, hostlen);
        host[hostlen - 1] = 0;
    }

    if (serv != NULL)
    {
        oe_strncpy_s(serv, servlen, result.serv, servlen);
        serv[servlen - 1] = 0;
    }

    return (int)result.error;
}

static oe_socket_provider_t oe_insecure_socket_provider = {
    oe_insecure_accept,
    oe_insecure_bind,
    oe_insecure_close,
    oe_insecure_connect,
    oe_insecure_getpeername,
    oe_insecure_getsockname,
    oe_insecure_getsockopt,
    oe_insecure_ioctl,
    oe_insecure_listen,
    oe_insecure_recv,
    oe_insecure_select,
    oe_insecure_send,
    oe_insecure_setsockopt,
    oe_insecure_shutdown,
};

oe_socket_t oe_socket_OE_NETWORK_INSECURE(
    _In_ int domain,
    _In_ int type,
    _In_ int protocol)
{
    oe_socket_t oesocket = OE_INVALID_SOCKET;
    socket_Result result;
    oe_result_t oe_result = ocall_socket(
        &result,
        (oe_socket_address_family_t)domain,
        (oe_socket_type_t)type,
        protocol);
    if (oe_result != OE_OK)
    {
        result.error = OE_ENETDOWN;
    }
    else
    {
        oesocket =
            oe_register_socket(&oe_insecure_socket_provider, result.hSocket);
        if (oesocket == OE_INVALID_SOCKET)
        {
            oe_socket_error_t retval;
            (void)ocall_closesocket(&retval, result.hSocket);
            result.error = OE_ENETDOWN;
        }
    }
    oe_wsa_set_last_error(OE_NETWORK_INSECURE, (int)result.error);
    return oesocket;
}