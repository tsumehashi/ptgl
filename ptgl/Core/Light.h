#ifndef PTGL_CORE_LIGHT_H_
#define PTGL_CORE_LIGHT_H_

#include <memory>

namespace ptgl {

class Light;
typedef std::shared_ptr<Light> LightPtr;

class Light {
public:
    Light();
    virtual ~Light();

    virtual void initializeConfiguration() {}

    virtual void update() {}
};

} /* namespace ptgl */

#endif /* PTGL_CORE_LIGHT_H_ */
