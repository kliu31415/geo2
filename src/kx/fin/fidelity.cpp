#include "kx/time.h"
#include "kx/net/kcurl.h"
#include "kx/debug.h"
#include "kx/multithread/shared_lock_set.h"
#include "kx/io.h"

#include <vector>
#include <string>
#include <map>
#include <fstream>

namespace kx { namespace fin { namespace fidelity {

static const std::string html_tickers_pattern = "dividends&symbols=";
static const std::string tickers_url = "https://eresearch.fidelity.com/eresearch/"
                                       "conferenceCalls.jhtml?tab=dividends&begindate=";
static const char html_tickers_end_char = '\"';
static std::vector<std::string> extract_tickers_from_html(const std::string &html)
{
    //I guess we're assuming '\"' doesn't appear in any symbol, which should be accurate enough.
    //If it does, some tickers will be too short (but the program shouldn't crash)

    std::vector<std::string> tickers;
    size_t pos = 0;
    while(pos < html.size()) {
        pos = html.find(html_tickers_pattern, pos);
        if(pos == std::string::npos)
            break;
        pos += html_tickers_pattern.size();
        if(pos == html.size()) {
            log_error("malformed Fidelity ex div HTML had pattern at end");
            break;
        }
        size_t last = pos;
        while(html[last] != html_tickers_end_char) {
            last++;
            if(last == html.size()) {
                log_error("malformed Fidelity ex div HTML has unterminated symbol");
                goto out_of_loop;
            }
        }
        tickers.push_back(html.substr(pos, last-pos));
        pos++;
    }
    out_of_loop:
    //HTML has some weird encodings, like "%3A" means ":", that we should convert to our format
    for(auto &ticker: tickers) {
        size_t colon_pos = 0;
        while(colon_pos < ticker.size()) {
            colon_pos = ticker.find("%3A");
            if(colon_pos == std::string::npos)
                break;
            ticker.erase(ticker.begin() + colon_pos, ticker.begin() + colon_pos + 3);
            ticker.insert(ticker.begin() + colon_pos, ':');
            colon_pos++;
        }
    }
    std::sort(tickers.begin(), tickers.end());
    return tickers;
}

static std::vector<std::string> load_ex_div_tickers(std::ifstream *input_file)
{
    input_file->seekg(0, std::ios::end);
    auto len = input_file->tellg();
    input_file->seekg(0, std::ios::beg);
    std::string contents;
    contents.resize(len);
    input_file->read(contents.data(), len);
    auto tickers = split_str(contents, '\n');
    erase_remove(&tickers, "");
    return tickers;
}

static SharedLock_Set<Time> date_locks;

std::vector<std::string> get_ex_div_tickers(Time date)
{
    k_expects(date.to_int64(Time::Length::ns) % ((int64_t)(1e9 * 86400)) == 0);

    std::string input_file_path = "kx_data/fidelity/" + date.to_str(Time::Format::YYYY_mm_DD);

    {
        auto shared_lock_guard = date_locks.get_lock_shared_guard(date);

        std::ifstream input_file(input_file_path, std::ios::binary);
        if(!input_file.fail())
            return load_ex_div_tickers(&input_file);
    }

    auto lock_guard = date_locks.get_lock_guard(date);

    //check if another thread has already downloaded the data, in which case we don't need to
    std::ifstream input_file(input_file_path, std::ios::binary);
    if(!input_file.fail())
        return load_ex_div_tickers(&input_file);

    //otherwise, download the data
    net::KCurl curl;
    auto curl_date_str = date.to_str(Time::Format::YYYY_mm_DD);
    curl_date_str = curl_date_str.substr(5, 2) + '/' + curl_date_str.substr(8, 2) + '/' +
                    curl_date_str.substr(0, 4);
    curl.set_URL(tickers_url + curl_date_str);
    curl.send_request();
    auto tickers = extract_tickers_from_html(curl.get_last_response());

    io::make_folder("kx_data/fidelity");
    auto output_file_path = "kx_data/fidelity/" + date.to_str(Time::Format::YYYY_mm_DD);
    std::ofstream output_file(output_file_path, std::ios::binary);
    if(output_file.fail()) {
        log_error("failed to make output file \"" + output_file_path + "\"");
    } else {
        std::string output;
        for(auto &ticker: tickers) {
            output += ticker;
            output += '\n';
        }
        output_file.write(output.data(), output.size());
    }

    return tickers;
}

}}}
