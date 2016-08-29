package pmurray_at_bigpond_dot_com.fishtank;

import android.app.Activity;
import android.content.Intent;

import pmurray_at_bigpond_dot_com.arddrive.BluetoothService;

/**
 * Created by pmurray on 27/08/2016.
 */
public class MoonProtocol {

    public static class InfoPacket {
        /*
        struct StatusBuffer {
          byte messageMark[1];  // 0-1
          byte rgb[3];          // 1-4
          byte numpixels[2];    // 4-6
          byte moonWidth[2];    // 6-8
          byte moonBright[1];   // 8-9
          byte moonrise[4];     // 9-13
          byte moonset[4];      // 13-17
          byte time[4];         // 17-21
          byte flags[1];        // 21-22
        };
        */
        byte r, g, b;
        int rgb;
        short numPixels;
        short moonWidth;
        byte moonBright;
        int moonrise;
        int moonset;
        int time;
        boolean fast;
    }

    static void sendRefreshStatusMessage(Activity context) {
        byte[] msg = new byte[1];
        msg[0] = '?';
        BluetoothService.startActionSendMessage(context, (int) '?', msg);
    }

    static void sendChangeColorMessage(Activity context, byte r, byte g, byte b) {
        byte[] msg = new byte[4];
        msg[0] = 'C';
        msg[1] = r;
        msg[2] = g;
        msg[3] = b;
        BluetoothService.startActionSendMessage(context, (int) 'C', msg);
    }

    private static void sendByteMessage(Activity context, byte action, byte n) {
        byte[] msg = new byte[2];
        msg[0] = action;
        msg[1] = n;
        BluetoothService.startActionSendMessage(context, (int) action, msg);
    }


    private static void sendShortMessage(Activity context, byte action, short n) {
        byte[] msg = new byte[3];
        msg[0] = action;
        msg[1] = (byte) (n >> 8);
        msg[2] = (byte) (n >> 0);
        BluetoothService.startActionSendMessage(context, (int) action, msg);
    }

    private static void sendTimeMessage(Activity context, byte action, int time) {
        while (time < 0) time += 60 * 60 * 24;
        time %= 60 * 60 * 24;
        byte[] msg = new byte[5];
        msg[0] = action;
        msg[1] = (byte) (time >> 24);
        msg[2] = (byte) (time >> 16);
        msg[3] = (byte) (time >> 8);
        msg[4] = (byte) (time >> 0);
        BluetoothService.startActionSendMessage(context, (int) action, msg);
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

    static void sendSetFastMessage(Activity context, boolean fast) {
        sendByteMessage(context, (byte) '!', fast ? (byte) 1 : (byte) 0);
    }

    static InfoPacket decodeInfo(Intent intent) {
        if (intent == null) return null;
        if (!BluetoothService.BROADCAST_MESSAGE_RECEIVED.equals(intent.getAction())) return null;
        byte[] msg = intent.getByteArrayExtra(BluetoothService.EXTRA_BYTES);
        if (msg == null) return null;
        if (msg.length != 22) return null;
        InfoPacket info = new InfoPacket();

        /*
        struct StatusBuffer {
          byte messageMark[1];  // 0-1
          byte rgb[3];          // 1-4
          byte numpixels[2];    // 4-6
          byte moonWidth[2];    // 6-8
          byte moonBright[1];   // 8-9
          byte moonrise[4];     // 9-13
          byte moonset[4];      // 13-17
          byte time[4];         // 17-21
          byte flags[1];        // 21-22
        };
        */

        info.r = msg[1];
        info.g = msg[2];
        info.b = msg[3];
        for(int i = 1;i<4;i++) {
            info.rgb = (short)((info.rgb<<8) | (((int)msg[i]) & 0xFF));

        }
        for(int i = 4;i<6;i++) {
            info.numPixels = (short)((info.numPixels<<8) | (((int)msg[i]) & 0xFF));

        }
        for(int i = 6;i<8;i++) {
            info.moonWidth = (short)((info.moonWidth<<8) | (((int)msg[i]) & 0xFF));
        }
        info.moonBright = msg[8];
        for(int i = 9;i<13;i++) {
            info.moonrise = (int)((info.moonrise<<8) | (((int)msg[i]) & 0xFF));
        }
        for(int i = 13;i<17;i++) {
            info.moonset = (int)((info.moonset<<8) | (((int)msg[i]) & 0xFF));
        }
        for(int i = 17;i<21;i++) {
            info.time = (int)((info.time<<8) | (((int)msg[i]) & 0xFF));
        }
        info.fast = (msg[21] & 1) != 0;

        return info;

    }


}
