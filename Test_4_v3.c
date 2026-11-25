/*
 * IMT Bank - version 3
 *  - Accounts are saved to / loaded from a file (bank_data.bin)
 *  - Passwords are stored as salted SHA-256 hashes (never in plain text)
 *  - Admin password is chosen on first run, typed hidden, never printed
 *  - Changing a password requires the old one
 *  - Money is stored in cents (integer) and strictly validated
 *    (rejects NaN, Infinity, exponents, negatives, overflow)
 *  - Account states: Active / Restricted / Closed
 *
 * Build: gcc -std=c11 -Wall -Wextra -Wpedantic Test_4_v3.c -o bank
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>5
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int64_t money_t; /* amount in cents */

#define DATA_FILE "bank_data.bin"
#define TMP_FILE "bank_data.tmp"
#define ADMIN_ID_VALUE 1234
#define ADULT_AGE 21
#define MIN_PASS_LEN 6
#define MAX_PASS_LEN 32
#define MAX_LOGIN_TRIES 3
#define MAX_AMOUNT_CENTS ((money_t)100000000000LL) /* 1,000,000,000.00 */

enum
{
    STATE_CLOSED = 0,
    STATE_ACTIVE = 1,
    STATE_RESTRICTED = 2
};

struct New_Account
{
    char Name[100];
    char Address[100];
    u32 Age;
    money_t Balance;
    u64 National_ID;
    u64 Guardian_National_ID;
    u64 Bank_Account;
    u8 State;
    u8 Salt[16];
    u8 Hash[32];
    struct New_Account *Next; /* must stay LAST (not saved) */
};

static struct New_Account *Head = NULL;
static u8 g_admin_salt[16];
static u8 g_admin_hash[32];

/* ======================= SHA-256 ======================= */
static const u32 K256[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

#define ROR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

static void sha256_block(u32 st[8], const u8 *p)
{
    u32 w[64], a, b, c, d, e, f, g, h;
    for (int i = 0; i < 16; i++)
        w[i] = ((u32)p[i * 4] << 24) | ((u32)p[i * 4 + 1] << 16) | ((u32)p[i * 4 + 2] << 8) | p[i * 4 + 3];
    for (int i = 16; i < 64; i++)
    {
        u32 s0 = ROR(w[i - 15], 7) ^ ROR(w[i - 15], 18) ^ (w[i - 15] >> 3);
        u32 s1 = ROR(w[i - 2], 17) ^ ROR(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    a = st[0];
    b = st[1];
    c = st[2];
    d = st[3];
    e = st[4];
    f = st[5];
    g = st[6];
    h = st[7];
    for (int i = 0; i < 64; i++)
    {
        u32 S1 = ROR(e, 6) ^ ROR(e, 11) ^ ROR(e, 25);
        u32 ch = (e & f) ^ (~e & g);
        u32 t1 = h + S1 + ch + K256[i] + w[i];
        u32 S0 = ROR(a, 2) ^ ROR(a, 13) ^ ROR(a, 22);
        u32 mj = (a & b) ^ (a & c) ^ (b & c);
        u32 t2 = S0 + mj;
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    st[0] += a;
    st[1] += b;
    st[2] += c;
    st[3] += d;
    st[4] += e;
    st[5] += f;
    st[6] += g;
    st[7] += h;
}

/* One-shot SHA-256 for short messages (len <= 119 bytes). */
static void sha256(const u8 *data, size_t len, u8 out[32])
{
    u32 st[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    u8 buf[128];
    size_t total = (len + 9 <= 64) ? 64 : 128;
    memset(buf, 0, sizeof buf);
    memcpy(buf, data, len);
    buf[len] = 0x80;
    u64 bits = (u64)len * 8;
    for (int i = 0; i < 8; i++)
        buf[total - 1 - (size_t)i] = (u8)(bits >> (8 * i));
    for (size_t off = 0; off < total; off += 64)
        sha256_block(st, buf + off);
    for (int i = 0; i < 8; i++)
    {
        out[i * 4] = (u8)(st[i] >> 24);
        out[i * 4 + 1] = (u8)(st[i] >> 16);
        out[i * 4 + 2] = (u8)(st[i] >> 8);
        out[i * 4 + 3] = (u8)(st[i]);
    }
}

/* ======================= Randomness & passwords ======================= */
static void fill_random(u8 *buf, size_t n)
{
    FILE *f = fopen("/dev/urandom", "rb");
    if (f != NULL)
    {
        size_t got = fread(buf, 1, n, f);
        fclose(f);
        if (got == n)
            return;
    }
    /* Fallback (Windows): weaker, but still different for every call. */
    static int seeded = 0;
    if (!seeded)
    {
        srand((unsigned)time(NULL) ^ (unsigned)clock());
        seeded = 1;
    }
    for (size_t i = 0; i < n; i++)
        buf[i] = (u8)(rand() & 0xFF);
}

static void hash_password(const u8 salt[16], const char *pw, u8 out[32])
{
    u8 tmp[16 + MAX_PASS_LEN];
    size_t len = strlen(pw);
    if (len > MAX_PASS_LEN)
        len = MAX_PASS_LEN;
    memcpy(tmp, salt, 16);
    memcpy(tmp + 16, pw, len);
    sha256(tmp, 16 + len, out);
}

static int hashes_equal(const u8 *a, const u8 *b) /* constant time */
{
    u8 diff = 0;
    for (int i = 0; i < 32; i++)
        diff |= (u8)(a[i] ^ b[i]);
    return diff == 0;
}

static void set_account_password(struct New_Account *acc, const char *pw)
{
    fill_random(acc->Salt, sizeof acc->Salt);
    hash_password(acc->Salt, pw, acc->Hash);
}

static int check_account_password(const struct New_Account *acc, const char *pw)
{
    u8 h[32];
    hash_password(acc->Salt, pw, h);
    return hashes_equal(h, acc->Hash);
}

static int valid_password_format(const char *pw)
{
    size_t len = strlen(pw);
    if (len < MIN_PASS_LEN || len > MAX_PASS_LEN)
        return 0;
    for (size_t i = 0; i < len; i++)
        if (!isgraph((unsigned char)pw[i]))
            return 0; /* no spaces/control chars */
    return 1;
}

/* ======================= Console helpers ======================= */
static void Cyan(void) { printf("\033[1;36m"); }
static void reset(void) { printf("\033[0m"); }
static void clear_screen(void) { printf("\033[2J\033[H"); }

static void flush_line(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
    {
    }
}

static int read_line(char *buf, size_t size)
{
    if (fgets(buf, (int)size, stdin) == NULL)
        return 0;
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n')
        buf[len - 1] = '\0';
    else
        flush_line();
    return 1;
}

/* Reads a line without echoing it to the screen. */
static int read_secret(char *buf, size_t size)
{
#ifdef _WIN32
    size_t n = 0;
    int c;
    while ((c = _getch()) != '\r' && c != '\n' && c != EOF)
    {
        if (c == '\b')
        {
            if (n > 0)
                n--;
            continue;
        }
        if (n + 1 < size)
            buf[n++] = (char)c;
    }
    buf[n] = '\0';
    putchar('\n');
    return 1;
#else
    struct termios oldt, newt;
    int tty = isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &oldt) == 0;
    if (tty)
    {
        newt = oldt;
        newt.c_lflag &= ~(tcflag_t)ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    }
    int ok = read_line(buf, size);
    if (tty)
    {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        putchar('\n');
    }
    return ok;
#endif
}

static int read_u64(u64 *out)
{
    char buf[64];
    char *end;
    if (!read_line(buf, sizeof buf))
        exit(0);
    if (!isdigit((unsigned char)buf[0]))
        return 0;
    *out = strtoull(buf, &end, 10);
    return *end == '\0';
}

static int read_int(int *out)
{
    u64 v;
    if (!read_u64(&v) || v > 1000000)
        return 0;
    *out = (int)v;
    return 1;
}

static void pause_screen(void)
{
    char tmp[8];
    printf("\nPress Enter to continue...");
    read_line(tmp, sizeof tmp);
}

/* ======================= Money ======================= */
/* Strict parser: digits, optional '.' and 1-2 decimals. Nothing else.
 * So "nan", "inf", "1e9", "-5", "+5", "0x10", "1,5" are all rejected. */
static int parse_money_cents(const char *s, money_t *out)
{
    while (*s == ' ' || *s == '\t')
        s++;
    if (!isdigit((unsigned char)*s))
        return 0;

    money_t ip = 0;
    int digits = 0;
    while (isdigit((unsigned char)*s))
    {
        if (++digits > 10)
            return 0; /* prevents overflow */
        ip = ip * 10 + (*s - '0');
        s++;
    }
    money_t frac = 0;
    if (*s == '.')
    {
        s++;
        int fd = 0;
        if (!isdigit((unsigned char)*s))
            return 0;
        while (isdigit((unsigned char)*s))
        {
            if (++fd > 2)
                return 0;
            frac = frac * 10 + (*s - '0');
            s++;
        }
        if (fd == 1)
            frac *= 10;
    }
    while (*s == ' ' || *s == '\t')
        s++;
    if (*s != '\0')
        return 0;

    money_t cents = ip * 100 + frac;
    if (cents > MAX_AMOUNT_CENTS)
        return 0;
    *out = cents;
    return 1;
}

static int read_money(money_t *out, int allow_zero)
{
    char buf[64];
    if (!read_line(buf, sizeof buf))
        exit(0);
    if (!parse_money_cents(buf, out))
        return 0;
    if (!allow_zero && *out == 0)
        return 0;
    return 1;
}

static void print_money(money_t c)
{
    printf("%lld.%02lld", (long long)(c / 100), (long long)(c % 100));
}

static int would_overflow(money_t balance, money_t amount)
{
    return balance > MAX_AMOUNT_CENTS - amount;
}

/* ======================= Accounts ======================= */
static struct New_Account *Find_Account(u64 id)
{
    for (struct New_Account *t = Head; t != NULL; t = t->Next)
        if (t->Bank_Account == id)
            return t;
    return NULL;
}

static u64 generate_BankID(void)
{
    u64 id;
    do
    {
        u8 r[10];
        fill_random(r, sizeof r);
        id = (u64)(1 + r[0] % 9);
        for (int i = 1; i < 10; i++)
            id = id * 10 + (u64)(r[i] % 10);
    } while (Find_Account(id) != NULL);
    return id;
}

static void generate_password(char out[MAX_PASS_LEN + 1])
{
    u8 r[4];
    fill_random(r, sizeof r);
    u32 v = ((u32)r[0] << 24) | ((u32)r[1] << 16) | ((u32)r[2] << 8) | r[3];
    snprintf(out, MAX_PASS_LEN + 1, "%06u", 100000u + v % 900000u);
}

static const char *state_name(u8 s)
{
    switch (s)
    {
    case STATE_ACTIVE:
        return "Active";
    case STATE_RESTRICTED:
        return "Restricted (deposit only)";
    default:
        return "Closed";
    }
}

static void print_all(const struct New_Account *x)
{
    printf("\nName: %s\n", x->Name);
    printf("Address: %s\n", x->Address);
    printf("Age: %u\n", x->Age);
    if (x->Age >= ADULT_AGE)
        printf("National ID: %llu\n", (unsigned long long)x->National_ID);
    else
        printf("Guardian ID: %llu\n", (unsigned long long)x->Guardian_National_ID);
    printf("Bank ID: %llu\n", (unsigned long long)x->Bank_Account);
    printf("Balance: ");
    print_money(x->Balance);
    printf("\n");
    const char *color = x->State == STATE_ACTIVE       ? "\033[1;32m"
                        : x->State == STATE_RESTRICTED ? "\033[1;33m"
                                                       : "\033[1;31m";
    printf("Status: %s%s\033[0m\n\n", color, state_name(x->State));
}

/* ======================= File storage ======================= */
static const char MAGIC[4] = {'I', 'M', 'T', 'B'};
#define FILE_VERSION 1u

static int save_data(void)
{
    FILE *f = fopen(TMP_FILE, "wb");
    if (f == NULL)
        return 0;

    u32 count = 0;
    for (struct New_Account *t = Head; t != NULL; t = t->Next)
        count++;
    u32 version = FILE_VERSION;
    u32 recsize = (u32)offsetof(struct New_Account, Next);

    int ok = fwrite(MAGIC, 1, 4, f) == 4 && fwrite(&version, sizeof version, 1, f) == 1 && fwrite(&recsize, sizeof recsize, 1, f) == 1 && fwrite(&count, sizeof count, 1, f) == 1 && fwrite(g_admin_salt, 1, sizeof g_admin_salt, f) == sizeof g_admin_salt && fwrite(g_admin_hash, 1, sizeof g_admin_hash, f) == sizeof g_admin_hash;

    for (struct New_Account *t = Head; ok && t != NULL; t = t->Next)
        ok = fwrite(t, recsize, 1, f) == 1;

    if (fclose(f) != 0)
        ok = 0;
    if (!ok)
    {
        remove(TMP_FILE);
        return 0;
    }

#ifdef _WIN32
    remove(DATA_FILE); /* rename() can't overwrite on Windows */
#endif
    return rename(TMP_FILE, DATA_FILE) == 0;
}

static void save_or_warn(void)
{
    if (!save_data())
        printf("\033[1;31mWARNING: could not save data to %s\033[0m\n", DATA_FILE);
}

/* Returns 1 = loaded, 0 = no file yet (first run), -1 = file is damaged */
static int load_data(void)
{
    FILE *f = fopen(DATA_FILE, "rb");
    if (f == NULL)
        return 0;

    char magic[4];
    u32 version, recsize, count;
    u32 expect = (u32)offsetof(struct New_Account, Next);

    if (fread(magic, 1, 4, f) != 4 || memcmp(magic, MAGIC, 4) != 0 || fread(&version, sizeof version, 1, f) != 1 || version != FILE_VERSION || fread(&recsize, sizeof recsize, 1, f) != 1 || recsize != expect || fread(&count, sizeof count, 1, f) != 1 || count > 10000000u || fread(g_admin_salt, 1, sizeof g_admin_salt, f) != sizeof g_admin_salt || fread(g_admin_hash, 1, sizeof g_admin_hash, f) != sizeof g_admin_hash)
    {
        fclose(f);
        return -1;
    }

    struct New_Account *tail = NULL;
    for (u32 i = 0; i < count; i++)
    {
        struct New_Account *a = (struct New_Account *)calloc(1, sizeof *a);
        if (a == NULL || fread(a, recsize, 1, f) != 1)
        {
            free(a);
            fclose(f);
            return -1;
        }
        a->Name[sizeof a->Name - 1] = '\0';
        a->Address[sizeof a->Address - 1] = '\0';
        if (a->State > STATE_RESTRICTED)
            a->State = STATE_CLOSED;
        a->Next = NULL;
        if (tail == NULL)
            Head = a;
        else
            tail->Next = a;
        tail = a;
    }
    fclose(f);
    return 1;
}

static void FreeMemory(struct New_Account *head)
{
    while (head != NULL)
    {
        struct New_Account *next = head->Next;
        free(head);
        head = next;
    }
}

/* ======================= Password dialogs ======================= */
/* Asks for a new password twice. Returns 1 on success. */
static int ask_new_password(char out[MAX_PASS_LEN + 2])
{
    char again[MAX_PASS_LEN + 2];
    printf("Enter new password (%d-%d characters, no spaces): ", MIN_PASS_LEN, MAX_PASS_LEN);
    if (!read_secret(out, MAX_PASS_LEN + 2))
        exit(0);
    if (!valid_password_format(out))
    {
        printf("Invalid password format. Nothing changed.\n");
        return 0;
    }
    printf("Repeat the new password: ");
    if (!read_secret(again, sizeof again))
        exit(0);
    if (strcmp(out, again) != 0)
    {
        printf("Passwords do not match. Nothing changed.\n");
        return 0;
    }
    return 1;
}

static void setup_admin_password(void)
{
    char pw[MAX_PASS_LEN + 2];
    clear_screen();
    printf("First run: create the ADMIN password.\n\n");
    while (!ask_new_password(pw))
    {
    }
    fill_random(g_admin_salt, sizeof g_admin_salt);
    hash_password(g_admin_salt, pw, g_admin_hash);
    memset(pw, 0, sizeof pw);
    save_or_warn();
}

/* ======================= Operations ======================= */
static void Create_New_Account(void)
{
    struct New_Account *acc = (struct New_Account *)calloc(1, sizeof *acc);
    if (acc == NULL)
    {
        printf("Out of memory\n");
        return;
    }

    do
    {
        printf("Please Enter your Full Name\n");
        if (!read_line(acc->Name, sizeof acc->Name))
            exit(0);
    } while (acc->Name[0] == '\0');

    do
    {
        printf("Please Enter your Address\n");
        if (!read_line(acc->Address, sizeof acc->Address))
            exit(0);
    } while (acc->Address[0] == '\0');

    u64 age;
    printf("Enter Your Age\n");
    while (!read_u64(&age) || age == 0 || age > 120)
        printf("Invalid age, please try again:\n");
    acc->Age = (u32)age;

    u64 *id = (acc->Age >= ADULT_AGE) ? &acc->National_ID : &acc->Guardian_National_ID;
    if (acc->Age >= ADULT_AGE)
        printf("Enter your National ID (14 digits)\n");
    else
        printf("Under %d: enter your Guardian National ID (14 digits)\n", ADULT_AGE);
    while (!read_u64(id) || *id < 10000000000000ULL || *id > 99999999999999ULL)
        printf("Invalid ID, it must be exactly 14 digits. Try again:\n");

    printf("Please Enter your starting Balance (example: 250.75)\n");
    while (!read_money(&acc->Balance, 1))
        printf("Invalid amount (max 1000000000.00, up to 2 decimals). Try again:\n");

    char pw[MAX_PASS_LEN + 1];
    generate_password(pw);
    set_account_password(acc, pw);
    acc->Bank_Account = generate_BankID();
    acc->State = STATE_ACTIVE;
    acc->Next = NULL;

    if (Head == NULL)
        Head = acc;
    else
    {
        struct New_Account *t = Head;
        while (t->Next != NULL)
            t = t->Next;
        t->Next = acc;
    }
    save_or_warn();

    print_all(acc);
    printf("Your password is: \033[1;33m%s\033[0m\n", pw);
    printf("\033[1;33mWrite it down now - it is stored only as a hash and can't be shown again.\033[0m\n");
    memset(pw, 0, sizeof pw);
}

static void MakeTransaction(struct New_Account *Sender)
{
    if (Sender->State != STATE_ACTIVE)
    {
        printf("Your account is %s, transfers are not allowed.\n", state_name(Sender->State));
        return;
    }
    printf("The sender bank account: %llu\n\n", (unsigned long long)Sender->Bank_Account);
    printf("Enter the Bank Account ID you want to transfer money to\n");

    u64 to_id;
    if (!read_u64(&to_id))
        to_id = 0;
    struct New_Account *Receiver = Find_Account(to_id);

    if (Receiver == NULL)
    {
        printf("The Bank Account You Want To Send To Is Not Found\n");
        return;
    }
    if (Receiver == Sender)
    {
        printf("You can't send money to yourself\n");
        return;
    }
    if (Receiver->State == STATE_CLOSED)
    {
        printf("The receiving account is Closed\n");
        return;
    }

    printf("\nThe Received Client Bank Account ID: %llu\n", (unsigned long long)Receiver->Bank_Account);
    printf("Enter the amount of money that you want to send\n");

    money_t amount;
    if (!read_money(&amount, 0))
    {
        printf("Invalid amount\n");
        return;
    }
    if (Sender->Balance < amount)
    {
        printf("Insufficient funds\n");
        return;
    }
    if (would_overflow(Receiver->Balance, amount))
    {
        printf("Receiver's balance limit would be exceeded\n");
        return;
    }

    Sender->Balance -= amount;
    Receiver->Balance += amount;
    save_or_warn();

    printf("\nTransaction Successful\n\n");
    printf("\tSender's balance after transaction: ");
    print_money(Sender->Balance);
    printf("\n");
    printf("\tReceiver's balance after transaction: ");
    print_money(Receiver->Balance);
    printf("\n");
}

static void Get_Cash(struct New_Account *acc)
{
    if (acc->State != STATE_ACTIVE)
    {
        printf("Your account is %s, withdrawals are not allowed.\n", state_name(acc->State));
        return;
    }
    printf("Current balance: ");
    print_money(acc->Balance);
    printf("\n");
    printf("Enter amount of Cash you want\n");

    money_t amount;
    if (!read_money(&amount, 0))
    {
        printf("Invalid amount\n");
        return;
    }
    if (acc->Balance < amount)
    {
        printf("\nYour account doesn't have this amount of money\n");
        return;
    }

    acc->Balance -= amount;
    save_or_warn();
    printf("\n\tTransaction Complete\n\tYour balance now is: ");
    print_money(acc->Balance);
    printf("\n");
}

static void Deposit_in_Account(struct New_Account *acc)
{
    if (acc->State == STATE_CLOSED)
    {
        printf("Your account is Closed.\n");
        return;
    }
    printf("Current balance: ");
    print_money(acc->Balance);
    printf("\n");
    printf("Enter amount of Cash you want to deposit\n");

    money_t amount;
    if (!read_money(&amount, 0))
    {
        printf("Invalid amount\n");
        return;
    }
    if (would_overflow(acc->Balance, amount))
    {
        printf("Balance limit would be exceeded\n");
        return;
    }

    acc->Balance += amount;
    save_or_warn();
    printf("\n\tTransaction Complete\n\tYour balance now is: ");
    print_money(acc->Balance);
    printf("\n");
}

static void ChangePassword(struct New_Account *acc, int is_admin)
{
    char pw[MAX_PASS_LEN + 2];

    if (!is_admin) /* the client must prove who they are */
    {
        printf("Enter your CURRENT password: ");
        if (!read_secret(pw, sizeof pw))
            exit(0);
        if (!check_account_password(acc, pw))
        {
            printf("Wrong password. Nothing changed.\n");
            memset(pw, 0, sizeof pw);
            return;
        }
    }
    if (!ask_new_password(pw))
        return;

    set_account_password(acc, pw);
    memset(pw, 0, sizeof pw);
    save_or_warn();
    printf("\nPassword Changed Successfully\n");
}

static void Change_Account_State(struct New_Account *acc)
{
    printf("Current state: %s\n\n", state_name(acc->State));
    printf("Enter 1 to set Active\n"
           "Enter 2 to set Restricted (deposit only)\n"
           "Enter 3 to set Closed\n");
    int c;
    if (!read_int(&c))
        c = -1;

    switch (c)
    {
    case 1:
        acc->State = STATE_ACTIVE;
        break;
    case 2:
        acc->State = STATE_RESTRICTED;
        break;
    case 3:
        acc->State = STATE_CLOSED;
        break;
    default:
        printf("You entered a wrong number\n");
        return;
    }
    save_or_warn();
    printf("The account is now: %s\n", state_name(acc->State));
}

static void Account_Menu(struct New_Account *acc, int is_admin)
{
    int back = 0;
    while (!back)
    {
        clear_screen();
        printf("Account: %llu   State: %s\n\n", (unsigned long long)acc->Bank_Account, state_name(acc->State));
        printf("Enter 1 to Make Transaction\n"
               "Enter 2 to Change Account Password\n"
               "Enter 3 to Get Cash\n"
               "Enter 4 to Deposit in Account\n"
               "Enter 5 to Show Account Details\n");
        if (is_admin)
            printf("Enter 6 to Change Account Status\n");
        printf("Enter any other number to return to main page\n");

        int c;
        if (!read_int(&c))
            c = -1;
        clear_screen();

        switch (c)
        {
        case 1:
            MakeTransaction(acc);
            pause_screen();
            break;
        case 2:
            ChangePassword(acc, is_admin);
            pause_screen();
            break;
        case 3:
            Get_Cash(acc);
            pause_screen();
            break;
        case 4:
            Deposit_in_Account(acc);
            pause_screen();
            break;
        case 5:
            print_all(acc);
            pause_screen();
            break;
        case 6:
            if (is_admin)
            {
                Change_Account_State(acc);
                pause_screen();
            }
            else
                back = 1;
            break;
        default:
            back = 1;
            break;
        }
    }
    clear_screen();
}

/* ======================= Login ======================= */
static int admin_login(void)
{
    for (int tries = 0; tries < MAX_LOGIN_TRIES; tries++)
    {
        int id;
        char pw[MAX_PASS_LEN + 2];
        printf("Enter Admin ID\n");
        if (!read_int(&id))
            id = -1;
        printf("Enter Admin Password\n");
        if (!read_secret(pw, sizeof pw))
            exit(0);

        u8 h[32];
        hash_password(g_admin_salt, pw, h);
        memset(pw, 0, sizeof pw);
        if (id == ADMIN_ID_VALUE && hashes_equal(h, g_admin_hash))
            return 1;

        printf("Wrong ID or Password (%d/%d)\n\n", tries + 1, MAX_LOGIN_TRIES);
    }
    return 0;
}

static void client_session(void)
{
    clear_screen();
    for (int tries = 0; tries < MAX_LOGIN_TRIES; tries++)
    {
        u64 id;
        char pw[MAX_PASS_LEN + 2];
        printf("Enter your Bank Account ID\n");
        if (!read_u64(&id))
            id = 0;
        printf("Enter your Account Password\n");
        if (!read_secret(pw, sizeof pw))
            exit(0);

        struct New_Account *acc = Find_Account(id);
        int ok = acc != NULL && check_account_password(acc, pw);
        memset(pw, 0, sizeof pw);

        if (ok)
        {
            if (acc->State == STATE_CLOSED)
            {
                clear_screen();
                printf("This account is Closed. Please contact the bank.\n\n");
                return;
            }
            Account_Menu(acc, 0);
            return;
        }
        printf("Wrong Bank Account ID or Password (%d/%d)\n\n", tries + 1, MAX_LOGIN_TRIES);
    }
}

static void admin_session(void)
{
    if (!admin_login())
    {
        clear_screen();
        printf("Too many failed attempts.\n\n");
        return;
    }

    int back = 0;
    while (!back)
    {
        clear_screen();
        printf("Enter 1 to Create an account\n"
               "Enter 2 to Open An Existing Account\n"
               "Enter 3 to Return to the main page\n");
        int opt;
        if (!read_int(&opt))
            opt = -1;

        switch (opt)
        {
        case 1:
            clear_screen();
            Create_New_Account();
            pause_screen();
            break;
        case 2:
        {
            clear_screen();
            printf("Enter a Client Bank Account ID\n");
            u64 id;
            if (!read_u64(&id))
                id = 0;
            struct New_Account *acc = Find_Account(id);
            if (acc == NULL)
            {
                printf("The Bank Account You Want To Open Is Not Found\n");
                pause_screen();
            }
            else
                Account_Menu(acc, 1);
            break;
        }
        case 3:
            back = 1;
            clear_screen();
            break;
        default:
            clear_screen();
            printf("Invalid number, please try again\n");
            pause_screen();
            break;
        }
    }
}

/* ======================= main ======================= */
int main(void)
{
    int status = load_data();
    if (status < 0)
    {
        fprintf(stderr, "ERROR: %s is damaged or from another version.\n"
                        "It was NOT modified. Fix or move it, then run again.\n",
                DATA_FILE);
        FreeMemory(Head);
        return 1;
    }
    if (status == 0)
        setup_admin_password();

    int running = 1;
    while (running)
    {
        Cyan();
        printf("\t\tWelcome To IMT Bank Account\n\n");
        reset();
        printf("Enter 1 for Admin Window\n    OR    \nEnter 2 for Client Window\nEnter 3 to Exit\n");

        int choice;
        if (!read_int(&choice))
            choice = -1;

        switch (choice)
        {
        case 1:
            admin_session();
            break;
        case 2:
            client_session();
            break;
        case 3:
            running = 0;
            break;
        default:
            clear_screen();
            printf("Invalid input. Please enter 1, 2 or 3.\n\n");
            break;
        }
    }

    save_or_warn();
    FreeMemory(Head);
    printf("Goodbye!\n");
    return 0;
}
