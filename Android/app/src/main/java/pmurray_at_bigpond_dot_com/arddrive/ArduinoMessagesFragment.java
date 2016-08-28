package pmurray_at_bigpond_dot_com.arddrive;

import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.content.LocalBroadcastManager;
import android.support.v7.widget.Toolbar;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

import static pmurray_at_bigpond_dot_com.arddrive.BluetoothService.EXTRA_BYTES;
import static pmurray_at_bigpond_dot_com.arddrive.BluetoothService.EXTRA_BYTES_N;
import static pmurray_at_bigpond_dot_com.arddrive.BluetoothService.EXTRA_EXCEPTION;
import static pmurray_at_bigpond_dot_com.arddrive.BluetoothService.EXTRA_SERIAL_RX;
import static pmurray_at_bigpond_dot_com.arddrive.BluetoothService.EXTRA_SERIAL_TX;
import static pmurray_at_bigpond_dot_com.arddrive.BluetoothService.addBroadcastsNotIncludingBytes;

public class ArduinoMessagesFragment extends Fragment {

    static class BtBroadcastBean {

        final String mac;
        final String act;
        final int tx, rx, nb;
        final String uuid;
        final String txt;

        BtBroadcastBean(Intent msg) {
            if (msg.hasExtra(BluetoothDevice.EXTRA_DEVICE)) {
                BluetoothDevice device =
                        msg.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                mac = device.getName();
            } else {
                mac = "NO DEVICE";
            }

            String s = msg.getAction();
            if (s != null) s = s.substring(s.lastIndexOf('.') + 1);
            act = (s);

            if (msg.hasExtra(EXTRA_SERIAL_TX)) {
                tx = msg.getIntExtra(EXTRA_SERIAL_TX, -1);
            } else {
                tx = -1;
            }

            if (msg.hasExtra(EXTRA_SERIAL_RX)) {
                rx = msg.getIntExtra(EXTRA_SERIAL_RX, -1);
            } else rx = -1;


            if (msg.hasExtra(BluetoothDevice.EXTRA_UUID)) {
                uuid = (msg.getParcelableExtra(BluetoothDevice.EXTRA_UUID).toString());
            } else uuid = null;

            if (msg.hasExtra(EXTRA_BYTES_N)) {
                nb = msg.getIntExtra(EXTRA_BYTES_N, -1);
            } else nb = -1;

            StringBuilder sb = new StringBuilder();
            sb.append(' ');

            if (msg.hasExtra(EXTRA_BYTES)) {
                byte[] b = msg.getByteArrayExtra(EXTRA_BYTES);
                int bytes = nb == -1 || nb > b.length ? b.length : nb;
                sb.append(" [");
                for (int i = 0; i < bytes; i++) {
                    byte bb = b[i];
                    if (bb >= ' ' && bb < 127)
                        sb.append((char) bb);
                    else {
                        sb.append('[');
                        byte bbb = (byte) ((bb >> 4) & 0xf);
                        sb.append((char) (bbb + (bbb < 10 ? '0' : 'a' - 10)));
                        bbb = (byte) ((bb) & 0xf);
                        sb.append((char) (bbb + (bbb < 10 ? '0' : 'a' - 10)));
                        sb.append(']');
                    }
                }
                sb.append("]");
            }

            if (msg.hasExtra(EXTRA_EXCEPTION)) {
                sb.append(msg.getStringExtra(EXTRA_EXCEPTION));
            }

            txt = sb.toString();
        }
    }


    static class BroadcastsAdapter extends ArrayAdapter<BtBroadcastBean> {
        public BroadcastsAdapter(Context context, List<BtBroadcastBean> messages) {
            super(context, R.layout.device_list_item, messages);
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            // Get the data item for this position
            BtBroadcastBean msg = getItem(position);
            // Check if an existing view is being reused, otherwise inflate the view
            if (convertView == null) {
                convertView = LayoutInflater.from(getContext()).inflate(R.layout.broadcast_list_item, parent, false);
            }
            // Lookup view for data population
            TextView mac = (TextView) convertView.findViewById(R.id.broadcast_mac);
            TextView act = (TextView) convertView.findViewById(R.id.broadcast_action);
            TextView txt = (TextView) convertView.findViewById(R.id.broadcast_text);
            TextView nb = (TextView) convertView.findViewById(R.id.broadcast_nBytes);
            TextView rx = (TextView) convertView.findViewById(R.id.broadcast_rxSerial);
            TextView tx = (TextView) convertView.findViewById(R.id.broadcast_txSerial);
            TextView uuid = (TextView) convertView.findViewById(R.id.broadcast_uuid);

            mac.setText(msg.mac);
            act.setText(msg.act);
            txt.setText(msg.txt);
            nb.setText(msg.nb == -1 ? null : Integer.toString(msg.nb));
            tx.setText(msg.tx == -1 ? null : Integer.toString(msg.tx));
            rx.setText(msg.rx == -1 ? null : Integer.toString(msg.rx));
            uuid.setText(msg.uuid);

            return convertView;
        }
    }

    BroadcastsAdapter broadcastsAdapter;

    final BroadcastReceiver broadcastReciever = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            while (broadcastsAdapter.getCount() >= 20) {
                broadcastsAdapter.remove(broadcastsAdapter.getItem(0));
            }
            broadcastsAdapter.add(new BtBroadcastBean(intent));
        }
    };


    final BroadcastReceiver btReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            while (broadcastsAdapter.getCount() >= 20) {
                broadcastsAdapter.remove(broadcastsAdapter.getItem(0));
            }
            broadcastsAdapter.add(new BtBroadcastBean(intent));
        }
    };

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        broadcastsAdapter = new BroadcastsAdapter(getActivity(), new ArrayList<BtBroadcastBean>());
        ListView broadcastList = (ListView) getActivity().findViewById(R.id.arduinoMessageList);
        broadcastList.setAdapter(broadcastsAdapter);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        return inflater.inflate(R.layout.fragment_arduino_messages, container, false);
    }

    @Override
    public void onStart() {
        super.onStart();

        LocalBroadcastManager.getInstance(getActivity().getApplicationContext()).registerReceiver(broadcastReciever, addBroadcastsNotIncludingBytes(new IntentFilter()));

        getActivity().registerReceiver(btReceiver, BluetoothService.addBluetoothBroadcasts(new IntentFilter()));
    }

    @Override
    public void onStop() {
        LocalBroadcastManager.getInstance(getActivity().getApplicationContext()).unregisterReceiver(broadcastReciever);
        getActivity().unregisterReceiver(btReceiver);
        super.onStop();
    }
}
