#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>

using namespace std;

// ==================== Data Structures ====================

const int MAX_STRING_LENGTH = 65;
const int MAX_KEYWORD_LENGTH = 65;
const int MAX_USERNAME_LENGTH = 35;

struct Account {
    char userID[32];
    char password[32];
    char username[MAX_USERNAME_LENGTH];
    int privilege;
    
    Account() : privilege(0) {
        memset(userID, 0, sizeof(userID));
        memset(password, 0, sizeof(password));
        memset(username, 0, sizeof(username));
    }
};

struct Book {
    char ISBN[24];
    char name[MAX_STRING_LENGTH];
    char author[MAX_STRING_LENGTH];
    char keyword[MAX_STRING_LENGTH];
    double price;
    int quantity;
    
    Book() : price(0), quantity(0) {
        memset(ISBN, 0, sizeof(ISBN));
        memset(name, 0, sizeof(name));
        memset(author, 0, sizeof(author));
        memset(keyword, 0, sizeof(keyword));
    }
    
    bool operator<(const Book& other) const {
        return strcmp(ISBN, other.ISBN) < 0;
    }
};

struct Transaction {
    double amount;
    bool isIncome; // true for buy, false for import
};

struct LogEntry {
    char userID[32];
    char operation[256];
};

// ==================== File Storage System ====================

class FileStorage {
private:
    string filename;
    
public:
    FileStorage(const string& fname) : filename(fname) {
        // Initialize file if not exists
        ifstream test(filename);
        if (!test) {
            ofstream create(filename, ios::binary);
            create.close();
        }
        test.close();
    }
    
    template<typename T>
    void write(const T& data, streampos pos = -1) {
        fstream file(filename, ios::binary | ios::in | ios::out);
        if (pos >= 0) {
            file.seekp(pos);
        } else {
            file.seekp(0, ios::end);
        }
        file.write(reinterpret_cast<const char*>(&data), sizeof(T));
        file.close();
    }
    
    template<typename T>
    bool read(T& data, streampos pos) {
        ifstream file(filename, ios::binary);
        file.seekg(pos);
        file.read(reinterpret_cast<char*>(&data), sizeof(T));
        bool success = file.gcount() == sizeof(T);
        file.close();
        return success;
    }
    
    template<typename T>
    vector<T> readAll() {
        vector<T> result;
        ifstream file(filename, ios::binary);
        T data;
        while (file.read(reinterpret_cast<char*>(&data), sizeof(T))) {
            result.push_back(data);
        }
        file.close();
        return result;
    }
    
    void clear() {
        ofstream file(filename, ios::binary | ios::trunc);
        file.close();
    }
};

// ==================== Account Management ====================

class AccountManager {
private:
    FileStorage accountFile;
    map<string, Account> accountCache;
    vector<pair<string, string>> loginStack; // (userID, selectedISBN)
    
    void loadAccounts() {
        accountCache.clear();
        vector<Account> accounts = accountFile.readAll<Account>();
        for (const auto& acc : accounts) {
            accountCache[acc.userID] = acc;
        }
    }
    
    void saveAccounts() {
        accountFile.clear();
        for (const auto& pair : accountCache) {
            accountFile.write(pair.second);
        }
    }
    
public:
    AccountManager() : accountFile("accounts.dat") {
        loadAccounts();
        // Create root account if not exists
        if (accountCache.find("root") == accountCache.end()) {
            Account root;
            strcpy(root.userID, "root");
            strcpy(root.password, "sjtu");
            strcpy(root.username, "root");
            root.privilege = 7;
            accountCache["root"] = root;
            saveAccounts();
        }
    }
    
    int getCurrentPrivilege() {
        if (loginStack.empty()) return 0;
        return accountCache[loginStack.back().first].privilege;
    }
    
    string getCurrentUser() {
        if (loginStack.empty()) return "";
        return loginStack.back().first;
    }
    
    string getSelectedBook() {
        if (loginStack.empty()) return "";
        return loginStack.back().second;
    }
    
    void setSelectedBook(const string& isbn) {
        if (!loginStack.empty()) {
            loginStack.back().second = isbn;
        }
    }
    
    bool login(const string& userID, const string& password) {
        if (accountCache.find(userID) == accountCache.end()) return false;
        
        Account& acc = accountCache[userID];
        if (!password.empty() && password != acc.password) {
            // Check if current privilege is higher
            if (getCurrentPrivilege() <= acc.privilege) return false;
        }
        
        loginStack.push_back({userID, ""});
        return true;
    }
    
    bool logout() {
        if (loginStack.empty()) return false;
        loginStack.pop_back();
        return true;
    }
    
    bool registerAccount(const string& userID, const string& password, const string& username) {
        if (accountCache.find(userID) != accountCache.end()) return false;
        
        Account acc;
        strcpy(acc.userID, userID.c_str());
        strcpy(acc.password, password.c_str());
        strcpy(acc.username, username.c_str());
        acc.privilege = 1;
        
        accountCache[userID] = acc;
        saveAccounts();
        return true;
    }
    
    bool changePassword(const string& userID, const string& currentPassword, const string& newPassword) {
        if (accountCache.find(userID) == accountCache.end()) return false;
        
        Account& acc = accountCache[userID];
        if (!currentPassword.empty() && currentPassword != acc.password) {
            if (getCurrentPrivilege() != 7) return false;
        }
        
        strcpy(acc.password, newPassword.c_str());
        saveAccounts();
        return true;
    }
    
    bool addAccount(const string& userID, const string& password, int privilege, const string& username) {
        if (accountCache.find(userID) != accountCache.end()) return false;
        if (privilege >= getCurrentPrivilege()) return false;
        
        Account acc;
        strcpy(acc.userID, userID.c_str());
        strcpy(acc.password, password.c_str());
        strcpy(acc.username, username.c_str());
        acc.privilege = privilege;
        
        accountCache[userID] = acc;
        saveAccounts();
        return true;
    }
    
    bool deleteAccount(const string& userID) {
        if (accountCache.find(userID) == accountCache.end()) return false;
        
        // Check if account is logged in
        for (const auto& login : loginStack) {
            if (login.first == userID) return false;
        }
        
        accountCache.erase(userID);
        saveAccounts();
        return true;
    }
};

// ==================== Book Management ====================

class BookManager {
private:
    FileStorage bookFile;
    map<string, Book> bookCache;
    
    void loadBooks() {
        bookCache.clear();
        vector<Book> books = bookFile.readAll<Book>();
        for (const auto& book : books) {
            bookCache[book.ISBN] = book;
        }
    }
    
    void saveBooks() {
        bookFile.clear();
        for (const auto& pair : bookCache) {
            bookFile.write(pair.second);
        }
    }
    
    bool matchKeyword(const string& keywords, const string& keyword) {
        stringstream ss(keywords);
        string k;
        while (getline(ss, k, '|')) {
            if (k == keyword) return true;
        }
        return false;
    }
    
public:
    BookManager() : bookFile("books.dat") {
        loadBooks();
    }
    
    void selectBook(const string& isbn) {
        if (bookCache.find(isbn) == bookCache.end()) {
            Book book;
            strcpy(book.ISBN, isbn.c_str());
            bookCache[isbn] = book;
            saveBooks();
        }
    }
    
    bool modifyBook(const string& isbn, const string& newISBN, const string& name, 
                    const string& author, const string& keyword, double price) {
        if (bookCache.find(isbn) == bookCache.end()) return false;
        
        if (!newISBN.empty()) {
            if (newISBN == isbn) return false;
            if (bookCache.find(newISBN) != bookCache.end()) return false;
            
            Book book = bookCache[isbn];
            strcpy(book.ISBN, newISBN.c_str());
            bookCache.erase(isbn);
            bookCache[newISBN] = book;
        }
        
        Book& book = bookCache[newISBN.empty() ? isbn : newISBN];
        
        if (!name.empty()) strcpy(book.name, name.c_str());
        if (!author.empty()) strcpy(book.author, author.c_str());
        if (!keyword.empty()) strcpy(book.keyword, keyword.c_str());
        if (price >= 0) book.price = price;
        
        saveBooks();
        return true;
    }
    
    bool importBook(const string& isbn, int quantity, double totalCost) {
        if (bookCache.find(isbn) == bookCache.end()) return false;
        
        bookCache[isbn].quantity += quantity;
        saveBooks();
        return true;
    }
    
    bool buyBook(const string& isbn, int quantity, double& totalCost) {
        if (bookCache.find(isbn) == bookCache.end()) return false;
        
        Book& book = bookCache[isbn];
        if (book.quantity < quantity) return false;
        
        totalCost = book.price * quantity;
        book.quantity -= quantity;
        saveBooks();
        return true;
    }
    
    vector<Book> showBooks(const string& type, const string& value) {
        vector<Book> result;
        
        for (const auto& pair : bookCache) {
            const Book& book = pair.second;
            bool match = false;
            
            if (type.empty()) {
                match = true;
            } else if (type == "ISBN") {
                match = (strcmp(book.ISBN, value.c_str()) == 0);
            } else if (type == "name") {
                match = (strcmp(book.name, value.c_str()) == 0);
            } else if (type == "author") {
                match = (strcmp(book.author, value.c_str()) == 0);
            } else if (type == "keyword") {
                match = matchKeyword(book.keyword, value);
            }
            
            if (match) result.push_back(book);
        }
        
        sort(result.begin(), result.end());
        return result;
    }
};

// ==================== Log Management ====================

class LogManager {
private:
    FileStorage transactionFile;
    FileStorage logFile;
    
public:
    LogManager() : transactionFile("transactions.dat"), logFile("logs.dat") {}
    
    void recordTransaction(double amount, bool isIncome) {
        Transaction t;
        t.amount = amount;
        t.isIncome = isIncome;
        transactionFile.write(t);
    }
    
    void recordLog(const string& userID, const string& operation) {
        LogEntry entry;
        strcpy(entry.userID, userID.c_str());
        strcpy(entry.operation, operation.c_str());
        logFile.write(entry);
    }
    
    bool showFinance(int count, double& income, double& expenditure) {
        vector<Transaction> transactions = transactionFile.readAll<Transaction>();
        
        if (count == 0) {
            income = expenditure = 0;
            return true;
        }
        
        if (count > 0 && count > (int)transactions.size()) return false;
        
        int start = (count > 0) ? transactions.size() - count : 0;
        income = expenditure = 0;
        
        for (int i = start; i < (int)transactions.size(); i++) {
            if (transactions[i].isIncome) {
                income += transactions[i].amount;
            } else {
                expenditure += transactions[i].amount;
            }
        }
        
        return true;
    }
    
    string generateFinanceReport() {
        vector<Transaction> transactions = transactionFile.readAll<Transaction>();
        stringstream ss;
        ss << "=== Finance Report ===\n";
        ss << "Total Transactions: " << transactions.size() << "\n";
        
        double income = 0, expenditure = 0;
        for (const auto& t : transactions) {
            if (t.isIncome) income += t.amount;
            else expenditure += t.amount;
        }
        
        ss << fixed << setprecision(2);
        ss << "Total Income: " << income << "\n";
        ss << "Total Expenditure: " << expenditure << "\n";
        ss << "Net Profit: " << (income - expenditure) << "\n";
        
        return ss.str();
    }
    
    string generateEmployeeReport() {
        vector<LogEntry> logs = logFile.readAll<LogEntry>();
        map<string, int> userOps;
        
        for (const auto& log : logs) {
            userOps[log.userID]++;
        }
        
        stringstream ss;
        ss << "=== Employee Report ===\n";
        for (const auto& pair : userOps) {
            ss << "User: " << pair.first << ", Operations: " << pair.second << "\n";
        }
        
        return ss.str();
    }
    
    string generateLog() {
        vector<LogEntry> logs = logFile.readAll<LogEntry>();
        vector<Transaction> transactions = transactionFile.readAll<Transaction>();
        
        stringstream ss;
        ss << "=== System Log ===\n";
        ss << "Total Log Entries: " << logs.size() << "\n";
        for (const auto& log : logs) {
            ss << "[" << log.userID << "] " << log.operation << "\n";
        }
        
        return ss.str();
    }
};

// ==================== Main Program ====================

class BookstoreSystem {
private:
    AccountManager accountMgr;
    BookManager bookMgr;
    LogManager logMgr;
    
    vector<string> tokenize(const string& line) {
        vector<string> tokens;
        string token;
        bool inQuote = false;
        
        for (size_t i = 0; i < line.length(); i++) {
            char c = line[i];
            
            if (c == '"') {
                token += c;
                inQuote = !inQuote;
            } else if (c == ' ' && !inQuote) {
                if (!token.empty()) {
                    tokens.push_back(token);
                    token.clear();
                }
            } else {
                token += c;
            }
        }
        
        if (!token.empty()) {
            tokens.push_back(token);
        }
        
        return tokens;
    }
    
    string extractQuoted(const string& str) {
        size_t start = str.find('"');
        size_t end = str.rfind('"');
        if (start != string::npos && end != string::npos && start < end) {
            return str.substr(start + 1, end - start - 1);
        }
        return "";
    }
    
    string extractValue(const string& str) {
        size_t pos = str.find('=');
        if (pos != string::npos) {
            return str.substr(pos + 1);
        }
        return "";
    }
    
    bool isValidUserID(const string& str) {
        if (str.empty() || str.length() > 30) return false;
        for (char c : str) {
            if (!isalnum(c) && c != '_') return false;
        }
        return true;
    }
    
    bool isValidKeyword(const string& str) {
        if (str.empty()) return false;
        set<string> keywords;
        stringstream ss(str);
        string k;
        while (getline(ss, k, '|')) {
            if (k.empty()) return false;
            if (keywords.count(k)) return false;
            keywords.insert(k);
        }
        return true;
    }
    
    bool safeStoi(const string& str, int& result) {
        if (str.empty()) return false;
        try {
            size_t pos;
            result = stoi(str, &pos);
            if (pos != str.length()) return false;
            // Check for negative in contexts where it shouldn't be
            return true;
        } catch (...) {
            return false;
        }
    }
    
    bool safeStod(const string& str, double& result) {
        if (str.empty()) return false;
        try {
            size_t pos;
            result = stod(str, &pos);
            if (pos != str.length()) return false;
            return true;
        } catch (...) {
            return false;
        }
    }
    
    bool isValidPrice(const string& str) {
        if (str.empty()) return false;
        int dotCount = 0;
        for (size_t i = 0; i < str.length(); i++) {
            char c = str[i];
            if (c == '.') {
                dotCount++;
                if (dotCount > 1) return false;
            } else if (!isdigit(c)) {
                return false;
            }
        }
        return true;
    }
    
    bool isValidInteger(const string& str) {
        if (str.empty()) return false;
        for (char c : str) {
            if (!isdigit(c)) return false;
        }
        return true;
    }
    
public:
    void run() {
        string line;
        while (getline(cin, line)) {
            // Trim whitespace
            size_t start = line.find_first_not_of(" \t\r\n");
            size_t end = line.find_last_not_of(" \t\r\n");
            
            if (start == string::npos) continue; // Empty line
            
            line = line.substr(start, end - start + 1);
            
            if (!processCommand(line)) {
                cout << "Invalid\n";
            }
        }
    }
    
    bool processCommand(const string& line) {
        vector<string> tokens = tokenize(line);
        if (tokens.empty()) return true;
        
        string cmd = tokens[0];
        
        // Log command
        if (accountMgr.getCurrentPrivilege() > 0) {
            logMgr.recordLog(accountMgr.getCurrentUser(), line);
        }
        
        if (cmd == "quit" || cmd == "exit") {
            exit(0);
        }
        else if (cmd == "su") {
            if (tokens.size() < 2 || tokens.size() > 3) return false;
            string userID = tokens[1];
            string password = tokens.size() == 3 ? tokens[2] : "";
            if (!isValidUserID(userID)) return false;
            return accountMgr.login(userID, password);
        }
        else if (cmd == "logout") {
            if (accountMgr.getCurrentPrivilege() < 1) return false;
            return accountMgr.logout();
        }
        else if (cmd == "register") {
            if (tokens.size() != 4) return false;
            string userID = tokens[1];
            string password = tokens[2];
            string username = tokens[3];
            if (!isValidUserID(userID) || !isValidUserID(password)) return false;
            return accountMgr.registerAccount(userID, password, username);
        }
        else if (cmd == "passwd") {
            if (accountMgr.getCurrentPrivilege() < 1) return false;
            if (tokens.size() < 3 || tokens.size() > 4) return false;
            string userID = tokens[1];
            string currentPassword = tokens.size() == 4 ? tokens[2] : "";
            string newPassword = tokens.size() == 4 ? tokens[3] : tokens[2];
            if (!isValidUserID(userID) || !isValidUserID(newPassword)) return false;
            return accountMgr.changePassword(userID, currentPassword, newPassword);
        }
        else if (cmd == "useradd") {
            if (accountMgr.getCurrentPrivilege() < 3) return false;
            if (tokens.size() != 5) return false;
            string userID = tokens[1];
            string password = tokens[2];
            if (!isValidInteger(tokens[3])) return false;
            int privilege;
            if (!safeStoi(tokens[3], privilege)) return false;
            string username = tokens[4];
            if (!isValidUserID(userID) || !isValidUserID(password)) return false;
            if (privilege != 1 && privilege != 3 && privilege != 7) return false;
            return accountMgr.addAccount(userID, password, privilege, username);
        }
        else if (cmd == "delete") {
            if (accountMgr.getCurrentPrivilege() < 7) return false;
            if (tokens.size() != 2) return false;
            string userID = tokens[1];
            return accountMgr.deleteAccount(userID);
        }
        else if (cmd == "show") {
            if (accountMgr.getCurrentPrivilege() < 1) return false;
            
            if (tokens.size() == 1) {
                vector<Book> books = bookMgr.showBooks("", "");
                if (books.empty()) cout << "\n";
                for (const auto& book : books) {
                    cout << book.ISBN << "\t" << book.name << "\t" << book.author 
                         << "\t" << book.keyword << "\t" << fixed << setprecision(2) 
                         << book.price << "\t" << book.quantity << "\n";
                }
                return true;
            }
            else if (tokens[1] == "finance") {
                if (accountMgr.getCurrentPrivilege() < 7) return false;
                int count = -1;
                if (tokens.size() == 3) {
                    if (!isValidInteger(tokens[2])) return false;
                    if (!safeStoi(tokens[2], count)) return false;
                }
                double income, expenditure;
                if (!logMgr.showFinance(count, income, expenditure)) return false;
                if (count == 0) {
                    cout << "\n";
                } else {
                    cout << "+ " << fixed << setprecision(2) << income 
                         << " - " << expenditure << "\n";
                }
                return true;
            }
            else {
                string param = tokens[1];
                string type, value;
                
                if (param.substr(0, 6) == "-ISBN=") {
                    type = "ISBN";
                    value = param.substr(6);
                } else if (param.substr(0, 6) == "-name=") {
                    type = "name";
                    value = extractQuoted(param);
                } else if (param.substr(0, 8) == "-author=") {
                    type = "author";
                    value = extractQuoted(param);
                } else if (param.substr(0, 9) == "-keyword=") {
                    type = "keyword";
                    value = extractQuoted(param);
                } else {
                    return false;
                }
                
                if (value.empty()) return false;
                if (type == "keyword" && value.find('|') != string::npos) return false;
                
                vector<Book> books = bookMgr.showBooks(type, value);
                if (books.empty()) cout << "\n";
                for (const auto& book : books) {
                    cout << book.ISBN << "\t" << book.name << "\t" << book.author 
                         << "\t" << book.keyword << "\t" << fixed << setprecision(2) 
                         << book.price << "\t" << book.quantity << "\n";
                }
                return true;
            }
        }
        else if (cmd == "buy") {
            if (accountMgr.getCurrentPrivilege() < 1) return false;
            if (tokens.size() != 3) return false;
            string isbn = tokens[1];
            if (!isValidInteger(tokens[2])) return false;
            int quantity;
            if (!safeStoi(tokens[2], quantity)) return false;
            if (quantity <= 0) return false;
            
            double totalCost;
            if (!bookMgr.buyBook(isbn, quantity, totalCost)) return false;
            
            logMgr.recordTransaction(totalCost, true);
            cout << fixed << setprecision(2) << totalCost << "\n";
            return true;
        }
        else if (cmd == "select") {
            if (accountMgr.getCurrentPrivilege() < 3) return false;
            if (tokens.size() != 2) return false;
            string isbn = tokens[1];
            bookMgr.selectBook(isbn);
            accountMgr.setSelectedBook(isbn);
            return true;
        }
        else if (cmd == "modify") {
            if (accountMgr.getCurrentPrivilege() < 3) return false;
            string isbn = accountMgr.getSelectedBook();
            if (isbn.empty()) return false;
            
            string newISBN, name, author, keyword;
            double price = -1;
            set<string> usedParams;
            
            for (size_t i = 1; i < tokens.size(); i++) {
                string param = tokens[i];
                string paramType;
                
                if (param.substr(0, 6) == "-ISBN=") {
                    paramType = "ISBN";
                    newISBN = param.substr(6);
                    if (newISBN.empty()) return false;
                } else if (param.substr(0, 6) == "-name=") {
                    paramType = "name";
                    name = extractQuoted(param);
                    if (name.empty()) return false;
                } else if (param.substr(0, 8) == "-author=") {
                    paramType = "author";
                    author = extractQuoted(param);
                    if (author.empty()) return false;
                } else if (param.substr(0, 9) == "-keyword=") {
                    paramType = "keyword";
                    keyword = extractQuoted(param);
                    if (keyword.empty() || !isValidKeyword(keyword)) return false;
                } else if (param.substr(0, 7) == "-price=") {
                    paramType = "price";
                    string priceStr = param.substr(7);
                    if (!isValidPrice(priceStr)) return false;
                    if (!safeStod(priceStr, price)) return false;
                    if (price < 0) return false;
                } else {
                    return false;
                }
                
                if (usedParams.count(paramType)) return false;
                usedParams.insert(paramType);
            }
            
            if (usedParams.empty()) return false;
            
            if (!bookMgr.modifyBook(isbn, newISBN, name, author, keyword, price)) return false;
            
            if (!newISBN.empty()) {
                accountMgr.setSelectedBook(newISBN);
            }
            
            return true;
        }
        else if (cmd == "import") {
            if (accountMgr.getCurrentPrivilege() < 3) return false;
            string isbn = accountMgr.getSelectedBook();
            if (isbn.empty()) return false;
            if (tokens.size() != 3) return false;
            
            if (!isValidInteger(tokens[1])) return false;
            if (!isValidPrice(tokens[2])) return false;
            int quantity;
            double totalCost;
            if (!safeStoi(tokens[1], quantity)) return false;
            if (!safeStod(tokens[2], totalCost)) return false;
            if (quantity <= 0 || totalCost <= 0) return false;
            
            if (!bookMgr.importBook(isbn, quantity, totalCost)) return false;
            
            logMgr.recordTransaction(totalCost, false);
            return true;
        }
        else if (cmd == "log") {
            if (accountMgr.getCurrentPrivilege() < 7) return false;
            cout << logMgr.generateLog();
            return true;
        }
        else if (cmd == "report") {
            if (accountMgr.getCurrentPrivilege() < 7) return false;
            if (tokens.size() != 2) return false;
            
            if (tokens[1] == "finance") {
                cout << logMgr.generateFinanceReport();
            } else if (tokens[1] == "employee") {
                cout << logMgr.generateEmployeeReport();
            } else {
                return false;
            }
            return true;
        }
        
        return false;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(0);
    
    BookstoreSystem system;
    system.run();
    
    return 0;
}
