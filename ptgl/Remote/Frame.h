#pragma once
#include "Detail/Codec.h"
#include <algorithm>

namespace ptgl::remote {
enum class DrawOp : std::uint16_t {
    color=1, lineWidth, pointSize, point, line, box, sphere, cylinder, axis,
    pushMatrix, popMatrix, identity, translate, scale, transform, material
};
class Frame {
public:
    Frame& setColor(double r,double g,double b,double a=1) { return command(DrawOp::color,{r,g,b,a}); }
    Frame& setLineWidth(double width) { return command(DrawOp::lineWidth,{width}); }
    Frame& setPointSize(double size) { return command(DrawOp::pointSize,{size}); }
    Frame& setMaterial(double roughness,double reflectance) { return command(DrawOp::material,{roughness,reflectance}); }
    Frame& drawPoint(Vec3 p) { return command(DrawOp::point,{p[0],p[1],p[2]}); }
    Frame& drawLine(Vec3 a,Vec3 b) { return command(DrawOp::line,{a[0],a[1],a[2],b[0],b[1],b[2]}); }
    Frame& drawBox(Vec3 p,Quaternion q,Vec3 size) { return command(DrawOp::box,{p[0],p[1],p[2],q[0],q[1],q[2],q[3],size[0],size[1],size[2]}); }
    Frame& drawSphere(Vec3 p,double radius) { return command(DrawOp::sphere,{p[0],p[1],p[2],radius}); }
    Frame& drawCylinder(Vec3 p,Quaternion q,double length,double radius) { return command(DrawOp::cylinder,{p[0],p[1],p[2],q[0],q[1],q[2],q[3],length,radius}); }
    Frame& drawAxis(Vec3 p,Quaternion q,double length) { return command(DrawOp::axis,{p[0],p[1],p[2],q[0],q[1],q[2],q[3],length}); }
    Frame& pushMatrix() { return command(DrawOp::pushMatrix,{}); }
    Frame& popMatrix() { return command(DrawOp::popMatrix,{}); }
    Frame& identity() { return command(DrawOp::identity,{}); }
    Frame& translate(double x,double y,double z) { return command(DrawOp::translate,{x,y,z}); }
    Frame& scale(double x,double y,double z) { return command(DrawOp::scale,{x,y,z}); }
    Frame& transform(Vec3 p,Quaternion q) { return command(DrawOp::transform,{p[0],p[1],p[2],q[0],q[1],q[2],q[3]}); }
    const Bytes& data() const { return data_; }
    void clear() { data_.clear(); }
    static void validate(const Bytes& data) {
        detail::require(data.size()<=Limits::maxPayload-4,"Frame too large");
        detail::Reader r(data); int depth=0;
        const unsigned sizes[]={0,4,1,1,3,6,10,4,9,8,0,0,0,3,3,7,2};
        while(r.remaining()) {
            unsigned op=r.u16(); auto flags=r.u16(); auto bytes=r.u32();
            detail::require(op>=1 && op<=16 && flags==0 && bytes==sizes[op]*8,"Invalid drawing opcode");
            double v[10]{}; for(unsigned i=0;i<sizes[op];++i) v[i]=r.finite();
            if(op==unsigned(DrawOp::pushMatrix)) detail::require(++depth<=64,"Transform depth exceeded");
            if(op==unsigned(DrawOp::popMatrix)) detail::require(depth-->0,"Unbalanced transform");
            if(op==2 || op==3) detail::require(v[0]>0 && v[0]<=256,"Invalid line/point size");
            if(op==6 || op==8 || op==9 || op==15) {
                double norm=0; for(int i=3;i<7;++i) norm+=v[i]*v[i];
                detail::require(std::isfinite(norm) && norm>1e-20,"Invalid quaternion");
            }
            if(op==6) for(int i=7;i<10;++i) detail::require(v[i]>0,"Invalid box size");
            if(op==7) detail::require(v[3]>0,"Invalid radius");
            if(op==8) detail::require(v[7]>0 && v[8]>0,"Invalid cylinder size");
            if(op==9) detail::require(v[7]>0,"Invalid axis size");
            if(op==1) for(int i=0;i<4;++i) detail::require(v[i]>=0 && v[i]<=1,"Invalid color");
            if(op==16) for(int i=0;i<2;++i) detail::require(v[i]>=0 && v[i]<=1,"Invalid material");
        }
        detail::require(depth==0,"Unbalanced transform");
    }
private:
    Frame& command(DrawOp op,std::initializer_list<double> values) {
        detail::require(data_.size()+8+8*values.size()<=Limits::maxPayload-4,"Frame too large");
        detail::Writer w; w.u16(std::uint16_t(op)); w.u16(0); w.u32(std::uint32_t(values.size()*8));
        for(auto v:values) { detail::require(std::isfinite(v),"Non-finite drawing value"); w.f64(v); }
        data_.insert(data_.end(),w.data.begin(),w.data.end()); return *this;
    }
    Bytes data_;
};
}
