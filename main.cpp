#include <iostream>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

using namespace std;

const int MAX_INDEX_LEN = 64;
const int NUM_BUCKETS = 20;

struct Record {
    char index[MAX_INDEX_LEN + 1];
    int value;
    char valid;  // 1 if valid, 0 if deleted

    Record() : value(0), valid(1) {
        memset(index, 0, sizeof(index));
    }

    Record(const string& idx, int val) : value(val), valid(1) {
        strncpy(index, idx.c_str(), MAX_INDEX_LEN);
        index[MAX_INDEX_LEN] = '\0';
    }
};

int getBucket(const string& index) {
    unsigned int hash = 0;
    for (char c : index) {
        hash = hash * 31 + c;
    }
    return hash % NUM_BUCKETS;
}

string getFilename(int bucket) {
    return "bucket_" + to_string(bucket) + ".bin";
}

class FileStorage {
private:
    void loadBucket(int bucket, vector<Record>& records) {
        records.clear();
        string filename = getFilename(bucket);
        FILE* file = fopen(filename.c_str(), "rb");
        if (!file) return;

        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        if (fileSize > 0) {
            size_t numRecords = fileSize / sizeof(Record);
            records.reserve(numRecords);

            Record rec;
            while (fread(&rec, sizeof(Record), 1, file) == 1) {
                records.push_back(rec);
            }
        }

        fclose(file);
    }

    void saveBucket(int bucket, const vector<Record>& records) {
        string filename = getFilename(bucket);
        FILE* file = fopen(filename.c_str(), "wb");
        if (!file) return;

        for (const auto& rec : records) {
            fwrite(&rec, sizeof(Record), 1, file);
        }

        fclose(file);
    }

public:
    void insert(const string& index, int value) {
        int bucket = getBucket(index);
        vector<Record> records;
        loadBucket(bucket, records);

        for (const auto& rec : records) {
            if (rec.valid == 1 && strcmp(rec.index, index.c_str()) == 0 && rec.value == value) {
                return;
            }
        }

        records.push_back(Record(index, value));
        saveBucket(bucket, records);
    }

    void remove(const string& index, int value) {
        int bucket = getBucket(index);
        vector<Record> records;
        loadBucket(bucket, records);

        for (auto& rec : records) {
            if (rec.valid == 1 && strcmp(rec.index, index.c_str()) == 0 && rec.value == value) {
                rec.valid = 0;
                saveBucket(bucket, records);
                return;
            }
        }
    }

    vector<int> find(const string& index) {
        int bucket = getBucket(index);
        vector<Record> records;
        loadBucket(bucket, records);

        vector<int> result;
        for (const auto& rec : records) {
            if (rec.valid == 1 && strcmp(rec.index, index.c_str()) == 0) {
                result.push_back(rec.value);
            }
        }

        sort(result.begin(), result.end());
        return result;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    FileStorage storage;
    int n;
    cin >> n;

    for (int i = 0; i < n; i++) {
        string command;
        cin >> command;

        if (command == "insert") {
            string index;
            int value;
            cin >> index >> value;
            storage.insert(index, value);
        } else if (command == "delete") {
            string index;
            int value;
            cin >> index >> value;
            storage.remove(index, value);
        } else if (command == "find") {
            string index;
            cin >> index;
            vector<int> values = storage.find(index);

            if (values.empty()) {
                cout << "null\n";
            } else {
                for (size_t j = 0; j < values.size(); j++) {
                    if (j > 0) cout << " ";
                    cout << values[j];
                }
                cout << "\n";
            }
        }
    }

    return 0;
}