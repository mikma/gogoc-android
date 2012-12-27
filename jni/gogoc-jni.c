#include <assert.h>
#include <jni.h>
#include <string.h>

#define TAG "Gogoc"

// for __android_log_print(ANDROID_LOG_INFO, TAG, "formatted message");
#include <android/log.h>

// for native asset manager
#include <sys/types.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <pthread.h>

extern int indSigHUP; /* Declared in every unix platform tsp_local.c */

#define PREFIX_LENGTH		128
#define ROUTE_PREFIX		"::"
#define ROUTE_PREFIX_LEN	0

#define JAVA_LANG_RUNTIME_EXCEPTION "java.lang.RuntimeException"

#define FOO __android_log_print(ANDROID_LOG_INFO, TAG, "line %d", __LINE__)


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_t thread;
pthread_attr_t thread_attr;

JNIEnv *g_env;
jobject g_builder;

typedef struct event {
    enum {EVENT_NONE, EVENT_EXIT, EVENT_BUFFER} code;
    int buffer_no;
} event_t;

event_t event;

// (called automatically on load)
__attribute__((constructor)) static void onDlOpen(void)
{
}


static void requestBuffer(int num)
{
    pthread_mutex_lock(&mutex);
    event.buffer_no += num;
    if (event.code == EVENT_NONE) {
	event.code = EVENT_BUFFER;
    }
    /* __android_log_print(ANDROID_LOG_INFO, TAG, "request buffer %d + %d", */
    /* 			event.code, event.buffer_no); */
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

static void exitBuffer()
{
    pthread_mutex_lock(&mutex);
    event.code = EVENT_EXIT;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

static void *thread_start(void *arg)
{
    int code;

    __android_log_print(ANDROID_LOG_INFO, TAG, "thread_start");

    while (1) {
	pthread_mutex_lock(&mutex);
	while (event.code == EVENT_NONE) {
	    pthread_cond_wait(&cond, &mutex);
	}

	int code = event.code;

	switch (code) {
	case EVENT_EXIT:
	    __android_log_print(ANDROID_LOG_INFO, TAG, "thread event exit");
	    pthread_mutex_unlock(&mutex);
	    return;
	default:
	    // EVENT_NONE
	    break;
	}

	pthread_mutex_unlock(&mutex);
    }
}

void Java_org_nklog_gogoc_GogocService_startup(JNIEnv* env, jobject thiz,
                                               jobject builder)
{
    int res;
    int i;

    __android_log_print(ANDROID_LOG_INFO, TAG, "Start init");

    g_env = env;
    g_builder = (*g_env)->NewGlobalRef(env, builder);
    if (!g_builder) {
      jclass class = (*g_env)->FindClass(g_env, JAVA_LANG_RUNTIME_EXCEPTION);
      if (class) {
        (*g_env)->ThrowNew(g_env, class, "unable to get global ref");
      }
      return;
    }

    event.code = EVENT_NONE;
    event.buffer_no = 0;

    res = pthread_attr_init(&thread_attr);
    assert(res == 0);

    __android_log_print(ANDROID_LOG_INFO, TAG, "Begin create thread");
    res = pthread_create(&thread, &thread_attr,
			 &thread_start, NULL);
    __android_log_print(ANDROID_LOG_INFO, TAG, "End create thread");
    assert(res == 0);

    char *args[] = {NULL};
    __android_log_print(ANDROID_LOG_INFO, TAG, "Begin main");

    main(0, args);

    __android_log_print(ANDROID_LOG_INFO, TAG, "End init");
}

void Java_org_nklog_gogoc_GogocService_shutdown(JNIEnv* env, jclass clazz)
{
    __android_log_print(ANDROID_LOG_INFO, TAG, "Wake buffer thread");
    indSigHUP = 1;
    exitBuffer();
    pthread_join(thread, NULL);
    __android_log_print(ANDROID_LOG_INFO, TAG, "Joined buffer thread");
    thread = 0;
}

#if 1
/*
-----------------------------------------------------------------------------
 $Id: unix-main.c,v 1.1 2009/11/20 16:53:30 jasminko Exp $
-----------------------------------------------------------------------------
Copyright (c) 2001-2006 gogo6 Inc. All rights reserved.

  For license information refer to CLIENT-LICENSE.TXT

-----------------------------------------------------------------------------
*/

#include "platform.h"
#include "gogoc_status.h"

#include <signal.h>

#include "tsp_client.h"
#include "hex_strings.h"
#include "log.h"
#include "os_uname.h"

#ifdef HACCESS
#include "haccess.h"
#endif



// --------------------------------------------------------------------------
// Retrieves OS information and puts it nicely in a string ready for display.
//
// Defined in tsp_client.h
//
void tspGetOSInfo( const size_t len, char* buf )
{
  if( len > 0  &&  buf != NULL )
  {
#ifdef OS_UNAME_INFO
    snprintf( buf, len, "Built on ///%s///", OS_UNAME_INFO );
#else
    snprintf( buf, len, "Built on ///unknown UNIX/BSD/Linux version///" );
#endif
  }
}


// --------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  int rc;
#ifdef HACCESS
  haccess_status status = HACCESS_STATUS_OK;
#endif

#ifdef HACCESS
  /* Initialize the HACCESS module. */
  status = haccess_initialize();

  if (status != HACCESS_STATUS_OK) {
    DirectErrorMessage(HACCESS_LOG_PREFIX_ERROR GOGO_STR_HACCESS_ERR_CANT_INIT_MODULE);
    return ERR_HACCESS_INIT;
  }
#endif

  /* entry point */
  rc = tspMain(argc, argv);

#ifdef HACCESS
  /* The HACCESS module destructs (deinitialization). */
  status = haccess_destruct();

  if (status != HACCESS_STATUS_OK)
  {
    DirectErrorMessage(HACCESS_LOG_PREFIX_ERROR GOGO_STR_HACCESS_ERR_CANT_DO_SHUTDOWN);
  }
#endif

  return rc;
}
#endif

// --------------------------------------------------------------------------
// TunInit: Open and initialize the TUN interface.
//
sint32_t TunInit(const char *client_address_ipv6,
                 const char *client_dns_server_address_ipv6)
{
  sint32_t tunfd = -1;
  jclass clazz = 0;
  jclass parcel_fd_clazz = 0;
  jmethodID add_address;
  jmethodID add_dns_server;
  jmethodID add_route;
  jmethodID establish;
  jmethodID detach_fd;
  jobject address = 0;
  jobject dns_server = 0;
  jobject route = 0;
  jobject parcelfd = 0;

  /* struct ifreq ifr; */
  /* char iftun[128]; */
  /* unsigned long ioctl_nochecksum = 1; */

  __android_log_print(ANDROID_LOG_INFO, TAG, "TunInit");

  clazz = (*g_env)->GetObjectClass(g_env, g_builder);
  if (!clazz) {
    __android_log_print(ANDROID_LOG_ERROR, TAG, "No builder class");
    goto error;
  }

  add_address = (*g_env)->GetMethodID(g_env, clazz, "addAddress",
                                      "(Ljava/lang/String;I)Landroid/net/VpnService$Builder;");
  add_dns_server = (*g_env)->GetMethodID(g_env, clazz, "addDnsServer",
                                         "(Ljava/lang/String;)Landroid/net/VpnService$Builder;");
  add_route = (*g_env)->GetMethodID(g_env, clazz, "addRoute",
                                    "(Ljava/lang/String;I)Landroid/net/VpnService$Builder;");
  establish = (*g_env)->GetMethodID(g_env, clazz, "establish", "()Landroid/os/ParcelFileDescriptor;");

  if (!add_address || !add_dns_server || !add_route || !establish) {
    __android_log_print(ANDROID_LOG_ERROR, TAG, "failed builder methods: %p %p %p %p", add_address, add_dns_server, add_route, establish);
    goto error;
  }

  address = (*g_env)->NewStringUTF(g_env, client_address_ipv6);
  dns_server = (*g_env)->NewStringUTF(g_env, client_dns_server_address_ipv6);
  route = (*g_env)->NewStringUTF(g_env, ROUTE_PREFIX);
  
  if (!address | !dns_server || !route) {
    __android_log_print(ANDROID_LOG_ERROR, TAG, "failed builder parameter objects: %p %p %p", address, dns_server, route);
    goto error;
  }

  FOO;
  /* TODO check exceptions */
  (*g_env)->CallObjectMethod(g_env, g_builder, add_address, address, PREFIX_LENGTH);
  FOO;
  /* Broken access to google DNS on gogo6 */
  /* (*g_env)->CallObjectMethod(g_env, g_builder, add_dns_server, dns_server); */
  FOO;
  (*g_env)->CallObjectMethod(g_env, g_builder, add_route, route, ROUTE_PREFIX_LEN);
  FOO;

  parcelfd = (*g_env)->CallObjectMethod(g_env, g_builder, establish);
  FOO;

  if (!parcelfd) {
    __android_log_print(ANDROID_LOG_ERROR, TAG, "failed establish");
    goto error;
  }

  parcel_fd_clazz = (*g_env)->GetObjectClass(g_env, parcelfd);
  FOO;
  if (!parcel_fd_clazz) {
    __android_log_print(ANDROID_LOG_ERROR, TAG, "failed parcel class");
    goto error;
  }

  detach_fd = (*g_env)->GetMethodID(g_env, parcel_fd_clazz, "detachFd", "()I");
  FOO;
  if (!detach_fd) {
    __android_log_print(ANDROID_LOG_ERROR, TAG, "failed detach");
    goto error;
  }

  tunfd = (*g_env)->CallIntMethod(g_env, parcelfd, detach_fd);
  FOO;
  if (tunfd < 0) {
    __android_log_print(ANDROID_LOG_ERROR, TAG, "failed tunfd");
  }
  FOO;
  return tunfd;

 error:
#if 0
  if (clazz)
    (*g_env)->DeleteLocalRef(g_env, clazz);

  if (address)
    (*g_env)->DeleteLocalRef(g_env, address);

  if (dns_server)
    (*g_env)->DeleteLocalRef(g_env, dns_server);

  if (route)
    (*g_env)->DeleteLocalRef(g_env, route);

  if (parcelfd)
    (*g_env)->DeleteLocalRef(g_env, parcelfd);

  if (parcel_fd_clazz)
    (*g_env)->DeleteLocalRef(g_env, parcel_fd_clazz);
#endif

  return -1;
}
