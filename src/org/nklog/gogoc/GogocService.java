/* vim: set sw=4 ts=4: */

package org.nklog.gogoc;

import android.os.IBinder;
import android.os.ParcelFileDescriptor;
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
        public final static String STATUS_OFFLINE = "offline";
        public final static String STATUS_ONLINE = "online";
        public final static String STATUS_RUNNING = "running";

        private String mStatus = STATUS_OFFLINE;

	private Receiver receiver = null;

	private final String TAG = "GogocService";
	private Thread thread = null;

	private SharedPreferences preference;

        // Called from thread
        synchronized private void setStatus(String newStatus) {
                mStatus = newStatus;
                sendToActivity("S", mStatus);
        }

        synchronized private String getStatus() {
                return mStatus;
        }

        public class Builder extends VpnService.Builder {
                public ParcelFileDescriptor establish () {
                        ParcelFileDescriptor parcel = super.establish();
                        setStatus(STATUS_ONLINE);

                        return parcel;
                }
        }

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

	@Override
	public void onRevoke() {
		Log.d(TAG, "onRevoked called");
                setStatus(STATUS_OFFLINE);
                super.onRevoke();
	}

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
                                sendToActivity("S", getStatus());
			}
		}
	}

	private boolean startProcess() {
		int retcode = -1;
		Log.d(TAG, "startProcess, process");

                // TODO check if running

		retcode = -1;
		setStatus(STATUS_RUNNING);
		try {
			Log.d(TAG, "start new process: " + getFileStreamPath("gogoc").getAbsolutePath());
                        startup(new Builder());
                        // TODO
			// retcode = process.waitFor();
		} catch (Exception e) {
			Log.e(TAG, "gogoc failed", e);
		}
		if (retcode != 0) {
			sendToActivity("E", "gogoc");
			return false;
		} else {
			setStatus(STATUS_ONLINE);
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

        native private void startup(VpnService.Builder builder);
        native private void shutdown();
}
