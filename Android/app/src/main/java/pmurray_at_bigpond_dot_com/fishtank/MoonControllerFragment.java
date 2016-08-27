package pmurray_at_bigpond_dot_com.fishtank;

import android.app.ActionBar;
import android.app.Activity;
import android.app.FragmentTransaction;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentPagerAdapter;
import android.support.v4.view.PagerAdapter;
import android.support.v4.view.PagerTabStrip;
import android.support.v4.view.PagerTitleStrip;
import android.support.v4.view.ViewPager;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;

import pmurray_at_bigpond_dot_com.arddrive.BluetoothService;
import pmurray_at_bigpond_dot_com.arddrive.R;

public class MoonControllerFragment extends Fragment {

    MoonScheduleFragment schedule = new MoonScheduleFragment();
    MoonAppearanceFragment appearance = new MoonAppearanceFragment();

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {

        setHasOptionsMenu(true);
        return inflater.inflate(R.layout.fragment_moon_controller, container, false);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        final ActionBar actionBar = getActivity().getActionBar();
        // an initial fetch of the status
        MoonProtocol.sendRefreshStatusMessage(getActivity());

    }


    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        super.onCreateOptionsMenu(menu, inflater);
        inflater.inflate(R.menu.menu_moon_controller, menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();
        if (id == R.id.action_moon_appearance_tab) {
            //Do whatever you want to do
            return true;
        } else if (id == R.id.action_moon_schedule_tab) {
            //Do whatever you want to do
            return true;
        } else if (id == R.id.action_get_status) {
            MoonProtocol.sendRefreshStatusMessage(getActivity());
            return true;
        } else {
            return super.onOptionsItemSelected(item);
        }
    }
}
