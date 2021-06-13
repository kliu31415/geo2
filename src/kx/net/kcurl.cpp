#include "kx/net/kcurl.h"
#include "kx/debug.h"

#include <curl/curl.h>
#include <thread>

namespace kx { namespace net {

static_assert(std::is_same<CURL, void>::value);

static size_t curl_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size*nmemb);
    return size*nmemb;
}

void KCurl::CURL_Deleter::operator()(CURL *curl)
{
    curl_easy_cleanup(curl);
}

void KCurl::init(const InitPkey&)
{
    curl_global_init(CURL_GLOBAL_ALL);
}
void KCurl::quit(const QuitPkey&)
{
    curl_global_cleanup();
}
Result KCurl::multi_perform_block_until_done(std::vector<KCurl> *kcurls)
{
    //libcurl appears to not support more than INT_MAX active handles
    k_expects(kcurls->size() <= INT_MAX);

    CURLM *curlm = curl_multi_init();
    if(curlm == nullptr) {
        log_error("Creating CURLM failed in KCurl::multi_perform");
        return Result::Failure;
    }
    ScopeGuard curlm_dtor([=]{curl_multi_cleanup(curlm);});

    for(auto &kcurl: *kcurls)
        curl_multi_add_handle(curlm, kcurl.curl.get());

    Result ret = Result::Success;

    while(true) {
        int active_handles;
        CURLMcode result = curl_multi_perform(curlm, &active_handles);
        if(result != CURLM_OK) {
            log_error("CURLMcode indicated error: (got \"" +
                      to_str(curl_multi_strerror(result)) + "\")");
            ret = Result::Failure;
        }
        if(active_handles == 0)
            break;
        std::this_thread::yield();
    }

    return ret;
}
KCurl::KCurl():
    curl(curl_easy_init(), CURL_Deleter())
{

    if(curl == nullptr)
        log_error("Creating CURL failed in KCurl ctor");
    curl_easy_setopt(curl.get(), CURLOPT_CAINFO, cacert_path);
    curl_easy_setopt(curl.get(), CURLOPT_CAPATH, cacert_path);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, curl_callback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &buffer);
}
void KCurl::set_URL(const std::string &URL)
{
    curl_easy_setopt(curl.get(), CURLOPT_URL, URL.c_str());
}
void KCurl::set_POST_fields(const std::string &fields="")
{
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, fields.c_str());
}
Result KCurl::send_request()
{
    CURLcode result = curl_easy_perform(curl.get());
    if(result != CURLE_OK) {
        log_error("kcurl POST request failed: " + to_str(curl_easy_strerror(result)));
        return Result::Failure;
    }
    return Result::Success;
}
std::string KCurl::get_last_response() const
{
    return buffer;
}

}}
