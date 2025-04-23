#include "Interface_impl.h"
#include "Interface.h"
#include "feature_description.h"
#include "feature_exports.h"
#include "feature_types.h"
#include "utils/feature_utils.h"
#include <cstring>

namespace Feature_Interface {

#define countof(x) (sizeof(x) / sizeof(x[0]))

static const char* file_tag = "[jidl_feature] interface_1_0_impl";

void onRegister(const char* feature_name)
{
    FEATURE_LOG_INFO("onRegister: %s", feature_name);
}

void onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("onCreatre: FeatureProtoHandle %p", handle);
}

void onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
    FEATURE_LOG_INFO("onDestroy: FeatureProtoHandle %p", handle);
}

void onUnregister(const char* feature_name)
{
    FEATURE_LOG_INFO("onUnregister: %s", feature_name);
}

IAnimal* Interface::createDog(FtInt type)
{
    return makeInterface<dog>(type);
}

IBird* Interface::createPigeon()
{
    return makeInterface<pigeon>();
}

IChicken* Interface::createCock()
{
    return makeInterface<cock>();
}

IAnimal* Interface::createCat()
{
    return makeInterface<cat>();
}

void Interface::setAnimal(const IAnimal*& animal)
{
}

void Interface::flyFar(FtPromiseId pid, FtInt distance)
{
}

void Interface::print(FtVariParams vari_params)
{
}

cock::cock(FeatureInstanceHandle hInstance)
    : IChicken(hInstance)
{
    set_name(strdup("my cock"));
    set_breed(strdup("cock breed"));
}

cock::~cock()
{
    FEATURE_LOG_ERROR("~cock finalizer called !");
}

ft_utils::FtStringPtr cock::name() const
{
    return _name;
}

void cock::set_name(const ft_utils::FtStringPtr& val)
{
    _name = val;
}

FtInt cock::legCount() const
{
    return _legCount;
}

FtInt cock::eatFood(const ft_utils::RefPtr<FtArray>& food)
{
    return _eatFood;
}

ft_utils::FtStringPtr cock::run(FtInt distance, const ft_utils::FtStringPtr& destination)
{
    char buf[512];
    sprintf(buf, "cock run with %d leg, distance %d, destination %s", legCount(), distance, destination.ptr());
    return strdup(buf);
}

ArrayType string_array_type = {
    .header = { .type = COMPLEX_ARRAY, .size = sizeof(FtArray) },
    .element_type = FT_STRING
};

// cock
ft_utils::RefPtr<FtArray> cock::fly()
{
    FEATURE_LOG_INFO("cock fly...");
    auto strArray = makeArray<const char*>(4);
    for (int i = 0; i < strArray.size(); i++) {
        char buf[100];
        sprintf(buf, "cock flip wings %d", i + 4);
        strArray[i] = strdup(buf).drop();
    }
    return strArray.getShared();
}

ft_utils::FtStringPtr cock::breed() const
{
    return _breed;
}

void cock::set_breed(const ft_utils::FtStringPtr& breed)
{
    _breed = breed;
}
// IChicken
FtInt cock::weight() const
{
    return _weight;
}

void cock::set_weight(FtInt weight)
{
    _weight = weight;
}

void cock::walk(FtPromiseId pid)
{
    printf("%s::%s(), interface: %p, %s\n", file_tag, __FUNCTION__, getHandle(), "cock walk slowly");
    auto strArray = makeArray<const char*>(4);
    for (int i = 0; i < strArray.size(); i++) {
        char buf[100];
        sprintf(buf, "cock walk %d", i);
        strArray[i] = strdup(buf).drop();
    }
    FeaturePromiseResolve(getHandle(), pid, strArray.getShared().ptr());
}

dog::dog(FeatureInstanceHandle hInstance, FtInt type)
    : IAnimal(hInstance)
    , _type(type)
{
    _name = strdup("xiao huang");
}

dog::~dog()
{
    FEATURE_LOG_ERROR("~dog finalizer called !");
}

ft_utils::FtStringPtr dog::name() const
{
    return _name;
}

void dog::set_name(const ft_utils::FtStringPtr& val)
{
    _name = val;
}

FtInt dog::legCount() const
{
    return _legCount;
}

FtInt dog::eatFood(const ft_utils::RefPtr<FtArray>& food)
{
    FEATURE_LOG_INFO("dog eat food count: %d", food->_size);
    auto foodArray = food.getArray<const char*>();
    for (size_t i = 0; i < foodArray.size(); i++) {
        FtString food_name = foodArray[i];
        FEATURE_LOG_INFO("%d: %s", i, food_name);
    }
    _eatFood = foodArray.size();
    return _eatFood;
}

ft_utils::FtStringPtr dog::run(FtInt distance, const ft_utils::FtStringPtr& destination)
{
    char buf[512];
    sprintf(buf, "dog run with %d leg, distance %d, destination %s", legCount(), distance, destination.ptr());
    return strdup(buf);
}

cat::cat(FeatureInstanceHandle hInstance)
    : dog(hInstance, 0)
{
    set_name(strdup("mimi"));
}

ft_utils::FtStringPtr cat::run(FtInt distance, const ft_utils::FtStringPtr& destination)
{
    char buf[512];
    sprintf(buf, "cat %s run with %d leg, distance %d, destination %s", name().ptr(), legCount(), distance, destination.ptr());
    return strdup(buf);
}

pigeon::~pigeon()
{
    FEATURE_LOG_ERROR("~pigeon finalizer called !");
}

ft_utils::RefPtr<FtArray> pigeon::fly()
{
    FEATURE_LOG_INFO("pigeon fly...");
    auto strArray = makeArray<const char*>(4);
    for (int i = 0; i < strArray.size(); i++) {
        char buf[100];
        sprintf(buf, "cock flip wings %d", i + 4);
        strArray[i] = strdup(buf).drop();
    }
    // return strArray;
    return strArray.getShared();
}

ft_utils::FtStringPtr pigeon::breed() const
{
    return _breed;
}

void pigeon::set_breed(const ft_utils::FtStringPtr& breed)
{
    _breed = breed;
}

}