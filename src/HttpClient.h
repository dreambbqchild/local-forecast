#pragma once

#include <stddef.h>
#include <stdlib.h>

#include <chrono>
#include <iostream>
#include <thread>

#include <curl/curl.h>

class HttpClient
{
    public:
    inline static void Init() { curl_global_init(CURL_GLOBAL_DEFAULT); }

    template <typename fn>
    static void Get(const char* url, fn callback, void* userData)
    {
        using namespace std;
        auto curl = curl_easy_init();

        if(!curl)
        {
            cout << "Unable to initalize curl" << endl;
            exit(1);
        }

        while(true)
        {
            long httpCode = 0;
            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, userData);
            auto code = curl_easy_perform(curl);

            if(code != CURLE_OK)
            {
                cout << "Curl failed: " << code << endl;
                exit(1);
            }
            
            curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &httpCode);
            if(httpCode >= 300)
            {
                cout << "Http request failed: " << httpCode << endl;
                if(httpCode == 404)
                {
                    cout << "Will try " << url << "again..." << endl;
                    this_thread::sleep_for(10s);
                    cout << "Re-downloading " << url << endl;
                    continue;
                }

                exit(1);
            }

            break;
        }

        curl_easy_cleanup(curl);
    }
};