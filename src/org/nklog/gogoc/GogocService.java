/* vim: set sw=4 ts=4: */

package org.nklog.gogoc;

import android.os.IBinder;
import android.app.Activity;
import android.app.Service;

import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;

import android.net.VpnService;

import java.io.File;
import java.io.InputStream;
import java.io.OutputStream;

import android.util.Log;

import java.io.FileReader;
import java.io.BufferedReader;

import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.content.pm.PackageManager.NameNotFoundException;

public class GogocService extends VpnService
{
	private Receiver receiver = null;

	private final String TAG = "GogocService";
	private Thread thread = null;

	private SharedPreferences preference;

	@Override
	public void onCreate()
	{
		receiver = new Receiver();

		preference= PreferenceManager.getDefaultSharedPreferences(this);

		super.onCreate();
	}

	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		Log.d(TAG, "on start command");
		IntentFilter filter = new IntentFilter();
		filter.addAction("org.nklog.gogoc.GogocService");
		registerReceiver(receiver, filter);

                Log.d(TAG, "try to startProcess");
                if (thread == null) {
                        thread = new Thread(new GogocProcess());
                        thread.start();
		}
		return super.onStartCommand(intent, flags, startId);
	}

	@Override
	public void onDestroy() {
		unregisterReceiver(receiver);
		stopProcess();
		super.onDestroy();
	}

        /*
	@Override
	public IBinder onBind(Intent intent) {
		return null;
	}
        */

	private void sendToActivity(String tag, String content) {
		Log.d(TAG, "send to activity: " + tag + content);
		Intent intent = new Intent();
		intent.setAction("org.nklog.gogoc.GogocActivity");
		intent.putExtra("data", tag + content);
		sendBroadcast(intent);
	}

	private class Receiver extends BroadcastReceiver {
		@Override
		public void onReceive(Context context, Intent intent) {
			String command = intent.getStringExtra("command");
			Log.d(TAG, "receive " + command + " from activity");
			if (command.compareTo("quit") == 0) {
				stopProcess();
			} else if (command.compareTo("status") == 0) {
				try {
                                        // TODO return status
                                        // sendToActivity("S", "online");
                                        // sendToActivity("S", "offline");
				} catch (IllegalThreadStateException e) {
					sendToActivity("S", "running");
				}
			}
		}
	}

	private boolean startProcess() {
		int retcode = -1;
		Log.d(TAG, "startProcess, process");

                // TODO check if running

		retcode = -1;
		sendToActivity("S", "running");
		try {
			Log.d(TAG, "start new process: " + getFileStreamPath("gogoc").getAbsolutePath());
                        startup();
                        // TODO
			// retcode = process.waitFor();
		} catch (Exception e) {
			Log.e(TAG, "gogoc failed", e);
		}
		if (retcode != 0) {
			sendToActivity("E", "gogoc");
			return false;
		} else {
			sendToActivity("S", "online");
			try {
                                // TODO set DNS
			} catch (Exception e) {
			}
			return true;
		}
	}

	private void stopProcess() {
		int retcode;
		if (thread != null) {
			thread.interrupt();
			thread = null;
		}

                Log.d(TAG, "gogoc destroy");
                shutdown();

		// ok, we have killed the process, then remove the pid
		if (getFileStreamPath("gogoc.pid").exists()) {
			getFileStreamPath("gogoc.pid").delete();
		}

		if (getFileStreamPath("gogoc.log").exists()) {
			getFileStreamPath("gogoc.log").delete();
		}
	}

	private class GogocProcess implements Runnable {
		public GogocProcess() {
		}
		public void run() {
			try{
				startProcess();
			} catch (Exception e) {
				Log.d(TAG, "Process", e);
			}
		}
	}

        static {
                System.loadLibrary("gogocjni");
        }

        native private void startup();
        native private void shutdown();
}
