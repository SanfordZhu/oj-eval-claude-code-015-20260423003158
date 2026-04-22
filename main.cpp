#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

using namespace std;

const int MAX_INDEX_LEN = 64;
const int NUM_FILES = 20;
const int SLOTS_PER_FILE = 5000;  // 20 * 5000 = 100,000 total slots
const int TOTAL_SLOTS = NUM_FILES * SLOTS_PER_FILE;

struct Record {
    char index[MAX_INDEX_LEN + 1];
    int value;
    long next;  // position in file of next record in chain, -1 for end
    char valid; // 1 if valid, 0 if deleted

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
    string getFilename(int fileIndex) {
        return "file_" + to_string(fileIndex) + ".bin";
    }

    // Get file index and slot index within file
    pair<int, int> getLocation(size_t hash) {
        int fileIndex = hash % NUM_FILES;
        int slotIndex = (hash / NUM_FILES) % SLOTS_PER_FILE;
        return {fileIndex, slotIndex};
    }

    // Read slot (head pointer) from file
    long readSlot(int fileIndex, int slotIndex) {
        string filename = getFilename(fileIndex);
        fstream file(filename, ios::in | ios::out | ios::binary | ios::app);
        if (!file) return -1;

        // Slot table at beginning of file
        file.seekg(slotIndex * sizeof(long), ios::beg);
        long pos;
        file.read(reinterpret_cast<char*>(&pos), sizeof(long));
        return pos;
    }

    // Write slot (head pointer) to file
    void writeSlot(int fileIndex, int slotIndex, long pos) {
        string filename = getFilename(fileIndex);
        fstream file(filename, ios::in | ios::out | ios::binary);

        // If file doesn't exist, create and initialize slots
        if (!file) {
            file.open(filename, ios::out | ios::binary);
            // Initialize all slots to -1
            for (int i = 0; i < SLOTS_PER_FILE; i++) {
                long empty = -1;
                file.write(reinterpret_cast<const char*>(&empty), sizeof(long));
            }
            file.close();
            file.open(filename, ios::in | ios::out | ios::binary);
        }

        file.seekp(slotIndex * sizeof(long), ios::beg);
        file.write(reinterpret_cast<const char*>(&pos), sizeof(long));
    }

    // Read record from file position
    Record readRecord(int fileIndex, long pos) {
        if (pos == -1) {
            Record empty;
            empty.valid = 0;
            return empty;
        }

        string filename = getFilename(fileIndex);
        fstream file(filename, ios::in | ios::binary);
        file.seekg(pos, ios::beg);
        Record rec;
        file.read(reinterpret_cast<char*>(&rec), sizeof(Record));
        return rec;
    }

    // Write record to file position (append if pos == -1)
    long writeRecord(int fileIndex, long pos, const Record& rec) {
        string filename = getFilename(fileIndex);
        fstream file(filename, ios::in | ios::out | ios::binary | ios::app);

        if (pos == -1) {
            // Append to end
            file.seekp(0, ios::end);
            pos = file.tellp();
            file.write(reinterpret_cast<const char*>(&rec), sizeof(Record));
        } else {
            // Overwrite existing
            file.seekp(pos, ios::beg);
            file.write(reinterpret_cast<const char*>(&rec), sizeof(Record));
        }
        return pos;
    }

    size_t hashIndex(const string& index) {
        // Hash to 0..TOTAL_SLOTS-1
        size_t h = 0;
        for (char c : index) {
            h = h * 31 + c;
        }
        return h % TOTAL_SLOTS;
    }

public:
    void insert(const string& index, int value) {
        size_t h = hashIndex(index);
        auto [fileIndex, slotIndex] = getLocation(h);

        long headPos = readSlot(fileIndex, slotIndex);

        // Check for duplicate
        long curr = headPos;
        while (curr != -1) {
            Record rec = readRecord(fileIndex, curr);
            if (rec.valid == 1 && strcmp(rec.index, index.c_str()) == 0 && rec.value == value) {
                return;  // Already exists
            }
            curr = rec.next;
        }

        // Insert at head
        Record newRec(index, value, headPos);
        long newPos = writeRecord(fileIndex, -1, newRec);
        writeSlot(fileIndex, slotIndex, newPos);
    }

    void remove(const string& index, int value) {
        size_t h = hashIndex(index);
        auto [fileIndex, slotIndex] = getLocation(h);

        long headPos = readSlot(fileIndex, slotIndex);
        if (headPos == -1) return;

        // Check head
        Record headRec = readRecord(fileIndex, headPos);
        if (headRec.valid == 1 && strcmp(headRec.index, index.c_str()) == 0 && headRec.value == value) {
            // Mark as deleted
            headRec.valid = 0;
            writeRecord(fileIndex, headPos, headRec);

            // Update slot to next valid record
            long curr = headRec.next;
            while (curr != -1) {
                Record rec = readRecord(fileIndex, curr);
                if (rec.valid == 1) {
                    writeSlot(fileIndex, slotIndex, curr);
                    return;
                }
                curr = rec.next;
            }
            writeSlot(fileIndex, slotIndex, -1);
            return;
        }

        // Traverse rest of chain
        long curr = headRec.next;
        while (curr != -1) {
            Record rec = readRecord(fileIndex, curr);
            if (rec.valid == 1 && strcmp(rec.index, index.c_str()) == 0 && rec.value == value) {
                // Mark as deleted
                rec.valid = 0;
                writeRecord(fileIndex, curr, rec);
                return;
            }
            curr = rec.next;
        }
    }

    vector<int> find(const string& index) {
        size_t h = hashIndex(index);
        auto [fileIndex, slotIndex] = getLocation(h);

        vector<int> result;
        long curr = readSlot(fileIndex, slotIndex);

        while (curr != -1) {
            Record rec = readRecord(fileIndex, curr);
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