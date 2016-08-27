package pmurray_at_bigpond_dot_com.fishtank;

import android.app.Activity;

import pmurray_at_bigpond_dot_com.arddrive.BluetoothService;

/**
 * Created by pmurray on 27/08/2016.
 */
public class MoonProtocol {
    static void sendRefreshStatusMessage(Activity context) {
        byte[] msg = new byte[1];
        msg[0] = '?';
        BluetoothService.startActionSendMessage(context, msg);
    }

    static void sendChangeColorMessage(Activity context, byte r, byte g, byte b) {
        byte[] msg = new byte[4];
        msg[0] = 'C';
        msg[1] = r;
        msg[2] = g;
        msg[3] = b;
        BluetoothService.startActionSendMessage(context, msg);
    }

    private static void sendShortMessage(Activity context, byte action, short n) {
        byte[] msg = new byte[5];
        msg[0] = action;
        msg[3] = (byte) (n >> 8);
        msg[4] = (byte) (n >> 0);
        BluetoothService.startActionSendMessage(context, msg);
    }

    private static void sendByteMessage(Activity context, byte action, short n) {
        byte[] msg = new byte[5];
        msg[0] = action;
        msg[3] = (byte) (n >> 8);
        msg[4] = (byte) (n >> 0);
        BluetoothService.startActionSendMessage(context, msg);
    }

    private static void sendTimeMessage(Activity context, byte action, int time) {
        while(time < 0) time += 60*60*24;
        time %= 60*60*24;
        byte[] msg = new byte[5];
        msg[0] = action;
        msg[1] = (byte) (time >> 24);
        msg[2] = (byte) (time >> 16);
        msg[3] = (byte) (time >> 8);
        msg[4] = (byte) (time >> 0);
        BluetoothService.startActionSendMessage(context, msg);
    }

    static void sendSetTimeofdayMessage(Activity context, int time) {
        sendTimeMessage(context, (byte) 'T', time);
    }

    static void sendSetMoonriseTimeMessage(Activity context, int time) {
        sendTimeMessage(context, (byte) 'R', time);
    }

    static void sendSetMoonsetTimeMessage(Activity context, int time) {
        sendTimeMessage(context, (byte) 'S', time);
    }

    static void sendSetMoonWidthMessage(Activity context, short width) {
        sendShortMessage(context, (byte) 'W', width);
    }

    static void sendSetMoonBrightnessMessage(Activity context, byte brightness) {
        sendByteMessage(context, (byte) 'B', brightness);
    }

    static void sendNumpixelsMessage(Activity context, short numPixels) {
        sendShortMessage(context, (byte) 'N', numPixels);
    }
}
