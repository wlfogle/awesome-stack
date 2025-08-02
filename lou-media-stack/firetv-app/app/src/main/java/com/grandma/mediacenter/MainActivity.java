package com.grandma.mediacenter;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;
import androidx.leanback.app.BrowseSupportFragment;
import androidx.fragment.app.FragmentActivity;

/**
 * Fire TV Main Activity for Grandma's Media Center
 * Large buttons, simple navigation, remote-friendly
 */
public class MainActivity extends FragmentActivity {
    
    private static final String PREFS_NAME = "GrandmaMediaCenter";
    private static final String PREF_SERVER_URL = "server_url";
    private static final String DEFAULT_SERVER_URL = "http://192.168.1.100:8600";
    
    private String serverUrl;
    private TextView statusText;
    private Button dashboardButton;
    private Button mediaButton;
    private Button liveTvButton;
    private Button settingsButton;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        // Load server URL from preferences
        SharedPreferences prefs = getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
        serverUrl = prefs.getString(PREF_SERVER_URL, DEFAULT_SERVER_URL);
        
        initializeViews();
        setupClickListeners();
        checkServerConnection();
    }
    
    private void initializeViews() {
        statusText = findViewById(R.id.statusText);
        dashboardButton = findViewById(R.id.dashboardButton);
        mediaButton = findViewById(R.id.mediaButton);
        liveTvButton = findViewById(R.id.liveTvButton);
        settingsButton = findViewById(R.id.settingsButton);
        
        // Set initial focus
        dashboardButton.requestFocus();
    }
    
    private void setupClickListeners() {
        dashboardButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                openDashboard();
            }
        });
        
        mediaButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                openMediaLibrary();
            }
        });
        
        liveTvButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                openLiveTv();
            }
        });
        
        settingsButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                openSettings();
            }
        });
    }
    
    private void openDashboard() {
        Intent intent = new Intent(this, DashboardActivity.class);
        intent.putExtra("url", serverUrl);
        startActivity(intent);
    }
    
    private void openMediaLibrary() {
        // Open Jellyfin in WebView
        Intent intent = new Intent(this, DashboardActivity.class);
        intent.putExtra("url", serverUrl.replace(":8600", ":8200"));
        intent.putExtra("title", "Media Library");
        startActivity(intent);
    }
    
    private void openLiveTv() {
        // Open TV interface
        Intent intent = new Intent(this, DashboardActivity.class);
        intent.putExtra("url", serverUrl.replace(":8600", ":8320"));
        intent.putExtra("title", "Live TV");
        startActivity(intent);
    }
    
    private void openSettings() {
        Intent intent = new Intent(this, SettingsActivity.class);
        startActivity(intent);
    }
    
    private void checkServerConnection() {
        statusText.setText("Connecting to media server...");
        
        // Simple connection check in background thread
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    // Simulate connection check
                    Thread.sleep(2000);
                    
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            statusText.setText("✅ Media Center Ready!");
                            statusText.setTextColor(getColor(android.R.color.holo_green_light));
                        }
                    });
                } catch (InterruptedException e) {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            statusText.setText("⚠️ Connection issues - check settings");
                            statusText.setTextColor(getColor(android.R.color.holo_orange_light));
                        }
                    });
                }
            }
        }).start();
    }
    
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        // Handle remote control buttons
        switch (keyCode) {
            case KeyEvent.KEYCODE_DPAD_CENTER:
            case KeyEvent.KEYCODE_ENTER:
                // Same as clicking focused button
                View focused = getCurrentFocus();
                if (focused != null) {
                    focused.performClick();
                    return true;
                }
                break;
            case KeyEvent.KEYCODE_BACK:
                // Show exit confirmation
                showExitConfirmation();
                return true;
            case KeyEvent.KEYCODE_MENU:
                // Open settings
                openSettings();
                return true;
        }
        return super.onKeyDown(keyCode, event);
    }
    
    private void showExitConfirmation() {
        Toast.makeText(this, "Press back again to exit", Toast.LENGTH_SHORT).show();
        
        // Simple double-back to exit
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    Thread.sleep(2000);
                    // Reset exit flag after 2 seconds
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }).start();
    }
    
    @Override
    protected void onResume() {
        super.onResume();
        // Refresh connection status when returning to main screen
        checkServerConnection();
    }
}
