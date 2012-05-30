#include <ppc/yf_header.h>
#include <base_struct/yf_core.h>

ssize_t yf_sock_ntop(const yf_sock_addr_t *sa, char *text, size_t len, yf_int_t port)
{
        char *p;

        switch (sa->sa_family)
        {
        case AF_INET: {
                struct sockaddr_in *sin = (struct sockaddr_in *)sa;
                p = (char *)&sin->sin_addr;

                if (port)
                {
                        p = yf_snprintf(text, len, "%ud.%ud.%ud.%ud:%d",
                                        p[0], p[1], p[2], p[3], ntohs(sin->sin_port));
                }
                else {
                        p = yf_snprintf(text, len, "%ud.%ud.%ud.%ud",
                                        p[0], p[1], p[2], p[3]);
                }

                return (ssize_t)(p - text);
        }

#ifdef  IPV6
        case AF_INET6: {
                struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;

                if (!port)
                {
                        if (inet_ntop(AF_INET6, &sin6->sin6_addr, text, len) == NULL)
                                return YF_ERROR;
                        return yf_strlen(text);
                }

                text[0] = '[';
                if (inet_ntop(AF_INET6, &sin6->sin6_addr, text + 1, len - 1) == NULL)
                        return YF_ERROR;

                ssize_t txt_len = yf_strlen(text);
                if (txt_len + 2 + YF_INT16_LEN >= len)
                        return YF_ERROR;

                p = yf_snprintf(text + txt_len, "]:%d", ntohs(sin6->sin6_port));

                return (ssize_t)(p - text);
        }
#endif

#ifdef  AF_LOCAL
        case AF_LOCAL: {
                struct sockaddr_un *unp = (struct sockaddr_un *)sa;

                /* OK to have no pathname bound to the socket: happens on
                 * every connect() unless client calls bind() first. */
                if (unp->sun_path[0] == 0)
                        return YF_ERROR;

                p = yf_snprintf(text, len, "%s", unp->sun_path);
                if (p - text == len)
                        return YF_ERROR;

                return (ssize_t)(p - text);
        }
#endif
        default:
                return YF_ERROR;
        }
}

yf_int_t  yf_sock_get_port(const yf_sock_addr_t *sa)
{
        switch (sa->sa_family)
        {
        case AF_INET: {
                struct sockaddr_in *sin = (struct sockaddr_in *)sa;
                return sin->sin_port;
        }

#ifdef  IPV6
        case AF_INET6: {
                struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
                return sin6->sin6_port;
        }
#endif
        }

        return YF_ERROR;
}


void yf_sock_set_port(yf_sock_addr_t *sa, yf_int_t port)
{
        switch (sa->sa_family)
        {
        case AF_INET: {
                struct sockaddr_in *sin = (struct sockaddr_in *)sa;
                sin->sin_port = port;
                return;
        }

#ifdef  IPV6
        case AF_INET6: {
                struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
                sin6->sin6_port = port;
                return;
        }
#endif
        }
}

yf_int_t yf_sock_set_addr(yf_sock_addr_t *sa, const char *addr)
{
        switch (sa->sa_family)
        {
        case AF_INET: {
                struct sockaddr_in *sin = (struct sockaddr_in *)sa;
                sin->sin_addr.s_addr = yf_inet_aton(addr);
                CHECK_RV(sin->sin_addr.s_addr == INADDR_NONE, YF_ERROR);
                return YF_OK;
        }

#ifdef  IPV6
        case AF_INET6: {
                struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
                CHECK_RV(inet_pton(AF_INET6, addr, &sin6->sin6_addr) != 0, YF_ERROR);
                return YF_OK;
        }
#endif

#ifdef  AF_LOCAL
        case AF_LOCAL: {
                struct sockaddr_un *unp = (struct sockaddr_un *)sa;
                char *p = yf_snprintf(unp->sun_path, sizeof(unp->sun_path), "%s", addr);
                if (p - unp->sun_path >= sizeof(unp->sun_path))
                        return YF_ERROR;
                return YF_OK;
        }
#endif

        default:
                return YF_ERROR;
        }
}

yf_sock_len_t  yf_sock_len(yf_sock_addr_t *sa)
{
        switch (sa->sa_family)
        {
        case AF_INET: {
                return sizeof(struct sockaddr_in);
        }

#ifdef  IPV6
        case AF_INET6: {
                return sizeof(struct sockaddr_in6);
        }
#endif

#ifdef  AF_LOCAL
        case AF_LOCAL: {
                struct sockaddr_un *unp = (struct sockaddr_un *)sa;
                return SUN_LEN(unp);
        }
#endif

        default:
                return YF_ERROR;
        }
}


int  __yf_tcp_nocork(yf_socket_t s, int tcp_nocork)
{
#ifdef  YF_LINUX
        return setsockopt(s, IPPROTO_TCP, TCP_CORK,
                          (const void *)&tcp_nocork, sizeof(int));
#else
        return setsockopt(s, IPPROTO_TCP, TCP_NOPUSH,
                          (const void *)&tcp_nocork, sizeof(int));
#endif
}


