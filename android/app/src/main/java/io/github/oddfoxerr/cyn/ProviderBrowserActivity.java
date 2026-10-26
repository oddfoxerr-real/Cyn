// SPDX-License-Identifier: AGPL-3.0-or-later
package io.github.oddfoxerr.cyn;

import android.content.Intent;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.text.InputType;
import android.view.Gravity;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.webkit.CookieManager;
import android.webkit.WebChromeClient;
import android.webkit.WebResourceError;
import android.webkit.WebResourceRequest;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import com.google.android.material.button.MaterialButton;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import java.util.Locale;

/**
 * Searches through the provider's own mobile website inside CYN.
 * This does not impersonate a provider API or extract protected media streams.
 * Provider playback and sign-in rules continue to apply.
 */
public final class ProviderBrowserActivity extends AppCompatActivity {
    static final String YOUTUBE="youtube", SOUNDCLOUD="soundcloud";
    private static final int BACKGROUND=0xff141416, SURFACE=0xff222326, TEXT=0xffeeeeef, MUTED=0xffb8b8bc;
    private String provider=YOUTUBE;
    private EditText search;
    private TextView status;
    private WebView browser;
    private ProgressBar loading;
    private MaterialButton youtubeTab,soundcloudTab;

    static Uri searchUrl(String source,String query){
        if(query==null||query.trim().isEmpty()||query.length()>300)return null;
        if(YOUTUBE.equals(source))
            return Uri.parse("https://music.youtube.com/search?q="+Uri.encode(query.trim()));
        if(SOUNDCLOUD.equals(source))
            return Uri.parse("https://soundcloud.com/search/sounds?q="+Uri.encode(query.trim()));
        return null;
    }

    static Uri homeUrl(String source){
        if(YOUTUBE.equals(source))return Uri.parse("https://music.youtube.com/");
        if(SOUNDCLOUD.equals(source))return Uri.parse("https://soundcloud.com/");
        return null;
    }

    static boolean isProviderPage(String source,Uri url){
        if(url==null||!"https".equalsIgnoreCase(url.getScheme())||
            (url.getPort()!=-1&&url.getPort()!=443)||url.getHost()==null||
            (url.getEncodedAuthority()!=null&&url.getEncodedAuthority().contains("@")))return false;
        String host=url.getHost().toLowerCase(Locale.ROOT);
        if(YOUTUBE.equals(source))
            return host.equals("music.youtube.com")||host.equals("youtube.com")||
                   host.equals("www.youtube.com")||host.equals("m.youtube.com")||
                   host.equals("youtu.be");
        if(SOUNDCLOUD.equals(source))
            return host.equals("soundcloud.com")||host.equals("www.soundcloud.com")||
                   host.equals("m.soundcloud.com")||host.equals("on.soundcloud.com");
        return false;
    }

    private int dp(int size){return Math.round(getResources().getDisplayMetrics().density*size);}

    private MaterialButton action(String name){
        MaterialButton button=new MaterialButton(this);
        button.setText(name);button.setAllCaps(false);button.setTextSize(13);
        button.setMinHeight(dp(48));button.setCornerRadius(dp(24));
        button.setInsetTop(0);button.setInsetBottom(0);
        button.setTextColor(TEXT);
        button.setBackgroundTintList(android.content.res.ColorStateList.valueOf(SURFACE));
        return button;
    }

    @Override protected void onCreate(Bundle state){
        super.onCreate(state);
        getWindow().setStatusBarColor(BACKGROUND);
        getWindow().setNavigationBarColor(BACKGROUND);
        String selected=getIntent().getStringExtra("provider");
        provider=SOUNDCLOUD.equals(selected)?SOUNDCLOUD:YOUTUBE;

        LinearLayout shell=new LinearLayout(this);
        shell.setOrientation(LinearLayout.VERTICAL);
        shell.setBackgroundColor(BACKGROUND);
        shell.setPadding(dp(12),dp(8),dp(12),dp(8));
        setContentView(shell);

        LinearLayout heading=new LinearLayout(this);
        heading.setGravity(Gravity.CENTER_VERTICAL);
        MaterialButton back=action("‹ Back");
        back.setOnClickListener(v->finish());
        heading.addView(back,new LinearLayout.LayoutParams(dp(90),dp(48)));
        TextView title=new TextView(this);
        title.setText("CYN · Music search");
        title.setTextColor(TEXT);title.setTextSize(19);
        title.setGravity(Gravity.CENTER_VERTICAL);
        title.setPadding(dp(8),0,0,0);
        title.setAccessibilityHeading(true);
        heading.addView(title,new LinearLayout.LayoutParams(0,dp(48),1));
        shell.addView(heading);

        LinearLayout tabs=new LinearLayout(this);
        youtubeTab=action("YouTube Music");
        soundcloudTab=action("SoundCloud");
        LinearLayout.LayoutParams left=new LinearLayout.LayoutParams(0,dp(50),1);
        left.setMargins(0,0,dp(4),0);
        tabs.addView(youtubeTab,left);
        LinearLayout.LayoutParams right=new LinearLayout.LayoutParams(0,dp(50),1);
        right.setMargins(dp(4),0,0,0);
        tabs.addView(soundcloudTab,right);
        shell.addView(tabs);
        youtubeTab.setOnClickListener(v->switchProvider(YOUTUBE));
        soundcloudTab.setOnClickListener(v->switchProvider(SOUNDCLOUD));

        LinearLayout searchRow=new LinearLayout(this);
        searchRow.setGravity(Gravity.CENTER_VERTICAL);
        search=new EditText(this);
        search.setSingleLine(true);
        search.setHint("Search songs, artists, playlists");
        search.setHintTextColor(MUTED);
        search.setTextColor(TEXT);
        search.setTextSize(15);
        search.setInputType(InputType.TYPE_CLASS_TEXT);
        search.setImeOptions(EditorInfo.IME_ACTION_SEARCH);
        search.setContentDescription("Search music on selected service");
        searchRow.addView(search,new LinearLayout.LayoutParams(0,dp(56),1));
        MaterialButton submit=action("Search");
        submit.setOnClickListener(v->submitSearch());
        LinearLayout.LayoutParams searchBtn=new LinearLayout.LayoutParams(dp(90),dp(50));
        searchBtn.setMargins(dp(8),0,0,0);searchRow.addView(submit,searchBtn);
        search.setOnEditorActionListener((v,id,event)->{
            if(id==EditorInfo.IME_ACTION_SEARCH){submitSearch();return true;}
            return false;
        });
        shell.addView(searchRow);

        LinearLayout actions=new LinearLayout(this);
        MaterialButton save=action("Save track");
        MaterialButton external=action("Open original");
        MaterialButton previous=action("‹ Page");
        actions.addView(save,new LinearLayout.LayoutParams(0,dp(48),1));
        actions.addView(external,new LinearLayout.LayoutParams(0,dp(48),1));
        actions.addView(previous,new LinearLayout.LayoutParams(0,dp(48),1));
        shell.addView(actions);
        save.setOnClickListener(v->saveCurrent());
        external.setOnClickListener(v->openExternally());
        previous.setOnClickListener(v->{if(browser!=null&&browser.canGoBack())browser.goBack();});

        status=new TextView(this);
        status.setTextColor(MUTED);status.setTextSize(12);
        status.setMaxLines(2);status.setPadding(dp(8),dp(6),dp(8),dp(6));
        status.setText("Browse actual service search results inside CYN. Playback is provider-controlled.");
        status.setAccessibilityLiveRegion(View.ACCESSIBILITY_LIVE_REGION_POLITE);
        shell.addView(status);

        loading=new ProgressBar(this,null,android.R.attr.progressBarStyleHorizontal);
        loading.setMax(100);
        loading.setProgressTintList(android.content.res.ColorStateList.valueOf(TEXT));
        loading.setVisibility(View.INVISIBLE);
        shell.addView(loading,new LinearLayout.LayoutParams(-1,dp(3)));

        browser=new WebView(this);
        browser.setBackgroundColor(BACKGROUND);
        browser.setContentDescription("Provider music search results");
        WebSettings settings=browser.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setDomStorageEnabled(true);
        settings.setAllowContentAccess(false);
        settings.setAllowFileAccess(false);
        settings.setJavaScriptCanOpenWindowsAutomatically(false);
        settings.setSupportMultipleWindows(false);
        settings.setMixedContentMode(WebSettings.MIXED_CONTENT_NEVER_ALLOW);
        settings.setMediaPlaybackRequiresUserGesture(true);
        settings.setSafeBrowsingEnabled(true);
        browser.setWebChromeClient(new WebChromeClient(){
            @Override public void onProgressChanged(WebView view,int progress){
                if(loading==null)return;
                loading.setVisibility(progress>=100?View.INVISIBLE:View.VISIBLE);
                loading.setProgress(progress);
            }
        });
        browser.setWebViewClient(new WebViewClient(){
            @Override public boolean shouldOverrideUrlLoading(WebView view,WebResourceRequest request){
                Uri uri=request.getUrl();
                if(isProviderPage(provider,uri))return false;
                if(request.isForMainFrame()){
                    if("https".equalsIgnoreCase(uri.getScheme()))openSystem(uri);
                    else status.setText("Unsupported external link blocked");
                }
                return true;
            }
            @Override public void onReceivedError(WebView view,WebResourceRequest request,WebResourceError error){
                if(request.isForMainFrame())
                    status.setText("This provider page couldn't load inside CYN. Try Open original.");
            }
            @Override public void onPageFinished(WebView view,String url){
                if(status!=null&&isProviderPage(provider,Uri.parse(url)))
                    status.setText("Browsing "+(YOUTUBE.equals(provider)?"YouTube Music":"SoundCloud")+
                        " inside CYN · Save track adds a link to your Library");
            }
        });
        shell.addView(browser,new LinearLayout.LayoutParams(-1,0,1));

        switchStyle();
        String initialQuery=getIntent().getStringExtra("query");
        if(initialQuery!=null)search.setText(initialQuery);
        String initialUrl=getIntent().getStringExtra("url");
        Uri url=initialUrl==null?null:Uri.parse(initialUrl);
        if(url!=null&&isProviderPage(provider,url))browser.loadUrl(url.toString());
        else{
            Uri first=searchUrl(provider,initialQuery);
            if(first==null)first=homeUrl(provider);
            if(first!=null)browser.loadUrl(first.toString());
        }
    }

    private void switchStyle(){
        youtubeTab.setBackgroundTintList(android.content.res.ColorStateList.valueOf(
            YOUTUBE.equals(provider)?0xffd9dade:SURFACE));
        youtubeTab.setTextColor(YOUTUBE.equals(provider)?BACKGROUND:TEXT);
        soundcloudTab.setBackgroundTintList(android.content.res.ColorStateList.valueOf(
            SOUNDCLOUD.equals(provider)?0xffd9dade:SURFACE));
        soundcloudTab.setTextColor(SOUNDCLOUD.equals(provider)?BACKGROUND:TEXT);
    }

    private void switchProvider(String next){
        if(provider.equals(next))return;
        provider=next;switchStyle();
        Uri target=searchUrl(provider,search.getText().toString());
        if(target==null)target=homeUrl(provider);
        if(target!=null)browser.loadUrl(target.toString());
    }

    private void submitSearch(){
        Uri uri=searchUrl(provider,search.getText().toString());
        if(uri==null){search.setError("Enter a song or artist");return;}
        InputMethodManager keyboard=(InputMethodManager)getSystemService(INPUT_METHOD_SERVICE);
        if(keyboard!=null)keyboard.hideSoftInputFromWindow(search.getWindowToken(),0);
        browser.loadUrl(uri.toString());
    }

    private void saveCurrent(){
        Uri current=browser==null?null:Uri.parse(browser.getUrl());
        if(current==null||!LinkedMusic.isExactItem(current)){
            status.setText("Open a specific track or playlist result first, then tap Save track.");
            Toast.makeText(this,"Select a track or playlist first",Toast.LENGTH_SHORT).show();
            return;
        }
        EditText title=new EditText(this);
        title.setSingleLine(true);title.setTextColor(TEXT);
        title.setHintTextColor(MUTED);title.setHint("Song or playlist name");
        title.setInputType(InputType.TYPE_CLASS_TEXT);
        String pageTitle=browser.getTitle();
        if(pageTitle!=null&&!pageTitle.isEmpty())title.setText(pageTitle);
        new MaterialAlertDialogBuilder(this)
            .setTitle("Save to CYN Library")
            .setMessage("Saves a music link, not a downloaded file. Favorites remain separate.")
            .setView(title)
            .setNegativeButton("Cancel",null)
            .setPositiveButton("Save",(dialog,which)->{
                String name=title.getText().toString().trim();
                if(name.isEmpty())name="Saved "+LinkedMusic.provider(current)+" track";
                if(name.length()>160)name=name.substring(0,160);
                Intent result=new Intent();
                result.setData(current);
                result.putExtra("title",name);
                setResult(RESULT_OK,result);
                finish();
            }).show();
    }

    private void openSystem(Uri uri){
        try{startActivity(new Intent(Intent.ACTION_VIEW,uri).addCategory(Intent.CATEGORY_BROWSABLE));}
        catch(Exception e){Toast.makeText(this,"No compatible browser or music app",Toast.LENGTH_SHORT).show();}
    }

    private void openExternally(){
        if(browser!=null&&browser.getUrl()!=null){
            Uri link=Uri.parse(browser.getUrl());
            if(isProviderPage(provider,link))openSystem(link);
        }
    }

    @Override public void onBackPressed(){
        if(browser!=null&&browser.canGoBack())browser.goBack();else super.onBackPressed();
    }
    @Override protected void onDestroy(){
        if(browser!=null){
            browser.stopLoading();
            browser.loadUrl("about:blank");
            browser.destroy();
            browser=null;
        }
        super.onDestroy();
    }
}
