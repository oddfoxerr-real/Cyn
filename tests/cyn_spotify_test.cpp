#include "CynSpotify.h"
#include <cassert>
#include <iostream>
#include <atomic>
using namespace CynSpotify;
int main(int argc,char** argv) {
    _set_error_mode(_OUT_TO_STDERR);
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
    assert(argc==2);
    auto cfg=Config::parse(juce::JSON::parse(R"({"client_id":"existing-app","tokens":{"access_token":"test","expires_at":1791917000}})"));
    assert(cfg.client=="existing-app"&&cfg.access=="test"&&cfg.expires==1791917000000.0);
    cfg=Config::parse(juce::JSON::parse(R"({"spotify":{"clientId":"same-app","refresh_token":"test-refresh","expires_at":1791917000000}})"));
    assert(cfg.client=="same-app"&&cfg.refresh=="test-refresh");
    cfg=Config::parse(juce::JSON::parse(R"({"ember2_spotify_tokens":"{\"access_token\":\"fixture\"}"})"));
    assert(cfg.client==emberClientId&&cfg.access=="fixture");
    assert(!playback({401,{}}).connected);
    assert(playback({204,{}}).connected&&playback({204,{}}).title.isEmpty());
    auto episode=playback({200,juce::JSON::parse(R"({"is_playing":true,"currently_playing_type":"episode","item":{"name":"Test podcast","show":{"name":"Test show"},"external_urls":{"spotify":"https://evil.example/"}}})")});
    assert(episode.type=="episode"&&episode.artist=="Test show"&&episode.url.isEmpty());
    juce::File folder(juce::String::fromUTF8(argv[1]));folder.createDirectory();
    const auto file=folder.getChildFile("spotify.json");
    assert(file.replaceWithText(R"({"client_id":"existing-app","unrelated":"preserved","tokens":{"refresh_token":"fake-refresh","access_token":"old","expires_at":1}})"));
    std::atomic<int> refreshes{0},controls{0};std::atomic<bool> rateLimit{false};
    Transport mock=[&](const juce::String& url,const juce::String& method,const juce::String& headers,const juce::String& body) {
        if(url=="https://accounts.spotify.com/api/token") {
            assert(method=="POST"&&body.contains("client_id=existing-app")&&body.contains("refresh_token=fake-refresh"));
            ++refreshes;return Reply{200,juce::JSON::parse(R"({"access_token":"new-fixture","refresh_token":"rotated-fixture","expires_in":3600})")};
        }
        assert(url.startsWith("https://api.spotify.com/v1/me/player"));
        assert(headers.contains("Bearer new-fixture"));
        if(url.endsWith("/next")){assert(method=="POST");++controls;return Reply{204,{}};}
        if(rateLimit)return Reply{429,{},60};
        return Reply{200,juce::JSON::parse(R"({"is_playing":true,"currently_playing_type":"track","device":{"name":"This PC"},"progress_ms":5000,"item":{"name":"Fixture track","duration_ms":200000,"artists":[{"name":"Fixture artist"}],"external_urls":{"spotify":"https://open.spotify.com/track/fixture"}}})")};
    };
    {
        Worker worker(mock,folder);worker.choose(file);
        for(int i=0;i<500&&!worker.state().playing;++i)juce::Thread::sleep(30);
        if(!worker.state().playing)std::cerr<<worker.state().status<<std::endl;
        assert(worker.state().title=="Fixture track"&&worker.state().device=="This PC");assert(refreshes==1);
        const auto saved=juce::JSON::parse(file);assert(saved["unrelated"].toString()=="preserved");assert(saved["tokens"]["refresh_token"].toString()=="rotated-fixture");
        worker.control(4);for(int i=0;i<500&&controls==0;++i)juce::Thread::sleep(30);assert(controls==1);
        rateLimit=true;worker.control(1);
        for(int i=0;i<500&&worker.state().connected;++i)juce::Thread::sleep(30);
        assert(!worker.state().connected&&worker.state().title.isEmpty());
        worker.disconnect();
        for(int i=0;i<100&&folder.getChildFile("spotify-file.txt").loadFileAsString().isNotEmpty();++i)juce::Thread::sleep(30);
    }
    const auto before=refreshes.load();
    {Worker worker(mock,folder);juce::Thread::sleep(200);assert(!worker.state().connected);}
    assert(refreshes==before); // persisted disconnect must suppress automatic discovery
    std::cout<<"PASS: EMBER JSON layouts, refresh/rotation, preserved fields, playback, podcast type, controls, 429, stale clearing, persistent disconnect\n";
}
