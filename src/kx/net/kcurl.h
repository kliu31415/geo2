#pragma once

#include "kx/net/net.h"

#include <string>
#include <vector>
#include <memory>

namespace kx { namespace net {

/** NOTE: KCurl::quit might cause crashes if called multiple times, even if
 *  init() is called before each time. I think this is a bug with libcurl.
 *  You shouldn't have to call init() multiple times anyway, so this isn't
 *  a big issue.
*/

class KCurl
{
    struct CURL_Deleter
    {
        void operator()(void*);
    };

    /** CURL=void (we static assert this in kcurl.cpp).
     *  Make this const to prevent bugs.
     */
    const std::unique_ptr<void, CURL_Deleter> curl;
    std::string buffer;
public:
    static constexpr const char *cacert_path = "kx_data/cacert-2020-07-22.pem";

    static void init(const class InitPkey&);
    static void quit(const class QuitPkey&);

    /** The multi function blocks until all the input kcurls are finished.
     *  It does not necessarily check for errors in the individual kcurls.
     *  I'm not sure what happens if individual kcurls fail (I would guess
     *  that nothing would be printed indicating an error and get_last_response
     *  would return "").
     */
    static Result multi_perform_block_until_done(std::vector<KCurl> *kcurls);

    KCurl();

    ///you can't copy network connections
    KCurl(const KCurl&) = delete;
    KCurl &operator = (const KCurl&) = delete;

    ///nonmovable for simplicity
    KCurl(KCurl&&) = default;
    KCurl &operator = (KCurl&&) = default;

    void set_URL(const std::string &URL);
    void set_POST_fields(const std::string &fields);
    Result send_request();
    std::string get_last_response() const; ///returns "" if no request has been sent
};

}}
