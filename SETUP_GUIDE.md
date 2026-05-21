# MySQL Setup Guide for Voting System

## 📦 What to Download

### 1. MySQL Server
- **Download**: [MySQL Community Server](https://dev.mysql.com/downloads/mysql/)
- **Version**: 8.0 or later
- **Installation**: 
  - Run the installer
  - Choose "Developer Default" or "Server only"
  - Set root password (remember this for your code)
  - Complete the installation

### 2. MySQL Connector/C++
- **Download**: [MySQL Connector/C++](https://dev.mysql.com/downloads/connector/cpp/)
- **Version**: 9.3 or later
- **Installation**:
  - Extract to `C:\Program Files\MySQL\MySQL Connector C++ 9.3\`
  - Or update the path in `CMakeLists.txt` if installed elsewhere

### 3. vcpkg Dependencies (Already in project)
The following packages need to be installed via vcpkg:
- `cpr` - HTTP client library for Twilio API
- `nlohmann-json` - JSON parsing library

### 4. Visual Studio Build Tools (if not already installed)
- **Download**: [Visual Studio Build Tools](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022)
- **Required Components**:
  - C++ CMake tools for Windows
  - MSVC v143 - VS 2022 C++ x64/x86 build tools
  - Windows 10/11 SDK

---

## 🔧 Changes Made to CMakeLists.txt

1. **Added vcpkg toolchain support** - Automatically finds vcpkg if `VCPKG_ROOT` environment variable is set
2. **Added cpr and nlohmann-json libraries** - Required for Twilio API calls
3. **Made paths relative** - Changed hardcoded paths to use `${CMAKE_SOURCE_DIR}` for better portability

---

## 📝 Install vcpkg Packages

Open PowerShell in the project root and run:

```powershell
# Navigate to vcpkg directory
cd vcpkg

# Install required packages
.\vcpkg.exe install cpr:x64-windows
.\vcpkg.exe install nlohmann-json:x64-windows

# Or install both at once
.\vcpkg.exe install cpr nlohmann-json --triplet x64-windows
```

---

## 🏗️ Build Commands for x64 Native Tools

### Option 1: Using CMake from Command Line (Recommended)

```powershell
# Navigate to project root
cd E:\OOP\CP1

# Create build directory
mkdir build
cd build

# Configure with CMake (x64)
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake

# Build the project
cmake --build . --config Release

# The executable will be in: build\Release\voting_system.exe
```

### Option 2: Using Developer Command Prompt

```cmd
# Open "x64 Native Tools Command Prompt for VS 2022"
# Navigate to project
cd E:\OOP\CP1

# Create and enter build directory
mkdir build
cd build

# Configure
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build . --config Release
```

### Option 3: Using Visual Studio

1. Open Visual Studio 2022
2. File → Open → CMake...
3. Select `CMakeLists.txt` from project root
4. Select "x64-Release" configuration
5. Build → Build All (or press F7)

---

## ⚙️ Environment Variables to Set

Set these environment variables for Twilio (optional, but required for OTP features):

```powershell
# In PowerShell
$env:TWILIO_ACCOUNT_SID = "your_account_sid"
$env:TWILIO_AUTH_TOKEN = "your_auth_token"
$env:TWILIO_VERIFY_SID = "your_verify_service_sid"
```

Or set them permanently:
1. Right-click "This PC" → Properties
2. Advanced system settings → Environment Variables
3. Add new User/System variables

---

## 🗄️ MySQL Database Setup

1. **Start MySQL Server**:
   - Open Services (services.msc)
   - Find "MySQL80" (or your MySQL service)
   - Start it if not running

2. **Update Database Credentials** (if needed):
   - Edit `main.cpp` line 35
   - Change password from `"Karan@29137"` to your MySQL root password
   - Or create a new MySQL user:
     ```sql
     CREATE USER 'voting_user'@'localhost' IDENTIFIED BY 'your_password';
     GRANT ALL PRIVILEGES ON voting_db.* TO 'voting_user'@'localhost';
     FLUSH PRIVILEGES;
     ```

3. **Test Connection**:
   - The application will automatically create the database and tables on first run
   - Database name: `voting_db`
   - Tables: `users`, `votes`

---

## 🚀 Running the Application

```powershell
# Navigate to build directory
cd E:\OOP\CP1\build\Release

# Run the executable
.\voting_system.exe
```

---

## 🔍 Troubleshooting

### Error: "Cannot find mysqlcppconn"
- **Solution**: Verify MySQL Connector/C++ is installed at the path specified in `CMakeLists.txt`
- Update the path in `CMakeLists.txt` if installed elsewhere

### Error: "Cannot find cpr" or "Cannot find nlohmann_json"
- **Solution**: Install via vcpkg:
  ```powershell
  cd vcpkg
  .\vcpkg.exe install cpr nlohmann-json --triplet x64-windows
  ```

### Error: "MySQL connection failed"
- **Solution**: 
  - Ensure MySQL Server is running
  - Check username/password in `main.cpp` line 35
  - Verify MySQL is listening on port 3306

### Error: "LNK2019: unresolved external symbol"
- **Solution**: Ensure all libraries are properly linked in `CMakeLists.txt`
- Rebuild the project after installing vcpkg packages

---

## 📋 Quick Reference: Build Command

**For x64 Native Tools Command Prompt:**
```cmd
cd E:\OOP\CP1
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

**For PowerShell:**
```powershell
cd E:\OOP\CP1
New-Item -ItemType Directory -Force -Path build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

