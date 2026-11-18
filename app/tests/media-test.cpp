#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#include "../src/personal-core.h"
#include "../src/personal-media.h"
#include <iostream>
int main(){
    hext::MediaService service;if(!service.start("inspect"))return 1;
    std::optional<hext::MediaResult> result;auto until=GetTickCount64()+8000;
    while(GetTickCount64()<until){result=service.poll();if(result)break;Sleep(20);}
    if(!result){std::cerr<<"Windows media request did not complete\n";return 2;}
    std::cout<<"Native Windows media inspection: HRESULT="<<result->code<<", session="<<result->session<<", state="<<result->playback<<". Playback was not changed.\n";
    if(FAILED(result->code)&&result->code!=HRESULT_FROM_WIN32(ERROR_NOT_FOUND))return 3;
    std::atomic<bool> cancelled{false};auto spotify=hext::mediaCommand("inspect",cancelled,"spotify");
    std::cout<<"Native Spotify session lookup: HRESULT="<<spotify.code<<", session="<<spotify.session<<", identified="<<spotify.spotify<<". No track metadata was read and playback was not changed.\n";
    return SUCCEEDED(spotify.code)||spotify.code==HRESULT_FROM_WIN32(ERROR_NOT_FOUND)?0:4;
}
