// Spotify companion: reuse an EMBER session, never send audio or train on metadata.
#pragma once
#include <JuceHeader.h>
#include <functional>
#include <atomic>

namespace CynSpotify {
inline constexpr const char* emberClientId="c82b14bfdfa24024ab295548bd169aae";
struct Config {
    juce::String client,access,refresh,secret;
    double expires=0;
    static Config parse(juce::var root) {
        Config c;
        auto section=root["spotify"].isObject()?root["spotify"]:root;
        auto token=section;
        for(const auto* key:{"tokens","token_info","ember2_spotify_tokens"}) {
            if(section[key].isObject()) {token=section[key];break;}
            if(section[key].isString()) {token=juce::JSON::parse(section[key].toString());break;}
        }
        c.client=section["client_id"].toString();
        if(c.client.isEmpty()) c.client=section["clientId"].toString();
        if(c.client.isEmpty()) c.client=root["ember2_spotify_client_id"].toString();
        if(c.client.isEmpty()) c.client=emberClientId;
        c.secret=section["client_secret"].toString();
        c.access=token["access_token"].toString();c.refresh=token["refresh_token"].toString();
        c.expires=double(token["expires_at"]);
        if(c.expires>0&&c.expires<1.0e12)c.expires*=1000; // accept Spotipy seconds and EMBER milliseconds
        return c;
    }
};
struct Reply {int code=0;juce::var body;int retrySeconds=0;};
using Transport=std::function<Reply(const juce::String&,const juce::String&,const juce::String&,const juce::String&)>;
inline Reply request(const juce::String& url,const juce::String& method,const juce::String& headers,const juce::String& body) {
    Reply r;juce::StringPairArray responseHeaders;
    auto u=juce::URL(url);if(body.isNotEmpty())u=u.withPOSTData(body);
    auto stream=u.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
        .withHttpRequestCmd(method).withExtraHeaders(headers).withConnectionTimeoutMs(5000)
        .withNumRedirectsToFollow(0).withStatusCode(&r.code).withResponseHeaders(&responseHeaders));
    if(stream)r.body=juce::JSON::parse(stream->readString());
    for(int i=0;i<responseHeaders.size();++i)if(responseHeaders.getAllKeys()[i].equalsIgnoreCase("Retry-After"))
        r.retrySeconds=juce::jlimit(1,3600,responseHeaders.getAllValues()[i].getIntValue());
    return r;
}
inline juce::String error(int code) {
    if(code==401)return "Session expired. Export the signed-in Spotify session from EMBER again.";
    if(code==403)return "Spotify denied access. Check the existing app's permissions and account access.";
    if(code==404)return "Open Spotify and select a playback device.";
    if(code==429)return "Spotify is rate limiting requests. Retrying after its cooldown.";
    return "Spotify is unavailable. Check your connection.";
}
struct Playback {
    juce::String status="Drop your EMBER Spotify JSON here, or choose it below.",title,artist,device,type,url;
    bool connected=false,playing=false;
    int position=0,duration=0;
};
inline Playback playback(const Reply& reply) {
    Playback p;
    if(reply.code==204){p.connected=true;p.status="Connected - no active Spotify playback";return p;}
    if(reply.code!=200||!reply.body.isObject()){p.status=error(reply.code);return p;}
    p.connected=true;p.playing=bool(reply.body["is_playing"]);
    const auto item=reply.body["item"];
    p.title=item["name"].toString();p.device=reply.body["device"]["name"].toString();
    p.type=reply.body["currently_playing_type"].toString();
    p.position=int(reply.body["progress_ms"]);p.duration=int(item["duration_ms"]);
    if(auto* artists=item["artists"].getArray())for(const auto& artist:*artists){if(p.artist.isNotEmpty())p.artist+=", ";p.artist+=artist["name"].toString();}
    if(p.type=="episode")p.artist=item["show"]["name"].toString();
    p.url=item["external_urls"]["spotify"].toString();
    if(!p.url.startsWith("https://open.spotify.com/"))p.url.clear();
    p.status=(p.playing?"Playing":"Paused")+juce::String(" on Spotify")+(p.type=="episode"?" / podcast":p.type=="track"?" / music":"");
    return p;
}
class Worker final : private juce::Thread {
public:
    explicit Worker(Transport transport=request,juce::File folder=juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Cyn"))
        :juce::Thread("Cyn Spotify"),transport_(std::move(transport)) {
        selection_=folder.getChildFile("spotify-file.txt");
        const auto saved=selection_.loadFileAsString().trim();
        if(juce::File::isAbsolutePath(saved))file_=juce::File(saved);
        startThread(2);
    }
    ~Worker() override {signalThreadShouldExit();notify();stopThread(-1);}
    void choose(const juce::File& file) {
        const juce::ScopedLock guard(lock_);pending_=file;change_=true;notify();
    }
    void disconnect() {const juce::ScopedLock guard(lock_);pending_=juce::File();change_=true;notify();}
    void control(int action) {action_=action;notify();} // 1 play, 2 pause, 3 previous, 4 next
    Playback state() const {const juce::ScopedLock guard(lock_);return state_;}
private:
    void publish(const Playback& p){const juce::ScopedLock guard(lock_);state_=p;}
    bool load() {
        if(file_.getSize()>1024*1024)return false;
        const auto text=file_.loadFileAsString();
        if(text==loaded_)return config_.access.isNotEmpty()||config_.refresh.isNotEmpty();
        loaded_=text;config_=Config::parse(juce::JSON::parse(text));
        return config_.access.isNotEmpty()||config_.refresh.isNotEmpty();
    }
    bool refresh() {
        if(config_.refresh.isEmpty())return false;
        auto body="grant_type=refresh_token&refresh_token="+juce::URL::addEscapeChars(config_.refresh,true)
            +"&client_id="+juce::URL::addEscapeChars(config_.client,true);
        if(config_.secret.isNotEmpty())body+="&client_secret="+juce::URL::addEscapeChars(config_.secret,true);
        const auto reply=transport_("https://accounts.spotify.com/api/token","POST","Content-Type: application/x-www-form-urlencoded\r\n",body);
        if(reply.code==429){retryAt_=juce::Time::currentTimeMillis()+1000.0*juce::jmax(5,reply.retrySeconds);return false;}
        if(reply.code!=200||reply.body["access_token"].toString().isEmpty())return false;
        config_.access=reply.body["access_token"].toString();
        if(reply.body["refresh_token"].toString().isNotEmpty())config_.refresh=reply.body["refresh_token"].toString();
        config_.expires=juce::Time::currentTimeMillis()+1000.0*juce::jmax(60,int(reply.body["expires_in"]));
        // Update the same JSON layout, preserving EMBER fields. Never overwrite
        // a file that EMBER changed while the request was in flight.
        if(file_.loadFileAsString()==loaded_) {
            auto root=juce::JSON::parse(loaded_);auto section=root["spotify"].isObject()?root["spotify"]:root;
            auto token=section;juce::String stringKey;
            for(const auto* key:{"tokens","token_info","ember2_spotify_tokens"}){
                if(section[key].isObject()){token=section[key];break;}
                if(section[key].isString()){token=juce::JSON::parse(section[key].toString());stringKey=key;break;}
            }
            if(auto* obj=token.getDynamicObject()) {
                obj->setProperty("access_token",config_.access);obj->setProperty("refresh_token",config_.refresh);
                const double oldExpiry=double(token["expires_at"]);
                obj->setProperty("expires_at",oldExpiry>0&&oldExpiry<1.0e12?config_.expires/1000.0:config_.expires);
                if(stringKey.isNotEmpty())section.getDynamicObject()->setProperty(stringKey,juce::JSON::toString(token));
                const auto updated=juce::JSON::toString(root);
                juce::TemporaryFile temporary(file_);
                if(temporary.getFile().replaceWithText(updated)&&file_.loadFileAsString()==loaded_&&temporary.overwriteTargetFileWithTemporary())loaded_=updated;
            }
        }
        return true;
    }
    Reply api(const juce::String& path,const juce::String& method) {
        return transport_("https://api.spotify.com/v1/me/player"+path,method,"Authorization: Bearer "+config_.access+"\r\nAccept: application/json\r\n",{});
    }
    void run() override {
        bool discover=!selection_.existsAsFile();
        while(!threadShouldExit()) {
            {const juce::ScopedLock guard(lock_);if(change_){file_=pending_;change_=false;loaded_.clear();config_={};retryAt_=0;action_=0;discover=false;
                selection_.getParentDirectory().createDirectory();selection_.replaceWithText(file_.getFullPathName());state_=Playback{};}}
            if(!file_.existsAsFile()&&discover) {
                for(const auto& folder:{selection_.getParentDirectory(),juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory()})
                    for(const auto* name:{"spotify.json","ember-spotify.json","spotify_tokens.json"})if(folder.getChildFile(name).existsAsFile()){file_=folder.getChildFile(name);break;}
            }
            if(!file_.existsAsFile()){
                Playback p;if(file_!=juce::File())p.status="Spotify JSON was moved or removed. Choose it again.";
                publish(p);wait(2000);continue;
            }
            if(!load()){Playback p;p.status="This JSON has no Spotify session. Export it from EMBER after connecting Spotify.";publish(p);wait(3000);continue;}
            if(juce::Time::currentTimeMillis()<retryAt_){wait(500);continue;}
            if(config_.access.isEmpty()||(config_.expires>0&&config_.expires<juce::Time::currentTimeMillis()+30000)) {
                if(!refresh()){Playback p;p.status=error(retryAt_>juce::Time::currentTimeMillis()?429:401);publish(p);wait(5000);continue;}
            }
            const int action=action_.exchange(0);
            if(action>=1&&action<=4) {
                const juce::String paths[]={"/play","/pause","/previous","/next"};
                auto result=api(paths[action-1],action<=2?"PUT":"POST");
                if(result.code==401&&refresh())result=api(paths[action-1],action<=2?"PUT":"POST");
                if(result.code<200||result.code>=300){Playback p;p.status=error(result.code);publish(p);
                    if(result.code==429)retryAt_=juce::Time::currentTimeMillis()+1000.0*juce::jmax(5,result.retrySeconds);
                    wait(3000);continue;}
            }
            auto result=api("?additional_types=track,episode","GET");
            if(result.code==401&&refresh())result=api("?additional_types=track,episode","GET");
            if(result.code==429)retryAt_=juce::Time::currentTimeMillis()+1000.0*juce::jmax(5,result.retrySeconds);
            publish(playback(result));wait(3000);
        }
    }
    Transport transport_;mutable juce::CriticalSection lock_;Playback state_;
    juce::File file_,pending_,selection_;bool change_=false;Config config_;juce::String loaded_;
    std::atomic<int> action_{0};double retryAt_=0;
};
}
