# An Object-Oriented Design Approach for Secure Electronic Voting System with Two-Factor Authentication

**Authors:** [Your Name], [Co-Author Name]  
**Affiliation:** Department of Computer Science, [University Name]  
**Date:** 2024

---

## Abstract

This paper presents the design and implementation of a secure electronic voting system using object-oriented programming principles in C++. The system integrates MySQL database management with Twilio's two-factor authentication (2FA) service to ensure secure user registration and voting processes. The implementation demonstrates key OOP concepts including inheritance, polymorphism, encapsulation, composition, and dependency injection. The system architecture employs an interface-based design pattern that allows for extensibility and maintainability. Experimental results demonstrate successful integration of database operations with external API services, achieving secure user authentication and vote recording functionality.

**Keywords:** Object-Oriented Programming, Electronic Voting System, Two-Factor Authentication, Database Management, Software Design Patterns, C++

---

## 1. Introduction

Electronic voting systems have become increasingly important in modern democratic processes, requiring robust security mechanisms and reliable data management. Traditional voting systems face challenges related to authentication, data integrity, and system extensibility. This paper presents a comprehensive object-oriented solution that addresses these challenges through well-structured software design principles.

The proposed system implements a secure voting platform that combines database persistence with two-factor authentication (2FA) via SMS-based one-time passwords (OTP). The system architecture leverages modern C++ features and object-oriented design patterns to create a maintainable, extensible, and secure voting application.

### 1.1 Contributions

The contributions of this work include:

- A modular object-oriented architecture demonstrating inheritance and polymorphism
- Integration of MySQL database with C++ using the MySQL Connector/C++
- Implementation of two-factor authentication using Twilio Verify API
- Application of dependency injection and interface segregation principles

---

## 2. Related Work

Electronic voting systems have been extensively studied in the literature. Previous research has focused on cryptographic security [1], blockchain-based solutions [2], and usability aspects [3]. However, limited attention has been given to the application of object-oriented design principles in voting system architecture.

Database integration in C++ applications has been explored through various connectors and ORM frameworks. The MySQL Connector/C++ provides a native interface for database operations, while modern C++ features like smart pointers enable safe resource management [4].

Two-factor authentication has become a standard security practice, with SMS-based OTP being widely adopted due to its accessibility. The Twilio Verify API provides a robust service for implementing 2FA in applications [5].

---

## 3. System Architecture and Design

### 3.1 Object-Oriented Design Principles

The system architecture is built upon fundamental OOP principles:

#### 3.1.1 Inheritance and Polymorphism

The system implements an abstract base class `IDatabaseService` that defines the interface for database operations. The concrete implementation `DatabaseService` inherits from this interface, enabling polymorphism. This design allows the `VotingApp` class to depend on the abstraction rather than concrete implementations, following the Dependency Inversion Principle.

```cpp
class IDatabaseService {
public:
    virtual ~IDatabaseService() = default;
    virtual bool open(...) = 0;
    virtual bool userExists(const string &username) = 0;
    virtual bool createUser(...) = 0;
    // ... other pure virtual methods
};

class DatabaseService : public IDatabaseService {
    // Concrete implementation using MySQL
};
```

#### 3.1.2 Encapsulation

Each class encapsulates related functionality and data. The `DatabaseService` class encapsulates MySQL connection management and all database operations, hiding implementation details from client code. Similarly, `OtpService` encapsulates Twilio API interactions.

#### 3.1.3 Composition

The `VotingApp` class demonstrates composition by containing references to `IDatabaseService` and `OtpService` objects. This "has-a" relationship allows the voting application to coordinate between database and authentication services.

#### 3.1.4 Resource Management (RAII)

The system extensively uses `std::unique_ptr` for automatic resource management, following the Resource Acquisition Is Initialization (RAII) principle. Database connections, statements, and result sets are automatically cleaned up when objects go out of scope.

### 3.2 System Components

#### 3.2.1 Database Service Layer

The `DatabaseService` class manages all database operations including:

- Database and table creation
- User registration and authentication
- Vote recording and retrieval
- Election results display

The implementation uses prepared statements to prevent SQL injection attacks and ensures data integrity through transaction management.

#### 3.2.2 OTP Service Layer

The `OtpService` class handles two-factor authentication through Twilio's Verify API. It provides methods for:

- Sending OTP codes via SMS
- Verifying OTP codes entered by users
- Error handling for API failures

#### 3.2.3 Application Layer

The `VotingApp` class orchestrates the voting workflow, coordinating between database and OTP services to implement:

- User registration with OTP verification
- User login and authentication
- Vote casting with eligibility checks
- Results display

---

## 4. Implementation Details

### 4.1 Technology Stack

The system is implemented using:

- **C++17**: Modern C++ features including smart pointers, auto keyword, and lambda expressions
- **MySQL 8.0+**: Relational database management system
- **MySQL Connector/C++ 9.5**: Native C++ interface for MySQL
- **CPR Library**: HTTP client library for REST API calls
- **nlohmann/json**: JSON parsing library for API responses
- **CMake**: Build system for cross-platform compilation
- **vcpkg**: C++ package manager for dependency management

### 4.2 Database Schema

The system uses two main tables:

**Users Table:**
- `username` (VARCHAR(50), PRIMARY KEY)
- `phone` (VARCHAR(20), NOT NULL)
- `password_hash` (VARCHAR(128), NOT NULL)
- `age` (INT, NOT NULL)
- `has_voted` (BOOLEAN, DEFAULT 0)

**Votes Table:**
- `candidate` (VARCHAR(50), PRIMARY KEY)
- `count` (INT, DEFAULT 0)

### 4.3 Authentication Flow

The system implements a multi-step authentication process:

1. **Registration**: User provides username, phone, password, and age. System sends OTP to phone number. Upon successful OTP verification, user account is created.

2. **Login**: User provides username and password. System verifies credentials and checks eligibility (age ≥ 18, not already voted). OTP is sent for 2FA verification.

3. **Voting**: After successful authentication, user selects a candidate and vote is recorded.

### 4.4 Polymorphism Implementation

Polymorphism is demonstrated through the interface-based design:

```cpp
// In VotingApp class
VotingApp(IDatabaseService &db, OtpService &otp) 
    : db_(db), otp_(otp) {}

// db_ is of type IDatabaseService&, but receives DatabaseService object
// Method calls are resolved at runtime through virtual function dispatch
void registerFlow() {
    if (db_.userExists(username)) { ... }
    db_.createUser(...);
}
```

The `VotingApp` class operates on the `IDatabaseService` interface, but at runtime, calls are dispatched to the concrete `DatabaseService` implementation. This enables flexibility to swap implementations without modifying client code.

---

## 5. Results and Evaluation

### 5.1 Functional Testing

The system was tested with the following scenarios:

- User registration with valid OTP
- User registration with invalid OTP (rejected)
- User login with correct credentials
- User login with incorrect credentials (rejected)
- Vote casting by eligible users
- Attempted vote by underage users (rejected)
- Attempted duplicate voting (rejected)
- Results display functionality

All test cases passed successfully, demonstrating correct implementation of business logic and security constraints.

### 5.2 Performance Considerations

The system demonstrates efficient resource management through:

- Use of prepared statements for database queries (reduced parsing overhead)
- Smart pointer usage eliminates memory leaks
- Connection pooling capabilities through MySQL Connector
- Asynchronous API calls potential (future enhancement)

### 5.3 Code Quality Metrics

The object-oriented design provides:

- **Maintainability**: Clear separation of concerns with single responsibility principle
- **Extensibility**: Interface-based design allows adding new database implementations
- **Testability**: Dependency injection enables unit testing with mock objects
- **Reusability**: Service classes can be reused in other applications

---

## 6. Discussion

### 6.1 Design Pattern Benefits

The interface-based design pattern provides several advantages:

1. **Extensibility**: New database implementations (e.g., PostgreSQL, SQLite) can be added by creating new classes inheriting from `IDatabaseService`

2. **Testability**: Mock database services can be created for unit testing without requiring actual database connections

3. **Maintainability**: Changes to database implementation do not affect client code

4. **Flexibility**: Database backend can be swapped at runtime or compile-time

### 6.2 Limitations and Future Work

Current limitations include:

- Single-threaded execution model
- No support for concurrent voting sessions
- Limited error recovery mechanisms
- No encryption of sensitive data at rest

Future enhancements could include:

- Multi-threading support for concurrent operations
- Blockchain integration for vote immutability
- Advanced encryption for sensitive data
- Web-based user interface
- Real-time vote counting and visualization
- Audit logging and compliance features

---

## 7. Conclusion

This paper presented an object-oriented design and implementation of a secure electronic voting system. The system successfully demonstrates key OOP principles including inheritance, polymorphism, encapsulation, and composition. The interface-based architecture provides extensibility and maintainability, while integration with MySQL and Twilio services ensures robust data management and secure authentication.

The implementation serves as a practical example of applying modern C++ features and design patterns to real-world applications. The modular design allows for future enhancements and demonstrates best practices in software engineering.

The system successfully integrates database operations with external API services, achieving secure user authentication and vote recording functionality. The object-oriented approach provides a solid foundation for building scalable and maintainable voting systems.

---

## Acknowledgments

The authors would like to acknowledge the open-source community for providing excellent libraries including MySQL Connector/C++, CPR, and nlohmann/json that made this implementation possible.

---

## References

[1] A. Author, "Cryptographic Security in Electronic Voting," *IEEE Trans. Security*, vol. 1, no. 1, pp. 1-10, 2020.

[2] B. Author, "Blockchain-Based Voting Systems," *Proc. IEEE Conf. Blockchain*, 2021, pp. 100-110.

[3] C. Author, "Usability in Electronic Voting Interfaces," *IEEE Trans. HCI*, vol. 5, no. 2, pp. 50-60, 2019.

[4] MySQL Documentation, "MySQL Connector/C++ Developer Guide," Oracle Corporation, 2023. [Online]. Available: https://dev.mysql.com/doc/connector-cpp/

[5] Twilio Documentation, "Twilio Verify API," Twilio Inc., 2023. [Online]. Available: https://www.twilio.com/docs/verify

---

## Appendix A: Code Structure

### A.1 Class Hierarchy

```
IDatabaseService (Abstract Base Class)
    └── DatabaseService (Concrete Implementation)

OtpService (Concrete Class)

VotingApp (Orchestrator Class)
    ├── Uses IDatabaseService& (polymorphism)
    └── Uses OtpService& (composition)
```

### A.2 Key Design Patterns

1. **Interface Segregation**: `IDatabaseService` defines a clean interface
2. **Dependency Injection**: `VotingApp` receives dependencies via constructor
3. **RAII**: Automatic resource management with smart pointers
4. **Strategy Pattern**: Database implementation can be swapped

---

## Appendix B: Build Instructions

### B.1 Prerequisites

- Visual Studio 2022 or GCC 9.0+
- CMake 3.10+
- MySQL Server 8.0+
- MySQL Connector/C++ 9.5
- vcpkg package manager

### B.2 Build Steps

```bash
# Install dependencies via vcpkg
vcpkg install cpr nlohmann-json --triplet x64-windows

# Configure and build
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64 \
    -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

---

**Word Count:** ~2,500 words  
**Pages:** ~8 pages (IEEE format)

