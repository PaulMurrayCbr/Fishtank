package pmurray_at_bigpond_dot_com.fishtank;

import android.content.Context;
import android.net.Uri;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.ToggleButton;

import pmurray_at_bigpond_dot_com.arddrive.R;


public class MoonScheduleFragment extends Fragment {

    SeekBar moonriseBar;
    SeekBar moontimeBar;
    SeekBar moonsetBar;
    TextView moonriseValue;
    TextView moontimeValue;
    TextView moonsetValue;
    ToggleButton moonFast;

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {

        View rootView = inflater.inflate(R.layout.fragment_moon_schedule, container, false);

        moonriseBar = (SeekBar) rootView.findViewById(R.id.moonriseBar);
        moontimeBar = (SeekBar) rootView.findViewById(R.id.moonTimeBar);
        moonsetBar = (SeekBar) rootView.findViewById(R.id.moonsetBar);

        moonriseValue = (TextView) rootView.findViewById(R.id.moonriseBarValue);
        moontimeValue = (TextView) rootView.findViewById(R.id.moonTimeBarValue);
        moonsetValue = (TextView) rootView.findViewById(R.id.moonsetBarValue);
        moonFast = (ToggleButton) rootView.findViewById(R.id.moonFastTime);

        moonriseBar.setMax(24 * 60);
        moonriseBar.setProgress(6 * 60);
        moonriseValue.setText("06:00 PM");

        moontimeBar.setMax(24 * 60);
        moontimeBar.setProgress(12 * 60);
        moontimeValue.setText("00:00 AM");

        moonsetBar.setMax(24 * 60);
        moonsetBar.setProgress(18 * 60);
        moonsetValue.setText("06:00 AM");


        moonriseBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                moonriseValue.setText((progress / 60) % 12 + ":" + (progress % 60) + (progress < 12 * 60 ? " PM" : " AM"));
                if (fromUser) {
                    MoonProtocol.sendSetMoonriseTimeMessage(getActivity(), ((progress + (12 * 60)) % (24 * 60)) * 60);
                }
            }

            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });

        moontimeBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                moontimeValue.setText((progress / 60) % 12 + ":" + (progress % 60) + (progress < 12 * 60 ? " PM" : " AM"));
                if (fromUser) {
                    MoonProtocol.sendSetTimeofdayMessage(getActivity(), ((progress + (12 * 60)) % (24 * 60)) * 60);
                }
            }

            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });

        moonsetBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                moonsetValue.setText((progress / 60) % 12 + ":" + (progress % 60) + (progress < 12 * 60 ? " PM" : " AM"));
                if (fromUser) {
                    MoonProtocol.sendSetMoonsetTimeMessage(getActivity(), ((progress + (12 * 60)) % (24 * 60)) * 60);
                }
            }

            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });

        moonFast.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                MoonProtocol.sendSetFastMessage(getActivity(), b);
            }
        });


        return rootView;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
    }
}
