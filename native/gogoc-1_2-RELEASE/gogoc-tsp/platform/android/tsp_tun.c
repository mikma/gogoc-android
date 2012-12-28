/*
-----------------------------------------------------------------------------
 $Id: tsp_tun.c,v 1.1 2009/11/20 16:53:24 jasminko Exp $
-----------------------------------------------------------------------------
This source code copyright (c) gogo6 Inc. 2002-2006.

  For license information refer to CLIENT-LICENSE.TXT
  
-----------------------------------------------------------------------------
*/

/* Linux */

#include <sys/select.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "platform.h"
#include "gogoc_status.h"

#include <linux/if.h>
#include <linux/if_tun.h>

#include "tsp_tun.h"        // Local function prototypes.
#include "tsp_client.h"     // tspCheckForStopOrWait()
#include "net_ka.h"         // KA function prototypes and types.
#include "log.h"            // Display and logging prototypes and types.
#include "hex_strings.h"    // String litterals

#define TUN_BUFSIZE 2048    // Buffer size for TUN interface IO operations.

extern int indSigHUP;           // from tsp_local.c

int g_pipefd = -1;

// --------------------------------------------------------------------------
// Wake TunMainLoop and exit.
//
int TunStop(void)
{
  char buf[1] = "\0";

  indSigHUP = 1;

  if (g_pipefd < 0)
    return -1;
  Display(LOG_LEVEL_3, ELInfo, "TunStop", STR_GEN_STOPPING);
  return write(g_pipefd, buf, sizeof(buf));
}


// --------------------------------------------------------------------------
// TunMainLoop: Initializes Keepalive engine and starts it. Then starts a
//   loop to transfer data from/to the socket and tunnel.
//   This process is repeated until tspCheckForStopOrWait indicates a stop.
//
gogoc_status TunMainLoop(sint32_t tunfd, pal_socket_t Socket,
                     tBoolean keepalive, sint32_t keepalive_interval,
                     char *local_address_ipv6, char *keepalive_address)
{
  fd_set rfds;
  int count, maxfd, ret;
  unsigned char bufin[TUN_BUFSIZE];
  unsigned char bufout[TUN_BUFSIZE];
  struct timeval timeout;
  void* p_ka_engine = NULL;
  ka_status_t ka_status;
  ka_ret_t ka_ret;
  int ongoing = 1;
  gogoc_status status;
  int pipefd;

  {
    int pipefds[2];

    if( pipe(pipefds) < 0 )
    {
        Display( LOG_LEVEL_1, ELError, "TunMainLoop", STR_MISC_FAIL_TUN_INIT);
        return make_status(CTX_TUNNELLOOP, ERR_INTERFACE_SETUP_FAILED);
    }

    pipefd = pipefds[0];
    g_pipefd = pipefds[1];
  }

  keepalive = (keepalive_interval != 0) ? TRUE : FALSE;

  if( keepalive == TRUE )
  {
    // Initialize the keepalive engine.
    ka_ret = KA_init( &p_ka_engine, keepalive_interval * 1000,
                      local_address_ipv6, keepalive_address, AF_INET6 );
    if( ka_ret != KA_SUCCESS )
    {
      status = make_status(CTX_TUNNELLOOP, ERR_KEEPALIVE_ERROR);
      goto done;
    }

    // Start the keepalive loop(thread).
    ka_ret = KA_start( p_ka_engine );
    if( ka_ret != KA_SUCCESS )
    {
      KA_destroy( &p_ka_engine );
      status = make_status(CTX_TUNNELLOOP, ERR_KEEPALIVE_ERROR);
      goto done;
    }
  }

  // Data send loop.
  while( 1 )
  {
    // initialize status.
    status = STATUS_SUCCESS_INIT;

    if( tspCheckForStopOrWait( 0 ) != 0 )
    {
      // We've been notified to stop.
      ongoing = 0;
      break;
    }

#if 0
    if( keepalive == TRUE )
    {

      // Query the keepalive status.
      ka_status = KA_qry_status( p_ka_engine );
      switch( ka_status )
      {
      case KA_STAT_ONGOING:
      case KA_STAT_FIN_SUCCESS:
        break;

      case KA_STAT_FIN_TIMEOUT:
        KA_stop( p_ka_engine );
        status = make_status(CTX_TUNNELLOOP, ERR_KEEPALIVE_TIMEOUT);
        break;

      case KA_STAT_INVALID:
      case KA_STAT_FIN_ERROR:
      default:
        KA_stop( p_ka_engine );
        status = make_status(CTX_TUNNELLOOP, ERR_KEEPALIVE_ERROR);
        break;
      }

      // Reinit select timeout variable; select modifies it.
      // Use 500ms because we need to re-check keepalive status.
      timeout.tv_sec = 0;
      timeout.tv_usec = 500000;    // 500 milliseconds.
    }
    else
#endif
    {
      // Reinit select timeout variable; select modifies it.
      timeout.tv_sec = 7 * 24 * 60 * 60 ; // one week
      timeout.tv_usec = 0;
    }

    // Check if we're normal.
    if( status_number(status) != SUCCESS  ||  ongoing != 1 )
    {
      goto done;
    }

    FD_ZERO(&rfds);
    FD_SET(tunfd,&rfds);
    FD_SET(Socket,&rfds);
    FD_SET(pipefd,&rfds);
    maxfd = tunfd>Socket?tunfd:Socket;
    maxfd = pipefd>maxfd?pipefd:maxfd;

    ret = select( maxfd+1, &rfds, NULL, NULL, &timeout );
    if( ret > 0 )
    {
      if( FD_ISSET(tunfd,&rfds) )
      {
        // Data sent through UDP tunnel
        if ( (count = read(tunfd,bufout,sizeof(bufout) )) == -1 )
        {
          Display( LOG_LEVEL_1, ELError, "TunMainLoop",STR_NET_FAIL_R_TUN_DEV );
          status = make_status(CTX_TUNNELLOOP, ERR_TUNNEL_IO);
          goto done;
        }

        if (send(Socket,bufout,count,0) != count)
        {
          Display( LOG_LEVEL_1, ELError, "TunMainLoop",STR_NET_FAIL_W_SOCKET );
          status = make_status(CTX_TUNNELLOOP, ERR_TUNNEL_IO);
          goto done;
        }
      }

      if( FD_ISSET(Socket,&rfds) )
      {
        // Data received through UDP tunnel.
        count = recvfrom( Socket, bufin, TUN_BUFSIZE, 0, NULL, NULL );
        if( write(tunfd,bufin,count) != count )
        {
          Display( LOG_LEVEL_1, ELError, "TunMainLoop", STR_NET_FAIL_W_TUN_DEV );
          status = make_status(CTX_TUNNELLOOP, ERR_TUNNEL_IO);
          goto done;
        }
      }

      if( FD_ISSET(pipefd,&rfds) )
      {
        char buf[1];
        size_t len = sizeof(buf);
        int res;
        if ( (res = read( pipefd, buf, len)) == (int)len )
        {
          break;
        }
        else
        {
          Display( LOG_LEVEL_1, ELError, "TunMainLoop", STR_GEN_NETWORK_ERROR);
          status = make_status(CTX_TUNNELLOOP, ERR_TUNNEL_IO);
          goto done;
        }
      }
    }
  }   // while()

  /* Normal loop end */
  status = STATUS_SUCCESS_INIT;

  if( keepalive == TRUE )
  {
    // Stop keepalive engine.
    KA_stop( p_ka_engine );
  }

done:
  if( p_ka_engine != NULL )
  {
    KA_destroy( &p_ka_engine );
  }

  if( g_pipefd > -1)
  {
    close(g_pipefd);
    g_pipefd = -1;
  }

  if( pipefd > -1)
  {
    close(pipefd);
    pipefd = -1;
  }

  return status;
}
