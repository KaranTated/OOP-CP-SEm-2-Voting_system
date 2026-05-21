// voting_system_twilio.cpp
// Compile with: g++ -std=c++17 voting_system_twilio.cpp -o voting_system -lcpr -lssl -lcrypto -lsqlite3

#include <iostream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <map>
#include <iomanip>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <openssl/sha.h>
#include <sqlite3.h>

using json = nlohmann::json;

// ---------- Utility: SHA256 hashing (OpenSSL) ----------
std::string sha256(const std::string &str) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256_ctx;
    SHA256_Init(&sha256_ctx);
    SHA256_Update(&sha256_ctx, str.c_str(), str.size());
    SHA256_Final(hash, &sha256_ctx);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return ss.str();
}

// ---------- Database helpers (SQLite) ----------
sqlite3* db = nullptr;

bool init_db(const std::string &dbfile = "voting_system.db") {
    if (sqlite3_open(dbfile.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Error opening DB: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    const char* create_users_sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "username TEXT PRIMARY KEY,"
        "phone TEXT NOT NULL,"
        "password_hash TEXT NOT NULL,"
        "age INTEGER NOT NULL,"
        "has_voted INTEGER NOT NULL DEFAULT 0"
        ");";

    const char* create_votes_sql =
        "CREATE TABLE IF NOT EXISTS votes ("
        "candidate TEXT PRIMARY KEY,"
        "count INTEGER NOT NULL DEFAULT 0"
        ");";

    char* errmsg = nullptr;
    if (sqlite3_exec(db, create_users_sql, 0, 0, &errmsg) != SQLITE_OK) {
        std::cerr << "SQL error (users): " << errmsg << std::endl;
        sqlite3_free(errmsg);
        return false;
    }

    if (sqlite3_exec(db, create_votes_sql, 0, 0, &errmsg) != SQLITE_OK) {
        std::cerr << "SQL error (votes): " << errmsg << std::endl;
        sqlite3_free(errmsg);
        return false;
    }

    // Pre-populate candidate list if empty (example)
    const char* insert_candidates_sql =
        "INSERT OR IGNORE INTO votes (candidate, count) VALUES "
        "('Alice', 0), ('Bob', 0), ('Charlie', 0);";
    if (sqlite3_exec(db, insert_candidates_sql, 0, 0, &errmsg) != SQLITE_OK) {
        std::cerr << "SQL error (insert candidates): " << errmsg << std::endl;
        sqlite3_free(errmsg);
    }

    return true;
}

void close_db() {
    if (db) sqlite3_close(db);
}

bool user_exists(const std::string &username) {
    const char* q = "SELECT 1 FROM users WHERE username = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    bool exists = false;
    if (sqlite3_prepare_v2(db, q, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) exists = true;
    }
    sqlite3_finalize(stmt);
    return exists;
}

bool create_user(const std::string &username, const std::string &phone,
                 const std::string &password_hash, int age) {
    const char* q = "INSERT INTO users (username, phone, password_hash, age, has_voted) VALUES (?, ?, ?, ?, 0);";
    sqlite3_stmt* stmt = nullptr;
    bool ok = false;
    if (sqlite3_prepare_v2(db, q, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, phone.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, password_hash.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 4, age);
        if (sqlite3_step(stmt) == SQLITE_DONE) ok = true;
    }
    sqlite3_finalize(stmt);
    return ok;
}

bool verify_password_local(const std::string &username, const std::string &password_hash) {
    const char* q = "SELECT password_hash FROM users WHERE username = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    bool ok = false;
    if (sqlite3_prepare_v2(db, q, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* ph = sqlite3_column_text(stmt, 0);
            if (ph) {
                std::string stored = reinterpret_cast<const char*>(ph);
                ok = (stored == password_hash);
            }
        }
    }
    sqlite3_finalize(stmt);
    return ok;
}

bool get_user_phone_and_age(const std::string &username, std::string &phone_out, int &age_out, bool &has_voted_out) {
    const char* q = "SELECT phone, age, has_voted FROM users WHERE username = ? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    bool found = false;
    if (sqlite3_prepare_v2(db, q, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* p = sqlite3_column_text(stmt, 0);
            int age = sqlite3_column_int(stmt, 1);
            int has_voted = sqlite3_column_int(stmt, 2);
            phone_out = p ? reinterpret_cast<const char*>(p) : "";
            age_out = age;
            has_voted_out = (has_voted != 0);
            found = true;
        }
    }
    sqlite3_finalize(stmt);
    return found;
}

bool mark_user_voted(const std::string &username) {
    const char* q = "UPDATE users SET has_voted = 1 WHERE username = ?;";
    sqlite3_stmt* stmt = nullptr;
    bool ok = false;
    if (sqlite3_prepare_v2(db, q, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_DONE) ok = true;
    }
    sqlite3_finalize(stmt);
    return ok;
}

bool increment_vote_count(const std::string &candidate) {
    const char* q = "UPDATE votes SET count = count + 1 WHERE candidate = ?;";
    sqlite3_stmt* stmt = nullptr;
    bool ok = false;
    if (sqlite3_prepare_v2(db, q, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, candidate.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            ok = true;
        }
    }
    sqlite3_finalize(stmt);
    return ok;
}

void display_results() {
    const char* q = "SELECT candidate, count FROM votes;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, q, -1, &stmt, nullptr) == SQLITE_OK) {
        std::cout << "\n--- FINAL ELECTION RESULTS ---\n";
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* cname = sqlite3_column_text(stmt, 0);
            int cnt = sqlite3_column_int(stmt, 1);
            std::cout << (cname ? reinterpret_cast<const char*>(cname) : "Unknown") << " -> " << cnt << "\n";
        }
    }
    sqlite3_finalize(stmt);
}

// ---------- Twilio Verify API helpers (using cpr) ----------
std::string get_env_or_empty(const std::string &name) {
    const char* v = std::getenv(name.c_str());
    return v ? std::string(v) : std::string();
}

bool twilio_send_otp(const std::string &phone) {
    std::string accountSid = get_env_or_empty("TWILIO_ACCOUNT_SID");
    std::string authToken = get_env_or_empty("TWILIO_AUTH_TOKEN");   
    std::string verifyService = get_env_or_empty("TWILIO_VERIFY_SID");

    if (accountSid.empty() || authToken.empty() || verifyService.empty()) {
        std::cerr << "Twilio env vars not set. Cannot send OTP.\n";
        return false;
    }

    std::string url = "https://verify.twilio.com/v2/Services/" + verifyService + "/Verifications";

    cpr::Response r = cpr::Post(
        cpr::Url{url},
        cpr::Authentication{accountSid, authToken, cpr::AuthMode::BASIC},
        cpr::Payload{{"To", phone}, {"Channel", "sms"}}
    );

    if (r.status_code == 201 || r.status_code == 200) {
        std::cout << "OTP sent to " << phone << " (check your phone).\n";
        return true;
    } else {
        std::cerr << "Failed to request OTP. HTTP " << r.status_code << " Response: " << r.text << "\n";
        return false;
    }
}

bool twilio_verify_otp(const std::string &phone, const std::string &code) {
    std::string accountSid = get_env_or_empty("TWILIO_ACCOUNT_SID");
    std::string authToken = get_env_or_empty("TWILIO_AUTH_TOKEN");
    std::string verifyService = get_env_or_empty("TWILIO_VERIFY_SID");

    if (accountSid.empty() || authToken.empty() || verifyService.empty()) {
        std::cerr << "Twilio env vars not set. Cannot verify OTP.\n";
        return false;
    }

    std::string url = "https://verify.twilio.com/v2/Services/" + verifyService + "/VerificationCheck";

    cpr::Response r = cpr::Post(
        cpr::Url{url},
        cpr::Authentication{accountSid, authToken, cpr::AuthMode::BASIC},
        cpr::Payload{{"To", phone}, {"Code", code}}
    );

    if (r.status_code == 200) {
        try {
            auto j = json::parse(r.text);
            if (j.contains("status") && j["status"] == "approved") {
                return true;
            }
        } catch (...) {
            // parse error
        }
    } else {
        std::cerr << "Verification HTTP error: " << r.status_code << " body: " << r.text << "\n";
    }
    return false;
}

// ---------- Core flows ----------
void do_registration() {
    std::string username, phone, password;
    int age = 0;

    std::cout << "\n--- Register ---\nUsername: ";
    std::cin >> username;
    if (user_exists(username)) {
        std::cout << "User already exists. Try login.\n";
        return;
    }
    std::cout << "Phone (E.164, e.g. +91xxxxxxxxxx): ";
    std::cin >> phone;
    std::cout << "Password: ";
    std::cin >> password;
    std::cout << "Age: ";
    std::cin >> age;

    // Send OTP for registration
    if (!twilio_send_otp(phone)) {
        std::cout << "Could not send OTP. Registration aborted.\n";
        return;
    }

    std::string otp;
    std::cout << "Enter OTP sent to your phone: ";
    std::cin >> otp;

    if (!twilio_verify_otp(phone, otp)) {
        std::cout << "OTP verification failed. Registration aborted.\n";
        return;
    }

    std::string pass_hash = sha256(password);
    if (!create_user(username, phone, pass_hash, age)) {
        std::cout << "Registration failed (DB error).\n";
    } else {
        std::cout << "Registered successfully.\n";
    }
}

void do_login_and_vote() {
    std::string username, password;
    std::cout << "\n--- Login ---\nUsername: ";
    std::cin >> username;
    if (!user_exists(username)) {
        std::cout << "No such user. Please register first.\n";
        return;
    }
    std::cout << "Password: ";
    std::cin >> password;
    std::string pass_hash = sha256(password);

    if (!verify_password_local(username, pass_hash)) {
        std::cout << "Incorrect password.\n";
        return;
    }

    // Get phone and age
    std::string phone;
    int age = 0;
    bool has_voted = false;
    if (!get_user_phone_and_age(username, phone, age, has_voted)) {
        std::cout << "Error retrieving user info.\n";
        return;
    }

    // Send OTP for login
    if (!twilio_send_otp(phone)) {
        std::cout << "Could not send OTP. Login aborted.\n";
        return;
    }
    std::string otp;
    std::cout << "Enter OTP sent to your phone: ";
    std::cin >> otp;
    if (!twilio_verify_otp(phone, otp)) {
        std::cout << "OTP verification failed.\n";
        return;
    }

    // Age check
    if (age < 18) {
        std::cout << "You are under 18. Not eligible to vote.\n";
        return;
    }

    if (has_voted) {
        std::cout << "You have already voted. Multiple voting not allowed.\n";
        return;
    }

    // Allow voting
    std::cout << "\nCandidates:\n";
    // Show candidates from DB
    const char* q = "SELECT candidate FROM votes;";
    sqlite3_stmt* stmt = nullptr;
    std::vector<std::string> candidates;
    if (sqlite3_prepare_v2(db, q, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* c = sqlite3_column_text(stmt, 0);
            if (c) candidates.emplace_back(reinterpret_cast<const char*>(c));
        }
    }
    sqlite3_finalize(stmt);

    for (size_t i = 0; i < candidates.size(); ++i) {
        std::cout << i+1 << ". " << candidates[i] << "\n";
    }

    int choice;
    std::cout << "Enter candidate number to vote for: ";
    std::cin >> choice;
    if (choice < 1 || choice > (int)candidates.size()) {
        std::cout << "Invalid choice.\n";
        return;
    }
    std::string chosen = candidates[choice - 1];
    if (!increment_vote_count(chosen)) {
        std::cout << "Failed to record vote.\n";
        return;
    }
    if (!mark_user_voted(username)) {
        std::cout << "Failed to mark user as voted.\n";
        return;
    }

    std::cout << "Vote recorded for " << chosen << ". Thank you!\n";
}

int main() {
    if (!init_db()) {
        std::cerr << "Failed to initialize DB. Exiting.\n";
        return 1;
    }
    std::cout << "Voting system started.\n";

    while (true) {
        std::cout << "\nChoose: 1) Register 2) Login & Vote 3) Results & Exit\nSelection: ";
        int opt; std::cin >> opt;
        if (opt == 1) {
            do_registration();
        } else if (opt == 2) {
            do_login_and_vote();
        } else if (opt == 3) {
            display_results();
            break;
        } else {
            std::cout << "Invalid option.\n";
        }
    }

    close_db();
    std::cout << "Goodbye!\n";
    return 0;
}
