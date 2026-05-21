#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <cstdlib>
#include <memory>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <mysql/jdbc.h>

using json = nlohmann::json;
using namespace std;

// ---------- Utility ----------
static string getenv_safe(const string &name) {
    const char* val = std::getenv(name.c_str());
    return val ? string(val) : string();
}

static string simple_hash(const string &s) {
    size_t h = hash<string>{}(s);
    stringstream ss;
    ss << hex << h;
    return ss.str();
}

// ---------- OOP: DatabaseService with Inheritance ----------
class IDatabaseService {
public:
    virtual ~IDatabaseService() = default;

    virtual bool open(const string &host = "tcp://127.0.0.1:3306",
                      const string &user = "root",
                      const string &pass = "Karan@29137",
                      const string &db = "voting_db") = 0;

    virtual bool userExists(const string &username) = 0;
    virtual bool createUser(const string &username, const string &phone,
                            const string &password_hash, int age) = 0;
    virtual bool checkPassword(const string &username, const string &password_hash) = 0;
    virtual bool getUserInfo(const string &username, string &phone, int &age, bool &has_voted) = 0;
    virtual bool markVoted(const string &username) = 0;
    virtual bool incrementVote(const string &candidate) = 0;
    virtual vector<string> listCandidates() = 0;
    virtual void showResults() = 0;
};

class DatabaseService : public IDatabaseService {
public:
    DatabaseService() = default;

    bool open(const string &host = "tcp://127.0.0.1:3306",
              const string &user = "root",
              const string &pass = "Karan@29137",
              const string &db = "voting_db") override {
        try {
            driver_ = sql::mysql::get_driver_instance();
            connection_.reset(driver_->connect(host, user, pass));

            unique_ptr<sql::Statement> stmt(connection_->createStatement());
            stmt->execute("CREATE DATABASE IF NOT EXISTS " + db);
            connection_->setSchema(db);

            stmt->execute("CREATE TABLE IF NOT EXISTS users ("
                          "username VARCHAR(50) PRIMARY KEY,"
                          "phone VARCHAR(20) NOT NULL,"
                          "password_hash VARCHAR(128) NOT NULL,"
                          "age INT NOT NULL,"
                          "has_voted BOOLEAN DEFAULT 0)");

            stmt->execute("CREATE TABLE IF NOT EXISTS votes ("
                          "candidate VARCHAR(50) PRIMARY KEY,"
                          "count INT DEFAULT 0)");

            stmt->execute("INSERT IGNORE INTO votes(candidate, count) VALUES"
                          "('Alice',0),('Bob',0),('Charlie',0)");
            return true;
        } catch (sql::SQLException &e) {
            cerr << "[MySQL] Connection/Initialization Error: " << e.what() << endl;
            return false;
        }
    }

    bool userExists(const string &username) override {
        try {
            unique_ptr<sql::PreparedStatement> pstmt(
                connection_->prepareStatement("SELECT username FROM users WHERE username=? LIMIT 1"));
            pstmt->setString(1, username);
            unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            return res->next();
        } catch (...) { return false; }
    }

    bool createUser(const string &username, const string &phone, const string &password_hash, int age) override {
        try {
            unique_ptr<sql::PreparedStatement> pstmt(
                connection_->prepareStatement("INSERT INTO users(username, phone, password_hash, age, has_voted) VALUES (?,?,?,?,0)"));
            pstmt->setString(1, username);
            pstmt->setString(2, phone);
            pstmt->setString(3, password_hash);
            pstmt->setInt(4, age);
            pstmt->execute();
            return true;
        } catch (sql::SQLException &e) {
            cerr << "[MySQL] Insert failed: " << e.what() << endl;
            return false;
        }
    }

    bool checkPassword(const string &username, const string &password_hash) override {
        try {
            unique_ptr<sql::PreparedStatement> pstmt(
                connection_->prepareStatement("SELECT password_hash FROM users WHERE username=?"));
            pstmt->setString(1, username);
            unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            if (res->next()) {
                return res->getString("password_hash") == password_hash;
            }
        } catch (...) {}
        return false;
    }

    bool getUserInfo(const string &username, string &phone, int &age, bool &has_voted) override {
        try {
            unique_ptr<sql::PreparedStatement> pstmt(
                connection_->prepareStatement("SELECT phone, age, has_voted FROM users WHERE username=?"));
            pstmt->setString(1, username);
            unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            if (res->next()) {
                phone = res->getString("phone");
                age = res->getInt("age");
                has_voted = res->getBoolean("has_voted");
                return true;
            }
        } catch (...) {}
        return false;
    }

    bool markVoted(const string &username) override {
        try {
            unique_ptr<sql::PreparedStatement> pstmt(
                connection_->prepareStatement("UPDATE users SET has_voted=1 WHERE username=?"));
            pstmt->setString(1, username);
            pstmt->execute();
            return true;
        } catch (...) { return false; }
    }

    bool incrementVote(const string &candidate) override {
        try {
            unique_ptr<sql::PreparedStatement> pstmt(
                connection_->prepareStatement("UPDATE votes SET count = count + 1 WHERE candidate=?"));
            pstmt->setString(1, candidate);
            pstmt->execute();
            return true;
        } catch (...) { return false; }
    }

    vector<string> listCandidates() override {
        vector<string> out;
        try {
            unique_ptr<sql::Statement> stmt(connection_->createStatement());
            unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT candidate FROM votes ORDER BY candidate"));
            while (res->next()) out.push_back(res->getString("candidate"));
        } catch (...) {}
        return out;
    }

    void showResults() override {
        cout << "\n--- ELECTION RESULTS ---\n";
        try {
            unique_ptr<sql::Statement> stmt(connection_->createStatement());
            unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT candidate, count FROM votes ORDER BY count DESC"));
            while (res->next()) {
                cout << res->getString("candidate") << " -> " << res->getInt("count") << endl;
            }
        } catch (...) {
            cout << "[Error fetching results]" << endl;
        }
    }

private:
    sql::mysql::MySQL_Driver* driver_ = nullptr;
    unique_ptr<sql::Connection> connection_;
};

// ---------- Twilio OTP ----------
class OtpService {
public:
bool sendVerification(const string &to_phone) {
    const string sid = getenv_safe("TWILIO_ACCOUNT_SID");
    const string token = getenv_safe("TWILIO_AUTH_TOKEN");
    const string verifySid = getenv_safe("TWILIO_VERIFY_SID");

    if (sid.empty() || token.empty() || verifySid.empty()) {
        cerr << "[Twilio] Environment variables missing.\n";
        return false;
    }

    string url = "https://verify.twilio.com/v2/Services/" + verifySid + "/Verifications";
    cpr::Response r = cpr::Post(
        cpr::Url{url},
        cpr::Authentication{sid, token, cpr::AuthMode::BASIC},
        cpr::Payload{{"To", to_phone}, {"Channel", "sms"}}
    );

    // Debug: print what Twilio says
    cout << "[Twilio send] status=" << r.status_code << "\n";
    cout << "[Twilio send] body=" << r.text << "\n";

    return (r.status_code == 200 || r.status_code == 201);
}

    bool checkVerification(const string &to_phone, const string &code) {
        const string sid = getenv_safe("TWILIO_ACCOUNT_SID");
        const string token = getenv_safe("TWILIO_AUTH_TOKEN");
        const string verifySid = getenv_safe("TWILIO_VERIFY_SID");
        if (sid.empty() || token.empty() || verifySid.empty()) return false;

        string url = "https://verify.twilio.com/v2/Services/" + verifySid + "/VerificationCheck";
        cpr::Response r = cpr::Post(
            cpr::Url{url},
            cpr::Authentication{sid, token, cpr::AuthMode::BASIC},
            cpr::Payload{{"To", to_phone}, {"Code", code}}
        );
        if (r.status_code == 200) {
            try {
                auto j = json::parse(r.text);
                if (j.contains("status") && j["status"] == "approved") return true;
            } catch (...) {}
        }
        return false;
    }
};

// ---------- OOP: VotingApp orchestrates flows ----------
class VotingApp {
public:
    VotingApp(IDatabaseService &db, OtpService &otp) : db_(db), otp_(otp) {}

    void registerFlow() {
        string username, phone, password;
        int age;
        cout << "\n--- Register ---\n";
        cout << "Username: "; cin >> username;
        if (db_.userExists(username)) { cout << "User exists.\n"; return; }
        cout << "Phone (+91...): "; cin >> phone;
        cout << "Password: "; cin >> password;
        cout << "Age: "; cin >> age;

        cout << "Sending OTP...\n";
        if (!otp_.sendVerification(phone)) { cout << "Failed to send OTP.\n"; return; }
        string otpCode; cout << "Enter OTP: "; cin >> otpCode;
        if (!otp_.checkVerification(phone, otpCode)) { cout << "OTP failed.\n"; return; }

        if (db_.createUser(username, phone, simple_hash(password), age))
            cout << "Registration successful!\n";
        else cout << "Error creating user.\n";
    }

    void loginAndVoteFlow() {
        string username, password;
        cout << "\n--- Login ---\n";
        cout << "Username: "; cin >> username;
        if (!db_.userExists(username)) { cout << "No such user.\n"; return; }
        cout << "Password: "; cin >> password;

        if (!db_.checkPassword(username, simple_hash(password))) {
            cout << "Invalid password.\n"; return;
        }

        string phone; int age; bool voted;
        if (!db_.getUserInfo(username, phone, age, voted)) { cout << "DB error.\n"; return; }
        if (voted) { cout << "You already voted.\n"; return; }
        if (age < 18) { cout << "Underage.\n"; return; }

        cout << "Sending OTP to " << phone << "...\n";
        if (!otp_.sendVerification(phone)) { cout << "OTP send failed.\n"; return; }
        string otpCode; cout << "Enter OTP: "; cin >> otpCode;
        if (!otp_.checkVerification(phone, otpCode)) { cout << "OTP failed.\n"; return; }

        vector<string> cands = db_.listCandidates();
        cout << "\nCandidates:\n";
        for (size_t i = 0; i < cands.size(); ++i)
            cout << i+1 << ". " << cands[i] << endl;

        int ch; cout << "Select candidate #: "; cin >> ch;
        if (ch < 1 || ch > (int)cands.size()) { cout << "Invalid.\n"; return; }

        if (db_.incrementVote(cands[ch-1]) && db_.markVoted(username))
            cout << "Vote recorded for " << cands[ch-1] << ".\n";
        else cout << "Error saving vote.\n";
    }

    void showResults() { db_.showResults(); }

private:
    IDatabaseService &db_;
    OtpService &otp_;
};

// ---------- Main ----------
int main() {
    cout << "=== AI Voting System (MySQL + Twilio OTP) ===\n";

    DatabaseService db;
    if (!db.open()) return 1;
    OtpService otp;
    VotingApp app(db, otp);

    while (true) {
        cout << "\n1) Register\n2) Login & Vote\n3) Show Results\n4) Exit\nChoice: ";
        int ch; cin >> ch;
        if (ch == 1) app.registerFlow();
        else if (ch == 2) app.loginAndVoteFlow();
        else if (ch == 3) app.showResults();
        else if (ch == 4) break;
        else cout << "Invalid choice.\n";
    }

    cout << "Goodbye!\n";
    return 0;
}
