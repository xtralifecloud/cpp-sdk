package cloud.xtralife.simpleapp;

import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.os.Handler;

import java.util.Timer;
import java.util.TimerTask;

import cloud.xtralife.sdk.Clan;

public class MainActivity extends AppCompatActivity {

    //	We need a timer and handler to call the XtraLife_Update() function regularly.
    private Timer clanTimer = null;
    private Handler clanHandler = null;
    //	The 2 bridges to the 2 C++ functions we need to call.
    public native void XtraLifeSetupJNIBridge();
    public native void XtraLifeUpdateJNIBridge();

    static  {
        System.loadLibrary("xtralife");
        System.loadLibrary("simpleapp");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Clan.Init(this);

        //	First, we setup the environment by calling into C++.
        XtraLifeSetupJNIBridge();

        //	Then we immediately instantiate a timer (100ms) which will regularly
        //	poll to check if new notifications are pending.
        clanHandler = new Handler();
        clanTimer	= new Timer("clanTimer");
        clanTimer.schedule(new ClanUpdateTask(), 0, 100);

    }

    //	The timer will just call into C++ and ensure that incoming notifications
    //	are processed.
    private class ClanUpdateTask extends TimerTask {

        @Override
        public void run() {
            clanHandler.post(new Runnable() {

                public void run() {
                    // TODO Auto-generated method stub
                    XtraLifeUpdateJNIBridge();

                }

            });
        }
    }
}
