#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <functional>

using namespace std;

const int MAX_INDEX_LEN = 64;
const int HASH_SIZE = 10007;  // Prime number for hash table size
const string DATA_FILE = "data.bin";
const string HASH_FILE = "hash.bin";

struct Record {
    char index[MAX_INDEX_LEN + 1];  // +1 for null terminator
    int value;
    long next;  // file position of next record in chain, -1 for end
    char valid; // 1 if record is valid (not deleted), 0 if deleted

    Record() : value(0), next(-1), valid(1) {
        memset(index, 0, sizeof(index));
    }

    Record(const string& idx, int val, long nxt = -1) : value(val), next(nxt), valid(1) {
        strncpy(index, idx.c_str(), MAX_INDEX_LEN);
        index[MAX_INDEX_LEN] = '\0';
    }
};

class FileStorage {
private:
    fstream dataFile;
    fstream hashFile;

    // Simple hash function for strings
    size_t hash(const string& s) const {
        std::hash<string> hasher;
        return hasher(s) % HASH_SIZE;
    }

    // Read hash table entry (file position)
    long readHashEntry(size_t hashIdx) {
        hashFile.seekg(hashIdx * sizeof(long), ios::beg);
        long pos;
        hashFile.read(reinterpret_cast<char*>(&pos), sizeof(long));
        return pos;
    }

    // Write hash table entry
    void writeHashEntry(size_t hashIdx, long pos) {
        hashFile.seekp(hashIdx * sizeof(long), ios::beg);
        hashFile.write(reinterpret_cast<const char*>(&pos), sizeof(long));
    }

    // Read record from file position
    Record readRecord(long pos) {
        dataFile.seekg(pos, ios::beg);
        Record rec;
        dataFile.read(reinterpret_cast<char*>(&rec), sizeof(Record));
        return rec;
    }

    // Write record to file position
    void writeRecord(long pos, const Record& rec) {
        dataFile.seekp(pos, ios::beg);
        dataFile.write(reinterpret_cast<const char*>(&rec), sizeof(Record));
    }

    // Append record to end of file
    long appendRecord(const Record& rec) {
        dataFile.seekp(0, ios::end);
        long pos = dataFile.tellp();
        writeRecord(pos, rec);
        return pos;
    }

public:
    FileStorage() {
        // Open or create data file
        dataFile.open(DATA_FILE, ios::in | ios::out | ios::binary | ios::app);
        if (!dataFile) {
            dataFile.open(DATA_FILE, ios::out | ios::binary);
            dataFile.close();
            dataFile.open(DATA_FILE, ios::in | ios::out | ios::binary | ios::app);
        }

        // Open or create hash file
        hashFile.open(HASH_FILE, ios::in | ios::out | ios::binary);
        if (!hashFile) {
            // Initialize hash file with -1 (empty)
            hashFile.open(HASH_FILE, ios::out | ios::binary);
            for (int i = 0; i < HASH_SIZE; i++) {
                long empty = -1;
                hashFile.write(reinterpret_cast<const char*>(&empty), sizeof(long));
            }
            hashFile.close();
            hashFile.open(HASH_FILE, ios::in | ios::out | ios::binary);
        }
    }

    ~FileStorage() {
        if (dataFile.is_open()) dataFile.close();
        if (hashFile.is_open()) hashFile.close();
    }

    void insert(const string& index, int value) {
        // Check if already exists
        size_t h = hash(index);
        long pos = readHashEntry(h);

        // Traverse chain to check for duplicate
        long curr = pos;
        while (curr != -1) {
            Record rec = readRecord(curr);
            if (rec.valid == 1 && strcmp(rec.index, index.c_str()) == 0 && rec.value == value) {
                return;  // Already exists
            }
            curr = rec.next;
        }

        // Create new record and insert at head
        Record newRec(index, value, pos);
        long newPos = appendRecord(newRec);
        writeHashEntry(h, newPos);
    }

    void remove(const string& index, int value) {
        size_t h = hash(index);
        long pos = readHashEntry(h);

        if (pos == -1) return;

        // Check first record
        long curr = pos;
        Record rec = readRecord(curr);

        if (rec.valid == 1 && strcmp(rec.index, index.c_str()) == 0 && rec.value == value) {
            // Mark as deleted
            rec.valid = 0;
            writeRecord(curr, rec);

            // Update hash table to point to next valid record
            long next = rec.next;
            while (next != -1) {
                Record nextRec = readRecord(next);
                if (nextRec.valid == 1) {
                    writeHashEntry(h, next);
                    return;
                }
                next = nextRec.next;
            }
            // No valid records left in chain
            writeHashEntry(h, -1);
            return;
        }

        // Traverse rest of chain
        long prev = curr;
        curr = rec.next;

        while (curr != -1) {
            rec = readRecord(curr);
            if (rec.valid == 1 && strcmp(rec.index, index.c_str()) == 0 && rec.value == value) {
                // Mark as deleted
                rec.valid = 0;
                writeRecord(curr, rec);

                // We need to update previous valid record's next pointer to skip this deleted record
                // Find previous valid record
                long prevValid = prev;
                while (prevValid != -1) {
                    Record prevRec = readRecord(prevValid);
                    if (prevRec.valid == 1) {
                        // Update its next pointer to skip deleted record
                        prevRec.next = rec.next;
                        writeRecord(prevValid, prevRec);
                        return;
                    }
                    prevValid = prevRec.next;
                }
                // No previous valid record (shouldn't happen)
                return;
            }
            prev = curr;
            curr = rec.next;
        }
    }

    vector<int> find(const string& index) {
        vector<int> result;
        size_t h = hash(index);
        long pos = readHashEntry(h);

        // Traverse chain
        long curr = pos;
        while (curr != -1) {
            Record rec = readRecord(curr);
            if (rec.valid == 1 && strcmp(rec.index, index.c_str()) == 0) {
                result.push_back(rec.value);
            }
            curr = rec.next;
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