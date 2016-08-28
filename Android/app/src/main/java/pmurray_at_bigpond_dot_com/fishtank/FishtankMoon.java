package pmurray_at_bigpond_dot_com.fishtank;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.support.annotation.NonNull;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v4.app.ActivityCompat;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;

import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentPagerAdapter;
import android.support.v4.view.ViewPager;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;

import android.widget.TextView;

import pmurray_at_bigpond_dot_com.arddrive.ArduinoMessagesFragment;
import pmurray_at_bigpond_dot_com.arddrive.R;
import pmurray_at_bigpond_dot_com.arddrive.SelectDevice;

public class FishtankMoon extends AppCompatActivity {
    private static final int RETRY_CHOOSE_BLUETOOTH = 0xBEEF + 1;

    private ViewPager mViewPager;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_fishtank_moon);

        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        mViewPager = (ViewPager) findViewById(R.id.container);
        mViewPager.setAdapter(new FragmentPagerAdapter(getSupportFragmentManager()) {
            @Override
            public int getCount() {
                return 3;
            }

            @Override
            public Fragment getItem(int position) {
                switch (position) {
                    case 0:
                        return new MoonScheduleFragment();
                    case 1:
                        return new MoonAppearanceFragment();
                    case 2:
                        return new ArduinoMessagesFragment();
                }
                return null;
            }

            @Override
            public CharSequence getPageTitle(int position) {
                switch (position) {
                    case 0:
                        return "Schedule";
                    case 1:
                        return "Appearance";
                    case 2:
                        return "Messages";
                }
                return null;
            }

        });
    }


    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_fishtank_moon, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_select_device) {
            chooseBluetooth();
            return true;
        } else if (id == R.id.action_get_arduino_status) {
            MoonProtocol.sendRefreshStatusMessage(this);
            return true;
        }

        return super.onOptionsItemSelected(item);
    }


    protected void chooseBluetooth() {
        if (obtainBlueToothPermission(RETRY_CHOOSE_BLUETOOTH)) {
            startActivity(new Intent(this, SelectDevice.class));
        }
    }

    protected boolean obtainBlueToothPermission(final int callbackCode) {
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH) == PackageManager.PERMISSION_GRANTED) {
            return true;
        } else {
            if (ActivityCompat.shouldShowRequestPermissionRationale(this, Manifest.permission.BLUETOOTH)) {
                Snackbar.make(findViewById(android.R.id.content), R.string.bluetoothRationale, Snackbar.LENGTH_INDEFINITE)
                        .setAction("OK", new View.OnClickListener() {
                            @Override
                            public void onClick(View view) {
                                retry(callbackCode);
                            }
                        })
                        .setAction("Cancel", null)
                        .show();
            } else {
                ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.BLUETOOTH}, callbackCode);
            }
            return false;
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String permissions[], @NonNull int[] grantResults) {
        for (int r : grantResults) {
            if (r != PackageManager.PERMISSION_GRANTED) {
                Snackbar.make(findViewById(android.R.id.content), R.string.permissionRefused, Snackbar.LENGTH_LONG).show();
                return;
            }
        }

        retry(requestCode);
    }

    protected void onActivityResult(int requestCode,
                                    int resultCode,
                                    Intent data) {
        if (resultCode == RESULT_OK) {
            retry(requestCode);
        } else {
            Snackbar.make(findViewById(android.R.id.content), R.string.activityUnsuccessful, Snackbar.LENGTH_LONG).show();
        }
    }

    private void retry(int requestCode) {
        switch (requestCode) {
            case RETRY_CHOOSE_BLUETOOTH:
                chooseBluetooth();
                break;
        }
    }


}
