#include "Interface_impl.h"
#include "feature_description.h"
#include "feature_exports.h"
#include "feature_types.h"
#include "Interface.h"
#include "utils/feature_utils.h"

namespace Feature_Interface {

#define countof(x) (sizeof(x) / sizeof(x[0]))

static const char* file_tag = "[jidl_feature] interface_1_0_impl";

void onRegister(const char* feature_name)
{
}

void onCreate(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
}

void onRequired(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    auto* p = Interface::newInstance(handle);
    FeatureSetObjectData(handle, p);
}

void onDetached(FeatureRuntimeContext ctx, FeatureInstanceHandle handle)
{
    Interface* pInstance = ft_utils::From<Interface>(handle);
    if (pInstance) {
        delete pInstance;
        FeatureSetObjectData(handle, nullptr);
    }
}

void onDestroy(FeatureRuntimeContext ctx, FeatureProtoHandle handle)
{
}

void onUnregister(const char* feature_name)
{
}

dog* Interface::createDog(AppendData append_data, FtInt type)
{
    return new dog(getHandle(), type);
}

pigeon* Interface::createPigeon(AppendData append_data)
{
    return new pigeon(getHandle());
}

cock* Interface::createCock(AppendData append_data)
{
    return new cock(getHandle());
}

IAnimal* Interface::createCat(AppendData append_data)
{
    return new cat(getHandle());
}

void Interface::setAnimal(AppendData append_data, ft_utils::RefPtr<IAnimal> animal)
{
}

void Interface::flyFar(AppendData append_data, FtPromiseId pid, FtInt distance)
{
}

void Interface::print(AppendData append_data, FtVariParams vari_params)
{
}

cock::cock(FeatureInstanceHandle hInstance)
    : _hInst(hInstance)
{
    char* str = static_cast<char*>(FeatureInstanceAllocType(getHandle(), strlen("my cock") + 1, FT_STRING));
    strcpy(str, "my cock");
    ft_utils::FtStringPtr value = ft_utils::FtStringPtr::adopt(str);
    setName({.i32 = 0}, value);
    str = static_cast<char*>(FeatureInstanceAllocType(getHandle(), strlen("cock breed") + 1, FT_STRING));
    strcpy(str, "cock breed");
    setBreed({.i32 = 0}, ft_utils::FtStringPtr::adopt(str));
}

ft_utils::FtStringPtr cock::name(AppendData append_data) const
{
    return _name;
}

void cock::setName(AppendData append_data, const ft_utils::FtStringPtr& val)
{
    _name = val;
}

FtInt cock::legCount(AppendData append_data) const
{
    return _legCount;
}

FtInt cock::eatFood(AppendData append_data, const ft_utils::RefPtr<FtArray>& food)
{
    return _eatFood;
}

ft_utils::FtStringPtr cock::run(AppendData append_data, FtInt distance, const ft_utils::FtStringPtr& destination)
{
    char buf[512];
    sprintf(buf, "cock run with %d leg, distance %d, destination %s", legCount({ .i32 = 0 }), distance, destination.ptr());
    char* ret = (char*)FeatureInstanceAllocType(getHandle(), strlen(buf) + 1, FT_STRING);
    strcpy(ret, buf);
    return ft_utils::FtStringPtr::adopt(ret);
}

ArrayType string_array_type = {
    .header = { .type = COMPLEX_ARRAY, .size = sizeof(FtArray) },
    .element_type = FT_STRING
};

// cock
ft_utils::RefPtr<FtArray> cock::fly(AppendData append_data)
{
    FEATURE_LOG_INFO("cock fly...");
    auto strArray = ft_utils::RefPtr<FtArray>::adopt(
        static_cast<FtArray*>(FeatureInstanceAllocType(getHandle(), sizeof(FtArray), FT_MK_COMPLEX(&string_array_type))));
    strArray->_size = 4;
    strArray->_element = malloc(sizeof(FtString) * strArray->_size);
    for (int i = 0; i < 4; i++) {
        char* str = static_cast<char*>(FeatureMalloc(100, FT_STRING));
        sprintf(str, "cock flip wings %d", i + 4);
        ((char**)strArray->_element)[i] = str;
    }
    return strArray;
}

ft_utils::FtStringPtr cock::breed(AppendData append_data) const
{
    return _breed;
}

void cock::setBreed(AppendData append_data, const ft_utils::FtStringPtr& breed)
{
    _breed = breed;
}
// IChicken
FtInt cock::weight(AppendData append_data) const
{
    return _weight;
}

void cock::setWeight(AppendData append_data, FtInt weight)
{
    _weight = weight;
}

void cock::walk(AppendData append_data, FtPromiseId pid)
{
    printf("%s::%s(), interface: %p, %s\n", file_tag, __FUNCTION__, getHandle(), "cock walk slowly");
    auto strArray = ft_utils::RefPtr<FtArray>::adopt(
        static_cast<FtArray*>(FeatureInstanceAllocType(getHandle(), sizeof(FtArray), FT_MK_COMPLEX(&string_array_type))));
    strArray->_size = 4;
    strArray->_element = malloc(sizeof(char*) * 4);
    for (int i = 0; i < 4; i++) {
        char* str = static_cast<char*>(FeatureMalloc(100, FT_STRING));
        sprintf(str, "cock walk %d", i);
        ((char**)strArray->_element)[i] = str;
    }
    FeaturePromiseResolve(getHandle(), pid, strArray.ptr());
}

dog::dog(FeatureInstanceHandle hInstance, FtInt type)
    : _hInst(hInstance)
    , _type(type)
{
    char* name_str = (char*)FeatureInstanceAllocType(hInstance, strlen("xiao huang") + 1, FT_STRING);
    strcpy(name_str, "xiao huang");
    auto name = ft_utils::FtStringPtr::adopt(name_str);
    _name = name;
}

dog::~dog()
{
    FEATURE_LOG_ERROR("~dog finalizer called !");
}

ft_utils::FtStringPtr dog::name(AppendData append_data) const
{
    return _name;
}

void dog::setName(AppendData append_data, const ft_utils::FtStringPtr& val)
{
    _name = val;
}

FtInt dog::legCount(AppendData append_data) const
{
    return _legCount;
}

FtInt dog::eatFood(AppendData append_data, const ft_utils::RefPtr<FtArray>& food)
{
    FEATURE_LOG_INFO("dog eat food count: %d", food->_size);
    for (size_t i = 0; i < food->_size; i++) {
        FtString food_name = ((FtString*)food->_element)[i];
        FEATURE_LOG_INFO("%d: %s", i, food_name);
    }
    return _eatFood;
}

ft_utils::FtStringPtr dog::run(AppendData append_data, FtInt distance, const ft_utils::FtStringPtr& destination)
{
    char buf[512];
    sprintf(buf, "dog run with %d leg, distance %d, destination %s", legCount({ .i32 = 0 }), distance, destination.ptr());
    char* ret = (char*)FeatureInstanceAllocType(getHandle(), strlen(buf) + 1, FT_STRING);
    strcpy(ret, buf);
    return ft_utils::FtStringPtr::adopt(ret);
}

cat::cat(FeatureInstanceHandle hInstance)
    : dog(hInstance, 0)
{
    char* name_str = (char*)FeatureInstanceAllocType(hInstance, strlen("mimi") + 1, FT_STRING);
    strcpy(name_str, "mimi");
    auto name = ft_utils::FtStringPtr::adopt(name_str);
    setName({ .i32 = 0 }, name);
}

ft_utils::FtStringPtr cat::run(AppendData append_data, FtInt distance, const ft_utils::FtStringPtr& destination)
{
    char buf[512];
    sprintf(buf, "cat %s run with %d leg, distance %d, destination %s", name({ .i32 = 0 }).ptr(), legCount({ .i32 = 0 }), distance, destination.ptr());
    char* ret = (char*)FeatureInstanceAllocType(getHandle(), strlen(buf) + 1, FT_STRING);
    strcpy(ret, buf);
    return ft_utils::FtStringPtr::adopt(ret);
}

pigeon::~pigeon()
{
    FEATURE_LOG_ERROR("~pigeon finalizer called !");
}

ft_utils::RefPtr<FtArray> pigeon::fly(AppendData append_data)
{
    FEATURE_LOG_INFO("pigeon fly...");
    auto strArray = ft_utils::RefPtr<FtArray>::adopt(
        static_cast<FtArray*>(FeatureInstanceAllocType(getHandle(), sizeof(FtArray), FT_MK_COMPLEX(&string_array_type))));
    strArray->_size = 4;
    strArray->_element = malloc(sizeof(FtString) * strArray->_size);
    for (int i = 0; i < 4; i++) {
        char* str = static_cast<char*>(FeatureMalloc(100, FT_STRING));
        sprintf(str, "cock flip wings %d", i + 4);
        ((char**)strArray->_element)[i] = str;
    }
    // return strArray;
    return strArray;
}

ft_utils::FtStringPtr pigeon::breed(AppendData append_data) const
{
    return _breed;
}

void pigeon::setBreed(AppendData append_data, const ft_utils::FtStringPtr& breed)
{
    _breed = breed;
}

}