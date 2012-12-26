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

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_t thread;
pthread_attr_t thread_attr;

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

void Java_org_nklog_gogoc_GogocService_startup(JNIEnv* env, jclass clazz)
{
    int res;
    int i;

    __android_log_print(ANDROID_LOG_INFO, TAG, "Start init");
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
