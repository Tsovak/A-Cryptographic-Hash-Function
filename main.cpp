#include <iostream>
#include <vector>
#include <sstream>

using namespace std;

template<typename T, typename T_half, int size>

class double_uint {
private:
    void fromstring(const std::string &s);

    std::string tostring(void) const;

public:
    T lo_, hi_;
    enum type_size {
        tsize = size,
        hsize = size / 2,
        qsize = size / 4
    };

    double_uint(const double_uint &u) : lo_(u.lo_), hi_(u.hi_) {}

    double_uint(const T_half &hi1, const T_half &hi2, const T_half &lo1, const T_half &lo2)
            : lo_(T(lo2) + (T(lo1) << qsize)),
              hi_(T(hi2) + (T(hi1) << qsize)) {}

    double_uint(const T &hi, const T &lo) : lo_(lo), hi_(hi) {}

    explicit double_uint(const T &lo) : lo_(lo), hi_() {}

    explicit double_uint(const std::string &s) { fromstring(s); };

    double_uint(void) : lo_(), hi_() {}

    /* Hack - basically we are a little-endian integer type */
    explicit double_uint(const unsigned char *c) { memcpy(this, c, sizeof(*this)); }

    explicit double_uint(size_t len, const unsigned char *c) {
        if (len >= sizeof(*this)) {
            memcpy(this, c, sizeof(*this));
        } else {
            memcpy(this, c, len);
            memset(reinterpret_cast<unsigned char *>(this) + len, 0, sizeof(*this) - len);
        }
    }

    friend std::ostream &operator<<(std::ostream &o, const double_uint &x) {
        return o << std::string(x);
    }

    double_uint &operator=(int x) {
        lo_ = x;
        hi_ = T();
        return *this;
    }

    double_uint &operator=(const double_uint &u) {
        lo_ = u.lo_;
        hi_ = u.hi_;
        return *this;
    }

    operator T() const { return lo_; }

    operator T() { return lo_; }

    operator const std::string() const { return tostring(); }

    operator std::string() { return tostring(); }

    double_uint operator+=(const double_uint &u) {
        T old = lo_;
        lo_ += u.lo_;
        if (lo_ < old) hi_++;
        hi_ += u.hi_;
        return *this;
    }

    double_uint operator-=(const double_uint &u) {
        T old = lo_;
        lo_ -= u.lo_;
        if (lo_ > old) hi_--;
        hi_ -= u.lo_;
        return *this;
    }

    double_uint operator+(const double_uint &u) const {
        double_uint res(*this);
        return res += u;
    }

    double_uint operator-(const double_uint &u) const {
        double_uint res(*this);
        return res -= u;
    }

    double_uint operator++(void) {
        return *this += double_uint(1);
    }

    double_uint operator++(int) {
        double_uint res(*this);
        *this += double_uint(0, 1);
        return res;
    }

    double_uint operator-(void) {
        return double_uint(0) - *this;
    }

    double_uint operator+(void) const {
        return *this;
    }

    double_uint operator<<=(int x) {
        x &= (tsize - 1);
        if (x >= hsize) {
            hi_ = lo_ << (x - hsize);
            lo_ = 0;
            return *this;
        }

        hi_ <<= x;
        hi_ += lo_ >> (hsize - x);
        lo_ <<= x;

        return *this;
    }

    double_uint operator>>=(int x) {
        x &= (tsize - 1);
        if (x >= hsize) {
            lo_ = hi_ >> (x - hsize);
            hi_ = 0;
            return *this;
        }

        lo_ >>= x;
        lo_ += hi_ << (hsize - x);
        hi_ >>= x;

        return *this;
    }

    double_uint operator<<(int x) const {
        double_uint res(*this);
        return res <<= x;
    }

    double_uint operator>>(int x) const {
        double_uint res(*this);
        return res >>= x;
    }

    static double_uint halfmul(T x, T y) {
        T_half x1, x2, y1, y2;
        x1 = x;
        x2 = x >> qsize;
        y1 = y;
        y2 = y >> qsize;

        T xx1(x1);
        T xx2(x2);
        T yy1(y1);
        T yy2(y2);

        double_uint p1, p2, p3, p4;
        p1.lo_ = xx1 * yy1;
        p2.lo_ = xx1 * yy2;
        p3.lo_ = xx2 * yy1;
        p4.lo_ = xx2 * yy2;

        p2 <<= qsize;
        p3 <<= qsize;
        p4 <<= hsize;
        return p1 + p2 + p3 + p4;
    }

    double_uint operator*=(const double_uint &u) {
        double_uint p1 = halfmul(lo_, u.lo_);
        double_uint p2 = halfmul(lo_, u.hi_);
        double_uint p3 = halfmul(hi_, u.lo_);

        p2 <<= hsize;
        p3 <<= hsize;
        return (*this = p1 + p2 + p3);
    }

    double_uint operator*(const double_uint &u) const {
        double_uint res(*this);
        return res *= u;
    }

    double_uint operator^=(const double_uint &u) {
        lo_ ^= u.lo_;
        hi_ ^= u.hi_;
        return *this;
    }

    double_uint operator^(const double_uint &u) const {
        double_uint res(*this);
        return res ^= u;
    }

    /* Calculates (self * u, folded in half) xor u */
    double_uint fold(const double_uint &u) const {
        typedef double_uint<double_uint, T, tsize * 2> Tdouble;

        Tdouble x(Tdouble::halfmul(*this, u));

        return x.lo_ ^ x.hi_;
    }
};

template<class T, class T_half, int size>
std::string double_uint<T, T_half, size>::tostring(void) const {
    int num = size / 4;

    static const char hex_symb[] = "0123456789ABCDEF";

    /* Pad with zeros */
    std::string s(num, '0');

    double_uint<T, T_half, size> v(*this);

    for (int i = num - 1; i >= 0; i--) {
        s[i] = hex_symb[v.lo_ & 0xf];
        v >>= 4;
    }

    return s;
}

template<class T, class T_half, int size>
void double_uint<T, T_half, size>::fromstring(const std::string &s) {
    double_uint<T, T_half, size> temp;

    *this = 0;

    for (std::string::const_iterator i = s.begin(); i != s.end(); i++) {
        if ((*i >= '0') && (*i <= '9')) {
            temp = *i - '0';
        } else if ((*i >= 'A') && (*i <= 'F')) {
            temp = *i - 'A' + 10;
        } else if ((*i >= 'a') && (*i <= 'f')) {
            temp = *i - 'A' + 10;
        } else {
            throw std::runtime_error("Invalid hex character\n");
        }

        *this <<= 4;
        *this += temp;
    }
}

static std::string tostring(const __uint128_t &x) {
    int num = 128 / 4;

    static const char hex_symb[] = "0123456789ABCDEF";

    /* Pad with zeros */
    std::string s(num, '0');

    __uint128_t v(x);

    for (int i = num - 1; i >= 0; i--) {
        s[i] = hex_symb[v & 0xf];
        v >>= 4;
    }

    return s;
}

std::ostream &operator<<(std::ostream &o, const __uint128_t &x) {
    return o << tostring(x);
}

typedef double_uint<__uint128_t, unsigned long long, 256> u256;

void hash_step(const u256 &i1, const u256 &i2, u256 &o1, u256 &o2) {
    /*
     * Cube roots of primes.
     * (Square roots don't work as well when folded with zero.
     *  The high part becomes nearly all one bits.)
     *
     * They are scaled so that the uppermost bit is set.
     * Then the lowest bit is also set, so that the constant is odd
     */

    /* cbrt(2) */
    static const u256 t1(0xa14517cc6b945711, 0x1eed5b8adf128686,
                         0x144788148b18fde0, 0x30c00661b7d16e9d);

    /* cbrt(3) */
    static const u256 t2(0xb89ba24891f7b2e6, 0xef3f8b62b71933e0,
                         0x50c4a6157ab766cc, 0xfa2ba143e9029653);

    /* cbrt(5) */
    static const u256 t3(0xdae07de7f6269d97, 0xed0ddb59924b141a,
                         0x0ae36687aa58c29f, 0xe8293af2918f493b);

    /* cbrt(7) */
    static const u256 t4(0xf4daedd2c0c4edde, 0x50536bb743875dac,
                         0xfdb214852ccf272e, 0x53a3540f5e5aa011);

    /* cbrt(11) */
    static const u256 t5(0x8e55b096fcd22d4e, 0x3c1e6d4936833117,
                         0x0ae1a0b51ea515b2, 0x6ef98efb6ebf35e3);

    /* cbrt(13) */
    static const u256 t6(0x967c447c6d817406, 0x7bc5196b06dc9887,
                         0x214ac2f50046dc65, 0x0f9bfa326367aeb7);

    /* cbrt(17) */
    static const u256 t7(0xa48fe0a92bc653e6, 0xec03c7ed7e59981b,
                         0x3e3a27d8d8e54797, 0xd607fe20b08d6175);

    /* cbrt(19) */
    static const u256 t8(0xaac717b5769b6046, 0x27896d0e27f2c11e,
                         0x281e73be041f0383, 0xa937169045fb3849);

    /* cbrt(23) */
    static const u256 t9(0xb601eaa628c0c090, 0xac51900eab494a5c,
                         0x236edd364b4df8c4, 0x5a0cdaed7df05aed);

    o1 = i1;
    o2 = i2;

    o1 += (o2 ^ t1).fold(t1);
    o2 += (o1 ^ t2).fold(t2);
    o1 += (o2 ^ t3).fold(t3);
    o2 += (o1 ^ t4).fold(t4);
    o1 += (o2 ^ t5).fold(t5);
    o2 += (o1 ^ t6).fold(t6);
    o1 += (o2 ^ t7).fold(t7);
    o2 += (o1 ^ t8).fold(t8);
    o1 += (o2 ^ t9).fold(t9);
}

void hash_foldmul256(const std::vector<unsigned char> &input, unsigned char output[32]) {
    size_t len = input.size();
    size_t offset = 0;

    /* Assume less than 64 levels of recursion for now */
    u256 p1[64], p2[64];

    unsigned long long filled = 0;

    int i, j;

    for (len = input.size();; len -= 32, offset += 32) {
        u256 t1(len, offset);
        u256 t2(len, &input[offset]);

        hash_step(t1, t2, t1, t2);

        filled++;

        for (i = 0; i < 64; i++) {
            if (filled & (1ull << i)) break;

            /* These xors are irreversible */
            hash_step(p1[i] ^ t2, t1 ^ p2[i], t1, t2);
        }

        p1[i] = t1;
        p2[i] = t2;

        if (len <= 32) break;
    }

    /* Find the first value */
    for (i = 0; i < 64; i++) {
        if (filled & (1ull << i)) break;
    }

    /* Handle the remaining partial evaluations */
    for (j = i + 1; j < 64; j++) {
        if (!(filled & (1ull << i))) continue;

        /* These xors are irreversible */
        hash_step(p1[i] ^ p2[j], p1[j] ^ p2[i], p1[i], p2[i]);
    }

    /* Final irreversible step - only return a single output */
    memcpy(output, &p1[i], 32);
}

typedef vector<unsigned char> bufferType;


int main() {

    u256 *ll1 = new u256('A', 'Cryptographic', 'Hash', 'Function');
    u256 *ll2 = new u256(50000000, 6000000, 7000000, 8000000);
    u256 *ll3 = new u256(9000000000, 10000000000, 11000000000, 12000000000);
    u256 *ll4 = new u256(1300000000000000000, 14000000000000000000, 15000000000000000000, 16000000000000000000);

    hash_step(*ll1, *ll2, *ll3, *ll4);
    cout << tostring(ll1->lo_) << " : " << tostring(ll1->hi_) << endl;
    cout << tostring(ll2->lo_) << " : " << tostring(ll2->hi_) << endl;
    cout << tostring(ll3->lo_) << " : " << tostring(ll3->hi_) << endl;
    cout << tostring(ll4->lo_) << " : " << tostring(ll4->hi_) << endl << endl << endl;


    u256 str1 = (const u256 &) "Blue";
    u256 str2 = (const u256 &) "Red";
    u256 str3 = (const u256 &) "Orange";
    u256 str4 = (const u256 &) "Yellow";
    hash_step(str1, str2, str3, str4);
    cout << tostring(str1.lo_) << " : " << tostring(str1.hi_) << endl;
    cout << tostring(str2.lo_) << " : " << tostring(str2.hi_) << endl;
    cout << tostring(str3.lo_) << " : " << tostring(str3.hi_) << endl;
    cout << tostring(str4.lo_) << " : " << tostring(str4.hi_) << endl;

    const char *testdata = "A Cryptographic Hash Function";
    unsigned char *buffer = (unsigned char *) testdata;
    std::size_t size = strlen((const char *) buffer);
    std::vector<unsigned char> var(buffer, buffer + size);

    unsigned char output[32];
    hash_foldmul256(var, output);

    cout << output << endl << endl << endl;
    for (int i = 0; i < 32; i++) {
        cout << hex << output[i];
    }

    return 0;
}
