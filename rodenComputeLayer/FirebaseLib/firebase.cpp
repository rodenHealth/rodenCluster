#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/atomic.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <mutex>
#include <curl/curl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctime>
#include "../util/RodenLockedQueue.h"
#include "./firebase.h"

using namespace std;

// Constructor - creates VIDEO ID
FirebaseLib::FirebaseLib ()
{
    videoID = createBaseRecord();
}

// PUBLIC Method - updates a given frame in firebase
void FirebaseLib::updateFrame(int frameID, string frameData)
{
    char updateRequest[300];
    int updateRequestSize = sprintf(updateRequest, "{\"frame%s\": %s}", numToCharSuffix(frameID).c_str(), frameData.c_str());
    
    updateFirebase(updateRequest);
}

// PRIVATE Method - Creates the base record for the analysis 
string FirebaseLib::createBaseRecord()
{
    int httpCode(0);
    std::unique_ptr<std::string> httpData(new std::string());

    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    string uuidString = boost::uuids::to_string(uuid);

    CURL *hnd = curl_easy_init();
    
    curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(hnd, CURLOPT_WRITEDATA, httpData.get());
    curl_easy_setopt(hnd, CURLOPT_VERBOSE, 0);
    curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl_easy_setopt(hnd, CURLOPT_URL, "https://rodenweb.firebaseio.com/videos.json?auth=Yc8tTOqD9uo8Jq4rcT6uXxsGdqlBltpIuvX1wAoB");
    
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "postman-token: 53c637cd-3932-6c31-33db-8bd972c40b86");
    headers = curl_slist_append(headers, "cache-control: no-cache");
    headers = curl_slist_append(headers, "content-type: application/json");
    curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);
    

    char postField[150];
    int postFieldSize = sprintf(postField, "{\"%s\": {\"timestamp\":\"%s\"}}", uuidString.c_str(), "1");
    
    curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, postField);
    
    CURLcode ret = curl_easy_perform(hnd);

    if (httpCode == 200)
    {
        string returnValue = *httpData;
        cout << "Video ID: " << uuidString << endl;
    }

    return uuidString;
}

// PRIVATE Method - actual API call to firebase
bool FirebaseLib::updateFirebase(string data)
{
    int httpCode(0);
    std::unique_ptr<std::string> httpData(new std::string());

    char postURL[150];
    int postURLSize = sprintf(postURL, "https://rodenweb.firebaseio.com/videos/%s.json?auth=Yc8tTOqD9uo8Jq4rcT6uXxsGdqlBltpIuvX1wAoB", videoID.c_str());

    apiMutex.lock();
    CURL *hnd = curl_easy_init();
    
    curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(hnd, CURLOPT_WRITEDATA, httpData.get());
    curl_easy_setopt(hnd, CURLOPT_VERBOSE, 0);
    curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl_easy_setopt(hnd, CURLOPT_URL, postURL);
    
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "postman-token: 53c637cd-3932-6c31-33db-8bd972c40b86");
    headers = curl_slist_append(headers, "cache-control: no-cache");
    headers = curl_slist_append(headers, "content-type: application/json");
    curl_easy_setopt(hnd, CURLOPT_HTTPHEADER, headers);
    
    curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, data.c_str());
    
    CURLcode ret = curl_easy_perform(hnd);
    apiMutex.unlock();

    if (httpCode == 200)
    {
        string returnValue = *httpData;
    }

    return true;
}

// PRIVATE Method - used to keep firebase records in frame order (sequential read)
string FirebaseLib::numToCharSuffix(int num)
{
	string letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	
	string suffix;
	
	int aCount = num / 26;
	int sigDigit = num % 26;
	
	for (int i = 0; i < aCount; i++)
	{
		suffix.append("Z");
	}
	
	suffix += letters[sigDigit];
	
	return suffix;
}

size_t FirebaseLib::callback(
    const char* in,
    std::size_t size,
    std::size_t num,
    std::string* out)
{
    const std::size_t totalBytes(size * num);
    out->append(in, totalBytes);
    return totalBytes;
}