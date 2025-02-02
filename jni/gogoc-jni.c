#include <assert.h>
#include <jni.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <pal_types.h>

#define TAG "Gogoc"

// for __android_log_print(ANDROID_LOG_INFO, TAG, "formatted message");
#include <android/log.h>

#define PREFIX_LENGTH		128
#define ROUTE_PREFIX		"::"
#define ROUTE_PREFIX_LEN	0

#define JAVA_LANG_RUNTIME_EXCEPTION "java.lang.RuntimeException"

#define FOO __android_log_print(ANDROID_LOG_INFO, TAG, "line %d", __LINE__)


JNIEnv *g_env;
jobject g_builder;
jobject g_question_listener;

// (called automatically on load)
__attribute__((constructor)) static void onDlOpen(void)
{
  __android_log_print(ANDROID_LOG_INFO, TAG, "onDlOpen");
}

static int is_regular(const char *path)
{
  struct stat buf;

  memset(&buf, 0, sizeof(buf));

  if (stat(path, &buf) < 0)
    return 0;
  else
    return S_ISREG(buf.st_mode);
}

void Java_org_nklog_gogoc_GogocService_startup(JNIEnv* env, jobject thiz,
                                               jobject builder,
                                               jobject config_file,
                                               jobject question_listener)
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

    g_question_listener = (*g_env)->NewGlobalRef(env, question_listener);
    if (!g_question_listener) {
      jclass class = (*g_env)->FindClass(g_env, JAVA_LANG_RUNTIME_EXCEPTION);
      if (class) {
        (*g_env)->ThrowNew(g_env, class, "unable to get global ref");
      }
      /* TODO release g_builder */
      return;
    }

    jboolean is_copy = 0;
    const char *config_file_str = NULL;

    int argc = 0;
    const char *args[16];

    args[argc++] = "gogoc";

    if (config_file &&
        (config_file_str = (*g_env)->GetStringUTFChars(g_env, config_file, &is_copy)) &&
        is_regular(config_file_str)) {
      args[argc++] = "-f";
      args[argc++] = config_file_str;
    } else {
      args[argc++] = "-m";
      args[argc++] = "v6udpv4";           /* Only v6udpv4 supported by this app. */
    }
    __android_log_print(ANDROID_LOG_INFO, TAG, "Begin main: '%s' '%s' '%s'",
                        args[0], args[1], args[2]);

    main(argc, args);

    if (is_copy)
      (*g_env)->ReleaseStringUTFChars(g_env, config_file, config_file_str);

    __android_log_print(ANDROID_LOG_INFO, TAG, "End init");
}

void Java_org_nklog_gogoc_GogocService_shutdown(JNIEnv* env, jclass clazz)
{
    __android_log_print(ANDROID_LOG_INFO, TAG, "Wake gogo thread");
    TunStop();
}

#define ANSWER_NO 0
#define ANSWER_YES 1

sint32_t tspAskV(const char *question)
{
  sint32_t res = ANSWER_NO;
  jclass clazz = 0;
  jmethodID ask;
  jobject question_obj = NULL;

  __android_log_print(ANDROID_LOG_INFO, TAG, "tspAskV '%s'", question);

  if (!g_env || !g_question_listener)
    return ANSWER_NO;

  clazz = (*g_env)->GetObjectClass(g_env, g_question_listener);
  if (!clazz) {
    __android_log_print(ANDROID_LOG_ERROR, TAG, "No builder class");
    goto error;
  }

  ask = (*g_env)->GetMethodID(g_env, clazz, "ask",
                                      "(Ljava/lang/String;)Z");
  if (!ask) {
    __android_log_print(ANDROID_LOG_ERROR, TAG, "failed builder methods: %p", ask);
    goto error;
  }

  question_obj = (*g_env)->NewStringUTF(g_env, question);
  
  if (!question_obj) {
    __android_log_print(ANDROID_LOG_ERROR, TAG, "failed builder parameter objects: %p", question);
    goto error;
  }

  /* TODO check exceptions */
  res = (*g_env)->CallBooleanMethod(g_env, g_question_listener,
                                    ask, question_obj);

 error:
  if (clazz)
    (*g_env)->DeleteLocalRef(g_env, clazz);

  if (question_obj)
    (*g_env)->DeleteLocalRef(g_env, question_obj);

  return res;
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

extern int indSigHUP;           // from tsp_local.c

// --------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  int rc;
#ifdef HACCESS
  haccess_status status = HACCESS_STATUS_OK;
#endif

  indSigHUP = 0;

#ifdef HACCESS
  /* Initialize the HACCESS module. */
  status = haccess_initialize();

  if (status != HACCESS_STATUS_OK) {
    DirectErrorMessage(HACCESS_LOG_PREFIX_ERROR GOGO_STR_HACCESS_ERR_CANT_INIT_MODULE);
    return ERR_HACCESS_INIT;
  }
#endif

  /* entry point */
  __android_log_print(ANDROID_LOG_INFO, TAG, "Main begin");
  rc = tspMain(argc, argv);
  __android_log_print(ANDROID_LOG_INFO, TAG, "Main end");

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

  /* TODO check exceptions */
  (*g_env)->CallObjectMethod(g_env, g_builder, add_address, address, PREFIX_LENGTH);
  /* Broken access to google DNS on gogo6 */
  /* (*g_env)->CallObjectMethod(g_env, g_builder, add_dns_server, dns_server); */
  (*g_env)->CallObjectMethod(g_env, g_builder, add_route, route, ROUTE_PREFIX_LEN);

  parcelfd = (*g_env)->CallObjectMethod(g_env, g_builder, establish);

  if (!parcelfd) {
    __android_log_print(ANDROID_LOG_ERROR, TAG, "failed establish");
    goto error;
  }

  parcel_fd_clazz = (*g_env)->GetObjectClass(g_env, parcelfd);
  if (!parcel_fd_clazz) {
    __android_log_print(ANDROID_LOG_ERROR, TAG, "failed parcel class");
    goto error;
  }

  detach_fd = (*g_env)->GetMethodID(g_env, parcel_fd_clazz, "detachFd", "()I");
  if (!detach_fd) {
    __android_log_print(ANDROID_LOG_ERROR, TAG, "failed detach");
    goto error;
  }

  tunfd = (*g_env)->CallIntMethod(g_env, parcelfd, detach_fd);
  if (tunfd < 0) {
    __android_log_print(ANDROID_LOG_ERROR, TAG, "failed tunfd");
  }
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
