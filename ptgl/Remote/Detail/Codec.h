#pragma once
#include "../Types.h"
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace ptgl::remote::detail {
class ProtocolError : public std::runtime_error {
public: explicit ProtocolError(const char* message) : std::runtime_error(message) {}
};
inline void require(bool condition, const char* message) { if (!condition) throw ProtocolError(message); }
inline bool validUtf8(const std::string& s) {
    for (std::size_t i=0; i<s.size();) {
        auto c=static_cast<unsigned char>(s[i++]);
        if (c<0x80) { if (c==0) return false; continue; }
        unsigned n, cp, minimum;
        if (c>=0xc2 && c<=0xdf) { n=1; cp=c&31; minimum=0x80; }
        else if (c>=0xe0 && c<=0xef) { n=2; cp=c&15; minimum=0x800; }
        else if (c>=0xf0 && c<=0xf4) { n=3; cp=c&7; minimum=0x10000; }
        else return false;
        if (s.size()-i<n) return false;
        while (n--) { c=static_cast<unsigned char>(s[i++]); if ((c&0xc0)!=0x80) return false; cp=(cp<<6)|(c&63); }
        if (cp<minimum || cp>0x10ffff || (cp>=0xd800 && cp<=0xdfff)) return false;
    }
    return true;
}
struct Writer {
    Bytes data;
    template<class T> void integer(T value) {
        static_assert(std::is_unsigned<T>::value, "Unsigned wire integer");
        for (unsigned i=0; i<sizeof(T); ++i) data.push_back(std::uint8_t(value>>(i*8)));
    }
    void u8(std::uint8_t v) { integer(v); }
    void u16(std::uint16_t v) { integer(v); }
    void u32(std::uint32_t v) { integer(v); }
    void u64(std::uint64_t v) { integer(v); }
    void i64(std::int64_t v) { std::uint64_t bits; std::memcpy(&bits,&v,8); u64(bits); }
    void f32(float v) { static_assert(sizeof(float)==4 && std::numeric_limits<float>::is_iec559,"IEEE float32"); std::uint32_t b; std::memcpy(&b,&v,4); u32(b); }
    void f64(double v) { static_assert(sizeof(double)==8 && std::numeric_limits<double>::is_iec559,"IEEE float64"); std::uint64_t b; std::memcpy(&b,&v,8); u64(b); }
    void text(const std::string& v) { require(v.size()<=4096 && validUtf8(v),"Invalid UTF-8 string"); u32(std::uint32_t(v.size())); data.insert(data.end(),v.begin(),v.end()); }
    void bytes(const Bytes& v) { data.insert(data.end(),v.begin(),v.end()); }
};
class Reader {
public:
    Reader(const void* p, std::size_t n) : p_(static_cast<const std::uint8_t*>(p)), n_(n) {}
    explicit Reader(const Bytes& bytes) : Reader(bytes.data(),bytes.size()) {}
    std::size_t remaining() const { return n_-i_; }
    template<class T> T integer() {
        require(remaining()>=sizeof(T),"Truncated integer"); T value=0;
        for (unsigned b=0; b<sizeof(T); ++b) value |= T(p_[i_++]) << (8*b);
        return value;
    }
    std::uint8_t u8() { return integer<std::uint8_t>(); }
    std::uint16_t u16() { return integer<std::uint16_t>(); }
    std::uint32_t u32() { return integer<std::uint32_t>(); }
    std::uint64_t u64() { return integer<std::uint64_t>(); }
    std::int64_t i64() { auto b=u64(); std::int64_t v; std::memcpy(&v,&b,8); return v; }
    float f32() { auto b=u32(); float v; std::memcpy(&v,&b,4); return v; }
    double f64() { auto b=u64(); double v; std::memcpy(&v,&b,8); return v; }
    double finite() { double v=f64(); require(std::isfinite(v),"Non-finite value"); return v; }
    bool boolean() { auto v=u8(); require(v<=1,"Invalid boolean"); return v!=0; }
    std::string text() { auto n=u32(); require(n<=4096 && n<=remaining(),"Invalid string length"); std::string s(reinterpret_cast<const char*>(p_+i_),n); i_+=n; require(validUtf8(s),"Invalid UTF-8"); return s; }
    Bytes bytes(std::size_t n) { require(n<=remaining(),"Truncated bytes"); Bytes v; if(n) v.assign(p_+i_,p_+i_+n); i_+=n; return v; }
    void end() const { require(remaining()==0,"Trailing payload"); }
private: const std::uint8_t* p_; std::size_t n_, i_=0;
};
} // namespace ptgl::remote::detail
