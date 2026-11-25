# Bank-Account-Management-System
A C-based banking management system that allows administrators to manage bank accounts and clients to perform banking operations, including deposits, withdrawals, money transfers, and balance inquiries. It uses linked lists for dynamic account management and provides basic login and input validation features.
# Bank (Version 3)

A console-based banking management system written in **C11**, featuring persistent binary storage, salted SHA-256 password hashing, strict integer-cent financial validation, and account state management.

---

## Features

- **Persistent Storage (`bank_data.bin`):**
  - Saves and loads accounts and admin credentials using a custom binary format (`IMTB` magic header, version check, and record size verification).
  - Writes to a temporary file (`bank_data.tmp`) first and atomically replaces `bank_data.bin` to protect data integrity.
- **Password Security:**
  - Passwords are never stored in plain text.
  - Uses a unique 16-byte random salt per account/admin combined with **SHA-256** and constant-time hash comparison.
  - Hidden terminal input (`termios` on POSIX, `_getch` on Windows) for password entry.
  - Admin password is set securely on first run; client password changes require verifying the current password.
- **Strict Money Handling:**
  - Balances and transactions are stored as integer cents (`int64_t`) to avoid floating-point precision issues.
  - Strict parser rejects `NaN`, `Infinity`, scientific notation (`1e9`), negative/signed values, invalid decimals, and overflow (`MAX_AMOUNT_CENTS = 100,000,000,000` cents / `1,000,000,000.00`).
- **Account States:**
  - `Active` — Full access (transfers, withdrawals, deposits, viewing details, changing password).
  - `Restricted (deposit only)` — Deposits allowed; withdrawals and outgoing transfers blocked.
  - `Closed` — Account login, transfers, withdrawals, and deposits blocked.
- **Admin & Client Workflows:**
  - **Admin (`ID: 1234`):** Create accounts (with auto-generated 10-digit Bank IDs and 6-digit initial passwords), open existing client accounts, perform operations on client accounts, and change account states.
  - **Client:** Log in with Bank Account ID and password (up to 3 attempts), transfer funds, withdraw cash, deposit funds, view account details, and change password.

---

## Build & Run

### Requirements
- A C11-compatible compiler (`gcc` or `clang`)

### Compile
```bash
gcc -std=c11 -Wall -Wextra -Wpedantic Test_4_v3.c -o bank
