package pmurray_at_bigpond_dot_com.fishtank;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.content.LocalBroadcastManager;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.NumberPicker;
import android.widget.SeekBar;
import android.widget.TextView;

import com.flask.colorpicker.ColorPickerView;
import com.flask.colorpicker.OnColorSelectedListener;

import pmurray_at_bigpond_dot_com.arddrive.BluetoothService;
import pmurray_at_bigpond_dot_com.arddrive.R;

import static pmurray_at_bigpond_dot_com.arddrive.BluetoothService.startActionSendMessage;

public class MoonAppearanceFragment extends Fragment {

    NumberPicker stripLengthPicker;
    SeekBar brightnessBar;
    TextView brightnessValue;
    SeekBar widthBar;
    TextView widthValue;
    ColorPickerView colourPicker;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {

        View rootView = inflater.inflate(R.layout.fragment_moon_appearance, container, false);

        stripLengthPicker = (NumberPicker) rootView.findViewById(R.id.stripLengthPicker);
        brightnessBar = (SeekBar) rootView.findViewById(R.id.moonBrightnessBar);
        brightnessValue = (TextView) rootView.findViewById(R.id.moonBrightnessBarValue);
        widthBar = (SeekBar) rootView.findViewById(R.id.moonWidthBar);
        widthValue = (TextView) rootView.findViewById(R.id.moonWidthBarValue);
        colourPicker = (ColorPickerView) rootView.findViewById(R.id.color_picker_view);

        brightnessBar.setMax(255);

        stripLengthPicker.setMinValue(1);
        stripLengthPicker.setMaxValue(9999);
        stripLengthPicker.setValue(24);
        widthBar.setMax(24); // meh

        brightnessValue.setText(Integer.toString(4));
        widthValue.setText(Integer.toString(24));

        stripLengthPicker.setOnValueChangedListener(new NumberPicker.OnValueChangeListener() {
                                                        @Override
                                                        public void onValueChange(NumberPicker picker, int oldVal, int newVal) {
                                                            widthBar.setMax(newVal);
                                                            if (widthBar.getProgress() > newVal) {
                                                                widthBar.setProgress(newVal);
                                                            }
                                                            MoonProtocol.sendNumpixelsMessage(getActivity(), (short) newVal);
                                                        }
                                                    }
        );


        brightnessBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                brightnessValue.setText(Integer.toString(progress));
                if (fromUser) {
                    MoonProtocol.sendSetMoonBrightnessMessage(getActivity(), (byte) progress);
                }
            }

            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });

        widthBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                widthValue.setText(Integer.toString(progress));
                if (fromUser) {
                    MoonProtocol.sendSetMoonWidthMessage(getActivity(), (byte) progress);
                }
            }

            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });

        colourPicker.addOnColorSelectedListener(new OnColorSelectedListener() {
            @Override
            public void onColorSelected(int selectedColor) {
                MoonProtocol.sendChangeColorMessage(getActivity(),
                        (byte) (selectedColor >> 16),
                        (byte) (selectedColor >> 8),
                        (byte) (selectedColor >> 0));
            }
        });


        return rootView;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
    }

    final BroadcastReceiver broadcastReciever = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if(!BluetoothService.BROADCAST_MESSAGE_RECEIVED.equals(intent.getAction())) return;
            MoonProtocol.InfoPacket info = MoonProtocol.decodeInfo(intent);
            if(info == null) return;

            stripLengthPicker.setValue(info.numPixels);
            brightnessBar.setProgress(info.moonBright);
            widthBar.setProgress(info.moonWidth);
            colourPicker.setColor(info.rgb, true);
            ColorPickerView colourPicker;

        }
    };

    @Override
    public void onStart() {
        super.onStart();
        IntentFilter f = new IntentFilter();
        f.addAction(BluetoothService.BROADCAST_MESSAGE_RECEIVED);
        LocalBroadcastManager.getInstance(getActivity().getApplicationContext()).registerReceiver(broadcastReciever, f);
    }

    @Override
    public void onStop() {
        LocalBroadcastManager.getInstance(getActivity().getApplicationContext()).unregisterReceiver(broadcastReciever);
        super.onStop();
    }

}
