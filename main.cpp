#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <map>

using namespace std;

const int MAX_INDEX_LEN = 64;
const int NUM_BUCKETS = 20;  // Max 20 files allowed

struct Record {
    char index[MAX_INDEX_LEN + 1];
    int value;
    bool valid;  // true if not deleted

    Record() : value(0), valid(true) {
        memset(index, 0, sizeof(index));
    }

    Record(const string& idx, int val) : value(val), valid(true) {
        strncpy(index, idx.c_str(), MAX_INDEX_LEN);
        index[MAX_INDEX_LEN] = '\0';
    }
};

int getBucket(const string& index) {
    // Simple hash to get bucket 0-19
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
    // For each operation, we load bucket into memory, process, then save
    void loadBucket(int bucket, vector<Record>& records) {
        records.clear();
        string filename = getFilename(bucket);
        ifstream file(filename, ios::binary);
        if (!file) return;

        Record rec;
        while (file.read(reinterpret_cast<char*>(&rec), sizeof(Record))) {
            records.push_back(rec);
        }
    }

    void saveBucket(int bucket, const vector<Record>& records) {
        string filename = getFilename(bucket);
        ofstream file(filename, ios::binary | ios::trunc);
        for (const auto& rec : records) {
            file.write(reinterpret_cast<const char*>(&rec), sizeof(Record));
        }
    }

public:
    void insert(const string& index, int value) {
        int bucket = getBucket(index);
        vector<Record> records;
        loadBucket(bucket, records);

        // Check if already exists
        for (auto& rec : records) {
            if (rec.valid && strcmp(rec.index, index.c_str()) == 0 && rec.value == value) {
                return;  // Already exists
            }
        }

        // Add new record
        records.push_back(Record(index, value));
        saveBucket(bucket, records);
    }

    void remove(const string& index, int value) {
        int bucket = getBucket(index);
        vector<Record> records;
        loadBucket(bucket, records);

        for (auto& rec : records) {
            if (rec.valid && strcmp(rec.index, index.c_str()) == 0 && rec.value == value) {
                rec.valid = false;  // Mark as deleted
                saveBucket(bucket, records);
                return;
            }
        }
        // Not found, nothing to do
    }

    vector<int> find(const string& index) {
        int bucket = getBucket(index);
        vector<Record> records;
        loadBucket(bucket, records);

        vector<int> result;
        for (const auto& rec : records) {
            if (rec.valid && strcmp(rec.index, index.c_str()) == 0) {
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