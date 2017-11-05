#pragma once

using namespace std;

class FirebaseLib {
private:
    mutex apiMutex;
    string videoID;
    string createBaseRecord();
    string numToCharSuffix(int);
    bool updateFirebase(string);
    static size_t callback(
        const char* in,
        std::size_t size,
        std::size_t num,
        std::string* out);

public:
    FirebaseLib ();
    string getVideoId();
    void updateFrame(int, string);
};
